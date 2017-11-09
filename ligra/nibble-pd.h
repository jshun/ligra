// This code is part of the project "Smaller and Faster: Parallel
// Processing of Compressed Graphs with Ligra+", presented at the IEEE
// Data Compression Conference, 2015.
// Copyright (c) 2015 Julian Shun, Laxman Dhulipala and Guy Blelloch
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#ifndef BYTECODE_H
#define BYTECODE_H

typedef unsigned char uchar;

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include "parallel.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define PARALLEL_DEGREE 1000

#define LAST_BIT_SET(b) (b & (0x8))
#define EDGE_SIZE_PER_BYTE 3

#define decode_val_nibblecode(_arr, _location, _result)    \
  {							    \
    _result = 0;					    \
    int shift = 0;					    \
    int cont = 1;					    \
    while(cont) {					    \
      int tmp = _arr[(*_location>>1)];			    \
      /* get appropriate nibble based on location but avoid conditional */ \
      tmp = (tmp >> (!((*_location)++ & 1) << 2));	   	\
      _result |= ((long)(tmp & 0x7) << shift);				\
       shift +=3;							\
      cont = tmp & 0x8;							\
    }									\
  }							    \

/*
  Nibble-encodes a value, with params:
    start  : pointer to byte arr
    offset : offset into start, in number of 1/2 bytes
    val    : the val to encode
*/
long encode_nibbleval(uchar* start, long offset, long val) {
  uchar currNibble = val & 0x7;
  while (val > 0) {
    uchar toWrite = currNibble;
    val = val >> 3;
    currNibble = val & 0x7;
    if(val > 0) toWrite |= 0x8;
    if(offset & 1) {
      start[offset/2] |= toWrite;
    } else {
      start[offset/2] = (toWrite << 4);
    }
    offset++;
  }
  return offset;
}

/**
  Decodes the first edge, which is specially sign encoded.
*/
inline uintE decode_first_edge(uchar* &start, long* location, uintE source) {
  long val;
  decode_val_nibblecode(start, location, val)
  long sign = val & 1;
  val >>= 1; // get rid of sign
  long result = source;
  if (sign) result += val;
  else result -= val;
  return result;
}

/*
  Decodes an edge, but does not add back the
*/
inline uintE decode_next_edge(uchar* &start, long* location) {
  long val;
  decode_val_nibblecode(start, location, val);
  return val;
}

/*
  The main decoding work-horse. First eats the specially coded first
  edge, and then eats the remaining |d-1| many edges that are normally
  coded.
*/
template <class T>
  inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true) {
  if (degree > 0) {
    long numChunks = 1+(degree-1)/PARALLEL_DEGREE;
    uintE* pOffsets = (uintE*) edgeStart; //use beginning of edgeArray for offsets into edge list
    uchar* start = edgeStart + (numChunks-1)*sizeof(uintE);
    //do first chunk
    long end = min<long>(PARALLEL_DEGREE,degree);
    long location = 0;
    // Eat first edge, which is compressed specially

    uintE startEdge = decode_first_edge(start,&location,source);

    if(!t.srcTarg(source,startEdge,0)) return;
    for (uintE edgeID = 1; edgeID < end; edgeID++) {
      // Eat the next 'edge', which is a difference, and reconstruct edge.
      uintE edgeRead = decode_next_edge(start,&location);
      uintE edge = startEdge + edgeRead;
      startEdge = edge;
      if(!t.srcTarg(source,startEdge,edgeID)) return;
    }
    //do remaining chunks in parallel
    granular_for(i, 1, numChunks, par, {
      long o = i*PARALLEL_DEGREE;
      long end = min<long>(o+PARALLEL_DEGREE,degree);
      // Eat first edge, which is compressed specially
      long location = pOffsets[i-1];
      uintE startEdge = decode_first_edge(edgeStart,&location,source);
      if(!t.srcTarg(source,startEdge,o)) end = 0;
      for (uintE edgeID = o+1; edgeID < end; edgeID++) {
	// Eat the next 'edge', which is a difference, and reconstruct edge.
	uintE edgeRead = decode_next_edge(edgeStart,&location);
	uintE edge = startEdge + edgeRead;
	startEdge = edge;
	if(!t.srcTarg(source,startEdge,edgeID)) break;
      }
    });
  }
}

