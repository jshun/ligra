#ifndef STREAMVBYTE_H
#define STREAMVBYTE_H

#include "parallel.h"
#include "utils.h"
#include <x86intrin.h>
#include <emmintrin.h>
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

inline intE eatWeight(uchar* &start, uchar* &dOffset, intT shift){
	uchar fb = *start;
	uintT checkCode = (fb >> shift) & 0x3;
	intE edgeRead = 0;
	intE *edgeReadPtr = &edgeRead;
//	cout << "eatWeight" << endl;
	uchar signBit = 0;
	if(checkCode == 0){
		edgeRead = *dOffset;
		// check sign bit and then get rid of it from actual value 
		signBit =(uchar)(((edgeRead) & 0x80) >> 7);
		edgeRead &= 0x7F;
		dOffset += 1;
	}
	else if(checkCode == 1){
		memcpy(&edgeReadPtr, dOffset, 2);
		signBit = (uchar)(((1 << 15) & edgeRead) >> 15);
		edgeRead = edgeRead & 0x7FFF; 
		dOffset += 2;
	}
	else if(checkCode == 2){
		memcpy(&edgeReadPtr, dOffset, 3);
		signBit = (uchar)(((edgeRead) & (1 <<23)) >> 23);
		edgeRead = edgeRead & 0x7FFFFF;
		dOffset += 3;
	}
	else{
		memcpy(&edgeReadPtr, dOffset, 4);
		signBit = (uchar)(((edgeRead) & (1 << 31)) >> 31);
		edgeRead = (edgeRead) & 0x7FFFFFFF; 
		dOffset += 4;
	}

//	cout << "sign bit: " << signBit << endl;
	return (signBit) ? -edgeRead : edgeRead;

}

inline intE eatFirstEdge(uchar* &start, uintE source, uchar* &dOffset){
	uchar fb = *start;
	uintT checkCode = (fb) & 0x3;
	intE edgeRead = 0;
	intE *edgeReadPtr = &edgeRead;
	uchar signBit = 0;
	if(checkCode == 0){
		edgeRead = *dOffset;
		// check sign bit and then get rid of it from actual value 
		signBit = (uchar)(((edgeRead) & 0x80) >> 7);
		edgeRead &= 0x7F;
		dOffset += 1;
	}
	else if(checkCode == 1){
		memcpy(&edgeReadPtr, dOffset, 2);
		signBit =(uchar) (((1 << 15) & edgeRead) >> 15);
		edgeRead = edgeRead & 0x7FFF; 
		dOffset += 2;
	}
	else if(checkCode == 2){
		memcpy(&edgeReadPtr, dOffset, 3);
		signBit = (uchar)(((edgeRead) & (1 <<23)) >> 23);
		edgeRead = edgeRead & 0x7FFFFF;
		dOffset += 3;
	}
	else{
		memcpy(&edgeReadPtr, dOffset, 4);
		signBit = (uchar)((edgeRead) & (1 << 31)) >> 31;
		edgeRead = (edgeRead) & 0x7FFFFFFF; 
		dOffset += 4;
	}
//	cout << "signBit: " << signBit << endl;
//	cout << "source: " << source << " edgeRead: " << edgeRead << "sign bit: " << signBit << endl;
	return (signBit) ? source - edgeRead : source + edgeRead;

}

