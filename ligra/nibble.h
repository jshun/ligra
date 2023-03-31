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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmath>
#include <fstream>
#include <iostream>
#include "parallel.h"
#include "utils.h"

#define LAST_BIT_SET(b) (b & (0x8))
#define EDGE_SIZE_PER_BYTE 3

#define decode_val_nibblecode(_arr, _location, _result)                    \
  {                                                                        \
    _result = 0;                                                           \
    int shift = 0;                                                         \
    int cont = 1;                                                          \
    while (cont) {                                                         \
      int tmp = _arr[(*_location >> 1)];                                   \
      /* get appropriate nibble based on location but avoid conditional */ \
      tmp = (tmp >> (!((*_location)++ & 1) << 2));                         \
      _result |= ((long)(tmp & 0x7) << shift);                             \
      shift += 3;                                                          \
      cont = tmp & 0x8;                                                    \
    }                                                                      \
  }

/*
  Nibble-encodes a value, with params:
    start  : pointer to byte arr
    offset : offset into start, in number of 1/2 bytes
    val    : the val to encode
*/
long encode_nibbleval(uchar *start, long offset, long val) {
  uchar currNibble = val & 0x7;
  while (val > 0) {
    uchar toWrite = currNibble;
    val = val >> 3;
    currNibble = val & 0x7;
    if (val > 0) toWrite |= 0x8;
    if (offset & 1) {
      start[offset / 2] |= toWrite;
    } else {
      start[offset / 2] = (toWrite << 4);
    }
    offset++;
  }
  return offset;
}

/**
  Decodes the first edge, which is specially sign encoded.
*/
inline uintE decode_first_edge(uchar *&start, long *location, uintE source) {
  long val;
  decode_val_nibblecode(start, location, val) long sign = val & 1;
  val >>= 1;  // get rid of sign
  long result = source;
  if (sign)
    result += val;
  else
    result -= val;
  return result;
}

/*
  Decodes an edge, but does not add back the
*/
inline uintE decode_next_edge(uchar *&start, long *location) {
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
inline void decode(T t, uchar *edgeArr, const uintE &source,
                   const uintT &degree, const bool par = true) {
  uintE edgesRead = 0;
  long location = 0;
  if (degree > 0) {
    uintE startEdge = decode_first_edge(edgeArr, &location, source);
    uintE prevEdge = startEdge;
    if (!t.srcTarg(source, startEdge, edgesRead)) {
      return;
    }

    for (edgesRead = 1; edgesRead < degree; edgesRead++) {
      uintE edgeRead = decode_next_edge(edgeArr, &location);
      uintE edge = edgeRead + prevEdge;
      prevEdge = edge;
      if (!t.srcTarg(source, edge, edgesRead)) {
        break;
      }
    }
  }
}

// decode edges for weighted graph
template <class T>
inline void decodeWgh(T t, uchar *edgeStart, const uintE &source,
                      const uintT &degree, const bool par = true) {
  uintT edgesRead = 0;
  long location = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially
    uintE startEdge = decode_first_edge(edgeStart, &location, source);
    intE weight = decode_first_edge(edgeStart, &location, 0);
    if (!t.srcTarg(source, startEdge, weight, edgesRead)) {
      return;
    }
    for (edgesRead = 1; edgesRead < degree; edgesRead++) {
      uintE edgeRead = decode_next_edge(edgeStart, &location);
      uintE edge = startEdge + edgeRead;
      startEdge = edge;
      intE weight = decode_first_edge(edgeStart, &location, 0);
      if (!t.srcTarg(source, edge, weight, edgesRead)) {
        break;
      }
    }
  }
}

