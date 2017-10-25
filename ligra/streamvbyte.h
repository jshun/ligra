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

// to use this compression scheme, compile with STREAMVBYTE=1 make -j

typedef unsigned char uchar;

// decode weights
inline intE eatWeight(uchar* &start, uchar* &dOffset, intT shift){
	uchar fb = *start;
	// check two bit code in control stream
	uintT checkCode = (fb >> shift) & 0x3;
	intE edgeRead = 0;
	intE *edgeReadPtr = &edgeRead;
	uchar signBit = 0;
	// 1 byte
	if(checkCode == 0){
		edgeRead = *dOffset;
		// check sign bit and then get rid of it from actual value 
		signBit =(uchar)(((edgeRead) & 0x80) >> 7);
		edgeRead &= 0x7F;
		// incrememnt the offset to data by 1 byte
		dOffset += 1;
	}
	// 2 bytes
	else if(checkCode == 1){
		memcpy(&edgeReadPtr, dOffset, 2);
		signBit = (uchar)(((1 << 15) & edgeRead) >> 15);
		edgeRead = edgeRead & 0x7FFF; 
		dOffset += 2;
	}
	// 3 bytes
	else if(checkCode == 2){
		memcpy(&edgeReadPtr, dOffset, 3);
		signBit = (uchar)(((edgeRead) & (1 <<23)) >> 23);
		edgeRead = edgeRead & 0x7FFFFF;
		dOffset += 3;
	}
	// 4 bytes
	else{
		memcpy(&edgeReadPtr, dOffset, 4);
		signBit = (uchar)(((edgeRead) & (1 << 31)) >> 31);
		edgeRead = (edgeRead) & 0x7FFFFFFF; 
		dOffset += 4;
	}

	return (signBit) ? -edgeRead : edgeRead;
}

// decode first edge
inline intE eatFirstEdge(uchar* &start, uintE source, uchar* &dOffset){
	uchar fb = *start;
	// check the two bit code in control stream
	uintT checkCode = (fb) & 0x3;
	intE edgeRead = 0;
	intE *edgeReadPtr = &edgeRead;
	uchar signBit = 0;
	// 1 byte
	if(checkCode == 0){
		edgeRead = *dOffset;
		// check sign bit and then get rid of it from actual value 
		signBit = (uchar)(((edgeRead) & 0x80) >> 7);
		edgeRead &= 0x7F;
		dOffset += 1;
	}
	// 2 bytes
	else if(checkCode == 1){
		memcpy(&edgeReadPtr, dOffset, 2);
		signBit =(uchar) (((1 << 15) & edgeRead) >> 15);
		edgeRead = edgeRead & 0x7FFF; 
		dOffset += 2;
	}
	// 3 bytes
	else if(checkCode == 2){
		memcpy(&edgeReadPtr, dOffset, 3);
		signBit = (uchar)(((edgeRead) & (1 <<23)) >> 23);
		edgeRead = edgeRead & 0x7FFFFF;
		dOffset += 3;
	}
	// 4 bytes
	else{
		memcpy(&edgeReadPtr, dOffset, 4);
		signBit = (uchar)((edgeRead) & (1 << 31)) >> 31;
		edgeRead = (edgeRead) & 0x7FFFFFFF; 
		dOffset += 4;
	}
	return (signBit) ? source - edgeRead : source + edgeRead;
}

// decode remaining edges
inline uintE eatEdge(uchar* &start, uchar* &dOffset, intT shift){
	uchar fb = *start;
	// check two bit code in control stream
	uintT checkCode = (fb >> shift) & 0x3;
	uintE edgeRead = 0;
	uintE *edgeReadPtr = &edgeRead;
	// 1 byte
	if(checkCode == 0){
		edgeRead = *dOffset; 
		dOffset += 1;
	}
	// 2 bytes
	else if(checkCode == 1){
		memcpy(&edgeReadPtr, dOffset, 2);
		dOffset += 2;
	}
	// 3 bytes
	else if(checkCode == 2){
		memcpy(&edgeReadPtr, dOffset, 3);
		dOffset += 3;
	}
	// 4 bytes
	else{
		memcpy(&edgeReadPtr, dOffset, 4);
		dOffset += 4;
	}
	
	return edgeRead;
}