//decode edges for weighted graph
template <class T>
  inline void decodeWgh(T t, uchar* edgeStart, const uintE &source,const uintT &degree, const bool par=true) {
  if(degree > 0){
    long numChunks = 1+(degree-1)/PARALLEL_DEGREE;
    uintE* pOffsets = (uintE*) edgeStart; //use beginning of edgeArray for offsets into edge list
    uchar* start = edgeStart + (numChunks-1)*sizeof(uintE);
    //do first chunk
    long end = min<long>(PARALLEL_DEGREE,degree);
    long location = 0;
    // Eat first edge, which is compressed specially
    uintE startEdge = decode_first_edge(start,&location,source);
    intE weight = decode_first_edge(start,&location,0);
    if(!t.srcTarg(source,startEdge,weight,0)) return;
    for (uintE edgeID = 1; edgeID < end; edgeID++) {
      // Eat the next 'edge', which is a difference, and reconstruct edge.
      uintE edgeRead = decode_next_edge(start,&location);
      uintE edge = startEdge + edgeRead;
      startEdge = edge;
      intE weight = decode_first_edge(start,&location,0);
      if(!t.srcTarg(source,startEdge,weight,edgeID)) return;
    }
    //do remaining chunks in parallel
    granular_for(i, 1, numChunks, par, {
      long o = i*PARALLEL_DEGREE;
      long end = min<long>(o+PARALLEL_DEGREE,degree);
      long location = pOffsets[i-1];
      // Eat first edge, which is compressed specially
      uintE startEdge = decode_first_edge(edgeStart,&location,source);
      intE weight = decode_first_edge(edgeStart,&location,0);
      if(!t.srcTarg(source,startEdge, weight, o)) end = 0;
      for (uintE edgeID = o+1; edgeID < end; edgeID++) {
	uintE edgeRead = decode_next_edge(edgeStart,&location);
	uintE edge = startEdge + edgeRead;
	startEdge = edge;
	intE weight = decode_first_edge(edgeStart,&location,0);
	if(!t.srcTarg(source, edge, weight, edgeID)) break;
      }
    });
  }
}

/*
  Takes:
    1. The edge array of chars to write into
    2. The current offset into this array
    3. The vertices degree
    4. The vertices vertex number
    5. The array of saved out-edges we're compressing
  Returns:
    The new offset into the edge array
*/
long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree,
                                uintE vertexNum, uintE *savedEdges) {
  if (degree > 0) {
    long startOffset = currentOffset;
    long numChunks = 1+(degree-1)/PARALLEL_DEGREE;
    uintE* pOffsets = (uintE*) edgeArray; //use beginning of edgeArray for offsets into edge list
    currentOffset += 2*(numChunks-1)*sizeof(uintE);
    for(long i=0;i<numChunks;i++) {
      long o = i*PARALLEL_DEGREE;
      long end = min<long>(PARALLEL_DEGREE,degree-o);
      uintE* myEdges = savedEdges + o;
      if(i>0) pOffsets[i-1] = currentOffset - startOffset;
      // Compress the first edge whole, which is signed difference coded
      long preCompress = (long) myEdges[0] - vertexNum;
      long toCompress = labs(preCompress);
      intE sign = 1;
      if (preCompress < 0) {
	sign = 0;
      }
      toCompress = (toCompress << 1) | sign;
      long temp = currentOffset;
      currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);
      long val;
      for (uintT edgeI=1; edgeI < end; edgeI++) {
	// Store difference between cur and prev edge.
	uintE difference = myEdges[edgeI] -
	  myEdges[edgeI - 1];
	temp = currentOffset;
	currentOffset = encode_nibbleval(edgeArray, currentOffset, difference);
	//for debugging only
      }
    }
  }
  return currentOffset;
}

/*
  Compresses the edge set in parallel.
*/
uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m, uintE* Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE*, n);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n+1);
  {parallel_for(long i=0; i<n; i++) {
    charsUsedArr[i] = ceil((Degrees[i] * 9) / 8) + 4;
  }}
  long toAlloc = sequence::plusScan(charsUsedArr,charsUsedArr,n);
  uintE* iEdges = newA(uintE,toAlloc);
  {parallel_for(long i=0; i<n; i++) {
      edgePts[i] = iEdges+charsUsedArr[i];
      long charsUsed =
	sequentialCompressEdgeSet((uchar *)(iEdges+charsUsedArr[i]),
				  0, Degrees[i],
				  i, edges + offsets[i]);
      // convert units from #1/2 bytes -> #bytes, round up to make it
      //byte-aligned
      charsUsed = (charsUsed+1) / 2;
      charsUsedArr[i] = charsUsed;
  }}
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace; // in bytes
  free(charsUsedArr);

  uchar *finalArr = newA(uchar, totalSpace);
  cout << "total space requested is : " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace*8 / (float)m;
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;

  {parallel_for(long i=0; i<n; i++) {
      long o = compressionStarts[i];
    memcpy(finalArr + o, (uchar *)(edgePts[i]), compressionStarts[i+1]-o);
    offsets[i] = o;
  }}
  offsets[n] = totalSpace;
  free(iEdges);
  free(edgePts);
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m * 4) << endl;
  return ((uintE*)finalArr);
}


