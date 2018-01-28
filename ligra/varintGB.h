#ifndef STREAMVBYTE_H
#define STREAMVBYTE_H

#include "parallel.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>


typedef unsigned char uchar;

// decode weights
inline intE eatWeight(uchar controlKey, long & dOffset, intT shift, long controlOffset, uchar* start){
  uintT checkCode = (controlKey >> shift) & 0x3;
  uintE edgeRead = 0;
  bool signBit;
  switch(checkCode) {
    // 1 byte
  case 0:
    edgeRead = start[dOffset];
    signBit =(((edgeRead) & 0x80) >> 7);
    edgeRead &= 0x7F;
    dOffset += 1;
    break;
    // 2 bytes
  case 1:
    memcpy(&edgeRead, start + dOffset, 2);
    signBit = (((1 << 15) & edgeRead) >> 15);
    edgeRead = edgeRead & 0x7FFF; 
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
    memcpy(&edgeRead, start + dOffset, 3);
    signBit = (((edgeRead) & (1 <<23)) >> 23);
    edgeRead = edgeRead & 0x7FFFFF;
    dOffset += 3;
    break;
    // 4 bytes
  default:
    memcpy(&edgeRead, start+dOffset, 4);
    signBit = (((edgeRead) & (1 << 31)) >> 31);
    edgeRead = (edgeRead) & 0x7FFFFFFF; 
    dOffset += 4;
  }

  return (signBit) ? -edgeRead : edgeRead;
}

// decode first edge
inline intE eatFirstEdge(uchar controlKey, uintE source, long &dOffset, long controlOffset, uchar* start){
  // check the two bit code in control stream
  uintT checkCode = (controlKey) & 0x3;
  uchar signBit;
  uintE edgeRead = 0;
  // 1 byte
  switch(checkCode) {
  case 0:
    edgeRead = (uintE)(start[dOffset] & 0x7f);
    // check sign bit and then get rid of it from actual value 
    signBit = start[dOffset] & 0x80;
    dOffset += 1;
    break;
    // 2 bytes
  case 1:
    edgeRead = (uintE)(start[dOffset]) + ((uintE)(start[dOffset+1] & 0x7f) << 8);
    signBit = 0x80 & start[dOffset+1];
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
    edgeRead = (uintE)(start[dOffset]) + ((uintE)start[dOffset+1] << 8) + ((uintE)(start[dOffset+2] & 0x7f) << 16);
    signBit = 0x80 & start[dOffset+2];
    dOffset += 3;
    break;
    // 4 bytes
  default:
    edgeRead = (uintE)(start[dOffset]) + ((uintE)start[dOffset+1] << 8) + ((uintE)start[dOffset+2] << 16) + ((uintE)(start[dOffset+3] & 0x7f) << 24);
    signBit = start[dOffset+3] & 0x80;
    dOffset += 4;
  }
  return (signBit) ? source - edgeRead : source + edgeRead;
}

// decode remaining edges
inline uintE eatEdge(uchar controlKey, long &dOffset, long controlOffset, uchar* start){
  // check two bit code in control stream
  //uintT checkCode = (controlKey >> shift) & 0x3;
  uintE edgeRead;

  switch(controlKey & 0x3) {
    // 1 byte
  case 0:
    edgeRead = (uintE)start[dOffset++]; 
    break;
    // 2 bytes
  case 1:
    edgeRead = (uintE)(start[dOffset]) + ((uintE)start[dOffset+1] << 8);
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
    edgeRead = (uintE)(start[dOffset]) + ((uintE)start[dOffset+1] << 8) + ((uintE)start[dOffset+2] << 16);
    dOffset += 3;
    break;
    // 4 bytes
  default:
    edgeRead = (uintE)(start[dOffset]) + ((uintE)start[dOffset+1] << 8) + ((uintE)start[dOffset+2] << 16) + ((uintE)start[dOffset+3] << 24);
    dOffset += 4;
  }
  return edgeRead;
}

uchar compressFirstEdge(uchar* &start, long controlOffset, long dataOffset, uintE source, uintE target){	
  intE preCompress = (intE) target - source;
  uintE toCompress = abs(preCompress);
  uintT signBit = (preCompress < 0) ? 1:0;
  uchar code; 
  uintE *toCompressPtr = &toCompress;
  // check how many bytes is required to store the data and a sign bit (MSB)
  if(toCompress < (1 << 7)) {
    // concatenate sign bit with data and store
    toCompress |= (signBit << 7);
    memcpy(&start[dataOffset], toCompressPtr, 1);
    code = 0;
  }
  else if(toCompress < (1 << 15)){
    toCompress |= (signBit << 15);
    memcpy(&start[dataOffset], toCompressPtr, 2);
    code = 1;
  }
  else if(toCompress < (1 << 23)){
    toCompress |= (signBit << 23);
    memcpy(&start[dataOffset], toCompressPtr, 3);
    code = 2;
  }
  else{
    toCompress |= (signBit << 31);
    memcpy(&start[dataOffset], toCompressPtr, 4);
    code = 3;
  }
  return code;
}

