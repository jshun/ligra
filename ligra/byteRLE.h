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

#define LAST_BIT_SET(b) (b & (0x80))
#define EDGE_SIZE_PER_BYTE 7


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
  //int sign = (fb & 0x40) ? -1 : 1;
  intE edgeRead = (fb & 0x3f);
  if (LAST_BIT_SET(fb)) {
    int shiftAmount = 6;
    //shiftAmount += 6;
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
  //edgeRead *= sign;
  return (fb & 0x40) ? source - edgeRead : source + edgeRead;
}

/*
  The main decoding work-horse. First eats the specially coded first
  edge, and then eats the remaining |d-1| many edges that are normally
  coded.
*/
template <class T>
  inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true) {
  uintE edgesRead = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially
    uintE startEdge = eatFirstEdge(edgeStart,source);
    if (!t.srcTarg(source,startEdge,edgesRead)) {
      return;
    }
    uintT i = 0;
    edgesRead = 1;
    while(1) {
      if(edgesRead == degree) return;
      uchar header = edgeStart[i++];
      uint numbytes = 1 + (header & 0x3);
      uint runlength = 1 + (header >> 2);
      switch(numbytes) {
      case 1:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i++] + startEdge;
	  startEdge = edge;
	  if (!t.srcTarg(source, edge, edgesRead++)) {
	    return;
	  }
	}
	break;
      case 2:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + startEdge;
	  i += 2;
	  startEdge = edge;
	  if (!t.srcTarg(source, edge, edgesRead++)) {
	    return;
	  }
	}
	break;
      case 3:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + (((uintE) edgeStart[i+2]) << 16) + startEdge;
	  i += 3;
	  startEdge = edge;
	  if (!t.srcTarg(source, edge, edgesRead++)) {
	    return;
	  }
	}
	break;
      default:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + (((uintE) edgeStart[i+2]) << 16) + (((uintE) edgeStart[i+3]) << 24) + startEdge;
	  i+=4;
	  startEdge = edge;
	  if (!t.srcTarg(source, edge, edgesRead++)) {
	    return;
	  }
	}
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

long compressEdges(uchar *start, long curOffset, uintE* savedEdges, uintE edgeI, int numBytes, uintT runlength) {
  //header
  uchar header = numBytes - 1;
  header |= ((runlength-1) << 2);
  start[curOffset++] = header;

  for(int i=0;i<runlength;i++) {
    uintE e = savedEdges[edgeI+i] - savedEdges[edgeI+i-1];
    int bytesUsed = 0;
    for(int j=0; j<numBytes; j++) {
      uchar curByte = e & 0xff;
      e = e >> 8;
      start[curOffset++] = curByte;
      bytesUsed++;
    }
  }
  return curOffset;
}