static inline intE _decode_data(uint8_t **dataPtrPtr, uint8_t code){
	uint8_t *dataPtr = *dataPtrPtr;
	uintE edgeRead = 0;
	uintE* edgeReadPtr = &edgeRead;
//	code &= 0x3;
	if(code ==0){
		edgeRead = *dataPtr;
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
	return edgeRead;
}


template <class T>
void svb_decode_scalar(size_t edgesRead, uchar* dataPointer, uchar* controlPointer, uintT degree, intE* previousEdge, T t, const uintE &source, bool firstEdge){
	if(degree >  0){
	intE edge = *previousEdge;	
	uchar shift = 0;
	uintT key = *controlPointer++;
	uintT i = 0;
	intE val;
	intE* valPtr;
	cout << "edgesRead: " << edgesRead << endl;
//	cout << "svb_decode_scalar " << endl;
	if (firstEdge == 1){
		cout << "first edge" << endl;
		uchar firstKey = key & 0x3;
		uchar signBit = 0;
		if(firstKey == 0){
			val = *dataPointer;
			signBit = (uchar)(((val) & 0x80) >> 7);
			val = val & 0x7F;
			dataPointer += 1;
		}
		else if(firstKey == 1){
			memcpy(&valPtr, dataPointer, 2);
			signBit = (uchar)(((1 << 15) & val) >> 15);
			val = val & 0x7FFF;
			dataPointer += 2;
		}
		else if(firstKey == 2){
			memcpy(&valPtr, dataPointer, 3);
			signBit = (uchar)(((1 << 23) & val) >> 23);
			val = val & 0x7FFFFF;
			dataPointer += 3;
		}
		else{
			memcpy(&valPtr, dataPointer, 4);
			signBit = (uchar)(((1 << 31) & val) >> 31);
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
		val = _decode_data(&dataPointer, (key >> shift) & 0x3);
		edge = edge + val; 
		if(!t.srcTarg(source, edge, edgesRead))
			break;
		edgesRead++;
		// do someting with value... Pass in t and call t.Src? need to "undo" differential encoding
		shift+= 2;	
	}
	}
}

inline uintE eatEdge(uchar* &start, uchar* &dOffset, intT shift){
	// check if should be start++ or start
//	cout << "eat edge" << endl;
	
	uchar fb = *start;
	uintT checkCode = (fb >> shift) & 0x3;
	uintE edgeRead = 0;
	uintE *edgeReadPtr = &edgeRead;
	if(checkCode == 0){
		edgeRead = *dOffset; 
		dOffset += 1;
	}
	else if(checkCode == 1){
		memcpy(&edgeReadPtr, dOffset, 2);
		dOffset += 2;
	}
	else if(checkCode == 2){
		memcpy(&edgeReadPtr, dOffset, 3);
		dOffset += 3;
	}
	else{
		memcpy(&edgeReadPtr, dOffset, 4);
		dOffset += 4;
	}
	
	return edgeRead;
}

intT compressFirstEdge(uchar *start, long controlOffset, long dataOffset, uintE source, uintE target){
	// Check if I need to do this for my scheme as well.. I think I do..
	uchar* saveStart = start;
	long saveOffset = dataOffset;
	
	intE preCompress = (intE) target - source;
	// need to figure out how to deal with/store sign bit..

	// Check if positive or negative -- change code to incorporate one extra bit -- MSB for first edge will be designated as a sign bit and should be handled by the decoding function accordingly
	intE toCompress = abs(preCompress);
	intT signBit = (preCompress < 0) ? 1:0;
	intT code; 
	// check how many bytes is required to store the data and a sign bit (MSB)
	if(toCompress < (1 << 7)) {
		start[dataOffset] = (signBit << 7) | toCompress;
		code = 0;
	}
	else if(toCompress < (1 << 15)){
		start[dataOffset] = (signBit << 15) | toCompress;
		code = 1;
	}
	else if(toCompress < (1 << 23)){
		start[dataOffset] = (signBit << 23) | toCompress;
		code = 2;
	}
	else{
		start[dataOffset] = (signBit << 31) | toCompress;
		code = 3;
	}
	
	// testing 
//	cout << "Compressing first edge: " << toCompress << endl;
	return code;
}


uintT encode_data(uintE d,  long dataOffset, uchar* &edgeArray){
	uintT code;
//	cout << "difference: " << d << endl;
	if(d < (1 << 8)){
		memcpy((&edgeArray[dataOffset]),&d,1);
		code = 0;
	}
	else if( d < (1 << 16)){
		memcpy((&edgeArray[dataOffset]),&d,2);
		//edgeArray[dataOffset] = d;
		code = 1;
	}
	else if (d < (1 << 24)){
		memcpy((&edgeArray[dataOffset]),&d,3);
		//edgeArray[dataOffset] = d;
		code = 2;
	}
	else{
		memcpy((&edgeArray[dataOffset]),&d,4);
		//edgeArray[dataOffset] = d; 
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
	uintT shift = 4;
	uintE difference;

		for (uintT edgeI = 1; edgeI < degree; edgeI++){
			difference = savedEdges[edgeI].first - savedEdges[edgeI - 1].first;
			if(shift == 8){
				shift = 0;
				start[storeCurrentOffset] = storeKey;
				storeCurrentOffset+= sizeof(uchar);
				storeKey = 0;
			}		
			// figure out control 
	//		code = encode_data(difference, storeDOffset);
			storeDOffset += (code + 1)*sizeof(uchar); 
			storeKey |= (code << shift);
			shift += 2;
			if(shift == 8){
				shift = 0;
				start[storeCurrentOffset] = storeKey;
				storeCurrentOffset+= sizeof(uchar);
				storeKey = 0;
			}		
			// figure out control 
			code = compressFirstEdge(start, storeCurrentOffset, storeDOffset, 0, savedEdges[edgeI].second);
			storeDOffset += (code + 1)*sizeof(uchar); 
			storeKey |= (code << shift);
			shift +=2;
			
		}
	return storeDOffset;
}

long compressEdge(uchar *start, long currentOffset, uintE *savedEdges, uintT key, long dataCurrentOffset, uintT degree){
	uintT code = 0;
	uintT storeKey = key;
	long storeDOffset = dataCurrentOffset;
//	long storeCurrentOffset = currentOffset;
	uintT count = degree/4;
	// pass im first edge with sign bit already concatenated? Otherwie need to change count to (degree-1)/4
	uintE difference[degree];
//	uintE *diffPtr = &difference;
	for(uintT i = 1; i < degree; i++){
		difference[i] = savedEdges[i] - savedEdges[i-1];
	}
	// the dataCurrentOffset should point to same place as currentOffset to begin with?
	
	return storeDOffset;
}


long svb_encode_scalar(const uint32_t *in, long controlOff, long dataOff, uintT countLeft, uchar* &start){
	if (countLeft = 0){
		return dataOff;
	}	
	uintT shift = 0;
	uintT key = 0;
	for(uintT c = 0; c < countLeft; c++){
		if(shift == 8){
			shift = 0;
			start[controlOff] = key;
			controlOff++;
			key = 0;
		}
		uintT val = in[c];
		uchar code = encode_data(val, dataOff, start);
		dataOff += (code+1);
		key |= code << shift;
		shift += 2;
	}
	start[controlOff] = key;
	return dataOff;	
}

// This function writes the control bits and returns the length of data needd by these four elements
size_t streamvbyte_encode4(__m128i in, long outData, long outCode, uchar* &start){

	const u128 Ones = {.i8 = {1, 1, 1, 1, 1, 1, 1, 1,1, 1, 1, 1, 1, 1, 1, 1}};

	#define shifter (1 | 1 << 9| 1 << 18)
	const u128 Shifts = {.u32 = {shifter, shifter, shifter, shifter}};
	const u128 LaneCodes = {.i8 = {0, 3, 2, 3, 1, 3, 2, 3, -1, -1, -1, -1, -1, -1, -1, -1}};

	const u128 GatherHi = {.i8 = {15, 11, 7, 3, 15, 11, 7, 3, -1, -1, -1, -1, -1, -1, -1, -1}};

	#define concat (1 | 1 << 10 | 1 << 20 | 1 << 30)
	#define sum (1 | 1 << 8 | 1 << 16 | 1 << 24)
	const u128 Aggregators = {.u32 = {concat, sum, 0, 0}};	
	__m128i mins = _mm_min_epu8(in, Ones.i128);
	__m128i bytemaps = _mm_mullo_epi32(mins, Shifts.i128);
	__m128i lanecodes = _mm_shuffle_epi8(LaneCodes.i128, bytemaps);
	__m128i hibytes = _mm_shuffle_epi8(lanecodes, GatherHi.i128);

	u128 codeAndLength = {.i128 = _mm_mullo_epi32(hibytes, Aggregators.i128)};
	uint8_t code = codeAndLength.i8[3];
	size_t length = codeAndLength.i8[7] + 4; 
	
	__m128i Shuf = *(__m128i *)&encodingShuffleTable[code]; 
	__m128i outAligned = _mm_shuffle_epi8(in, Shuf);
	_mm_storeu_si128((__m128i *)(&start[outData]), outAligned);

	//_mm_storeu_si128((__m128i *)(start + sizeof(uchar)*outData), outAligned);
//	cout << "code: " << code << endl;
	start[outCode] = code;
//	cout << "length: " << length << endl;
	return length;
}

uintT streamvbyte_encode_quad(uintT *in, long outData, long outControl, uchar *edgeArray){
	__m128i vin = _mm_loadu_si128((__m128i *) in);
	return streamvbyte_encode4(vin, outData, outControl, edgeArray);
}

// need to convert dataPtr into a long type offset..
long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, uintE *savedEdges){
	if(degree > 0){
		uintT controlLength = (degree + 3)/4;
		long dataOffset = currentOffset + controlLength;
	//	uchar *dataPtr = edgeArray + ((degree + 3)/4);
		uintT count = degree/4;
		uintE difference[degree];
	//	uintE *diffPtr = &difference;
		intE preCompress = (intE)savedEdges[0] - vertexNum;
		intE toCompress = abs(preCompress);
		intE signBit = (preCompress < 0) ? 1:0;
		if(toCompress < (1 << 7)){
			toCompress = (signBit << 7) | toCompress;
		}
		else if(toCompress < (1 << 15)){
			toCompress = (signBit << 15) | toCompress;
		}
		else if(toCompress < (1 << 23)){
			toCompress = (signBit << 23) | toCompress;
		}
		else{
			toCompress = (signBit << 31) | toCompress;
		}
		difference[0] = toCompress; // need to figure out size and concatenate signBit
		for(uintT i = 1; i < degree; i++){
			difference[i] = savedEdges[i] - savedEdges[i-1];
		}
		degree -= 4*count;
		uintT length = 0;
		for(uintT i = 0; i < count; i++){
			length = streamvbyte_encode_quad(diffPtr + 4*i, dataOffset, currentOffset, edgeArray);
			dataOffset += length;
			currentOffset++;
		}
		uintE* diffPtr = difference;
	/*	for(uintT edgeI = 0; edgeI < count; edgeI++){
			__m128i vin = _mm_loadu_si128((__m128i *)(difference + 4*edgeI));
			dataOffset += streamvbyte_encode4(vin, dataOffset, currentOffset, edgeArray);
			currentOffset++;
		}*/
		// encode any leftovers (count) mod 4
		dataOffset = svb_encode_scalar(diffPtr + 4*count, currentOffset, dataOffset, degree, edgeArray); // need to change what this is assigned to..
	//	return dataPtr;
		return dataOffset; 
	}
	else{
		return currentOffset;
	}
}

uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m, uintE* Degrees){
	// compresses edges for vertices in parallel & prints compression stats
	cout << "parallel compressing, (n,m) = (" << n << "," << m << ")"  << endl;
	uintE **edgePts = newA(uintE*, n);
	uintT *degrees = newA(uintT, n+1); 
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
	{parallel_for(long i=0;i<n;i++){
		degrees[i] = Degrees[i];
	//	charsUsedArr[i] = ceil((degrees[i]*9)/8)+4;
		charsUsedArr[i] = (degrees[i] + 3)/4 + degrees[i]*4;
	}}

	cout << "after for loop " << endl;

	degrees[n] = 0;
	sequence::plusScan(degrees, degrees, n+1);
	long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
	uintE* iEdges = newA(uintE, toAlloc);
	cout << "second loop begin" << endl;	
	{parallel_for(long i=0;i<n;i++){
//		cout << "i begin " << i << endl;
		edgePts[i] = iEdges+charsUsedArr[i];
		long charsUsed = sequentialCompressEdgeSet((uchar *)(iEdges+charsUsedArr[i]), 0, degrees[i+1]-degrees[i], i, edges + offsets[i]);
		charsUsedArr[i] = charsUsed;
	//	cout << "i end " << i << endl;
	}}
	cout << "after second loop" << endl;

	// produce the total space needed for all compressed lists in chars
	long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
	compressionStarts[n] = totalSpace;
	free(degrees);
	free(charsUsedArr);
	
	uchar *finalArr =  newA(uchar, totalSpace);
	cout << "total space requested is: " << totalSpace << endl;
	float avgBitsPerEdge = (float)totalSpace*8 / (float)m;
	cout << "Average bits per edge: " << avgBitsPerEdge << endl;

	{parallel_for(long i=0; i<n; i++){
		long o = compressionStarts[i];
		memcpy(finalArr + o, (uchar *)(edgePts[i]), compressionStarts[i+1] - o);
		offsets[i] = o;
	}}

	offsets[n] = totalSpace;
	free(iEdges);
	free(edgePts);
	free(compressionStarts);
	cout << "finished compressing, bytes used = " << totalSpace << endl;
	cout << "would have been, " << (m*4) << endl;
	return ((uintE *)finalArr);
}


static inline __m128i _decode_avx(uintT key, uchar **dataPtrPtr){
	uint8_t len = lengthTable[key];
	__m128i Data = _mm_loadu_si128((__m128i *)*dataPtrPtr);
	__m128i Shuf = *(__m128i *)&shuffleTable[key];

	Data = _mm_shuffle_epi8(Data,Shuf); 
	*dataPtrPtr += len; 
	return Data; 
}

//figure out what "out" should be
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
// skeleton of decoding functions
template <class T>
	inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
	size_t edgesRead = 0;
	if(degree > 0) {
//		cout << "inside decode" << endl;
		uchar controlLength = (degree+3)/4;
//		cout << "keybytes1: " << keybytes << endl;
		uchar *dataPtr = edgeStart + controlLength;
		uint64_t keybytes = degree/4;
//		cout << "*dataPtr " << *dataPtr << endl;
		uchar *controlPtr = edgeStart;
//		cout << "controlPtr " << *controlPtr << endl;
		__m128i Data;
		size_t edgesRead = 0;
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
			//	_write_avx(edgeStart + 4, Data, prevEdge);
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

// call svb_decode_scalar but don't return it
	}

	controlPtr += (degree/4) & ~7;	
	svb_decode_scalar(edgesRead, dataPtr, controlPtr, (degree & 31), &prevEdge, t, source, firstEdge);
//	svb_decode_scalar(edgesRead, dataPtr, controlPtr, degree, &prevEdge, t, source, firstEdge);

	}

}
  

