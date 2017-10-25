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

uintT encode_data(uintE d, long dataCurrentOffset, uchar*start){
	uintT code;
//	cout << "difference: " << d << endl;
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
			code = encode_data(difference, storeDOffset, start);
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
	long storeCurrentOffset = currentOffset;
	uintT shift = 2;
	uintE difference;
	for (uintT edgeI = 1; edgeI < degree; edgeI++){
			difference = savedEdges[edgeI] - savedEdges[edgeI - 1];
	//		cout << "edge, edge, difference: " << savedEdges[edgeI] << " " << savedEdges[edgeI - 1] << " " << difference << endl;
			if(shift == 8){
				shift = 0;
				start[storeCurrentOffset] = storeKey;
				storeCurrentOffset += sizeof(uchar);
				storeKey = 0;
			}		
			// figure out control 
			code = encode_data(difference, storeDOffset, start);
			storeDOffset += (code + 1)*sizeof(uchar); 
			storeKey |= (code << shift);
			shift +=2;
	}
	
	return storeDOffset;
}


long sequentialCompressEdgeSet(uchar *edgeArray, long currentOffset, uintT degree, uintE vertexNum, uintE *savedEdges){
	// First save offset of first edge compression, then loop through the vertex's degree number - 1 edges
	// Return offset of last int
	if(degree > 0){
		// number of bytes needed for control bits
		uintT controlLength = (degree + 3)/4;
		// offset where to start storing data (after control bytes)
		long dataCurrentOffset = currentOffset + controlLength;
		// shift and key always = 0 for first edge
		uintT key = compressFirstEdge(edgeArray, currentOffset, dataCurrentOffset, vertexNum, savedEdges[0]);
		dataCurrentOffset += (1 + key)*sizeof(uchar);
		if(degree == 1){
			edgeArray[currentOffset] = key;
			return dataCurrentOffset;
		}
		// scalar version:
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
	//	charsUsedArr[i] = ceil((degrees[i]*9)/8)+4;
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

// skeleton of decoding functions
template <class T>
	inline void decode(T t, uchar* edgeStart, const uintE &source, const uintT &degree, const bool par=true){
	size_t edgesRead = 0;
	if(degree > 0) {
	uchar controlLength = (degree + 3)/4;
	uchar *dataOffset = edgeStart + controlLength;
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
	}
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
		// weight
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