// store compressed data
uchar encode_data(uintE d, long dataCurrentOffset, uchar* &start){
  uchar code;
  uintE* diffPtr = &d;
  if(d < (1 << 8)){
    memcpy(&start[dataCurrentOffset], diffPtr, 1);
    code = 0;
  }
  else if( d < (1 << 16)){
    memcpy(&start[dataCurrentOffset], diffPtr, 2);
    code = 1;
  }
  else if (d < (1 << 24)){
    memcpy(&start[dataCurrentOffset], diffPtr, 3);
    code = 2;
  }
  else{
    memcpy(&start[dataCurrentOffset], diffPtr, 4);	
    code = 3;
  }
	
  return code;
}

typedef pair<uintE,intE> intEPair;

long compressWeightedEdge(uchar *start, long currentOffset, intEPair *savedEdges, uintT key, long dataCurrentOffset, uintT degree){
  uintT code = 0;
  uintT storeKey = key;
  long storeDOffset = dataCurrentOffset;
  long storeCurrentOffset = currentOffset;
  // shift 0 and shift 2 used by first edge and first weight
  uintT shift = 4;
  uintE difference;

  for (uintT edgeI = 1; edgeI < degree; edgeI++){
    // differential encoding
    difference = savedEdges[edgeI].first - savedEdges[edgeI - 1].first;
    // if finished filling byte in control stream, reset variables and increment control pointer
    if(shift == 8){
      shift = 0;
      start[storeCurrentOffset] = storeKey;
      storeCurrentOffset+= sizeof(uchar);
      storeKey = 0;
    }		
    // encode and store data in the data stream
    code = encode_data(difference, storeDOffset, start);
    storeDOffset += (code + 1)*sizeof(uchar); 
    // add code to current byte in control stream
    storeKey |= (code << shift);
    shift += 2;
    // must check again if at end of the byte in control stream
    if(shift == 8){
      shift = 0;
      start[storeCurrentOffset] = storeKey;
      storeCurrentOffset+= sizeof(uchar);
      storeKey = 0;
    }		
    // compress the weights in same manner as first edge (ie with sign bit)
    code = compressFirstEdge(start, storeCurrentOffset, storeDOffset, 0, savedEdges[edgeI].second);
    storeDOffset += (code + 1)*sizeof(uchar); 
    storeKey |= (code << shift);
    shift +=2;
			
  }
  return storeDOffset;
}

long compressEdge(uchar* &start, long currentOffset, uintE *savedEdges, long dataCurrentOffset, uintT degree){
  uchar code = 0;
  uchar storeKey = 0;
  long storeDOffset = dataCurrentOffset;
  long storeCurrentOffset = currentOffset;
 uintT block_four = degree/4;
 uintT remaining = degree - 4*block_four; 
  uintE difference;
  for (uintT edgeI = 1; edgeI < block_four; edgeI++){
    // differential encoding
    uintT index = 4*edgeI;
    difference = savedEdges[index] - savedEdges[index - 1];
    code = encode_data(difference, storeDOffset, start);
    storeDOffset += (code + 1); 
    storeKey |= (code);   

    difference = savedEdges[index+1] - savedEdges[index];
    code = encode_data(difference, storeDOffset, start);
    storeDOffset += (code + 1); 
    storeKey |= (code << 2);
     
    difference = savedEdges[index+2] - savedEdges[index + 1];
    code = encode_data(difference, storeDOffset, start);
    storeDOffset += (code + 1); 
    storeKey |= (code << 4);
    
    difference = savedEdges[index+3] - savedEdges[index +2];
    code = encode_data(difference, storeDOffset, start);
    storeDOffset += (code + 1); 
    storeKey |= (code << 6);
	// store header byte      
   start[storeCurrentOffset] = storeKey;
     storeCurrentOffset = storeDOffset;
   storeDOffset++; 
   storeKey = 0; 
 }

uintE shift = 0;
uintT index = 4*block_four;
for(int i=0; i < remaining; i++){
    difference = savedEdges[index] - savedEdges[index - 1];
    code = encode_data(difference, storeDOffset, start);
    storeDOffset += (code + 1); 
    storeKey |= (code << shift);
    index++;   
    shift+=2;
}
if(remaining>0){
   start[storeCurrentOffset] = storeKey;
  }
else{
	storeDOffset--;
}
   return storeDOffset;
}

