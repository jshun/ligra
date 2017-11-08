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
inline intE eatWeight(uchar controlKey, long* dOffset, intT shift, long controlOffset, uchar* start){
//	uchar fb = start[controlOffset];
	// check two bit code in control stream
	uintT checkCode = (controlKey >> shift) & 0x3;
	intE edgeRead = 0;
	intE *edgeReadPtr = &edgeRead;
	uchar signBit = 0;
	long saveOffset = *dOffset;
	// 1 byte
	if(checkCode == 0){
		edgeRead = start[saveOffset];
		// check sign bit and then get rid of it from actual value 
		signBit =(uchar)(((edgeRead) & 0x80) >> 7);
		edgeRead &= 0x7F;
		// incrememnt the offset to data by 1 byte
		(*dOffset) += 1;
	}
	// 2 bytes
	else if(checkCode == 1){
		memcpy(&edgeReadPtr, start + saveOffset*sizeof(uchar), 2);
		signBit = (uchar)(((1 << 15) & edgeRead) >> 15);
		edgeRead = edgeRead & 0x7FFF; 
		(*dOffset) += 2;
	}
	// 3 bytes
	else if(checkCode == 2){
		memcpy(&edgeReadPtr, start + saveOffset*sizeof(uchar), 3);
		signBit = (uchar)(((edgeRead) & (1 <<23)) >> 23);
		edgeRead = edgeRead & 0x7FFFFF;
		(*dOffset) += 3;
	}
	// 4 bytes
	else{
		memcpy(&edgeReadPtr, start+saveOffset*sizeof(uchar), 4);
		signBit = (uchar)(((edgeRead) & (1 << 31)) >> 31);
		edgeRead = (edgeRead) & 0x7FFFFFFF; 
		(*dOffset) += 4;
	}

	return (signBit) ? -edgeRead : edgeRead;
}

// decode first edge
inline intE eatFirstEdge(uchar controlKey, uintE source, long* dOffset, long controlOffset, uchar* start){
//	uchar fb = start[controlOffset];
	// check the two bit code in control stream
	uintT checkCode = (controlKey) & 0x3;
//	checkCode = 0;
	intE edgeRead = 0;
	intE *edgeReadPtr = &edgeRead;
	uchar signBit = 0;
	long saveOffset = *dOffset;
	// 1 bytei
//	cout << "sizeof(intE) " << sizeof(intE) << " sizeof(uintE) " << sizeof(uintE) << " sizeof(uchar) " << sizeof(uchar) << endl;
	if(checkCode == 0){
		edgeRead = start[saveOffset];
		// check sign bit and then get rid of it from actual value 
		signBit = (uchar)(((edgeRead) & 0x80) >> 7);
		edgeRead &= 0x7F;
		(*dOffset) += 1;
	}
	// 2 bytes
	else if(checkCode == 1){
		memcpy(&edgeRead, &start[saveOffset], 2);
		signBit =(uchar) (((1 << 15) & edgeRead) >> 15);
		edgeRead = edgeRead & 0x7FFF; 
		(*dOffset) += 2;
	}
	// 3 bytes
	else if(checkCode == 2){
		memcpy(&edgeRead, &start[saveOffset], 3);
//		memcpy(&edgeRead, start+saveOffset*sizeof(uchar), 3);
		signBit = (uchar)(((edgeRead) & (1 <<23)) >> 23);
		edgeRead = edgeRead & 0x7FFFFF;
		(*dOffset) += 3;
	}
	// 4 bytes
	else{
		memcpy(&edgeRead, &start[saveOffset], 4);
//		memcpy(&edgeRead, start+saveOffset*sizeof(uchar), 4);
		signBit = (uchar)((edgeRead) & (1 << 31)) >> 31;
		edgeRead = (edgeRead) & 0x7FFFFFFF; 
		(*dOffset) += 4;
	}
	return (signBit) ? source - edgeRead : source + edgeRead;
}