typedef pair<uintE,intE> intEPair;

/*
  Takes:
    1. The edge array of chars to write into
    2. The current offset into this array
    3. The vertices degree
    4. The vertices vertex number
    5. The array of saved out-edges we're compressing
  Returns:
    The new offset into the edge array
*/
long sequentialCompressWeightedEdgeSet
(uchar *edgeArray, long currentOffset, uintT degree,
 uintE vertexNum, intEPair *savedEdges) {
  if (degree > 0) {
    long startOffset = currentOffset;
    long numChunks = 1+(degree-1)/PARALLEL_DEGREE;
    uintE* pOffsets = (uintE*) edgeArray; //use beginning of edgeArray for offsets into edge list
    currentOffset += 2*(numChunks-1)*sizeof(uintE);
    for(long i=0;i<numChunks;i++) {
      long o = i*PARALLEL_DEGREE;
      long end = min<long>(PARALLEL_DEGREE,degree-o);
      intEPair* myEdges = savedEdges + o;
      if(i>0) pOffsets[i-1] = currentOffset - startOffset;

      // Compress the first edge whole, which is signed difference coded
      //target ID
      intE preCompress = myEdges[0].first - vertexNum;
      intE toCompress = abs(preCompress);
      intE sign = 1;
      if (preCompress < 0) {
	sign = 0;
      }
      toCompress = (toCompress<<1)|sign;
      currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

      //weight
      intE weight = myEdges[0].second;
      if (weight < 0) sign = 0; else sign = 1;
      toCompress = (abs(weight)<<1)|sign;
      currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

      for (uintT edgeI=1; edgeI < end; edgeI++) {
	// Store difference between cur and prev edge.
	uintE difference = myEdges[edgeI].first -
	  myEdges[edgeI - 1].first;

	//compress difference
	currentOffset = encode_nibbleval(edgeArray, currentOffset, difference);

	//compress weight

	weight = myEdges[edgeI].second;
	if (weight < 0) sign = 0; else sign = 1;
	toCompress = (abs(weight)<<1)|sign;
	currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);
      }
    }
  }
  return currentOffset;
}

/*
  Compresses the weighted edge set in parallel.
*/
uchar *parallelCompressWeightedEdges(intEPair *edges, uintT *offsets, long n, long m, uintE* Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE*, n);
  uintT *degrees = newA(uintT, n+1);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n+1);
  {parallel_for(long i=0; i<n; i++) {
    degrees[i] = Degrees[i];
    charsUsedArr[i] = 2*(ceil((degrees[i] * 9) / 8) + 4); //to change
  }}
  degrees[n] = 0;
  sequence::plusScan(degrees,degrees, n+1);
  long toAlloc = sequence::plusScan(charsUsedArr,charsUsedArr,n);
  uintE* iEdges = newA(uintE,toAlloc);
  {parallel_for(long i=0; i<n; i++) {
    edgePts[i] = iEdges+charsUsedArr[i];
    long charsUsed =
      sequentialCompressWeightedEdgeSet((uchar *)(iEdges+charsUsedArr[i]),
                0, degrees[i+1]-degrees[i],
                i, edges + offsets[i]);
    charsUsed = (charsUsed+1) / 2;
    charsUsedArr[i] = charsUsed;
  }}

  // produce the total space needed for all compressed lists in # of 1/2 bytes
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;
  free(degrees);
  free(charsUsedArr);

  uchar *finalArr = newA(uchar, totalSpace);
  cout << "total space requested is : " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace*8 / (float)m;
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;

  {parallel_for(long i=0; i<n; i++) {
      long o = compressionStarts[i];
    memcpy(finalArr + o, (uchar *)(edgePts[i]), compressionStarts[i+1]-o);
    offsets[i] = o;
  }}
  offsets[n] = totalSpace;
  free(iEdges);
  free(edgePts);
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m * 8) << endl;
  return finalArr;
}

#endif
