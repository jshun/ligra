#ifndef BITPACKING_VEC_H
#define BITPACKING_VEC_H

#include <x86intrin.h>
#include <emmintrin.h>
#include "parallel.h"
#include "utils.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <cmath>

/*
compiler flag: BPSIMD=1
 
status: not working but close
	- correct output when run decoder & works for most applications except BFS (must be memory access issue when using sparse representation)
	- the encoded file sizes are the same as bp.h
	- I've tried writing versions that only use one of the blocksizes (i.e. a version for block size 16, 32, etc) and all have the correct decoder output but segfault with BFS
Functions in this file used for BP:
	Encoding: parallelCompressEdges --> sequentialCompressEdgeSet --> compressFirstEdge --> bp_8, bp_16, bp_24, bp_32 (used depending on bitwidth) --> bp
	Decoding: decode --> eatFirstEdge --> bp_decode8, bp_decode16, bp_decode24, bp_decode32

Using vertical interleaved method for encoding/decoding (refer to Lemire's papers and blog posts)
*/
typedef unsigned char uchar;
#define LAST_BIT_SET(b) (b & (0x80))
#define EDGE_SIZE_PER_BYTE 7

// decode weights
inline intE eatWeight(uchar controlKey, long & dOffset, intT shift, long controlOffset, uchar* start){
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

// encode first edge
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

long bp(uintE* diff, uint block_size,uchar b, long currentOffset, uchar* &saveArray){
	// save bit width in header byte
	saveArray[currentOffset] = b;	
	currentOffset++;
	uint index = 0;
	uintE int_store = 0; 
	uint num_bytes = b/8; 
	// iterate over numbers in block
	for(uint i=0; i < block_size; i++){
		int_store = diff[index++];
		// save each byte of an element separately (char array)
		for(uint j = 0; j < num_bytes; j++){
			saveArray[currentOffset] = int_store >> j*8;
			currentOffset++;
		}	
	}
	return currentOffset;
}

long bp_8(uintE* diff, uint block_size, long currentOffset, uchar* &saveArray){
        const __m128i *in = (const __m128i * )(diff);
        __m128i outReg, inReg;
        const __m128i mask = _mm_set1_epi32((1U << 8) -1);
	for(long i=0; i < block_size; i+= 16){
		inReg = _mm_and_si128(_mm_loadu_si128(in),mask);
		outReg = inReg;
		inReg = _mm_and_si128(_mm_loadu_si128(in+1),mask);
		outReg = _mm_or_si128(outReg, _mm_slli_epi32(inReg, 8));

		inReg = _mm_and_si128(_mm_loadu_si128(in+2),mask);
		outReg = _mm_or_si128(outReg, _mm_slli_epi32(inReg, 16));
	
		inReg = _mm_and_si128(_mm_loadu_si128(in+3), mask);
		outReg = _mm_or_si128(outReg, _mm_slli_epi32(inReg, 24));
                _mm_storeu_si128((__m128i *) &saveArray[currentOffset], outReg);
		in+=4;
		currentOffset+=16;
        }
        return currentOffset;
}

long bp_16(uintE* diff, uint block_size, long currentOffset, uchar* &saveArray){
        const __m128i *in =  (const __m128i * )(diff);
	__m128i outReg, inReg;
	const __m128i mask = _mm_set1_epi32((1U << 16) -1);
	for(uint i=0; i < block_size; i+= 16){
		inReg = _mm_and_si128(_mm_loadu_si128(in), mask);
		outReg = inReg;

		inReg = _mm_and_si128(_mm_loadu_si128(in+1), mask);
		outReg = _mm_or_si128(outReg, _mm_slli_epi32(inReg, 16));
		_mm_storeu_si128( (__m128i *)&saveArray[currentOffset], outReg);
		currentOffset+=16;

		inReg = _mm_and_si128(_mm_loadu_si128(in + 2), mask);
		outReg = inReg;
		inReg = _mm_and_si128(_mm_loadu_si128(in+3), mask);
		outReg = _mm_or_si128(outReg, _mm_slli_epi32(inReg, 16));
		_mm_storeu_si128( (__m128i *)&saveArray[currentOffset], outReg);

		currentOffset+=16;
		in+=4;
		} 
	return currentOffset;
}

long bp_24(uintE* diff, uint block_size, long currentOffset, uchar* &saveArray){
	const __m128i *in =(const __m128i *)(diff);
	__m128i outReg, inReg;
	const __m128i mask = _mm_set1_epi32((1U << 24)-1);
	for(uint i=0; i < block_size; i += 16){
		// load in 24 bit 4 integers
		inReg = _mm_and_si128(_mm_loadu_si128(in++), mask);
		outReg = inReg;
		
		// load in 24 bits of next 4 integers
		inReg = _mm_and_si128(_mm_loadu_si128(in++), mask);
		// store 8 bits of these 4 numbers
		outReg = _mm_or_si128(outReg, _mm_slli_epi32(inReg, 24));
		_mm_storeu_si128( (__m128i *)&saveArray[currentOffset], outReg);
		currentOffset+=16;

		// store in remaining 16 bits of the 4 numbers
		outReg = _mm_srli_epi32(inReg, 24-16);
		// get next 4 numbers
		inReg = _mm_and_si128(_mm_loadu_si128(in++),mask);
		// store first 16 bits of 3rd set of 4 numbers
		outReg = _mm_or_si128(outReg, _mm_slli_epi32(inReg, 16));
		_mm_storeu_si128((__m128i *) &saveArray[currentOffset], outReg);
		currentOffset+=16;
		
		// store remaining 8 bits of 4 numbers
		outReg = _mm_srli_epi32(inReg, 24-8);
		// get last set of four numbers
		inReg = _mm_and_si128(_mm_loadu_si128(in++), mask);
		// storing 24 bits of last 4 numbers
		outReg = _mm_or_si128(outReg, _mm_slli_epi32(inReg, 8));
		_mm_storeu_si128((__m128i *)&saveArray[currentOffset], outReg);

		currentOffset+=16;
	}
	return currentOffset;
}

long bp_32(uintE* diff, uint block_size, long currentOffset, uchar* &saveArray){
	const __m128i *in = (const __m128i *)(diff);
	__m128i outReg, inReg;

	for(uint i=0; i < block_size; i+=16){
		inReg = _mm_loadu_si128(in++);
		outReg = inReg;
		_mm_storeu_si128((__m128i *)&saveArray[currentOffset],outReg);
		currentOffset+=16;

		inReg = _mm_loadu_si128(in++);
		outReg = inReg;
		_mm_storeu_si128((__m128i *)&saveArray[currentOffset],outReg);
		currentOffset+=16;

		inReg = _mm_loadu_si128(in++);
		outReg = inReg;
		_mm_storeu_si128((__m128i *)&saveArray[currentOffset],outReg);
		currentOffset+=16;

		inReg = _mm_loadu_si128(in++);
		outReg = inReg;
		_mm_storeu_si128((__m128i *)&saveArray[currentOffset],outReg);

		currentOffset+=16;	
	}
	return currentOffset;
}
long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, uintE *savedEdges){
	// use to find bit width needed per block
	uint max_number = 0;
	uint block_size = 0;
	if (degree > 0){
		// decide block_size based on degree
		if(degree > 128){
			block_size = 128;
		}
		else if(degree > 64){
			block_size = 64;
		}
		else if(degree > 32){
			block_size = 32;
		}
		else if(degree > 16){
			block_size = 16;
		}
		else if(degree > 8){
			block_size = 8;
		}
		else{
			block_size = 4;
		}

		currentOffset = compressFirstEdge(edgeArray, currentOffset, vertexNum, savedEdges[0]);
		uchar b;
		long k,j;
		long start, end;
		
		uint num_blocks = (degree-1)/block_size;
				// find number of remaining elements in partial block
		uint num_remaining = (degree-1) - block_size*num_blocks;
				uint max_number = 0;
				// iterate over blocks (last iteration doesn't store anything if num_remaining==0)
		
		uintE difference[block_size];
		if (block_size > 8){
				for(k=0; k < num_blocks; k++){	
					// indices for diff array
					start = k*block_size;
					end = block_size*(k+1);
					max_number = 0;
					uint index = 0;
					for(j=start;j<end;j++){
							difference[index] = savedEdges[j+1] - savedEdges[j];
							max_number = (max_number < difference[index]) ? difference[index] : max_number;		
							index++;
					}
					// decide bit width depending on the max number
					if(max_number < (1 << 8)){
						edgeArray[currentOffset] = 8;	
						currentOffset++;
						currentOffset = bp_8(difference, block_size, currentOffset, edgeArray);
					}
					else if(max_number < (1 << 16)){
						edgeArray[currentOffset] = 16;	
						currentOffset++;
						currentOffset = bp_16(difference, block_size, currentOffset, edgeArray);
					}
					else if(max_number < (1 << 24)){
						edgeArray[currentOffset] = 24;	
						currentOffset++;
						currentOffset = bp_24(difference, block_size, currentOffset, edgeArray);
					}
					else{
						edgeArray[currentOffset] = 32;	
						currentOffset++;
						currentOffset = bp_32(difference, block_size, currentOffset, edgeArray);
					}
				}}
		else{
			for(k=0; k < num_blocks; k++){	
					// indices for diff array
					start = k*block_size;
					end = block_size*(k+1);
					max_number = 0;
					uint index = 0;
					for(j=start;j<end;j++){
							difference[index] = savedEdges[j+1] - savedEdges[j];
							max_number = (max_number < difference[index]) ? difference[index] : max_number;		
							index++;
					}
					// decide bit width depending on the max number
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
				currentOffset = bp(difference, block_size, b, currentOffset, edgeArray);
				}
		}
		if(num_remaining>0){
			// indices for diff array
			start = num_blocks*block_size;
			end = degree-1;
			max_number = 0;
			uint index = 0;
			for(j=start;j<end;j++){
				difference[index] = savedEdges[j+1] - savedEdges[j];
				max_number = (max_number < difference[index]) ? difference[index] : max_number;	
				index++;	
			}
			// decide bit width depending on the max number
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
			currentOffset = bp(difference, num_remaining, b, currentOffset, edgeArray);
			}
		}
		return currentOffset;
}

uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m, uintE* Degrees){
	cout << "parallel compressing, (n,m) = (" << n << "," << m << ")"  << endl;
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
	{parallel_for(long i=0;i<n;i++){
	// similar calculations to sequentialCompressEdgeSet -- used to calculate memory needed to allocate
	uint max_number = 0;
	long count = 0;
	uint block_size = 0;
		if (Degrees[i] > 0){
		if(Degrees[i] > 128){
			block_size = 128;
		}
		else if(Degrees[i] > 64){
			block_size = 64;
		}
		else if(Degrees[i] > 32){
			block_size = 32;
		}
		else if(Degrees[i] > 16){
			block_size = 16;
		}
		else if(Degrees[i] > 8){
			block_size = 8;
		}
		else{
			block_size = 4;
		}
 	uintE* edgePtr = edges+offsets[i];
	uintE edge = *edgePtr;

	  intE preCompress = (intE) *edgePtr - i;
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
	  count++;

	  uchar curByte = toCompress & 0x7f;
	  while ((curByte > 0) || (toCompress > 0)) {
	    uchar toWrite = curByte;
	    toCompress = toCompress >> 7;
	    curByte = toCompress & 0x7f;
	    if (toCompress > 0) {
	      toWrite |= 0x80;
	    }
	    count++;
	  }

		uint num_blocks = (Degrees[i]-1)/block_size;
		uint num_remaining = (Degrees[i]-1) - block_size*num_blocks;
		count += num_blocks;
		if(num_remaining > 0){
			count++;
		}
			uintE prevEdge = *edgePtr;	
			uintE difference;
			long k,j;
		for(k=0; k < 1+ num_blocks; k++){	
			uchar b = 0;
			uint start = k*block_size;
			uint end = block_size*(k+1);
			if(k== num_blocks){
				end = Degrees[i]-1;
			}
			max_number = 0;
			for(j= start;j<end;j++){
					edge = *(edgePtr+j+1);
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
				if(k ==num_blocks && num_remaining >0){
					count+= (num_remaining)*b/8;
				}
				else if(k < num_blocks){
					count += (block_size)*b/8;
				}
				else{}
			
		}
	}
			charsUsedArr[i] = count;
	}}
	long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
	compressionStarts[n] = totalSpace;
	uchar *finalArr = newA(uchar, totalSpace);	
	free(charsUsedArr);
	for(long i=0;i<n;i++){
		
		long charsUsed = sequentialCompressEdgeSet((uchar *)(finalArr+compressionStarts[i]), 0, Degrees[i], i, edges + offsets[i]);
		offsets[i] = compressionStarts[i];
	}
	cout << "after sequentialCompress loop " << endl;
	offsets[n] = totalSpace;
	
	cout << "total space requested is: " << totalSpace << endl;
	float avgBitsPerEdge = (float)totalSpace*8 / (float)m;
	cout << "Average bits per edge: " << avgBitsPerEdge << endl;
	free(compressionStarts);
	cout << "finished compressing, bytes used = " << totalSpace << endl;
	cout << "would have been, " << (m*4) << endl;
	return ((uintE *)finalArr);
}

template <class T>
bool bp_decode8(uintE &startEdge, long &currentOffset, const uintE &source, uint block_size, T t, uchar* edgeStart, size_t edgesRead){
	uintE edge[4] = {0,0,0,0};
	__m128i outReg, inReg, temp;
	const __m128i mask = _mm_set1_epi32((1U << 8)-1);
	for(uint i=0; i < block_size; i+=16){
		inReg = _mm_loadu_si128(( __m128i *)&edgeStart[currentOffset]);
		outReg = _mm_and_si128(inReg, mask);
		// use shifts and sets to add differences to previous edge values (compute prefix sum)
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		// final edge values
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}

		outReg = _mm_and_si128(_mm_srli_epi32(inReg, 8), mask);
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		// final edge values
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
	
		outReg = _mm_and_si128(_mm_srli_epi32(inReg, 16), mask);
	
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		// final edge values
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
		outReg = _mm_srli_epi32(inReg, 24);
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		// final edge values
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		} 
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}

		currentOffset += 16;
	}
	return 0;
}