// decode remaining edges
inline uintE eatEdge(uchar controlKey, long* dOffset, intT shift, long controlOffset, uchar* start){
//	uchar fb = start[controlOffset];
	// check two bit code in control stream
	uintT checkCode = (controlKey >> shift) & 0x3;
//	cout << "fb: " << (uintE)(fb) << " checkCode: " << checkCode << endl;

	//hard code checkCode for debugging
//	checkCode = 0;

	uintE edgeRead = 0;
	uintE *edgeReadPtr = &edgeRead;
	long saveOffset = *dOffset;
	// 1 byte
	if(checkCode == 0){
		edgeRead = (uintE)(start[saveOffset]); 
		(*dOffset) += 1;
//		cout << "case0 " << endl;
	}
	// 2 bytes
	else if(checkCode == 1){
//		memcpy(edgeReadPtr, dataPtr, 2);
		memcpy(&edgeRead, &start[saveOffset], 2);
//		memcpy(&edgeRead, start+saveOffset*sizeof(uchar), 2);
		(*dOffset) += 2;
//		cout << "case1" << endl;
	}
	// 3 bytes
	else if(checkCode == 2){
		memcpy(&edgeRead, &start[saveOffset], 3);	
//		memcpy(&edgeRead, start+saveOffset*sizeof(uchar), 3);
//		memcpy(edgeReadPtr, dataPtr, 3);
		(*dOffset) += 3;
//		cout << "case3" << endl;
	}
	// 4 bytes
	else{
		memcpy(&edgeRead, &start[saveOffset], 4);
//		memcpy(&edgeRead, start+saveOffset*sizeof(uchar), 4);
//		memcpy(edgeReadPtr, dataPtr, 4);
		(*dOffset) += 4;
//		cout << "case4" << endl;
	}
//	cout <<"*dOffset after: " <<  (int)(**dOffset) << endl;	
	return edgeRead;
}
uchar compressFirstEdge(uchar* &start, long controlOffset, long dataOffset, uintE source, uintE target){
//	uchar* saveStart = start;
//	long saveOffset = dataOffset;
	
	intE preCompress = (intE) target - source;
	intE toCompress = abs(preCompress);
	intT signBit = (preCompress < 0) ? 1:0;
	uchar code; 
	intE *toCompressPtr = &toCompress;
	// check how many bytes is required to store the data and a sign bit (MSB)
//	cout << "first edge dOffset (should be 0) : " << dataOffset << endl;
	if(toCompress < (1 << 7)) {
		// concatenate sign bit with data and store
		toCompress |= (signBit << 7);
		memcpy(&start[dataOffset], toCompressPtr, 1);
	//	start[dataOffset] = (signBit << 7) | toCompress;
		code = 0;
	}
	else if(toCompress < (1 << 15)){
		toCompress |= (signBit << 15);
		memcpy(&start[dataOffset], toCompressPtr, 2);
	//	start[dataOffset] = (signBit << 15) | toCompress;
		code = 1;
//		cout << "first edge case 1" << endl;
	}
	else if(toCompress < (1 << 23)){
		toCompress |= (signBit << 23);
		memcpy(&start[dataOffset], toCompressPtr, 3);
	//	memcpy(&start[dataOffset], toCompressPtr, 3);
	//	start[dataOffset] = (signBit << 23) | toCompress;
		code = 2;
	}
	else{
		toCompress |= (signBit << 31);
		memcpy(&start[dataOffset], toCompressPtr, 4);
//		memcpy(&start[dataOffset], toCompressPtr, 4);
//		start[dataOffset] = (signBit << 31) | toCompress;
		code = 3;
	}
//	cout << "first edge" << endl;
	return code;
}

