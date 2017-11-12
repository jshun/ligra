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

// to use this compression scheme, compile with STREAMCASE=1 make -j

typedef unsigned char uchar;

// decode weights
inline intE eatWeight(uchar* &start, uchar* &dOffset, intT shift){
	uchar fb = *start;
	// check two bit code in control stream
	uintT checkCode = (fb >> shift) & 0x3;
	uintE edgeRead = 0;
	uintE *edgeReadPtr = &edgeRead;
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
	uintE edgeRead = 0;
	uintE *edgeReadPtr = &edgeRead;
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

template <class T>
inline bool eatEdge_scalar(uchar* &start, uchar* &dOffset, intT shift, const uintE &source, T t, size_t* edgesReadPtr, uintE* edgePtr){
	uchar fb = *start;
	// check two bit code in control stream
	uintT checkCode = (fb >> shift) & 0x3;
	uintE edgeRead = 0;
	uintE *edgeReadPtr = &edgeRead;
	bool break_var = 0;
	uintE edge = *edgePtr;
	size_t edgesRead = *edgesReadPtr;
	// 1 byte
	if(checkCode == 0){
		edgeRead = *dOffset; 
		dOffset += 1;
	}
	// 2 bytes
	else if(checkCode == 1){
		memcpy(&edgeRead, dOffset, 2);
		dOffset += 2;
	}
	// 3 bytes
	else if(checkCode == 2){
		memcpy(&edgeRead, dOffset, 3);
		dOffset += 3;
	}
	// 4 bytes
	else{
		memcpy(&edgeRead, dOffset, 4);
		dOffset += 4;
	}
	edge = *edgeReadPtr + edge;
	*edgePtr = edge;
	edgesRead++;
	 if (!t.srcTarg(source, edge, edgesRead)){
                break_var = 1;    
        }
	*edgesReadPtr = edgesRead;	
	return break_var;
}

// decode remaining edges
template <class T>
inline bool eatEdge(uchar* &start, uchar* &dOffset, uintE *edgePtr, const uintE &source, T t, size_t* edgesReadPtr){
	uintE edgeRead =0;
	uintE* edgeReadPtr = &edgeRead;
	bool break_var = 0;
	uintE edge = *edgePtr;
	uintT edgesRead = *edgesReadPtr;
 	uchar control_byte = *start;

	switch(control_byte){
			case 0: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 1: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 2: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 3: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 4: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 5: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 6: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 7: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 8: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 9: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 10: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 11: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 12: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 13: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 14: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 15: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 16: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 17: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 18: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 19: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 20: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 21: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 22: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 23: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 24: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 25: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 26: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 27: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 28: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 29: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 30: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 31: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 32: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 33: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 34: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 35: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 36: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 37: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 38: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 39: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 40: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 41: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 42: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 43: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 44: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 45: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 46: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 47: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 48: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 49: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 50: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 51: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 52: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 53: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 54: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 55: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 56: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 57: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 58: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 59: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 60: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 61: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 62: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 63: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 64: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 65: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 66: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 67: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 68: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 69: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 70: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 71: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 72: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 73: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 74: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 75: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 76: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 77: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 78: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 79: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 80: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 81: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 82: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 83: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 84: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 85: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 86: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 87: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 88: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 89: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 90: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 91: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 92: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 93: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 94: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 95: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 96: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 97: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 98: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 99: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 100: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 101: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 102: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 103: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 104: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 105: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 106: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 107: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 108: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 109: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 110: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 111: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 112: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 113: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 114: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 115: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 116: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 117: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 118: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 119: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 120: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 121: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 122: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 123: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 124: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 125: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 126: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 127: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 128: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 129: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 130: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 131: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 132: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 133: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 134: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 135: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 136: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 137: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 138: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 139: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 140: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 141: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 142: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 143: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 144: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 145: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 146: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 147: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 148: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 149: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 150: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 151: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 152: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 153: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 154: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 155: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 156: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 157: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 158: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 159: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 160: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 161: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 162: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 163: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 164: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 165: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 166: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 167: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 168: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 169: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 170: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 171: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 172: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 173: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 174: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 175: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 176: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 177: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 178: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 179: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 180: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 181: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 182: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 183: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 184: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 185: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 186: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 187: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 188: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 189: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 190: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 191: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 192: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 193: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 194: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 195: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 196: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 197: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 198: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 199: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 200: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 201: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 202: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 203: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 204: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 205: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 206: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 207: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 208: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 209: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 210: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 211: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 212: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 213: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 214: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 215: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 216: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 217: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 218: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 219: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 220: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 221: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 222: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 223: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 224: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 225: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 226: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 227: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 228: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 229: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 230: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 231: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 232: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 233: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 234: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 235: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 236: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 237: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 238: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 239: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 240: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 241: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 242: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 243: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 244: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 245: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 246: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 247: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 248: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 249: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 250: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 251: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 252: 
		{
			 memcpy(&edgeRead, dOffset, 1);
			 dOffset += 1;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 253: 
		{
			 memcpy(&edgeRead, dOffset, 2);
			 dOffset += 2;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 254: 
		{
			 memcpy(&edgeRead, dOffset, 3);
			 dOffset += 3;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}
		case 255: 
		{
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + *edgeReadPtr;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 memcpy(&edgeRead, dOffset, 4);
			 dOffset += 4;
			 edge = edge + edgeRead;
			 edgesRead++;
			 if (!t.srcTarg(source, edge, edgesRead)){
				 return 1;
				 }
			 break;
		}

	default: return 1;
	}
*edgesReadPtr = edgesRead;
*edgePtr = edge;
return 0;	
}

uchar compressFirstEdge(uchar *start, long controlOffset, long dataOffset, uintE source, uintE target){
	uchar* saveStart = start;
	long saveOffset = dataOffset;
	
	intE preCompress = (intE) target - source;
	uintE toCompress = abs(preCompress);
	uintT signBit = (preCompress < 0) ? 1:0;
	uchar code; 
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
//	cout << "first edge" << endl;
	return code;
}

// store compressed data
uchar encode_data(uintE d, long dataCurrentOffset, uchar*start){
	uchar code;
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
	uchar code = 0;
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
				storeCurrentOffset += sizeof(uchar);
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
//	uintT *degrees = newA(uintT, n+1); 
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
	{parallel_for(long i=0;i<n;i++){
//		degrees[i] = Degrees[i];
		charsUsedArr[i] = 5*Degrees[i];
//		charsUsedArr[i] = ceil((degrees[i]*9)/8) + 4;
	}}
//	degrees[n] = 0;
//	sequence::plusScan(charsUsedArr, charsUsedArr, n+1);
	long toAlloc = sequence::plusScan(charsUsedArr, charsUsedArr, n);
	uintE* iEdges = newA(uintE, toAlloc);

	{parallel_for(long i=0;i<n;i++){
		edgePts[i] = iEdges+charsUsedArr[i];
		long charsUsed = sequentialCompressEdgeSet((uchar *)(iEdges+charsUsedArr[i]), 0, Degrees[i], i, edges + offsets[i]);
		charsUsedArr[i] = charsUsed;
	}}
	// produce the total space needed for all compressed lists in chars
	long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
	compressionStarts[n] = totalSpace;
//	free(degrees);
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
		
	bool break_var = 0;
	uintE edge = startEdge;		
	if (degree < 5){
		uintT shift = 2;
//		cout << "degree: " << degree  << endl;
		for (int i=1; i < degree; i++){
//			cout << "in remaining loop degree<5" << endl;
			break_var = eatEdge_scalar(edgeStart, dataOffset, shift, source, t,&edgesRead, &edge); 
			if(break_var){
				break;
			}
			shift+= 2;		
		}
	}
	else{
		uintT shift = 2;
//	cout << "degree: " << degree << endl;
		for (int i=1; i < 4; i++){
//			cout << "in remaining loop1" << endl;
			break_var = eatEdge_scalar(edgeStart, dataOffset, shift, source, t,&edgesRead, &edge); 
			if(break_var){
				break;
			}
			shift+= 2;		
		}

	uchar block_four = degree/4;
	edgeStart++;
	for (int i= 1; i < block_four; i++){
		// decode stored value and add to previous edge value (stored using differential encoding)
		break_var = eatEdge(edgeStart, dataOffset, &edge, source, t, &edgesRead);
	//	cout << "edge: " << edge << " edgesRead: " << edgesRead << endl;
		if (break_var){
			break;
		}
		edgeStart++;
	}

	// finish any last edges for mod 4
	uchar remaining = degree - block_four*4;
	shift = 0;
//	cout << "degree: " << degree << " remaining: " << remaining << endl;
	for (int i=0; i < remaining; i++){
//		cout << "in remaining loop2" << endl;
		break_var = eatEdge_scalar(edgeStart, dataOffset, shift, source, t,&edgesRead, &edge); 
		if(break_var){
			break;
		}
		shift+= 2;		
		//edgeStart++;
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
			// remember to delete:
				uintE edgeRead = 0;	
		//	uintE edgeRead = eatEdge(edgeStart, dataOffset, shift);
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
