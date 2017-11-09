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

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include "parallel.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define LAST_BIT_SET(b) (b & (0x80))
#define EDGE_SIZE_PER_BYTE 7

typedef unsigned char uchar;

/* Reads the first edge of an out-edge list, which is the signed
   difference between the target and source.
*/

inline intE eatWeight(uchar* &start) {
  uchar fb = *start++;
  intE edgeRead = (fb & 0x3f);
  if (LAST_BIT_SET(fb)) {
    int shiftAmount = 6;
    while (1) {
      uchar b = *start;
      edgeRead |= ((b & 0x7f) << shiftAmount);
      start++;
      if (LAST_BIT_SET(b))
        shiftAmount += EDGE_SIZE_PER_BYTE;
      else
        break;
    }
  }
  return (fb & 0x40) ? -edgeRead : edgeRead;
}

inline intE eatFirstEdge(uchar* &start, uintE source) {
  uchar fb = *start++;
  intE edgeRead = (fb & 0x3f);
  if (LAST_BIT_SET(fb)) {
    int shiftAmount = 6;
    while (1) {
      uchar b = *start;
      edgeRead |= ((b & 0x7f) << shiftAmount);
      start++;
      if (LAST_BIT_SET(b))
        shiftAmount += EDGE_SIZE_PER_BYTE;
      else
        break;
    }
  }
  return (fb & 0x40) ? source - edgeRead : source + edgeRead;
}

/*
  Reads any edge of an out-edge list after the first edge.
*/
inline uintE eatEdge(uchar* &start) {
  uintE edgeRead = 0;
  int shiftAmount = 0;

  while (1) {
    uchar b = *start++;
    edgeRead += ((b & 0x7f) << shiftAmount);
    if (LAST_BIT_SET(b))
      shiftAmount += EDGE_SIZE_PER_BYTE;
    else
      break;
  }
  return edgeRead;
}

/*
  The main decoding work-horse. First eats the specially coded first
  edge, and then eats the remaining |d-1| many edges that are normally
  coded.
*/
template <class T>
  inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true) {
  size_t edgesRead = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially
    uintE startEdge = eatFirstEdge(edgeStart,source);
    if (!t.srcTarg(source,startEdge,edgesRead)) {
      return;
    }
    for (edgesRead = 1; edgesRead < degree; edgesRead++) {
      // Eat the next 'edge', which is a difference, and reconstruct edge.
      uintE edgeRead = eatEdge(edgeStart);
      uintE edge = startEdge + edgeRead;
      startEdge = edge;
      if (!t.srcTarg(source, edge, edgesRead)) {
        break;
      }
    }
  }
}

//decode edges for weighted graph
template <class T>
  inline void decodeWgh(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true) {
  uintT edgesRead = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially
    uintE startEdge = eatFirstEdge(edgeStart,source);
    intE weight = eatWeight(edgeStart);
    if (!t.srcTarg(source, startEdge, weight, edgesRead)) {
      return;
    }
    for (edgesRead = 1; edgesRead < degree; edgesRead++) {
      uintE edgeRead = eatEdge(edgeStart);
      uintE edge = startEdge + edgeRead;
      startEdge = edge;
      intE weight = eatWeight(edgeStart);
      if (!t.srcTarg(source, edge, weight, edgesRead)) {
        break;
      }
    }
  }
}

/*
  Compresses the first edge, writing target-source and a sign bit.
*/
long compressFirstEdge(uchar *start, long offset, uintE source, uintE target) {
  uchar* saveStart = start;
  long saveOffset = offset;

  intE preCompress = (intE) target - source;
  int bytesUsed = 0;
  uchar firstByte = 0;
  intE toCompress = abs(preCompress);
  firstByte = toCompress & 0x3f; // 0011|1111
  if (preCompress < 0) {
    firstByte |= 0x40;
  }
  toCompress = toCompress >> 6;
  if (toCompress > 0) {
    firstByte |= 0x80;
  }
  start[offset] = firstByte;
  offset++;

  uchar curByte = toCompress & 0x7f;
  while ((curByte > 0) || (toCompress > 0)) {
    bytesUsed++;
    uchar toWrite = curByte;
    toCompress = toCompress >> 7;
    // Check to see if there's any bits left to represent
    curByte = toCompress & 0x7f;
    if (toCompress > 0) {
      toWrite |= 0x80;
    }
    start[offset] = toWrite;
    offset++;
  }
  return offset;
}