//finish encoding remainders
long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, uintE *savedEdges){
  if(degree > 0){

     long dataCurrentOffset = currentOffset + 1;
     uchar key = compressFirstEdge(edgeArray, currentOffset, dataCurrentOffset, vertexNum, savedEdges[0]);
    // offset data pointer by amount of space required by first edge
    dataCurrentOffset += (1 + key);
    if(degree == 1){
      edgeArray[currentOffset] = key;
      return dataCurrentOffset;
     }
    uchar num_in_first_block = (degree > 4) ? 4 : degree; 
   uintT shift = 2;   
  uintE difference =0;
uchar code = 0;
// compress rest of edges in first block of four
 for (uchar i = 1; i < num_in_first_block; i++){
	difference = savedEdges[i] - savedEdges[i-1];
	code = encode_data(difference, dataCurrentOffset, edgeArray);
    	dataCurrentOffset += (code + 1); 
    	key |= (code << shift);
    	shift +=2;
  }
  edgeArray[currentOffset] = key;
if(degree > 4){  
  currentOffset = dataCurrentOffset;
  dataCurrentOffset++;
  dataCurrentOffset= compressEdge(edgeArray, currentOffset, savedEdges, dataCurrentOffset, degree);
 }  

  return dataCurrentOffset;
  }
  else{
    return currentOffset;
  }
}

uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m, uintE* Degrees){
  // compresses edges for vertices in parallel & prints compression stats
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")"  << endl;
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n+1);
  for(long i=0;i<n;i++){
    long count = 0;
    if (Degrees[i] > 0){
      // bytes needed for headers -- one per every four edges
	count = (Degrees[i]+3)/4;
      uintE* edgePtr = edges+offsets[i];
      uintE edge = *edgePtr;	
	// compute bytes for first edge
      intE preCompress = (intE)(edge - i);
      uintE toCompress = abs(preCompress);
      if(toCompress < (1 << 7)){
	count++;
      }
      else if(toCompress < (1 << 15)){
	count += 2;
      }
      else if(toCompress < (1 << 23)){
	count += 3;
      }
      else{ 
	count += 4;
      }

	// use to find diff -- starts at first edge value
      uintE prevEdge = *edgePtr;	
      uintE difference;
	// find diffs and compute bytes needed for rest of edges
      for(long j=1;j<Degrees[i];j++){
	difference  = *(edgePtr+j) - *(edgePtr+j-1);
	if(difference < (1<<8)){
	  count++;
	}	
	else if(difference < (1 << 16)){
	  count += 2;
	}
	else if(difference < (1 << 24)){
	  count += 3;
	}
	else {
	  count +=4;
	}
      }
    }
    charsUsedArr[i] = count;
  }
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  cout << "totalSpace " << totalSpace << endl;
  compressionStarts[n] = totalSpace;
  uchar *finalArr = newA(uchar, totalSpace);
 for(long i=0;i<n;i++){

      long charsUsed = sequentialCompressEdgeSet((uchar *)(finalArr+compressionStarts[i]), 0, Degrees[i], i, edges + offsets[i]);
	 offsets[i] = compressionStarts[i];
  }
    offsets[n] = totalSpace;
  free(charsUsedArr);
	
  cout << "total space requested is: " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace*8 / (float)m;
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m*4) << endl;
  return ((uintE *)finalArr);
}

template <class T>
inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
  size_t edgesRead = 0;
  if(degree > 0) {
    long currentOffset = 0;
    long dataOffset = currentOffset+1;
    uintT key = edgeStart[currentOffset];
    uintE startEdge = eatFirstEdge(key, source, dataOffset, currentOffset, edgeStart);
    if(!t.srcTarg(source,startEdge,edgesRead)){
      return;
    }
    size_t num_in_first_block = (degree >4) ? 4 : degree;
    uintT shift = 2;
    edgesRead++;
   for(uchar i = 1; i < num_in_first_block; i++){
	uintE edgeRead = eatEdge((key >> shift), dataOffset, currentOffset, edgeStart);
	uintE edge = edgeRead + startEdge; 
	startEdge = edge;
	if(!t.srcTarg(source, edge, edgesRead++)){
		return;
	}
	shift+=2;		
    }
if(degree > 4){
	long block_four = degree >> 2;
	long remaining = degree - 4*block_four;
	currentOffset = dataOffset;
	dataOffset++;
	for(long i=1; i < block_four; i++){
		key = edgeStart[currentOffset];
		uintE edgeRead = eatEdge(key, dataOffset,currentOffset, edgeStart);
		uintE edge = edgeRead + startEdge; 
		startEdge = edge;
		if(!t.srcTarg(source, edge, edgesRead++)){
			return;
		}
		edgeRead = eatEdge((key >> 2), dataOffset, currentOffset, edgeStart);
		edge = edgeRead + startEdge; 
		startEdge = edge;
		if(!t.srcTarg(source, edge, edgesRead++)){
			return;
		}
		edgeRead = eatEdge((key >> 4), dataOffset, currentOffset, edgeStart);
		edge = edgeRead + startEdge; 
		startEdge = edge;
		if(!t.srcTarg(source, edge, edgesRead++)){
			return;
		}
		edgeRead = eatEdge((key >> 6), dataOffset, currentOffset, edgeStart);
		edge = edgeRead + startEdge; 
		startEdge = edge;
		if(!t.srcTarg(source, edge, edgesRead++)){
			return;
		}
 		currentOffset = dataOffset;
		dataOffset++;
	}
	shift = 0;
	if(remaining > 0){
		key = edgeStart[currentOffset];
	}
	for(int i=0; i < remaining; i++){
		uintE edgeRead = eatEdge((key >> shift), dataOffset, currentOffset, edgeStart);
		uintE edge = edgeRead + startEdge; 
		startEdge = edge;
		if(!t.srcTarg(source,edge,edgesRead++)){
			return;
		}
		shift+=2;

	}
	}
	
  }
};

