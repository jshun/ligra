#ifndef STREAMVBYTE_VEC_H
#define STREAMVBYTE_VEC_H


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
#include "streamvbyte_shuffle_tables.h"
typedef unsigned char uchar;

typedef union M128{
	char i8[16];
	uint32_t u32[4];
	__m128i i128;

} u128;

// to use this compression scheme, compile with STREAMVEC=1 make -j

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
    //memcpy(&edgeRead, &start[dOffset], 2);
    signBit = 0x80 & start[dOffset+1];
    //signBit = 0x8000 & edgeRead;
    //edgeRead = edgeRead & 0x7FFF; 
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
    //memcpy(&edgeRead, &start[dOffset], 3);
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + ((start[dOffset+2] & 0x7f) << 16);
    signBit = 0x80 & start[dOffset+2];
    //signBit = 0x800000 & edgeRead;
    //edgeRead = edgeRead & 0x7FFFFF;
    dOffset += 3;
    break;
    // 4 bytes
  default:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + ((start[dOffset+3] & 0x7f) << 24);
    //memcpy(&edgeRead, &start[dOffset], 4);
    signBit = start[dOffset+3] & 0x80;
    //signBit = edgeRead & 0x80000000;
    //edgeRead = (edgeRead) & 0x7FFFFFFF; 
    dOffset += 4;
  }
  return (signBit) ? source - edgeRead : source + edgeRead;
}

static inline intE _decode_data(uint8_t **dataPtrPtr, uint8_t code){
	uint8_t *dataPtr = *dataPtrPtr;
	uintE edgeRead = 0;
	uintE* edgeReadPtr = &edgeRead;
//	code &= 0x3;
	if(code ==0){
		edgeRead = *dataPtr;
	//	cout << "*dataPtr: " << edgeRead << endl;
		dataPtr += 1;
	}
	else if(code ==1){
		memcpy(&edgeReadPtr, dataPtr, 2);
		dataPtr +=2;
	}
	else if(code ==2){
		memcpy(&edgeReadPtr, dataPtr, 3);
		dataPtr +=3;
	}
	else{
		memcpy(&edgeReadPtr, dataPtr, 4);
		dataPtr += 4;
	}
	
	*dataPtrPtr = dataPtr;
	//cout << "**dataPtrPtr: " << *(*dataPtrPtr) << endl;
	return edgeRead;
}



template <class T>
void svb_decode_scalar(size_t edgesRead, uchar* dataPointer, uchar* controlPointer, uintT degree, intE* previousEdge, T t, const uintE &source, bool firstEdge){
	if(degree >  0){
	intE edge = *previousEdge;	
	uintT shift = 0;
	uintT key = *controlPointer++;
	uintT i = 0;
	intE val;
	intE* valPtr;
//	cout << "edgesRead: " << edgesRead << endl;
//	cout << "svb_decode_scalar " << endl;
	if (firstEdge == 1){
	//	cout << "first edge" << endl;
		uchar firstKey = key & 0x3;
		uintT signBit = 0;
		if(firstKey == 0){
			val = *dataPointer;
		//	cout << "val with signBit: " << val << endl;
			signBit = (((val) & 0x80) >> 7);
			val &=  0x7F;
		//	cout << "val no signBit: " << val << endl;
			dataPointer += 1;
		}
		else if(firstKey == 1){
		//	cout << "firstKey 1" << endl;
			memcpy(&valPtr, dataPointer, 2);
			signBit = (((1 << 15) & val) >> 15);
			val = val & 0x7FFF;
			dataPointer += 2;
		}
		else if(firstKey == 2){
		//	cout << "firstkey 2: "<< endl;
			memcpy(&valPtr, dataPointer, 3);
			signBit = (((1 << 23) & val) >> 23);
			val = val & 0x7FFFFF;
			dataPointer += 3;
		}
		else{
		//	cout << "firstkey 3 " << endl;
			memcpy(&valPtr, dataPointer, 4);
			signBit = (((1 << 31) & val) >> 31);
			val = val & 0x7FFFFFFF;
			dataPointer += 4;
		}
		val = (signBit) ? source - val : source + val;	
		edge = edge + val;
		if(!t.srcTarg(source, edge, edgesRead)){
			return;
		}
		i=1;
		edgesRead++;
		firstEdge=0;
		shift+=2;
	}
	for (; i < degree; i++){
		if(shift == 8){
			shift = 0;
			key = *controlPointer++;
		}
		val = 0;
		val = _decode_data(&dataPointer, (key >> shift) & 0x3);
		edge = edge + val; 
//		cout << "edge: " << edge << " val: " << val << endl;
		if(!t.srcTarg(source, edge, edgesRead))
			break;
		edgesRead++;
		// do someting with value... Pass in t and call t.Src? need to "undo" differential encoding
		shift+= 2;	
	}
	}
}
template <class T>
static inline bool _write_avx(__m128i Vec, intE* prevEdge, const uintE &source, T t, size_t edgesRead){
//	__m128i shifted = _mm_slli_si128(Vec, 32);
//	shifted[0] = *prevEdge;
//	Vec = _mm_add_epi32(Vec, shifted);
	intE edge = Vec[0] + *prevEdge;
	if(!t.srcTarg(source, edge, edgesRead)){
		return 1;
	}
	edge = edge + Vec[1];
	if(!t.srcTarg(source, edge, edgesRead+1)){
		return 1;
	}
	edge = edge + Vec[2];
	if(!t.srcTarg(source, edge, edgesRead+2)){
		return 1;
	}
	edge = edge + Vec[3];
	if(!t.srcTarg(source, edge, edgesRead+3)){
		return 1;
	}
	*prevEdge = edge;
//	_mm_storeu_si128((__m128i *)out, Vec);
	return 0;
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
    //dOffset += 1;
    break;
    // 2 bytes
  case 1:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8);
    //memcpy(&edgeRead, &start[dOffset], 2);
    dOffset += 2;
    break;
    // 3 bytes
  case 2:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
    //memcpy(&edgeRead, &start[dOffset], 3);	
    dOffset += 3;
    break;
    // 4 bytes
  default:
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