/*
  Should provide the difference between this edge and the previous edge
*/

long compressEdge(uchar *start, long curOffset, uintE e) {
  uchar curByte = e & 0x7f;
  int bytesUsed = 0;
  while ((curByte > 0) || (e > 0)) {
    bytesUsed++;
    uchar toWrite = curByte;
    e = e >> 7;
    // Check to see if there's any bits left to represent
    curByte = e & 0x7f;
    if (e > 0) {
      toWrite |= 0x80;
    }
    start[curOffset] = toWrite;
    curOffset++;
  }
  return curOffset;
}

// Alternate implementation of packOutNgh, which does both the decoding and
// reencoding in one pass of the edges.
template <class P>
inline size_t pack(P pred, uchar* edge_start, const uintE &source, const uintE &degree) {
  size_t new_deg = 0;

  uintE last_read_edge = source;
  uintE last_write_edge;

  uchar* tail = edge_start; // write-finger
  uchar* cur = edge_start; // read-finger
  if (degree > 0) {
    uintE start_edge = eatFirstEdge(cur, source); // increments cur
    last_read_edge = start_edge;
    if (pred(source, start_edge)) {
      long offset = compressFirstEdge(tail, 0, source, start_edge);
      tail += offset;
      last_write_edge = start_edge;
      new_deg++;
    }
    for (size_t i = 1; i < degree; i++) {
      uintE cur_diff = eatEdge(cur);
      uintE cur_edge = last_read_edge + cur_diff;
      last_read_edge = cur_edge;
      if (pred(source, cur_edge)) {
        long offset;
        if (tail == edge_start) {
          offset = compressFirstEdge(tail, 0, source, cur_edge);
        } else {
          uintE difference = cur_edge - last_write_edge;
          offset = compressEdge(tail, 0, difference);
        }
        tail += offset;
        last_write_edge = cur_edge;
        new_deg++;
      }
    }
  }
  return new_deg;
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
    // Compress the first edge whole, which is signed difference coded
    currentOffset = compressFirstEdge(edgeArray, currentOffset,
                                       vertexNum, savedEdges[0]);
    for (uintT edgeI=1; edgeI < degree; edgeI++) {
      // Store difference between cur and prev edge.
      uintE difference = savedEdges[edgeI] -
                        savedEdges[edgeI - 1];
      currentOffset = compressEdge(edgeArray, currentOffset, difference);
    }
    // Increment nWritten after all of vertex n's neighbors are written
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
      charsUsedArr[i] = charsUsed;
  }}

  // produce the total space needed for all compressed lists in chars.
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;
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
  return ((uintE *)finalArr);
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
    // Compress the first edge whole, which is signed difference coded
    //target ID
    currentOffset = compressFirstEdge(edgeArray, currentOffset,
                                       vertexNum, savedEdges[0].first);
    //weight
    currentOffset = compressFirstEdge(edgeArray, currentOffset,
				      0,savedEdges[0].second);

    for (uintT edgeI=1; edgeI < degree; edgeI++) {
      // Store difference between cur and prev edge.
      uintE difference = savedEdges[edgeI].first -
                        savedEdges[edgeI - 1].first;

      //compress difference
      currentOffset = compressEdge(edgeArray, currentOffset, difference);

      //compress weight
      currentOffset = compressFirstEdge(edgeArray, currentOffset, 0, savedEdges[edgeI].second);
    }
    // Increment nWritten after all of vertex n's neighbors are written
  }
  return currentOffset;
}

/*
  Compresses the weighted edge set in parallel.
*/
uchar *parallelCompressWeightedEdges(intEPair *edges, uintT *offsets, long n, long m, uintE* Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE*, n);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n+1);
  {parallel_for(long i=0; i<n; i++) {
    charsUsedArr[i] = 2*(ceil((Degrees[i] * 9) / 8) + 4); //to change
  }}
  long toAlloc = sequence::plusScan(charsUsedArr,charsUsedArr,n);
  uintE* iEdges = newA(uintE,toAlloc);

  {parallel_for(long i=0; i<n; i++) {
    edgePts[i] = iEdges+charsUsedArr[i];
    long charsUsed =
      sequentialCompressWeightedEdgeSet((uchar *)(iEdges+charsUsedArr[i]), 0, Degrees[i],i, edges + offsets[i]);
    charsUsedArr[i] = charsUsed;
  }}

  // produce the total space needed for all compressed lists in chars.
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;
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