#define ONE_BYTE 256
#define TWO_BYTES 65536
#define THREE_BYTES 16777216
#define ONE_BYTE_SIGNED_MAX 128
#define ONE_BYTE_SIGNED_MIN -127
#define TWO_BYTES_SIGNED_MAX 32768
#define TWO_BYTES_SIGNED_MIN -32767
#define THREE_BYTES_SIGNED_MAX 8388608
#define THREE_BYTES_SIGNED_MIN -8388607

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
long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, uintE *savedEdges) {
  if (degree > 0) {
    // Compress the first edge whole, which is signed difference coded
    currentOffset = compressFirstEdge(edgeArray, currentOffset,
                                       vertexNum, savedEdges[0]);
    if (degree == 1) return currentOffset;
    uintE edgeI = 1;
    uintT runlength = 0;
    int numBytes = 0;
    while(1) {
      uintE difference = savedEdges[edgeI+runlength] -
	savedEdges[edgeI+runlength - 1];
      if(difference < ONE_BYTE) {
	if(!numBytes) {numBytes = 1; runlength++;}
	else if(numBytes == 1) runlength++;
	else {
	  //encode
	  currentOffset = compressEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      } else if(difference < TWO_BYTES) {
	if(!numBytes) {numBytes = 2; runlength++;}
	else if(numBytes == 2) runlength++;
	else {
	  //encode
	  currentOffset = compressEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      } else if(difference < THREE_BYTES) {
	if(!numBytes) {numBytes = 3; runlength++;}
	else if(numBytes == 3) runlength++;
	else {
	  //encode
	  currentOffset = compressEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      } else {
	if(!numBytes) {numBytes = 4; runlength++;}
	else if(numBytes == 4) runlength++;
	else {
	  //encode
	  currentOffset = compressEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }

      if(runlength == 64) {
	currentOffset = compressEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, runlength);
	edgeI += runlength;
	runlength = numBytes = 0;
      }

      if(runlength + edgeI == degree) {
	currentOffset = compressEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, runlength);
	break;
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
      charsUsedArr[i] = 2*(ceil((Degrees[i] * 9) / 8) + 4);
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

/*
  Compresses the first edge, writing target-source and a sign bit.
*/

int numBytesSigned (intE x) {
  if(x < ONE_BYTE_SIGNED_MAX && x > ONE_BYTE_SIGNED_MIN) return 1;
  else return 4;
}

template <class T>
  inline void decodeWgh(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true) {
  uintE edgesRead = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially
    uintE startEdge = eatFirstEdge(edgeStart,source);
    intE weight = eatWeight(edgeStart);
    if (!t.srcTarg(source,startEdge, weight, edgesRead)) {
      return;
    }

    uintT i = 0;
    edgesRead = 1;
    while(1) {
      if(edgesRead == degree) return;
      uchar header = edgeStart[i++];
      uint info = header & 0x7; //3 bits for info
      uint runlength = 1 + (header >> 3);
      switch(info) {
      case 0:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + startEdge;
	  startEdge = edge;
	  uintE w = edgeStart[i+1]; //highest bit is sign bit
	  intE weight = (w & 0x80) ? -(w & 0x7f) : (w & 0x7f);
	  i+=2;
	  if (!t.srcTarg(source, edge, weight, edgesRead++)) {
	    return;
	  }
	}
	break;
      case 1:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + startEdge;
	  startEdge = edge;
	  uintE w = edgeStart[i+2]; //highest bit is sign bit
	  intE weight = (w & 0x80) ? -(w & 0x7f) : (w & 0x7f);
	  i += 3;
	  if (!t.srcTarg(source, edge, weight, edgesRead++)) {
	    return;
	  }
	}
	break;
      case 2:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + (((uintE) edgeStart[i+2]) << 16) + startEdge;
	  startEdge = edge;
	  uintE w = edgeStart[i+3]; //highest bit is sign bit
	  intE weight = (w & 0x80) ? -(w & 0x7f) : (w & 0x7f);
	  i+=4;
	  if (!t.srcTarg(source, edge, weight, edgesRead++)) {
	    return;
	  }
	}
	break;
      case 3:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + (((uintE) edgeStart[i+2]) << 16) + (((uintE) edgeStart[i+3]) << 24) + startEdge;
	  startEdge = edge;
	  uintE w = edgeStart[i+4]; //highest bit is sign bit
	  intE weight = (w & 0x80) ? -(w & 0x7f) : (w & 0x7f);
	  i+=5;
	  if (!t.srcTarg(source, edge, weight, edgesRead++)) {
	    return;
	  }
	}
	break;
      case 4:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + startEdge;
	  startEdge = edge;
	  uintE w = edgeStart[i+1]; //highest bit is sign bit
	  intE weight = (w & 0x7f) + (((uintE) edgeStart[i+2]) << 7) + (((uintE) edgeStart[i+3]) << 15)  + (((uintE) edgeStart[i+4]) << 23);
	  if(w & 0x80) weight = -weight;
	  i+=5;
	  if (!t.srcTarg(source, edge, weight, edgesRead++)) {
	    return;
	  }
	}
	break;
      case 5:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + startEdge;
	  startEdge = edge;
	  uintE w = edgeStart[i+2]; //highest bit is sign bit
	  intE weight = (w & 0x7f) + (((uintE) edgeStart[i+3]) << 7) + (((uintE) edgeStart[i+4]) << 15)  + (((uintE) edgeStart[i+5]) << 23);
	  if(w & 0x80) weight = -weight;
	  i+=6;
	  if (!t.srcTarg(source, edge, weight, edgesRead++)) {
	    return;
	  }
	}
	break;
      case 6:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + (((uintE) edgeStart[i+2]) << 16) + startEdge;
	  startEdge = edge;
	  uintE w = edgeStart[i+3]; //highest bit is sign bit
	  intE weight = (w & 0x7f) + (((uintE) edgeStart[i+4]) << 7) + (((uintE) edgeStart[i+5]) << 15)  + (((uintE) edgeStart[i+6]) << 23);
	  if(w & 0x80) weight = -weight;
	  i+=7;
	  if (!t.srcTarg(source, edge, weight, edgesRead++)) {
	    return;
	  }
	}
      default:
	for(uint j = 0; j < runlength; j++) {
	  uintE edge = (uintE) edgeStart[i] + (((uintE) edgeStart[i+1]) << 8) + (((uintE) edgeStart[i+2]) << 16) + (((uintE) edgeStart[i+3]) << 24) + startEdge;
	  startEdge = edge;
	  uintE w = edgeStart[i+4]; //highest bit is sign bit
	  intE weight = (w & 0x7f) + (((uintE) edgeStart[i+5]) << 7) + (((uintE) edgeStart[i+6]) << 15)  + (((uintE) edgeStart[i+7]) << 23);
	  if(w & 0x80) weight = -weight;
	  i+=8;
	  if (!t.srcTarg(source, edge, weight, edgesRead++)) {
	    return;
	  }
	}
      }
    }
  }
}