intT compressFirstEdge(uchar *start, long controlOffset, long dataOffset, uintE source, uintE target){
	uchar* saveStart = start;
	long saveOffset = dataOffset;
	
	intE preCompress = (intE) target - source;
	intE toCompress = abs(preCompress);
	intT signBit = (preCompress < 0) ? 1:0;
	intT code; 
	// check how many bytes is required to store the data and a sign bit (MSB)
	if(toCompress < (1 << 7)) {
		// concatenate sign bit with data and store
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

	return code;
}

// store compressed data
uintT encode_data(uintE d, long dataCurrentOffset, uchar*start){
	uintT code;
	// figure out how many bytes needed to store value and set code accordingly
	if(d < (1 << 8)){
		start[dataCurrentOffset] = d;
		code = 0;
	}
	else if( d < (1 << 16)){
		start[dataCurrentOffset] = d;
		code = 1;
	}
	else if (d < (1 << 24)){
		start[dataCurrentOffset] = d;
		code = 2;
	}
	else{
		start[dataCurrentOffset] = d;
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

long compressEdge(uchar *start, long currentOffset, uintE *savedEdges, uintT key, long dataCurrentOffset, uintT degree){
	uintT code = 0;
	uintT storeKey = key;
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
				storeCurrentOffset += sizeof(uchar);
				storeKey = 0;
			}		
			// encode and store data in data stream
			code = encode_data(difference, storeDOffset, start);
			storeDOffset += (code + 1)*sizeof(uchar); 
			storeKey |= (code << shift);
			shift +=2;
	}
	
	return storeDOffset;
}


long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, uintE *savedEdges){
	if(degree > 0){
		// number of bytes needed for control stream
		uintT controlLength = (degree + 3)/4;
		// offset where to start storing data (after control bytes)
		long dataCurrentOffset = currentOffset + controlLength;
		// shift and key always = 0 for first edge
		uintT key = compressFirstEdge(edgeArray, currentOffset, dataCurrentOffset, vertexNum, savedEdges[0]);
		// offset data pointer by amount of space required by first edge
		dataCurrentOffset += (1 + key)*sizeof(uchar);
		if(degree == 1){
			edgeArray[currentOffset] = key;
			return dataCurrentOffset;
		}
		// scalar version: compress the rest of the edges
		return  compressEdge(edgeArray, currentOffset, savedEdges, key, dataCurrentOffset, degree);
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
		charsUsedArr[i] = (degrees[i] + 3)/4 + degrees[i]*4;
	}}
	degrees[n] = 0;
	sequence::plusScan(degrees, degrees, n+1);
	long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
	uintE* iEdges = newA(uintE, toAlloc);

	{parallel_for(long i=0;i<n;i++){
		edgePts[i] = iEdges+charsUsedArr[i];
		long charsUsed = sequentialCompressEdgeSet((uchar *)(iEdges+charsUsedArr[i]), 0, degrees[i+1]-degrees[i], i, edges + offsets[i]);
		charsUsedArr[i] = charsUsed;
	}}

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


template <class T>
	inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
	size_t edgesRead = 0;
	if(degree > 0) {
	// space used by control stream
	uchar controlLength = (degree + 3)/4;
	// set data pointer to location after control stream (ie beginning of data stream)
	uchar *dataOffset = edgeStart + controlLength;
		uintE startEdge = eatFirstEdge(edgeStart, source, dataOffset);
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
		// decode stored value and add to previous edge value (stored using differential encoding)
		uintE edgeRead = eatEdge(edgeStart, dataOffset, shift);
		edge = edge + edgeRead;
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
			uchar *dataOffset = edgeStart + controlLength;
			uintE startEdge = eatFirstEdge(edgeStart, source, dataOffset);	
			intE weight = eatWeight(edgeStart, dataOffset, 2);	
			if (!t.srcTarg(source, startEdge, weight, edgesRead)){
				return;
			}
			uintT shift = 4;
			uintE edge = startEdge;
			for (edgesRead = 1; edgesRead < degree; edgesRead++){
				// if finished reading byte in control stream, increment and reset shift
				if(shift == 8){
					edgeStart++;
					shift = 0;
				}
				uintE edgeRead = eatEdge(edgeStart, dataOffset, shift);
				edge = edge + edgeRead;
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
	uintT *degrees = newA(uintT, n+1);
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
	{parallel_for(long i=0; i<n; i++){
		degrees[i] = Degrees[i];
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
		uchar controlLength  = (degree + 3)/4;
		uchar *dataOffset  = edge_start + controlLength;	
		uintE start_edge = eatFirstEdge(cur, source, dataOffset);
		last_read_edge = start_edge;
		if (pred(source, start_edge)){
			long offset = compressFirstEdge(tail, 0, source, start_edge);
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
					offset = compressFirstEdge(tail, 0, source, cur_edge);
				}
				else{
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
#endif
