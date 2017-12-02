#ifndef BITPACKING_H
#define BITPACKING_H

#include "parallel.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>

// to use this compression scheme, compile with STREAMVBYTE=1 make -j

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
inline intE eatFirstEdge(uchar controlKey, uintE source, long & dOffset, long controlOffset, uchar* start){
  // check the two bit code in control stream
  uintT checkCode = (controlKey) & 0x3;
  bool signBit;
  uintE edgeRead;
  // 1 byte
  switch(checkCode) {
  case 0:
    edgeRead = start[dOffset] & 0x7f;
    // check sign bit and then get rid of it from actual value 
    signBit = start[dOffset] & 0x80;
    dOffset += 1;
    break;
    // 2 bytes
  case 1:
    edgeRead = start[dOffset] + ((start[dOffset+1] & 0x7f) << 8);
    signBit = 0x80 & start[dOffset+1];
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + ((start[dOffset+2] & 0x7f) << 16);
    signBit = 0x80 & start[dOffset+2];
    dOffset += 3;
    break;
    // 4 bytes
  default:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + ((start[dOffset+3] & 0x7f) << 24);
    signBit = start[dOffset+3] & 0x80;
    dOffset += 4;
  }
  return (signBit) ? source - edgeRead : source + edgeRead;
}

// decode remaining edges
inline uintE eatEdge(uchar controlKey, long & dOffset, intT shift, long controlOffset, uchar* start){
  // check two bit code in control stream
  uintT checkCode = (controlKey >> shift) & 0x3;
  uintE edgeRead;
  switch(checkCode) {
    // 1 byte
  case 0:
    edgeRead = start[dOffset++]; 
    break;
    // 2 bytes
  case 1:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8);
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
    dOffset += 3;
    break;
    // 4 bytes
  default:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
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

long compressEdge(uchar* &start, long currentOffset, uintE *savedEdges, uchar key, long dataCurrentOffset, uintT degree){
	uintT code = 0;
	uchar storeKey = key;
	long storeDOffset = dataCurrentOffset;
	long storeCurrentOffset = currentOffset;
	// shift 0 used by first edge
	uintT shift = 2;
	uintE difference;
	for (uintT edgeI = 1; edgeI < degree; edgeI++){
			// differential encoding
			difference = savedEdges[edgeI] - savedEdges[edgeI - 1];
			// check if used full byte in control stream; if yes, increment and reset vars
			if(shift == 8){
				shift = 0;
				start[storeCurrentOffset] = storeKey;
				storeCurrentOffset++;
				storeKey = 0;
			}		
			// encode and store data in data stream
			code = encode_data(difference, storeDOffset, start);
			storeDOffset += (code + 1); 
			storeKey |= (code << shift);
			shift +=2;
	}
	start[storeCurrentOffset] = storeKey;
	return storeDOffset;
}


/*

Calculate first edge -- concatenate with sign bit
Calculate and store differences between other edges
Pass those values into function to calculate b
Calculate numbers per block
Store b in first byte of block
Call correct fastpack function to pack values 

*/

long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, uintE *savedEdges){
	uint max_number = 0;
	int temp=0;
	int block_size = 0;

	if (degree > 0){
		// figure out block size
		if(degree >= 128){
			block_size = 128;
		}
		else if(degree >= 64){
			block_size = 64;
		}
		else if(degree >= 32){
			block_size = 32;
		}
		else if(degree >= 16){
			block_size = 16;
		}
		else if(degree >= 8){
			block_size = 8;
		}
		else{
			block_size = 4;
		}

		int num_blocks = degree/block_size;
		int num_remaining = degree - block_size*num_blocks;
	
		// calculate bit width per block & use 	;
			uintE edge = savedEdges[0];
			bool first_edge = 1;
			uintE prevEdge = savedEdges[0];	
			uintE difference[degree];
			uintE b[num_blocks];
			uint k,j;
			//calculate first edge -- double for loop over number of blocks and size of blocks
			//calculate singles using continue bit or 4 num header?
		for(k=0; k < num_blocks; k++){	
			for(j=k;j<block_size+k;j++){
				if(first_edge){		
					intE preCompress = (intE)(edge - vertexNum);
					uintE toCompress = abs(preCompress);
					if(toCompress < (1 << 7)){
						temp = (1<<7) | toCompress;
						max_number = (max_number < temp) ? temp : max_number;
					}
					else if(toCompress < (1 << 15)){
						temp = (1 << 15) | toCompress; 
						max_number = (max_number < temp) ? temp : max_number;
					}
					else if(toCompress < (1 << 23)){
						temp = (1 << 23) | toCompress;
						max_number = (max_number < temp) ? temp : max_number;		
					}
					else{ 
						temp = (1 << 31) | toCompress;
						max_number = (max_number < temp) ? temp : max_number;		
					}
					difference[0] = toCompress;
					first_edge= 0;
				}
				else{
					edge = savedEdges[j];
					difference[j] = edge - prevEdge;
					if(difference[j] > max_number){
						max_number = difference[j];
					}
					prevEdge = edge;	
				}
			if(max_number < (1 << 8)){
				b[k] = 8;
			}
			else if(max_number < (1 << 16)){
				b[k]=16;
			}
			else if(max_number < (1 << 24)){
				b[k]=24;
			}
			else{
				b[k]=32;
			}
			}
		}
		// figure out how many bits needed for remaining edges	
		parallel_for(long i=0;i<num_blocks;i++){
			int index = i*(ceil(b[i]*block_size/32)+1);
			// insert header with bitwidth
			edgeArray[index] = b[i];
			// call function for correct bit number? --> pass in edgeArray, differences, index, block_size?
		}
		for(j; j< degree; j++){
		//use diff array & encode remaining elements	
		//somehow need to keep track of currentOffset value && return at the end
		}	
		}	
		return currentOffset;
}

uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m, uintE* Degrees){
	// compresses edges for vertices in parallel & prints compression stats
	cout << "paralleli compressing, (n,m) = (" << n << "," << m << ")"  << endl;
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
	//long count;

	// for each vertex, find the max number of bits needed to represent edges and use that as bit width
	parallel_for(long i=0;i<n;i++){
//	for(long i=0; i<n; i++){
	int max_number = 0;
	int temp=0;
	long count = 0;
	int block_size = 0;

	if (Degrees[i] > 0){
		// figure out block size
		if(Degrees[i] >= 128){
			block_size = 128;
		}
		else if(Degrees[i] >= 64){
			block_size = 64;
		}
		else if(Degrees[i] >= 32){
			block_size = 32;
		}
		else if(Degrees[i] >= 16){
			block_size = 16;
		}
		else if(Degrees[i] >= 8){
			block_size = 8;
		}
		else{
			block_size = 4;
		}

		int num_blocks = Degrees[i]/block_size;
		int num_remaining = Degrees[i] - block_size*num_blocks;
		//calculate number of bytes needed for headers (1 byte per block and 2 bits per remaining edge)
		count += num_blocks + (num_remaining+3)/4;
		
		// calculate bit width per block & use 	
			uintE* edgePtr = edges+offsets[i];
			uintE edge = *edgePtr;
			bool first_edge = 1;
			uintE prevEdge = *edgePtr;	
			uintE difference;
			long k,j;
			//calculate first edge -- double for loop over number of blocks and size of blocks
			//calculate singles using continue bit or 4 num header?
		for(k=0; k < num_blocks; k++){	
			int b = 0;
			for(j=k;j<block_size+k;j++){
				if(first_edge){		
					intE preCompress = (intE)(edge - i);
					uintE toCompress = abs(preCompress);
					if(toCompress < (1 << 7)){
						temp = (1<<7) | toCompress;
						max_number = (max_number < temp) ? temp : max_number;
					}
					else if(toCompress < (1 << 15)){
						temp = (1 << 15) | toCompress; 
						max_number = (max_number < temp) ? temp : max_number;
					}
					else if(toCompress < (1 << 23)){
						temp = (1 << 23) | toCompress;
						max_number = (max_number < temp) ? temp : max_number;		
					}
					else{ 
						temp = (1 << 31) | toCompress;
						max_number = (max_number < temp) ? temp : max_number;		
					}
					first_edge=0;
				}
				else{
					edge = *(edgePtr+j);
					difference = edge - prevEdge;
					if(difference > max_number){
						max_number = difference;
					}
					prevEdge = edge;	
				}
			if(max_number < (1 << 8)){
				b = 8;
			}
			else if(max_number < (1 << 16)){
				b=16;
			}
			else if(max_number < (1 << 24)){
				b=24;
			}
			else{
				b=32;
			}
				count += block_size*b;
			}
		}
		// figure out how many bits needed for remaining edges	
		

		for(j; j< Degrees[i]; j++){
				if(first_edge){		
				intE preCompress = (intE)(edge - i);
				uintE toCompress = abs(preCompress);
				if(toCompress < (1 << 7)){
					count += 1;
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
				first_edge=0;
				}
				else{
					edge = *(edgePtr+j);
					difference = edge - prevEdge;
					prevEdge = edge;	
					if(difference < (1<<8)){
						count +=1;
					}
					else if(difference < (1 << 16)){
						count += 2;
					}
					else if(difference < (1 << 24)){
						count += 3;
					}
					else{
						count += 4;
					}
				}
		}	
		}
					charsUsedArr[i] = count;
	}
	long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
	compressionStarts[n] = totalSpace;
	uchar *finalArr = newA(uchar, totalSpace);	
	{parallel_for(long i=0;i<n;i++){

		long charsUsed = sequentialCompressEdgeSet((uchar *)(finalArr+compressionStarts[i]), 0, Degrees[i], i, edges + offsets[i]);
		offsets[i] = compressionStarts[i];
	}}
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
    // space used by control stream
    uintT controlLength = (degree + 3)/4;
    // set data pointer to location after control stream (ie beginning of data stream)
    long currentOffset = 0;
    long dataOffset = controlLength + currentOffset;
    //	cout << "before set key" << endl;
    uchar key = edgeStart[currentOffset];
    //	cout << "key: " << key << endl;
    //	uchar *dataOffset = (edgeStart + sizeof(uchar)*controlLength);
    uintE startEdge = eatFirstEdge(key, source, dataOffset, currentOffset, edgeStart);
    //	cout << "after startEdge" << endl;
    if(!t.srcTarg(source,startEdge,edgesRead)){
      return;
    }	
    uintT shift = 2;
    uintE edge = startEdge;
    for (edgesRead = 1; edgesRead < degree; edgesRead++){      
      if(shift == 8){
	currentOffset++;
	key = edgeStart[currentOffset];
	shift = 0;
      }
      // decode stored value and add to previous edge value (stored using differential encoding)
      uintE edgeRead = eatEdge(key, dataOffset, shift, currentOffset, edgeStart);
      edge = edgeRead + startEdge;
      startEdge = edge;
      if (!t.srcTarg(source, edge, edgesRead)){
	break;
      }
      shift += 2;
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
