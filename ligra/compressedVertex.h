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

namespace decode_compressed {
  template <class F>
  struct denseT {
    bool* nextArr, *vertexArr;
  denseT(bool* np, bool* vp) : nextArr(np), vertexArr(vp) {}
#ifndef WEIGHTED
    inline bool srcTarg(F &f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (vertexArr[target] && f.update(target, src)) nextArr[src] = 1;
      return f.cond(src);
    }
#else
    inline bool srcTarg(F &f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (vertexArr[target] && f.update(target, src, weight)) nextArr[src] = 1;
      return f.cond(src);
    }
#endif
  };

  template <class F>
  struct denseForwardT {
    bool* nextArr, *vertexArr;
  denseForwardT(bool* np, bool* vp) : nextArr(np), vertexArr(vp) {}
#ifndef WEIGHTED
    inline bool srcTarg(F &f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (f.cond(target) && f.updateAtomic(src,target)) nextArr[target] = 1;
      return true;
    }
#else
    inline bool srcTarg(F &f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (f.cond(target) && f.updateAtomic(src,target, weight)) nextArr[target] = 1;
      return true;
    }
#endif
  };

  template <class F>
  struct sparseT {
    uintT v, o;
    uintE *outEdges;
  sparseT(uintT vP, uintT oP, uintE *outEdgesP) : v(vP), o(oP), outEdges(outEdgesP) {}
#ifndef WEIGHTED
    inline bool srcTarg(F &f, const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (f.cond(target) && f.updateAtomic(v, target))
        outEdges[o + edgeNumber] = target;
      else outEdges[o + edgeNumber] = UINT_E_MAX;
      return true; }
#else
    inline bool srcTarg(F &f, const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (f.cond(target) && f.updateAtomic(v, target, weight))
        outEdges[o + edgeNumber] = target;
      else outEdges[o + edgeNumber] = UINT_E_MAX;
      return true; }
#endif
  };

  template<class V, class F>
  inline void decodeInNghBreakEarly(V* v, long i, bool* vertexSubset, F &f, bool* next, bool parallel = 0) {
    uintE d = v->getInDegree();
    uchar *nghArr = v->getInNeighbors();
#ifdef WEIGHTED
        decodeWgh(denseT<F>(next, vertexSubset), f, nghArr, i, v->getInDegree());
#else
        decode(denseT<F>(next, vertexSubset), f, nghArr, i, v->getInDegree());
#endif
  }

  template<class V, class F>
  inline void decodeOutNgh(V* v, long i, bool* vertexSubset, F &f, bool* next) {
    uintE d = v->getInDegree();
    uchar *nghArr = v->getOutNeighbors();
#ifdef WEIGHTED
        decodeWgh(denseForwardT<F>(next, vertexSubset), f, nghArr, i, v->getOutDegree());
#else
        decode(denseForwardT<F>(next, vertexSubset), f, nghArr, i, v->getOutDegree());
#endif
  }

  template <class V, class F>
  inline void decodeOutNghSparse(V* v, long i, uintT o, F &f, uintE* outEdges) {
    uchar *nghArr = v->getOutNeighbors();
#ifdef WEIGHTED
    decodeWgh(sparseT<F>(i, o, outEdges), f, nghArr, i, v->getOutDegree());
#else
    decode(sparseT<F>(i, o, outEdges), f, nghArr, i, v->getOutDegree());
#endif
  }

}

struct compressedSymmetricVertex {
  uchar* neighbors;
  uintT degree;
  uchar* getInNeighbors() { return neighbors; }
  uchar* getOutNeighbors() { return neighbors; }
  intT getInNeighbor(intT j) { return -1; } //should not be called
  intT getOutNeighbor(intT j) { return -1; } //should not be called
  uintT getInDegree() { return degree; }
  uintT getOutDegree() { return degree; }
  void setInNeighbors(uchar* _i) { neighbors = _i; }
  void setOutNeighbors(uchar* _i) { neighbors = _i; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
  void flipEdges() {}
  void del() {}

  template<class F>
  inline void decodeInNghBreakEarly(long i, bool* vertexSubset, F &f, bool* next, bool parallel = 0) {
    decode_compressed::decodeInNghBreakEarly<compressedSymmetricVertex, F>(this, i, vertexSubset, f, next, parallel);
  }

  template<class F>
  inline void decodeOutNgh(long i, bool* vertexSubset, F &f, bool* next) {
    decode_compressed::decodeOutNgh<compressedSymmetricVertex, F>(this, i, vertexSubset, f, next);
  }

  template <class F>
  inline void decodeOutNghSparse(long i, uintT o, F &f, uintE* outEdges) {
    decode_compressed::decodeOutNghSparse<compressedSymmetricVertex, F>(this, i, o, f, outEdges);
  }
};

struct compressedAsymmetricVertex {
  uchar* inNeighbors;
  uchar* outNeighbors;
  uintT outDegree;
  uintT inDegree;
  uchar* getInNeighbors() { return inNeighbors; }
  uchar* getOutNeighbors() { return outNeighbors; }
  intT getInNeighbor(intT j) { return -1; } //should not be called
  intT getOutNeighbor(intT j) { return -1; } //should not be called
  uintT getInDegree() { return inDegree; }
  uintT getOutDegree() { return outDegree; }
  void setInNeighbors(uchar* _i) { inNeighbors = _i; }
  void setOutNeighbors(uchar* _i) { outNeighbors = _i; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
  void flipEdges() { swap(inNeighbors,outNeighbors); 
    swap(inDegree,outDegree); }
  void del() {}

  template<class F>
  inline void decodeInNghBreakEarly(long i, bool* vertexSubset, F &f, bool* next, bool parallel = 0) {
    decode_compressed::decodeInNghBreakEarly<compressedAsymmetricVertex, F>(this, i, vertexSubset, f, next, parallel);
  }

  template<class F>
  inline void decodeOutNgh(long i, bool* vertexSubset, F &f, bool* next) {
    decode_compressed::decodeOutNgh<compressedAsymmetricVertex, F>(this, i, vertexSubset, f, next);
  }

  template <class F>
  inline void decodeOutNghSparse(long i, uintT o, F &f, uintE* outEdges) {
    decode_compressed::decodeOutNghSparse<compressedAsymmetricVertex, F>(this, i, o, f, outEdges);
  }
};

#endif
