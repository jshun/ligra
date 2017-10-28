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
		memcpy((edgeArray+dataOffset),&d,1);
		code = 0;
	}
	else if( d < (1 << 16)){
		memcpy((edgeArray+dataOffset),&d,2);
		//edgeArray[dataOffset] = d;
		code = 1;
	}
	else if (d < (1 << 24)){
		memcpy((edgeArray+dataOffset),&d,3);
		//edgeArray[dataOffset] = d;
		code = 2;
	}
	else{
		memcpy((edgeArray+dataOffset),&d,4);
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
	for (uintT edgeI = 1; edgeI < count; edgeI++){

	// the dataCurrentOffset should point to same place as currentOffset to begin with?
	
	return storeDOffset;
}


long svb_encode_scalar(const uint32_t *in, long controlOff, long dataOff, uintT countLeft, uchar* &start){
	if (countLeft = 0){
		return dataOff;
	
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
		dataOff += sizeof(uchar)*(code+1);
		key |= code << shift;
		shift += 2;
	}
	start[controlOff] = key;
	return dataOff;	
}

// This function writes the control bits and returns the length of data needd by these four elements
size_t streamvbyte_encode4(__m128i in, long outData, long outCode);

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
	
	_mm_storeu_si128((__m128i *)(start + outData), outAligned);
	cout << "code: " << code << endl;
	start[outCode] = code;
	cout << "length: " << length << endl;
	return length;
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
		intE preCompress = savedEdges[0] - vertexNum;
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
		for(uintT edgeI = 0; edgeI < count; edgeI++){
<<<<<<< HEAD
			__m128i vin = _mm_loadu_si128((__m128i *)(difference + 4*edgeI));
			dataOffset += streamvbyte_encode4(vin, dataOffset, currentOffset, edgeArray);
			currentOffset++;
		}
		// encode any leftovers (count) mod 4
		dataOffset = svb_encode_scalar(difference + 4*count, currentOffset, dataOffset, degree - 4*count, edgeArray); // need to change what this is assigned to..
	//	return dataPtr;
		return dataOffset; 
=======
			__m128i vin = _mm_loadu_si128((__m128i *)(difference + 4*count));
			dataPtr += streamvbyte_encode4(vin, dataPtr, controlPtr);
			controlPtr++;
		}
		// encode any leftovers (count) mod 4
		dataPtr = svb_encode_scalar(difference + 4*count, controlPtr, dataPtr, degree - 4*count); // need to change what this is assigned to..
	//	return dataPtr;
		return (dataPtr - edgeArray); 
>>>>>>> 8230330b003745994768532145f1ae9914ffb186
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


static inline __m128i _decode_avx(uintT key, const uint8_t *dataPtrPtr){
	uint8_t len = lengthTable[key];
	__m128i Data = _mm_loadu_si128((__m128i *)*dataPtrPtr);
	__m128i Shuf = *(__m128i *)&shuffleTable[key];

	Data = _mm_shuffle_epi8(Data,Shuf); 
	*dataPtrPtr += len; 
	return Data; 
}


static inline void _write_avx(uintT *out, __m128i Vec){
	_mm_storeu_si128((__m128i *)out, Vec);
}
// skeleton of decoding functions
template <class T>
	inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
	size_t edgesRead = 0;
	if(degree > 0) {
	uchar controlLength = (degree + 3)/4;
	uchar *dataPtr = edgeStart + controlLength;
	if (controlLength >= 8){
		int64_t Offset = -(int64_t)controlLength /8 + 1; 
		const uint64_t *keyPtr64  = (const uint64_t *)edgeStart - Offset;
		uint64_t nextkeys;
		memcpy(&nextkeys, keyPtr64+Offset, sizeof(nextkeys));
		uint64_t keys;
	for(; Offset !=0 ; ++Offset){
		keys = nextkeys;
		memcpy(&nextkeys, keyPtr64 + Offset + 1, sizeof(nextkeys));
		// need to figure out where first edge is & check sign bit...may want to come up with different method for decoding sign..

		Data = _decode_avx((keys & 0xFF), &dataPtr		
	}

		
	}
	/*

		uintE startEdge = eatFirstEdge(edgeStart, source, dataOffset);
//		cout << "first decompressed edge: " << startEdge << endl;
		if(!t.srcTarg(source,startEdge,edgesRead)){
			return;
		}
	
	uintT shift = 2;
	uintE edge = startEdge;
	for (edgesRead = 1; edgesRead < degree; edgesRead++){
		if(shift == 8){
			edgeStart++;
			shift = 0;
		}
		// need to figure out how much to increment dataOffset by
		uintE edgeRead = eatEdge(edgeStart, dataOffset, shift);
		edge = edge + edgeRead;
//		cout << "decompressed edgeread and edge value : " << edgeRead << " " << edge << endl;
		startEdge = edge;
		if (!t.srcTarg(source, edge, edgesRead)){
			break;
		}
		shift += 2;
	}
	}*/
};

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
