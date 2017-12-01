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

template <class T>
inline bool eatByte(uchar controlKey, long & dOffset, uintE source, T t, uintE & startEdge, long & edgesRead, uchar* start){
  uintE edgeRead;// = 0;
  //uintE startEdge = *startEdgePtr;
  uintE edge;// = 0; 
  //uintE edgesRead = *edgesReadPtr;
  //uintT num_bytes = 0;
  switch(controlKey){
  case 0: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 1: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 2: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 3: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 4: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 5: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 6: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 7: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 8: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 9: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 10: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 11: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 12: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 13: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 14: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 15: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 16: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 17: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 18: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 19: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 20: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 21: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 22: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 23: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 24: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 25: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 26: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 27: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 28: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 29: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 30: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 31: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 32: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 33: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 34: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 35: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 36: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 37: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 38: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 39: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 40: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 41: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 42: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 43: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 44: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 45: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 46: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 47: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 48: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 49: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 50: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 51: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 52: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 53: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 54: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 55: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 56: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 57: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 58: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 59: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 60: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 61: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 62: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 63: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 64: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 65: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 66: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 67: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 68: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 69: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 70: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 71: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 72: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 73: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 74: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 75: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 76: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 77: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 78: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 79: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 80: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 81: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 82: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 83: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 84: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 85: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 86: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 87: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 88: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 89: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 90: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 91: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 92: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 93: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 94: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 95: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 96: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 97: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 98: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 99: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 100: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 101: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 102: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 103: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 104: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 105: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 106: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 107: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 108: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 109: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 110: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 111: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 112: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 113: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 114: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 115: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 116: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 117: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 118: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 119: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 120: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 121: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 122: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 123: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 124: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 125: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 126: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 127: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 128: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 129: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 130: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 131: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 132: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 133: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 134: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 135: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 136: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 137: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 138: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 139: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 140: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 141: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 142: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 143: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 144: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 145: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 146: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 147: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 148: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 149: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 150: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 151: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 152: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 153: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 154: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 155: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 156: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 157: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 158: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 159: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 160: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 161: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 162: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 163: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 164: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 165: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 166: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 167: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 168: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 169: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 170: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 171: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 172: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 173: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 174: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 175: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 176: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 177: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 178: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 179: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 180: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 181: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 182: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 183: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 184: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 185: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 186: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 187: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 188: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 189: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 190: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 191: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 192: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 193: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 194: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 195: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 196: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 197: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 198: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 199: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 200: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 201: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 202: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 203: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 204: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 205: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 206: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 207: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 208: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 209: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 210: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 211: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 212: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 213: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 214: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 215: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 216: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 217: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 218: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 219: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 220: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 221: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 222: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 223: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 224: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 225: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 226: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 227: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 228: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 229: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 230: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 231: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 232: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 233: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 234: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 235: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 236: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 237: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 238: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 239: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 240: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 241: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 242: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 243: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 244: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 245: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 246: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 247: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 248: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 249: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 250: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 251: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 252: 
    {
      edgeRead = start[dOffset++];
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 253: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8);
      dOffset += 2;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  case 254: 
    {
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16);
      dOffset += 3;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      break;
    }
  default: 
    { 
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
      edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
      dOffset += 4;
      edge = startEdge + edgeRead;
      startEdge = edge;
      edgesRead++;
      if (!t.srcTarg(source, edge, edgesRead)) return 1;
    }
  }  	
  asm volatile ("" : "+r" (edge)); 
  //*edgesReadPtr = edgesRead;
  //*startEdgePtr = startEdge;
  return 0;
  };

// decode remaining edges
inline uintE eatEdge(uchar controlKey, long & dOffset, intT shift, long controlOffset, uchar* start){
  // check two bit code in control stream
  uintT checkCode = (controlKey >> shift) & 0x3;
  uintE edgeRead = 0;
  switch(checkCode) {
    // 1 byte
  case 0:
    edgeRead = start[dOffset++]; 
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
    //		memcpy(&edgeRead, &start[dOffset], 3);	
    dOffset += 3;
    break;
    // 4 bytes
  default:
    edgeRead = start[dOffset] + (start[dOffset+1] << 8) + (start[dOffset+2] << 16) + (start[dOffset+3] << 24);
    //		memcpy(&edgeRead, &start[dOffset], 4);
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
	if(toCompress < (1 << 7)) {
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
	long *charsUsedArr = newA(long, n);
	long *compressionStarts = newA(long, n+1);
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

		
//		cout << "source, charsUsed " << i << ", " << count << endl;
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
//	cout << "in decode" << endl;
	long edgesRead = 0;
	if(degree > 0) {
		// space used by control stream
		uintT controlLength = (degree + 3)/4;
		// set data pointer to location after control stream (ie beginning of data stream)
		long currentOffset = 0;
		long dataOffset = controlLength + currentOffset;
		uchar key = edgeStart[currentOffset];
		uintE startEdge = eatFirstEdge(key, source, dataOffset, currentOffset, edgeStart);
		if(!t.srcTarg(source,startEdge,edgesRead)){
			return;
		}
		// if degree <= 4, process remaining. If > 4, process the other three edges in first edge control byte
		long scalar_count = (degree > 4) ? 4 : degree;
		uintT shift = 2;
		uintE edge = startEdge;
		for(size_t i = 1; i < scalar_count; i++){
			uintE edgeRead = eatEdge(key, dataOffset, shift, currentOffset, edgeStart);
			edge = edgeRead + startEdge;
			asm volatile ("" : "+r" (edge)); 
			startEdge = edge;
			edgesRead++;
			if(!t.srcTarg(source, edge, edgesRead)){
				return;
			}
			shift += 2;
		}
		// process the rest of the bytes
	if(degree > 4){
		long count_four = degree/4;
		scalar_count = degree - 4*count_four;
		currentOffset++;
		key = edgeStart[currentOffset];
		bool break_var = 0;
		for(long i = 1; i < count_four; i++){
			//process bytes!
			break_var = eatByte(key, dataOffset, source, t, startEdge, edgesRead, edgeStart);
			if(break_var){
				return;
			}
			currentOffset++;
			key = edgeStart[currentOffset];
		}
		shift = 0;
		for(uintT i = 0; i < scalar_count; i++){
			uintE edgeRead = eatEdge(key, dataOffset, shift, currentOffset, edgeStart);
			edge = edgeRead + startEdge;
			asm volatile ("" : "+r" (edge)); 
			startEdge = edge;
			edgesRead++;
			if(!t.srcTarg(source, edge, edgesRead)){
				break;
			}
			shift += 2;
		}
	}
//cout << "edgesRead " << edgesRead << " degree " << degree << endl;
//	cout << "source, dataOffset " << source << ", " << dataOffset << endl;

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