//	return svb_decode_scalar(edgeStart, controlPtr + consumedkeys, dataPtr, degree & 31);


template <class T>

	inline void decodeWgh(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
		size_t edgesRead = 0;
		if (degree > 0){
			// add 2* inside parantheses or outside?
			uchar controlLength = (2*degree+3)/4;
			uchar *dataOffset = edgeStart + controlLength;
			uintE startEdge = eatFirstEdge(edgeStart, source, dataOffset);	
			//cout << "decode Wgh" << endl;	
			intE weight = eatWeight(edgeStart, dataOffset, 2);	
			if (!t.srcTarg(source, startEdge, weight, edgesRead)){
				return;
			}
			uintT shift = 4;
			uintE edge = startEdge;
			for (edgesRead = 1; edgesRead < degree; edgesRead++){
				if(shift == 8){
					edgeStart++;
					shift = 0;
				}
				uintE edgeRead = eatEdge(edgeStart, dataOffset, shift);
				edge = edge + edgeRead;
				//startEdge = edge;
				shift += 2;

				if(shift == 8){
					edgeStart++;
					shift = 0;
				}
				intE weight = eatWeight(edgeStart, dataOffset, shift);
				shift += 2;
				if (!t.srcTarg(source, edge, weight, edgesRead)){
					break;
				}
		
			}
		}
	}


long sequentialCompressWeightedEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, intEPair *savedEdges){
	if (degree > 0){
		// add 2* inside or outwside parantheses
		uintT controlLength = (2*degree + 3)/4;
		long dataCurrentOffset = currentOffset + controlLength;
		// target ID
		uintT key = compressFirstEdge(edgeArray, currentOffset, dataCurrentOffset, vertexNum, savedEdges[0].first);
		dataCurrentOffset += (1 + key)*sizeof(uchar); 
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
	uintT *degrees = newA(uintT, n+1);
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
	{parallel_for(long i=0; i<n; i++){
		degrees[i] = Degrees[i];
	//	charsUsedArr[i] = 2*(ceil((degrees[i]*9)/8) + 4);
		charsUsedArr[i] = (2*degrees[i]+3)/4 + 8*degrees[i];

	}}

	degrees[n] = 0;
	sequence::plusScan(degrees,degrees,n+1);
	long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
	uintE* iEdges = newA(uintE, toAlloc);
	{parallel_for(long i = 0; i < n; i++){
		edgePts[i] = iEdges + charsUsedArr[i];
		long charsUsed = sequentialCompressWeightedEdgeSet((uchar *)(iEdges + charsUsedArr[i]), 0, degrees[i+1] - degrees[i], i, edges + offsets[i]);
	charsUsedArr[i] = charsUsed;
	}}
	
	long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
	compressionStarts[n] = totalSpace;
	free(degrees);
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

template <class P>
inline size_t pack(P pred, uchar* edge_start, const uintE &source, const uintE &degree){
	size_t new_deg = 0;
	uintE last_read_edge = source;
	uintE last_write_edge;
	uchar* tail = edge_start;
	uchar* cur = edge_start;
	cout << "pack" << endl;
	if (degree > 0) {
		// not sure that this is correct...
		uchar controlLength  = (degree + 3)/4;
		uchar *dataOffset  = edge_start + controlLength;	
		uintE start_edge = eatFirstEdge(cur, source, dataOffset);
		last_read_edge = start_edge;
		if (pred(source, start_edge)){
			long offset = 0; //compressFirstEdge(tail, 0, source, start_edge);
			tail += offset;
			last_write_edge = start_edge;
			new_deg++;
		}
		for (size_t i = 1; i < degree; i++){
			uintE cur_diff = edgeEat(cur);
			uintE cur_edge = last_read_edge + cur_diff;
			last_read_edge = cur_edge;
			if (pred(source, cur_edge){
				long offset;
				if (tail == edge_start){
					offset = 0; //compressFirstEdge(tail, 0, source, cur_edge);
				}
				else{
					uintE difference = cur_edge - last_write_edge;
					offset = 0; //compressEdge(tail, 0, difference);
				}
			tail += offset;
			last_write_edge = cur_edge;
			new_deg++;

			}
		}
	}
	return new_deg;
}	
#endif
