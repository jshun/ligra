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
  //	uchar fb = start[controlOffset];
  // check two bit code in control stream
  uintT checkCode = (controlKey >> shift) & 0x3;
  uintE edgeRead = 0;
  bool signBit;
  switch(checkCode) {
    // 1 byte
  case 0:
    edgeRead = start[dOffset];
    // check sign bit and then get rid of it from actual value 
    signBit =(((edgeRead) & 0x80) >> 7);
    edgeRead &= 0x7F;
    // incrememnt the offset to data by 1 byte
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
  //cout << "in eatFirstEdge case " << checkCode << endl;
  // 1 byte
  switch(checkCode) {
  case 0:
    edgeRead = start[dOffset] & 0x7f;
    // check sign bit and then get rid of it from actual value 
  //cout << "in eatFirstEdge case " << checkCode << endl;
    signBit = start[dOffset] & 0x80;
    dOffset += 1;
    break;
    // 2 bytes
  case 1:
    edgeRead = start[dOffset] + ((start[dOffset+1] & 0x7f) << 8);
    //memcpy(&edgeRead, &start[dOffset], 2);
  //cout << "in eatFirstEdge case " << checkCode << endl;
    signBit = 0x80 & start[dOffset+1];
    //signBit = 0x8000 & edgeRead;
    //edgeRead = edgeRead & 0x7FFF; 
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
    //memcpy(&edgeRead, &start[dOffset], 3);
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + ((start[dOffset+2] & 0x7f) << 16);
  //cout << "in eatFirstEdge case " << checkCode << endl;
    signBit = 0x80 & start[dOffset+2];
    //signBit = 0x800000 & edgeRead;
    //edgeRead = edgeRead & 0x7FFFFF;
    dOffset += 3;
    break;
    // 4 bytes
  default:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + ((start[dOffset+3] & 0x7f) << 24);
    //memcpy(&edgeRead, &start[dOffset], 4);
  //cout << "in eatFirstEdge case " << checkCode << endl;
    signBit = start[dOffset+3] & 0x80;
    //signBit = edgeRead & 0x80000000;
    //edgeRead = (edgeRead) & 0x7FFFFFFF; 
    dOffset += 4;
  }
//cout << "ending first edge function " << endl;
  return (signBit) ? source - edgeRead : source + edgeRead;
}

// decode remaining edges
inline uintE eatEdge(uchar controlKey, long &dOffset, intT shift, long controlOffset, uchar* start){
  // check two bit code in control stream
  uintT checkCode = (controlKey >> shift) & 0x3;
  uintE edgeRead;

  switch(checkCode) {
    // 1 byte
  case 0:
    edgeRead = start[dOffset++]; 
//  cout << "in eatEdge case " << checkCode << endl;
    //dOffset += 1;
    break;
    // 2 bytes
  case 1:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8);
  //cout << "in eatEdge case " << checkCode << endl;
    //memcpy(&edgeRead, &start[dOffset], 2);
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
  //cout << "in eatEdge case " << checkCode << endl;
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
    //memcpy(&edgeRead, &start[dOffset], 3);	
    dOffset += 3;
    break;
    // 4 bytes
  default:
  //cout << "in eatEdge case " << checkCode << endl;
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
    //memcpy(&edgeRead, &start[dOffset], 4);
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
  //  cout << "seq 1 1" << endl;
  }
  else if(toCompress < (1 << 15)){
    toCompress |= (signBit << 15);
    memcpy(&start[dataOffset], toCompressPtr, 2);
    code = 1;
//    cout << "seq 1 2" << endl;
  }
  else if(toCompress < (1 << 23)){
    toCompress |= (signBit << 23);
    memcpy(&start[dataOffset], toCompressPtr, 3);
    code = 2;
  //  cout << "seq 1 3" << endl;
  }
  else{
    toCompress |= (signBit << 31);
    memcpy(&start[dataOffset], toCompressPtr, 4);
    code = 3;
    //cout << "seq 1 4" << endl;
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
  // shift 0 used by first edge
 uintT block_four = degree/4;
 uintT remaining = degree - 4*block_four; 
  uintE difference;
   //uchar index = 4;
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
      
//   cout << "storeKey " << (uintT) storeKey << endl;
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
  //cout << "remaining storeKey : " << (uintT) storeKey << endl;
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
   //cout << "current, data Offset " << currentOffset << " " << dataCurrentOffset << endl; 
    uchar key = compressFirstEdge(edgeArray, currentOffset, dataCurrentOffset, vertexNum, savedEdges[0]);
    // offset data pointer by amount of space required by first edge
    dataCurrentOffset += (1 + key);
    if(degree == 1){
      edgeArray[currentOffset] = key;
      return dataCurrentOffset;
     }
    // scalar version: compress the rest of the edges
   uchar num_in_first_block = (degree > 4) ? 4 : degree; 
   uintT shift = 2;   
  uintE difference =0;
	uchar code = 0;
 for (uchar i = 1; i < num_in_first_block; i++){
	difference = savedEdges[i] - savedEdges[i-1];
	code = encode_data(difference, dataCurrentOffset, edgeArray);
    	dataCurrentOffset += (code + 1); 
    	key |= (code << shift);
    	shift +=2;
  }
 // cout << "key " << (uintT) key << endl;
  edgeArray[currentOffset] = key;
// compress remaining edges not in first block if still remaining edges to compress 
if(degree > 4){  
  currentOffset = dataCurrentOffset;
  dataCurrentOffset++;
  dataCurrentOffset= compressEdge(edgeArray, currentOffset, savedEdges, dataCurrentOffset, degree);
 }  

  return dataCurrentOffset;
    //		return (dataCurrentOffset+1);
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
  //long count;
// uint test_counter2 = 0;
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
  //  test_counter2 += count;
  }
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  cout << "totalSpace " << totalSpace << endl;
  cout << "alloc test counter 2 " << test_counter2 << endl;
  compressionStarts[n] = totalSpace;
  uchar *finalArr = newA(uchar, totalSpace);
 // debugging	