template <class T>
bool bp_decode16(uintE &startEdge, long &currentOffset, const uintE &source, uint block_size, T t, uchar* edgeStart, size_t &edgesRead){
	__m128i outReg, inReg, temp;
	uintE edge[4] = {0,0,0,0};
	const __m128i mask = _mm_set1_epi32((1U << 16)-1);
	for(uint i=0; i < block_size; i+=16){
		inReg = _mm_loadu_si128(( __m128i *)&edgeStart[currentOffset]);		
		outReg = _mm_and_si128(inReg, mask);
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}

		outReg = _mm_srli_epi32(inReg, 16);
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}

		currentOffset+=16;
		inReg = _mm_loadu_si128((__m128i *)&edgeStart[currentOffset]);		
		outReg = _mm_and_si128(inReg, mask);
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}

		outReg = _mm_srli_epi32(inReg, 16);
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}

		currentOffset+=16;


	}
	return 0;
}

template <class T>
bool bp_decode24(uintE &startEdge, long &currentOffset, const uintE &source, uint block_size, T t, uchar* edgeStart, size_t &edgesRead){
	__m128i outReg, inReg, temp;
	uintE edge[4] = {0,0,0,0};
	const __m128i mask = _mm_set1_epi32((1U << 24)-1);
	for(uint i=0; i < block_size; i+=16){
		inReg = _mm_loadu_si128((__m128i *)&edgeStart[currentOffset]);
		outReg = _mm_and_si128(inReg, mask);
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
		currentOffset += 16;
		outReg = _mm_srli_epi32(inReg, 24);
		inReg = _mm_loadu_si128((__m128i *)&edgeStart[currentOffset]);
		outReg = _mm_or_si128(outReg, _mm_and_si128(_mm_slli_epi32(inReg, 24-16),mask));
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
		outReg = _mm_srli_epi32(inReg, 16);
		currentOffset+=16;
		inReg = _mm_loadu_si128((__m128i *)&edgeStart[currentOffset]);
		outReg = _mm_or_si128(outReg, _mm_and_si128(_mm_slli_epi32(inReg, 24-8),mask));
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
		
		outReg = _mm_srli_epi32(inReg, 8);
		currentOffset+=16;
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
	}
	return 0;
}