long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, uintE *savedEdges){
	if(degree > 0){
		// number of bytes needed for control stream
		uintT controlLength = (degree + 3)/4;
		// offset where to start storing data (after control bytes)
		long dataCurrentOffset = currentOffset + controlLength;
		// shift and key always = 0 for first edge
		uchar key = compressFirstEdge(edgeArray, currentOffset, dataCurrentOffset, vertexNum, savedEdges[0]);
		// offset data pointer by amount of space required by first edge
		dataCurrentOffset += (1 + key);
		if(degree == 1){
			edgeArray[currentOffset] = key;
			return dataCurrentOffset;
//			return (dataCurrentOffset+1);
		}
		// scalar version: compress the rest of the edges
		dataCurrentOffset= compressEdge(edgeArray, currentOffset, savedEdges, key, dataCurrentOffset, degree);
		return dataCurrentOffset;
//		return (dataCurrentOffset+1);
	}
	else{
		return currentOffset;
	}
}

static inline __m128i _decode_avx(uintT key, uchar **dataPtrPtr){
	uint8_t len = lengthTable[key];
	__m128i Data = _mm_loadu_si128((__m128i *)*dataPtrPtr);
	__m128i Shuf = *(__m128i *)&shuffleTable[key];

	Data = _mm_shuffle_epi8(Data,Shuf); 
	*dataPtrPtr += len; 
	return Data; 
}


