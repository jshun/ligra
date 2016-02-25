#ifndef COMPRESSED_VERTEX_H
#define COMPRESSED_VERTEX_H

#ifndef PD 
#ifdef BYTE
#include "byte.h"
#elif defined NIBBLE
#include "nibble.h"
#else
#include "byteRLE.h"
#endif
#else //decode in parallel
#ifdef BYTE
#include "byte-pd.h"
#elif defined NIBBLE
#include "nibble-pd.h"
#else
#include "byteRLE-pd.h"
#endif
#endif

struct compressedSymmetricVertex {
  uchar* neighbors;
  uintT degree;
  uchar* getInNeighbors() { return neighbors; }
  uchar* getOutNeighbors() { return neighbors; }
  uintT getInDegree() { return degree; }
  uintT getOutDegree() { return degree; }
  void setInNeighbors(uchar* _i) { neighbors = _i; }
  void setOutNeighbors(uchar* _i) { neighbors = _i; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
  void flipEdges() {}
  void del() {}
};

struct compressedAsymmetricVertex {
  uchar* inNeighbors;
  uchar* outNeighbors;
  uintT outDegree;
  uintT inDegree;
  uchar* getInNeighbors() { return inNeighbors; }
  uchar* getOutNeighbors() { return outNeighbors; }
  uintT getInDegree() { return inDegree; }
  uintT getOutDegree() { return outDegree; }
  void setInNeighbors(uchar* _i) { inNeighbors = _i; }
  void setOutNeighbors(uchar* _i) { outNeighbors = _i; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
  void flipEdges() { swap(inNeighbors,outNeighbors); 
    swap(inDegree,outDegree); }
  void del() {}
};

#endif