long compressWeightedEdges(uchar *start, long curOffset, intEPair* savedEdges, uintE edgeI, int numBytes, int numBytesWeight, uintT runlength) {
  //header
  //use 3 bits for info on bytes needed
  uchar header = numBytes - 1;
  if(numBytesWeight == 4) header |= 4;
  header |= ((runlength-1) << 3);  //use 5 bits for run length
  start[curOffset++] = header;
  int bytesUsed = 0;

  for(int i=0;i<runlength;i++) {
    uintE e = savedEdges[edgeI+i].first - savedEdges[edgeI+i-1].first;
    for(int j=0; j<numBytes; j++) {
      uchar curByte = e & 0xff;
      e = e >> 8;
      start[curOffset++] = curByte;
      bytesUsed++;
    }
    intE w = savedEdges[edgeI+i].second;
    uintE wMag = abs(w);
    uchar curByte = wMag & 0x7f;

    wMag = wMag >> 7;
    if(w < 0) start[curOffset++] = curByte | 0x80;
    else start[curOffset++] = curByte;
    bytesUsed++;
    for(int j=1; j<numBytesWeight; j++) {
      curByte = wMag & 0xff;
      wMag = wMag >> 8;
      start[curOffset++] = curByte;
      bytesUsed++;
    }
  }
  return curOffset;
}

long sequentialCompressWeightedEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, intEPair *savedEdges) {
  if (degree > 0) {
    currentOffset = compressFirstEdge(edgeArray, currentOffset,
                                       vertexNum, savedEdges[0].first);

    currentOffset = compressFirstEdge(edgeArray, currentOffset,
                                       0, savedEdges[0].second);
    if (degree == 1) return currentOffset;

    uintE edgeI = 1;
    uintT runlength = 0;
    int numBytes = 0, numBytesWeight = 0;
    while(1) {
      uintE difference = savedEdges[edgeI+runlength].first -
	savedEdges[edgeI+runlength - 1].first;
      intE weight = savedEdges[edgeI+runlength].second;
      if(difference < ONE_BYTE && numBytesSigned(weight) == 1) {
	if(!numBytes) {numBytes = 1; numBytesWeight = 1; runlength++;}
	else if(numBytes == 1 && numBytesWeight == 1) runlength++;
	else {
	  //encode
	  currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }
      else if(difference < ONE_BYTE && numBytesSigned(weight) == 4) {
	if(!numBytes) {numBytes = 1; numBytesWeight = 4; runlength++;}
	else if(numBytes == 1 && numBytesWeight == 4) runlength++;
	else {
	  //encode
	  currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }
      else if(difference < TWO_BYTES && numBytesSigned(weight) == 1) {
	if(!numBytes) {numBytes = 2; numBytesWeight = 1; runlength++;}
	else if(numBytes == 2 && numBytesWeight == 1) runlength++;
	else {
	  //encode
	  currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }
      else if(difference < TWO_BYTES && numBytesSigned(weight) == 4) {
	if(!numBytes) {numBytes = 2; numBytesWeight = 4; runlength++;}
	else if(numBytes == 2 && numBytesWeight == 4) runlength++;
	else {
	  //encode
	  currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }
      else if(difference < THREE_BYTES && numBytesSigned(weight) == 1) {
	if(!numBytes) {numBytes = 3; numBytesWeight = 1; runlength++;}
	else if(numBytes == 3 && numBytesWeight == 1) runlength++;
	else {
	  //encode
	  currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }
      else if(difference < THREE_BYTES && numBytesSigned(weight) == 4) {
	if(!numBytes) {numBytes = 3; numBytesWeight = 4; runlength++;}
	else if(numBytes == 3 && numBytesWeight == 4) runlength++;
	else {
	  //encode
	  currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }
      else if(numBytesSigned(weight) == 1) {
	if(!numBytes) {numBytes = 4; numBytesWeight = 1; runlength++;}
	else if(numBytes == 4 && numBytesWeight == 1) runlength++;
	else {
	  //encode
	  currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }
      else {
	if(!numBytes) {numBytes = 4; numBytesWeight = 4; runlength++;}
	else if(numBytes == 4 && numBytesWeight == 4) runlength++;
	else {
	  //encode
	  currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	  edgeI += runlength;
	  runlength = numBytes = 0;
	}
      }

      if(runlength == 32) {
	currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	edgeI += runlength;
	runlength = numBytes = 0;
      }
      if(runlength + edgeI == degree) {
	currentOffset = compressWeightedEdges(edgeArray, currentOffset, savedEdges, edgeI, numBytes, numBytesWeight, runlength);
	break;
      }
    }
  }
  return currentOffset;
}

uchar *parallelCompressWeightedEdges(intEPair *edges, uintT *offsets, long n, long m, uintE* Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE*, n);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n+1);
  {parallel_for(long i=0; i<n; i++) {
    charsUsedArr[i] = 4*(ceil((Degrees[i] * 9) / 8) + 4); //to change
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