template <class T>
bool bp_decode32(uintE &startEdge, long &currentOffset, const uintE &source, uint block_size, T t, uchar* edgeStart, size_t &edgesRead){
	__m128i outReg, temp;
	uintE edge[4] = {0,0,0,0};
	for(uint i=0; i < block_size; i+=16){
		outReg = _mm_loadu_si128((__m128i *)&edgeStart[currentOffset]);
		currentOffset+=16;
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
		outReg = _mm_loadu_si128((__m128i *)&edgeStart[currentOffset]);
		currentOffset+=16;
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
		outReg = _mm_loadu_si128((__m128i *)&edgeStart[currentOffset]);
		currentOffset+=16;
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF));
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}
		outReg = _mm_loadu_si128((__m128i *)&edgeStart[currentOffset]);
		currentOffset+=16;
		temp = _mm_add_epi32(outReg, _mm_slli_si128(outReg, 4));
		outReg = _mm_set1_epi32(startEdge);
		temp = _mm_add_epi32(temp, _mm_slli_si128(temp, 8));
		temp = _mm_add_epi32(temp, outReg);
		edge[0] = _mm_cvtsi128_si32 (temp);
		edge[1] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0x55));
		edge[2] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp,0xAA));
		edge[3] = _mm_cvtsi128_si32(_mm_shuffle_epi32(temp, 0xFF)); 
		startEdge = edge[3];
		if(!t.srcTarg(source, edge[0], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[1], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[2], edgesRead++)){
			return 1;
		}
		if(!t.srcTarg(source, edge[3], edgesRead++)){
			return 1;
		}

	}
	return 0;
}
template <class T>
inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
  if(degree > 0) {
	uintE startEdge = 0;
 	size_t edgesRead = 0;	
	// decode first edge
	startEdge = eatFirstEdge(edgeStart, source);
	if(!t.srcTarg(source, startEdge, edgesRead)){
		return;
	}	
  	uint block_size = 0;
	uint num_blocks=0;
	// figure out block size based on degree (technically degree-1) because first edge processed -
	if(degree > 128){
		block_size = 128;
		num_blocks = (degree-1) >> 7;
	}
	else if(degree > 64){
		block_size = 64;
		num_blocks = (degree-1) >> 6;
	}
	else if(degree > 32){
		block_size = 32;
		num_blocks = (degree -1) >> 5;
	}
	else if(degree > 16){
		block_size = 16;
		num_blocks = (degree-1) >> 4;
	}
	else if(degree > 8){
		block_size = 8;
		num_blocks = (degree-1) >> 3;
	}
	else{
		block_size = 4;
		num_blocks = (degree-1) >> 2;
	}
	edgesRead = 1;
	uchar b = 0;

	uintE edge = 0;
	uint num_bytes = 0;
	bool break_var =0;
	long currentOffset = 0;
	uint num_remaining = (degree-1) - block_size*num_blocks;
	// if block size is > 8, use SIMD to decode blocks
	if(block_size > 8){		
	for(long i=0; i < num_blocks; i++){
			b = edgeStart[currentOffset];
			currentOffset++;
			switch(b){
				case 8:
					break_var = bp_decode8(startEdge, currentOffset, source, block_size, t, edgeStart, edgesRead);
					break;
				case 16:
					break_var = bp_decode16(startEdge, currentOffset, source, block_size, t, edgeStart, edgesRead);
					break;
				case 24:
					break_var = bp_decode24(startEdge, currentOffset, source, block_size, t, edgeStart, edgesRead);
					break;
				default:
					break_var = bp_decode32(startEdge, currentOffset, source, block_size, t, edgeStart, edgesRead);
					break;
			}
			if(break_var){
				return;
			}
			}
	}
	// otherwise, decode w/o SIMD
	else{
		for(long i=0; i < num_blocks; i++){
			b= edgeStart[currentOffset];
			currentOffset++;
			num_bytes = b >> 3;
			switch(num_bytes){
				case 1:
				for(uint i=0; i < block_size; i+=4){
					edge = (uintE) edgeStart[currentOffset++] + startEdge;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE) edgeStart[currentOffset++] + startEdge;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE) edgeStart[currentOffset++] + startEdge;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}

					edge = (uintE) edgeStart[currentOffset++] + startEdge;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}

				}
				break;
			case 2:
				for(uint i=0; i < block_size; i+=4){
					edge = (uintE) edgeStart[currentOffset] +( (uintE)edgeStart[currentOffset+1] << 8) + startEdge;
					currentOffset += 2;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE) edgeStart[currentOffset] +( (uintE)edgeStart[currentOffset+1] << 8) + startEdge;
					currentOffset += 2;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE) edgeStart[currentOffset] +( (uintE)edgeStart[currentOffset+1] << 8) + startEdge;
					currentOffset += 2;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE) edgeStart[currentOffset] +( (uintE)edgeStart[currentOffset+1] << 8) + startEdge;
					currentOffset += 2;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
				}
				break;

			case 3:
				for(uint i =0; i < block_size; i+=4){
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + startEdge;					
					currentOffset += 3;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + startEdge;					
					currentOffset += 3;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + startEdge;					
					currentOffset += 3;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}

					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + startEdge;					
					currentOffset += 3;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
				}
				break;
			default:
				for(uint i =0; i < block_size; i+=4){	
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + ((uintE)edgeStart[currentOffset+3] << 24) + startEdge;
					currentOffset += 4;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + ((uintE)edgeStart[currentOffset+3] << 24) + startEdge;
					currentOffset += 4;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + ((uintE)edgeStart[currentOffset+3] << 24) + startEdge;
					currentOffset += 4;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + ((uintE)edgeStart[currentOffset+3] << 24) + startEdge;
					currentOffset += 4;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
				}
				break;
		}



			}

		}

	// decode remaining
	if(num_remaining > 0){
		b = edgeStart[currentOffset];
		currentOffset++;
		num_bytes = b >> 3;
		switch(num_bytes){
			case 1:
				for(uint i=0; i < num_remaining; i++){
					edge = (uintE) edgeStart[currentOffset++] + startEdge;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
				}
				break;
			case 2:
				for(uint i=0; i < num_remaining; i++){
					edge = (uintE) edgeStart[currentOffset] +( (uintE)edgeStart[currentOffset+1] << 8) + startEdge;
					currentOffset += 2;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
				}
				break;

			case 3:
				for(uint i =0; i < num_remaining; i++){
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + startEdge;

					currentOffset += 3;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
				}
				break;
			default:
				for(uint i =0; i < num_remaining; i++){	
					edge = (uintE)edgeStart[currentOffset] + ((uintE)edgeStart[currentOffset+1] << 8) + ((uintE)edgeStart[currentOffset+2] << 16) + ((uintE)edgeStart[currentOffset+3] << 24) + startEdge;
					currentOffset += 4;
					startEdge = edge;
					if(!t.srcTarg(source, edge, edgesRead++)){
						return;
					}
				}
				break;
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
			uintE startEdge = 0;
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
		uintT key=0;
		dataCurrentOffset += (1 + key)*sizeof(uchar); 
		// compress weight of first edge
		uintT temp_key = 0;
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