// decode edges for weighted temporal graph
template <class T>
inline void decodeWghTemporal(T t, uchar *edgeStart, const uintE &source,
                              const uintT &degree, const bool par = true) {
  uintT edgesRead = 0;
  long location = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially
    uintE startEdge = decode_first_edge(edgeStart, &location, source);
    intE weight = decode_first_edge(edgeStart, &location, 0);
    // FIXME: make sure these two below are working as expected
    intE start_time = decode_first_edge(edgeStart, &location, 0);
    intE end_time = decode_first_edge(edgeStart, &location, 0);
    if (!t.srcTarg(source, startEdge, weight, start_time, end_time,
                   edgesRead)) {
      return;
    }
    for (edgesRead = 1; edgesRead < degree; edgesRead++) {
      uintE edgeRead = decode_next_edge(edgeStart, &location);
      uintE edge = startEdge + edgeRead;
      startEdge = edge;
      intE weight = decode_first_edge(edgeStart, &location, 0);
      // FIXME: make sure these two below are working as expected
      intE start_time = decode_first_edge(edgeStart, &location, 0);
      intE end_time = decode_first_edge(edgeStart, &location, 0);
      if (!t.srcTarg(source, edge, weight, start_time, end_time, edgesRead)) {
        break;
      }
    }
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
long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset,
                               uintT degree, uintE vertexNum,
                               uintE *savedEdges) {
  if (degree > 0) {
    // Compress the first edge whole, which is signed difference coded
    long preCompress = (long)savedEdges[0] - vertexNum;
    long toCompress = labs(preCompress);
    intE sign = 1;
    if (preCompress < 0) {
      sign = 0;
    }
    toCompress = (toCompress << 1) | sign;
    long temp = currentOffset;

    currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);
    long val;
    // for debugging only
    // decode_val_nibblecode(edgeArray,&temp,val);
    // if(val != toCompress) {cout << "decoding error! got "<<val<<" but
    // should've been "<<(savedEdges[0])<<" " <<vertexNum<<" "<<preCompress<<" "
    // << labs(preCompress)<<" " << toCompress<<" " << (val >> 1)<<endl;
    // exit(0);}
    for (uintT edgeI = 1; edgeI < degree; edgeI++) {
      // Store difference between cur and prev edge.
      uintE difference = savedEdges[edgeI] - savedEdges[edgeI - 1];
      temp = currentOffset;
      currentOffset = encode_nibbleval(edgeArray, currentOffset, difference);
      // for debugging only
      // decode_val_nibblecode(edgeArray,&temp,val);
      // if(val != difference) {cout << "decoding error! got "<<val<<" but
      // should've been "<<(savedEdges[0])<<" " <<vertexNum<<"
      // "<<difference<<endl; exit(0);}
    }
  }
  return currentOffset;
}

/*
  Compresses the edge set in parallel.
*/
uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m,
                             uintE *Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE *, n);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n + 1);
  {
    parallel_for(long i = 0; i < n; i++) {
      charsUsedArr[i] = ceil((Degrees[i] * 9) / 8) + 4;
    }
  }
  long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
  uintE *iEdges = newA(uintE, toAlloc);
  {
    parallel_for(long i = 0; i < n; i++) {
      edgePts[i] = iEdges + charsUsedArr[i];
      long charsUsed =
          sequentialCompressEdgeSet((uchar *)(iEdges + charsUsedArr[i]), 0,
                                    Degrees[i], i, edges + offsets[i]);
      // convert units from #1/2 bytes -> #bytes, round up to make it
      // byte-aligned
      charsUsed = (charsUsed + 1) / 2;
      charsUsedArr[i] = charsUsed;
    }
  }

  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;  // in bytes
  free(charsUsedArr);

  uchar *finalArr = newA(uchar, totalSpace);
  cout << "total space requested is : " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace * 8 / (float)m;
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;

  {
    parallel_for(long i = 0; i < n; i++) {
      long o = compressionStarts[i];
      memcpy(finalArr + o, (uchar *)(edgePts[i]), compressionStarts[i + 1] - o);
      offsets[i] = o;
    }
  }
  offsets[n] = totalSpace;
  free(iEdges);
  free(edgePts);
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m * 4) << endl;
  return ((uintE *)finalArr);
}

typedef pair<uintE, intE> intEPair;

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
long sequentialCompressWeightedEdgeSet(uchar *edgeArray, long currentOffset,
                                       uintT degree, uintE vertexNum,
                                       intEPair *savedEdges) {
  if (degree > 0) {
    // Compress the first edge whole, which is signed difference coded
    // target ID
    intE preCompress = savedEdges[0].first - vertexNum;
    intE toCompress = abs(preCompress);
    intE sign = 1;
    if (preCompress < 0) {
      sign = 0;
    }
    toCompress = (toCompress << 1) | sign;
    currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

    // weight
    intE weight = savedEdges[0].second;
    if (weight < 0)
      sign = 0;
    else
      sign = 1;
    toCompress = (abs(weight) << 1) | sign;
    currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

    for (uintT edgeI = 1; edgeI < degree; edgeI++) {
      // Store difference between cur and prev edge.
      uintE difference = savedEdges[edgeI].first - savedEdges[edgeI - 1].first;

      // compress difference
      currentOffset = encode_nibbleval(edgeArray, currentOffset, difference);

      // compress weight

      weight = savedEdges[edgeI].second;
      if (weight < 0)
        sign = 0;
      else
        sign = 1;
      toCompress = (abs(weight) << 1) | sign;
      currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);
    }
    // Increment nWritten after all of vertex n's neighbors are written
  }
  return currentOffset;
}