template <class T>
inline void decodeWgh(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
  size_t edgesRead = 0;
  if (degree > 0){
    // space needed for control stream
    uchar controlLength = (2*degree+3)/4;
    // set data pointer at beginning of data stream
    long currentOffset = 0; 
    long dataOffset = currentOffset + controlLength;
    uchar key = edgeStart[currentOffset];
    uintE startEdge = eatFirstEdge(key, source, dataOffset, currentOffset, edgeStart);	
    intE weight = eatWeight(key, dataOffset, 2, currentOffset, edgeStart);	
    if (!t.srcTarg(source, startEdge, weight, edgesRead)){
      return;
    }
    uintT shift = 4;
    uintE edge = startEdge;
    for (edgesRead = 1; edgesRead < degree; edgesRead++){
      // if finished reading byte in control stream, increment and reset shift
      if(shift == 8){
	currentOffset++;
	key= edgeStart[currentOffset];
	shift = 0;
      }
      uintE edgeRead = eatEdge((key >> shift), dataOffset, currentOffset, edgeStart);
      edge = edge + edgeRead;
      shift += 2;

      if(shift == 8){
	currentOffset++;
	key = edgeStart[currentOffset];
	shift = 0;
      }
      intE weight = eatWeight(key, dataOffset, shift, currentOffset, edgeStart);
      shift += 2;
      if (!t.srcTarg(source, edge, weight, edgesRead)){
	break;
      }
		
    }
  }
}


long sequentialCompressWeightedEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, intEPair *savedEdges){
  if (degree > 0){
    // space needed by control stream
    uintT controlLength = (2*degree + 3)/4;
    long dataCurrentOffset = currentOffset + controlLength;
    uintT key = compressFirstEdge(edgeArray, currentOffset, dataCurrentOffset, vertexNum, savedEdges[0].first);
    dataCurrentOffset += (1 + key)*sizeof(uchar); 
    // compress weight of first edge
    uintT temp_key = compressFirstEdge(edgeArray, currentOffset, dataCurrentOffset, 0, savedEdges[0].second);
    dataCurrentOffset += (temp_key + 1)*sizeof(uchar);
    key |= temp_key << 2;
    if(degree == 1){
      edgeArray[currentOffset] = key;
      return dataCurrentOffset;

    }		
    return compressWeightedEdge(edgeArray, currentOffset, savedEdges, key, dataCurrentOffset, degree);
  }
  else{
    return currentOffset;
  }

}


uchar *parallelCompressWeightedEdges(intEPair *edges, uintT *offsets, long n, long m, uintE* Degrees){
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE*, n);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n+1);
  {parallel_for(long i=0; i<n; i++){
      //		charsUsedArr[i] = (2*degrees[i]+3)/4 + 8*degrees[i];
      charsUsedArr[i] = 10*Degrees[i];
    }}
  long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
  uintE* iEdges = newA(uintE, toAlloc);
  {parallel_for(long i = 0; i < n; i++){
      edgePts[i] = iEdges + charsUsedArr[i];
      long charsUsed = sequentialCompressWeightedEdgeSet((uchar *)(iEdges + charsUsedArr[i]), 0, Degrees[i], i, edges + offsets[i]);
      charsUsedArr[i] = charsUsed;
    }}
	
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;
  free(charsUsedArr);
	
  uchar *finalArr = newA(uchar, totalSpace);
  cout << "total space requested is : " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace*8 / (float)m;
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;

  {parallel_for(long i=0; i < n; i++){
      long o = compressionStarts[i];
      memcpy(finalArr + o, (uchar *)(edgePts[i]), compressionStarts[i+1] - o);
      offsets[i] = o;
    }}
  offsets[n] = totalSpace;
  free(iEdges);	
  free(edgePts);
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m*8) << endl;
  return finalArr;
}
#endif