// long counter_test = 0;
 for(long i=0;i<n;i++){

      long charsUsed = sequentialCompressEdgeSet((uchar *)(finalArr+compressionStarts[i]), 0, Degrees[i], i, edges + offsets[i]);
  //     counter_test += charsUsed;
	 offsets[i] = compressionStarts[i];
  }
  //cout << "counter test " << counter_test << endl;
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
    //cout << "accessing1 " << endl;
    uintT key = edgeStart[currentOffset];
    //cout << "finished1" << endl;
    uintE startEdge = eatFirstEdge(key, source, dataOffset, currentOffset, edgeStart);
    if(!t.srcTarg(source,startEdge,edgesRead)){
      return;
    }
    size_t num_in_first_block = (degree >4) ? 4 : degree;
    uintT shift = 2;
//    uintE edge = startEdge; 
//    uintE edgeRead = 0;
    edgesRead++;
  // cout << "degree, numinfirstblock : " << degree << " " << (uintT) num_in_first_block << endl;
   for(size_t i = 1; i < num_in_first_block; i++){
	uintE edgeRead = eatEdge(key, dataOffset, shift, currentOffset, edgeStart);
	uintE edge = edgeRead + startEdge; 
	startEdge = edge;
	if(!t.srcTarg(source, edge, edgesRead++)){
	//	cout << "first for loop in break statement.." << endl;
		return;
	}
	//edgeRead =0;
	//edgesRead++;
	shift+=2;		
    }
if(degree > 4){
	long block_four = degree/4;
	long remaining = degree - 4*block_four;
//	cout << "block_four: " << block_four << endl;
	currentOffset = dataOffset;
	dataOffset++;
//	cout << "accessing2 " << endl;
//		cout << "finishe 2 " << endl;
//	cout << "deg4 current, data Offset " << currentOffset << " " << dataOffset << endl;
	for(long i=1; i < block_four; i++){
		key = edgeStart[currentOffset];
//		cout << "key : " << key << endl;
//		cout << "blockfour1 " << endl;	
		uintE edgeRead = eatEdge(key, dataOffset, 0, currentOffset, edgeStart);
		uintE edge = edgeRead + startEdge; 
		startEdge = edge;
	//	edgesRead++;
		if(!t.srcTarg(source, edge, edgesRead++)){
	//		cout << "block four break statement" << endl;
			return;
		}
//		cout << "blockfour2" << endl;
		edgeRead = eatEdge(key, dataOffset, 2, currentOffset, edgeStart);
		edge = edgeRead + startEdge; 
		startEdge = edge;
		if(!t.srcTarg(source, edge, edgesRead++)){
	//		cout << "block four break statement" << endl;
			return;
		}
//		cout << "blockfour3" << endl;
		edgeRead = eatEdge(key, dataOffset, 4, currentOffset, edgeStart);
		edge = edgeRead + startEdge; 
		startEdge = edge;
		if(!t.srcTarg(source, edge, edgesRead++)){
	//		cout << "block four break statement" << endl;
			return;
		}
//		cout << "blockfour4" << endl;
		edgeRead = eatEdge(key, dataOffset, 6, currentOffset, edgeStart);
		edge = edgeRead + startEdge; 
		startEdge = edge;
		if(!t.srcTarg(source, edge, edgesRead++)){
	//		cout << "block four break statement" << endl;
			return;
		}
 //		cout << "ending blcokfour" << endl;
		currentOffset = dataOffset;
		dataOffset++;
		
//		key = edgeStart[currentOffset];

	//	cout << "forloop current, data Offfest " << currentOffset << " " << dataOffset << endl;
	}
//	cout << "endblcokrfour" << endl;
//	cout << "after four for loop" << endl;
	shift = 0;
//	cout << "degree: " << degree << " new_degree " << degree_new << " remaining " << remaining << " block_four " << block_four << endl;
//	cout << "before remaining for loop" << endl;
	if(remaining > 0){
		key = edgeStart[currentOffset];
	}
	for(int i=0; i < remaining; i++){
		uintE edgeRead = eatEdge(key, dataOffset, shift, currentOffset, edgeStart);
		uintE edge = edgeRead + startEdge; 
//		cout << "inside for loop " << endl;
		startEdge = edge;
		if(!t.srcTarg(source,edge,edgesRead++)){
	//		cout << "remaining break statement" << endl;
			return;
		}
		shift+=2;

	}
//	cout << "edgesRead, degree : " << edgesRead << " " << degree << endl;
//	cout << "after remaining for loop" << endl;
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
      uintE edgeRead = eatEdge(key, dataOffset, shift, currentOffset, edgeStart);
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