/*
  Compresses the weighted edge set in parallel.
*/
uchar *parallelCompressWeightedEdges(intEPair *edges, uintT *offsets, long n,
                                     long m, uintE *Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE *, n);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n + 1);
  {
    parallel_for(long i = 0; i < n; i++) {
      charsUsedArr[i] = 2 * (ceil((Degrees[i] * 9) / 8) + 4);  // to change
    }
  }
  long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
  uintE *iEdges = newA(uintE, toAlloc);
  {
    parallel_for(long i = 0; i < n; i++) {
      edgePts[i] = iEdges + charsUsedArr[i];
      long charsUsed = sequentialCompressWeightedEdgeSet(
          (uchar *)(iEdges + charsUsedArr[i]), 0, Degrees[i], i,
          edges + offsets[i]);
      charsUsed = (charsUsed + 1) / 2;
      charsUsedArr[i] = charsUsed;
    }
  }

  // produce the total space needed for all compressed lists in # of 1/2 bytes
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;
  free(charsUsedArr);

  uchar *finalArr = newA(uchar, totalSpace);
  cout << "total space requested is : " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace * 8 / (float)m;
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;

  {
    parallel_for(long i = 0; i < n; i++) {
      long o = compressionStarts[i];
      memcpy(finalArr + o, (uchar *)(edgePts[i]), compressionStarts[i + 1] - o);
      offsets[i] = o;
    }
  }
  offsets[n] = totalSpace;
  free(iEdges);
  free(edgePts);
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m * 8) << endl;
  return finalArr;
}

typedef tuple<uintE, intE, intE, intE> intEQuadruple;

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
long sequentialCompressWeightedEdgeSet(uchar *edgeArray, long currentOffset,
                                       uintT degree, uintE vertexNum,
                                       intEQuadruple *savedEdges) {
  if (degree > 0) {
    // Compress the first edge whole, which is signed difference coded
    // target ID
    intE preCompress = get<0>(savedEdges[0]) - vertexNum;
    intE toCompress = abs(preCompress);
    intE sign = 1;
    if (preCompress < 0) {
      sign = 0;
    }
    toCompress = (toCompress << 1) | sign;
    currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

    // weight
    intE weight = get<1>(savedEdges[0]);
    if (weight < 0)
      sign = 0;
    else
      sign = 1;
    toCompress = (abs(weight) << 1) | sign;
    currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

    // start_time
    intE start_time = get<2>(savedEdges[0]);
    if (start_time < 0)
      sign = 0;
    else
      sign = 1;
    toCompress = (abs(start_time) << 1) | sign;
    currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

    // end_time
    intE end_time = get<3>(savedEdges[0]);
    if (end_time < 0)
      sign = 0;
    else
      sign = 1;
    toCompress = (abs(end_time) << 1) | sign;
    currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

    for (uintT edgeI = 1; edgeI < degree; edgeI++) {
      // Store difference between cur and prev edge.
      uintE difference =
          get<0>(savedEdges[edgeI]) - get<0>(savedEdges[edgeI - 1]);

      // compress difference
      currentOffset = encode_nibbleval(edgeArray, currentOffset, difference);

      // compress weight
      weight = get<1>(savedEdges[edgeI]);
      if (weight < 0)
        sign = 0;
      else
        sign = 1;
      toCompress = (abs(weight) << 1) | sign;
      currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

      // compress start_time
      start_time = get<2>(savedEdges[edgeI]);
      if (start_time < 0)
        sign = 0;
      else
        sign = 1;
      toCompress = (abs(start_time) << 1) | sign;
      currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);

      // compress end_time
      end_time = get<3>(savedEdges[edgeI]);
      if (end_time < 0)
        sign = 0;
      else
        sign = 1;
      toCompress = (abs(end_time) << 1) | sign;
      currentOffset = encode_nibbleval(edgeArray, currentOffset, toCompress);
    }
    // Increment nWritten after all of vertex n's neighbors are written
  }
  return currentOffset;
}

/*
  Compresses the weighted temporal edge set in parallel.
*/
uchar *parallelCompressWeightedTemporalEdges(intEQuadruple *edges,
                                             uintT *offsets, long n, long m,
                                             uintE *Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE *, n);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n + 1);
  {
    parallel_for(long i = 0; i < n; i++) {
      charsUsedArr[i] = 4 * (ceil((Degrees[i] * 9) / 8) + 4);  // to change
    }
  }
  long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
  uintE *iEdges = newA(uintE, toAlloc);
  {
    parallel_for(long i = 0; i < n; i++) {
      edgePts[i] = iEdges + charsUsedArr[i];
      long charsUsed = sequentialCompressWeightedTemporalEdgeSet(
          (uchar *)(iEdges + charsUsedArr[i]), 0, Degrees[i], i,
          edges + offsets[i]);
      charsUsed = (charsUsed + 1) / 2;
      charsUsedArr[i] = charsUsed;
    }
  }

  // produce the total space needed for all compressed lists in # of 1/2 bytes
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;
  free(charsUsedArr);

  uchar *finalArr = newA(uchar, totalSpace);
  cout << "total space requested is : " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace * 8 / (float)m;
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;

  {
    parallel_for(long i = 0; i < n; i++) {
      long o = compressionStarts[i];
      memcpy(finalArr + o, (uchar *)(edgePts[i]), compressionStarts[i + 1] - o);
      offsets[i] = o;
    }
  }
  offsets[n] = totalSpace;
  free(iEdges);
  free(edgePts);
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m * 16) << endl;
  return finalArr;
}

#endif