template <class T>
	inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
	size_t edgesRead = 0;
	if(degree > 0) {
//		cout << "inside decode" << endl;
		uintT controlLength = (degree+3)/4;
//		cout << "keybytes1: " << keybytes << endl;
		uchar *dataPtr = edgeStart + controlLength;
		uint64_t keybytes = degree/4;
//		cout << "*dataPtr " << *dataPtr << endl;
		uchar *controlPtr = edgeStart;
//		cout << "controlPtr " << *controlPtr << endl;
		__m128i Data;
		intE prevEdge = 0;
		bool firstEdge = 1;
		bool break_var = 0;
//		cout << "before for loop" << endl;
		if (keybytes >= 8){
			cout << "keybytes: " << keybytes << endl;
			int64_t Offset = -(int64_t) keybytes /8 + 1; 
			const uint64_t *keyPtr64  = (const uint64_t *)edgeStart - Offset;
			uint64_t nextkeys;
			memcpy(&nextkeys, keyPtr64+Offset, sizeof(nextkeys));
			uint64_t keys;
			__m128i shifted;
			for(; Offset !=0 ; ++Offset){
				keys = nextkeys;
				memcpy(&nextkeys, keyPtr64 + Offset + 1, sizeof(nextkeys));
				// need to figure out where first edge is & check sign bit...may want to come up with different method for decoding sign..
				Data = _decode_avx((keys & 0xFF), &dataPtr);
				if (firstEdge == 1){
					uchar firstKey = keys & 0x3;
					uchar signBit = 0;
					if(firstKey == 0){
						signBit = (uchar)(((Data[0]) & 0x80) >> 15);
						Data[0] = Data[0] & 0x7F;
					}
					else if(firstKey == 1){
						signBit = (uchar)(((1 << 15) & Data[0]) >> 15);
						Data[0] = Data[0] & 0x7FFF;
					}
					else if(firstKey == 2){
						signBit = (uchar)(((1 << 23) & Data[0]) >> 23);
						Data[0] = Data[0] & 0x7FFFFF;
					}
					else{
						signBit = (uchar)(((1 << 31) & Data[0]) >> 31);
						Data[0] = Data[0] & 0x7FFFFFFF;
					}
					Data[0] = (signBit) ? source - Data[0] : source + Data[0];	
					firstEdge = 0;
				}
				break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
				if(break_var)
					break;
				edgesRead += 4;
				Data = _decode_avx((keys & 0xFF00) >> 8, &dataPtr);
				break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
				if(break_var)
					break;
				edgesRead += 4;

				keys >>= 16;
				Data = _decode_avx((keys & 0xFF), &dataPtr);
				//_write_avx(edgeStart + 8, Data, prevEdge);	
				break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);		
				if(break_var)
					break;
				edgesRead += 4;
				Data = _decode_avx((keys & 0xFF00) >> 8, &dataPtr);
				//_write_avx(edgeStart + 12, Data, prevEdge);
				break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
				if(break_var)
					break;
				edgesRead +=4;

				keys >>= 16;
				Data = _decode_avx((keys & 0xFF), &dataPtr);
			//	_write_avx(edgeStart + 16, Data, prevEdge);
				break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
				if(break_var)
					break;
				edgesRead += 4;
				Data = _decode_avx((keys & 0xFF00) >> 8, &dataPtr);
			//	_write_avx(edgeStart + 20, Data, prevEdge);
				break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
				if(break_var)
					break;
				edgesRead += 4;

				keys >>= 16;
				Data = _decode_avx((keys & 0xFF), &dataPtr);
				break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
				if(break_var)
					break;
				edgesRead += 4;
			//	_write_avx(edgeStart + 24, Data, prevEdge);
				Data = _decode_avx((keys & 0xFF00) >> 8, &dataPtr);
			//	_write_avx(edgeStart + 28, Data, prevEdge);
				break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
				if(break_var)
					break;
				edgesRead += 4;
    			}	
	if(!break_var){
	      keys = nextkeys;
		cout << "!break_var if statement" << endl;
	      Data = _decode_avx((keys & 0xFF), &dataPtr);
	     // _write_avx(edgeStart, Data, prevEdge);
	      break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
//		if(break_var){
//			break;
//		}
		edgesRead += 4;
	      Data = _decode_avx((keys & 0xFF00) >> 8, &dataPtr);
		break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
	     // _write_avx(edgeStart + 4, Data, prevEdge);
//		if(break_var)
//			break;
		edgesRead += 4;

	      keys >>= 16;
	      Data = _decode_avx((keys & 0xFF), &dataPtr);
		break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
//		if(break_var)
//			break;
		edgesRead += 4;
	     // _write_avx(edgeStart + 8, Data, prevEdge);
	      Data = _decode_avx((keys & 0xFF00) >> 8, &dataPtr);
	     // _write_avx(edgeStart + 12, Data, prevEdge);
		break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
//		if(break_var)
//			break;
		edgesRead += 4;

	      keys >>= 16;
	      Data = _decode_avx((keys & 0xFF), &dataPtr);
		break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
//		if(break_var)
//			break;
		edgesRead += 4;
	     // _write_avx(edgeStart + 16, Data, prevEdge);
	      Data = _decode_avx((keys & 0xFF00) >> 8, &dataPtr);
		break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
		edgesRead += 4;
//		if(break_var)
//			break;
	     // _write_avx(edgeStart + 20, Data, prevEdge);

	      keys >>= 16;
	      Data = _decode_avx((keys & 0xFF), &dataPtr);
		break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
//		if(break_var)
//			break;
		edgesRead += 4;
	     // _write_avx(edgeStart + 24, Data, prevEdge);
	      Data = _decode_avx((keys & 0xFF00) >> 8, &dataPtr);
	     // _write_avx(edgeStart + 28, Data, prevEdge);
		break_var = _write_avx(Data, &prevEdge, source, t, edgesRead);
//		if(break_var)
//			break;
		edgesRead += 4;
	}	

	}
//	cout << "controlPtr: " << *controlPtr << endl;
	controlPtr += (degree/4) & ~7;	
//	cout << "controlPtr2: " << *controlPtr << endl;

//	cout << "degree : " << degree << " degree&31: " << (degree & 31) << endl;
	svb_decode_scalar(edgesRead, dataPtr, controlPtr, (degree & 31), &prevEdge, t, source, firstEdge);
	}

}

uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m, uintE* Degrees){
	// compresses edges for vertices in parallel & prints compression stats
	cout << "parallel compressing, (n,m) = (" << n << "," << m << ")"  << endl;
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
	//long count;
	parallel_for(long i=0;i<n;i++){
		long count = 0;
		if (Degrees[i] > 0){
			count = (Degrees[i]+3)/4;
			uintE* edgePtr = edges+offsets[i];
			uintE edge = *edgePtr;	
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
			uintE prevEdge = *edgePtr;	
			uintE difference;
			for(long j=1;j<Degrees[i];j++){
				edge = *(edgePtr+j);
				difference = edge - prevEdge;
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
				prevEdge = edge;	
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