// store compressed data
uchar encode_data(uintE d, long dataCurrentOffset, uchar* &start){
	uchar code;
	uintE* diffPtr = &d;
//	cout << "d: " << d << endl;
	// figure out how many bytes needed to store value and set code accordingly
	if(d < (1 << 8)){
//		memcpy(start+sizeof(uchar)*dataCurrentOffset, diffPtr, 1);
//		cout << "case0: " << *(start+dataCurrentOffset);
		memcpy(&start[dataCurrentOffset], diffPtr, 1);
	//	start[dataCurrentOffset] = d;
		code = 0;
//		cout << "case0" << endl;
	}
	else if( d < (1 << 16)){
//		memcpy(start+sizeof(uchar)*dataCurrentOffset, diffPtr, 2);
		memcpy(&start[dataCurrentOffset], diffPtr, 2);
	//	start[dataCurrentOffset] = d;
		code = 1;
//		cout << "case1 d: " << d << " *diffPtr: " << *diffPtr << endl;
	}
	else if (d < (1 << 24)){
	//	memcpy(start+sizeof(uchar)*dataCurrentOffset, diffPtr, 3);
		memcpy(&start[dataCurrentOffset], diffPtr, 3);
	//	start[dataCurrentOffset] = d;
		code = 2;
	}
	else{
	//	memcpy(start+sizeof(uchar)*dataCurrentOffset, diffPtr, 4);
		memcpy(&start[dataCurrentOffset], diffPtr, 4);	
//		start[dataCurrentOffset] = d;
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
//				cout << "storeCurrentOFfset: " << storeCurrentOffset << endl;
//				cout << "storeKey " << (uintE) storeKey << endl;
				start[storeCurrentOffset] = storeKey;
//				uchar test = start[storeCurrentOffset];
//				cout << "start[storecurrentoffset] : " << (uintE)(test) << endl;
				storeCurrentOffset++;
				storeKey = 0;
			}		
			// encode and store data in data stream
			code = encode_data(difference, storeDOffset, start);
			storeDOffset += (code + 1); 
			storeKey |= (code << shift);
		//	cout << "code " << code << " shift: " << shift << endl;
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

uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m, uintE* Degrees){
	// compresses edges for vertices in parallel & prints compression stats
	cout << "parallel compressing, (n,m) = (" << n << "," << m << ")"  << endl;
	uintE **edgePts = newA(uintE*, n);
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
	{parallel_for(long i=0;i<n;i++){		
		charsUsedArr[i] = 5*Degrees[i];
//		charsUsedArr[i] = ceil((degrees[i]*9)/8) + 4;
//		charsUsedArr[i] = (degrees[i] + 3)/4 + 4*degrees[i];
	}}
	long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
	uintE* iEdges = newA(uintE, toAlloc);

	{parallel_for(long i=0;i<n;i++){
		edgePts[i] = iEdges+charsUsedArr[i];
		long charsUsed = sequentialCompressEdgeSet((uchar *)(iEdges+charsUsedArr[i]), 0, Degrees[i], i, edges + offsets[i]);
		//cout << degrees[i] << " " << offsets[i] << " " << charsUsed << endl;
		charsUsedArr[i] = charsUsed;
	}}
	// produce the total space needed for all compressed lists in chars
	long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
	compressionStarts[n] = totalSpace;
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
//	cout << "in decode" << endl;
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
		uintE startEdge = eatFirstEdge(key, source, &dataOffset, currentOffset, edgeStart);
//	cout << "after startEdge" << endl;
		if(!t.srcTarg(source,startEdge,edgesRead)){
			return;
		}	
	uintT shift = 2;
	uintE edge = startEdge;
	for (edgesRead = 1; edgesRead < degree; edgesRead++){
//		cout << "key: " << (uintE)key << endl;
//		cout << "edgesRead" << edgesRead << endl;
		if(shift == 8){
//			cout << "*edgeStart before " << (uintE)(*edgeStart) << endl;
			currentOffset++;
			key = edgeStart[currentOffset];
//			cout << "*edgeStart after " << (uintE)(*edgeStart) << endl; 
			shift = 0;
		}
		// decode stored value and add to previous edge value (stored using differential encoding)
		uintE edgeRead = eatEdge(key, &dataOffset, shift, currentOffset, edgeStart);
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
			uintE startEdge = eatFirstEdge(key, source, &dataOffset, currentOffset, edgeStart);	
			intE weight = eatWeight(key, &dataOffset, 2, currentOffset, edgeStart);	
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
				uintE edgeRead = eatEdge(key, &dataOffset, shift, currentOffset, edgeStart);
				edge = edge + edgeRead;
				shift += 2;

				if(shift == 8){
					currentOffset++;
					key = edgeStart[currentOffset];
					shift = 0;
				}
				intE weight = eatWeight(key, &dataOffset, shift, currentOffset, edgeStart);
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

template <class P>
inline size_t pack(P pred, uchar* edge_start, const uintE &source, const uintE &degree){
	cout << "pack" << endl;
	size_t new_deg = 0;
	uintE last_read_edge = source;
	uintE last_write_edge;
	uchar* tail = edge_start;
	uchar* cur = edge_start;
	cout << "pack" << endl;
	if (degree > 0) {
		uchar controlLength  = (degree + 3)/4;
		long dataOffset  =  controlLength;
		uchar key = *edge_start;	
		uintE start_edge = eatFirstEdge(key, source, &dataOffset, 0, cur);
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
