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

  template <class F, class G, class VS>
  struct denseT {
    VS vs;
    F f;
    G g;
  denseT(F &_f, G &_g, VS& _vs) : f(_f), g(_g), vs(_vs) {}
#ifndef WEIGHTED
    inline bool srcTarg(const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (vs.isIn(target)) {
        auto m = f.update(target, src);
        g(src, m);
      }
      return f.cond(src);
    }
#else
    inline bool srcTarg(const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (vs.isIn(target)) {
        auto m = f.update(target, src, weight);
        g(src, m);
      }
      return f.cond(src);
    }
#endif
  };

  template <class F, class G>
  struct denseForwardT {
    F f;
    G g;
  denseForwardT(F &_f, G &_g) : f(_f), g(_g) {}
#ifndef WEIGHTED
    inline bool srcTarg(const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (f.cond(target)) {
        auto m = f.updateAtomic(src, target);
        g(target, m);
      }
      return true;
    }
#else
    inline bool srcTarg(const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (f.cond(target)) {
        auto m = f.updateAtomic(src, target, weight);
        g(target, m);
      }
      return true;
    }
#endif
  };

  template <class F, class G>
  struct sparseT {
    uintT v, o;
    F f;
    G g;
  sparseT(F &_f, G &_g, uintT vP, uintT oP) : f(_f), g(_g), v(vP), o(oP) { }
#ifndef WEIGHTED
    inline bool srcTarg(const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (f.cond(target)) {
        auto m = f.updateAtomic(v, target);
        g(target, o + edgeNumber, m);
      } else {
        g(target, o + edgeNumber);
      }
      return true; }
#else
    inline bool srcTarg(const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (f.cond(target)) {
        auto m = f.updateAtomic(v, target, weight);
        g(target, o + edgeNumber, m);
      } else {
        g(target, o + edgeNumber);
      }
      return true; }
#endif
  };

  template <class F, class G>
  struct sparseTSeq {
    uintT v, o;
    F f;
    G g;
    size_t& k;
  sparseTSeq(F &_f, G &_g, uintT vP, uintT oP, size_t& _k) : f(_f), g(_g), v(vP), o(oP), k(_k) { }
#ifndef WEIGHTED
    inline bool srcTarg(const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (f.cond(target)) {
        auto m = f.updateAtomic(v, target);
        if (g(target, o + k, m)) {
          k++;
        }
      }
      return true; }
#else
    inline bool srcTarg(const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (f.cond(target)) {
        auto m = f.updateAtomic(v, target, weight);
        if (g(target, o + k, m)) {
          k++;
        }
      }
      return true; }
#endif
  };

  template <class F>
  struct sparseTCount {
    uintT v;
    size_t& ct;
    F f;
  sparseTCount(F &_f, uintT vP, size_t& _ct) : f(_f), v(vP), ct(_ct) {}
#ifndef WEIGHTED
    inline bool srcTarg(const uintE &src, const uintE &target, const uintT &edgeNumber) {
      if (f(v, target)) { ct++; }
      return true; }
#else
    inline bool srcTarg(const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      if (f(v, target)) { ct++; }
      return true; }
#endif
  };

  template <class E, class F, class G>
  struct sparseTE {
    uintT v, o;
    F f;
    G g;
  sparseTE(F &_f, G &_g, uintT vP, uintT oP) : f(_f), g(_g), v(vP), o(oP){}
#ifndef WEIGHTED
    inline bool srcTarg(const uintE &src, const uintE &target, const uintT &edgeNumber) {
      auto val = f(src, target);
      g(target, o + edgeNumber, val);
      return true; }
#else
    inline bool srcTarg(const uintE &src, const uintE &target, const intE &weight, const uintT &edgeNumber) {
      auto val = f(src, target);
      g(target, o + edgeNumber, val);
      return true; }
#endif
  };

  template<class V, class F, class G, class VS>
  inline void decodeInNghBreakEarly(V* v, long i, VS& vertexSubset, F &f, G &g, bool parallel = 0) {
    uintE d = v->getInDegree();
    uchar *nghArr = v->getInNeighbors();
#ifdef WEIGHTED
        decodeWgh(denseT<F, G, VS>(f, g, vertexSubset), nghArr, i, v->getInDegree());
#else
        decode(denseT<F, G, VS>(f, g, vertexSubset), nghArr, i, v->getInDegree());
#endif
  }

  template<class V, class F, class G>
  inline void decodeOutNgh(V* v, long i, F &f, G &g) {
    uintE d = v->getInDegree();
    uchar *nghArr = v->getOutNeighbors();
#ifdef WEIGHTED
        decodeWgh(denseForwardT<F, G>(f, g), nghArr, i, v->getOutDegree());
#else
        decode(denseForwardT<F, G>(f, g), nghArr, i, v->getOutDegree());
#endif
  }

  template <class V, class F, class G>
  inline void decodeOutNghSparse(V* v, long i, uintT o, F &f, G &g) {
    uchar *nghArr = v->getOutNeighbors();
#ifdef WEIGHTED
    decodeWgh(sparseT<F, G>(f, g, i, o), nghArr, i, v->getOutDegree());
#else
    decode(sparseT<F, G>(f, g, i, o), nghArr, i, v->getOutDegree());
#endif
  }

  template <class V, class F, class G>
  inline size_t decodeOutNghSparseSeq(V* v, long i, uintT o, F &f, G &g) {
    uchar *nghArr = v->getOutNeighbors();
    size_t k = 0;
#ifdef WEIGHTED
    decodeWgh(sparseTSeq<F, G>(f, g, i, o, k), nghArr, i, v->getOutDegree(), false);
#else
    decode(sparseTSeq<F, G>(f, g, i, o, k), nghArr, i, v->getOutDegree(), false);
#endif
    return k;
  }

  // TODO(laxmand): This looks very similar to decodeOutNghSparseSeq. Can we
  // merge?
  template <class V, class F>
  inline size_t countOutNgh(V* v, long i, F &f) {
    uchar *nghArr = v->getOutNeighbors();
    size_t ct = 0;
#ifdef WEIGHTED
    decodeWgh(sparseTCount<F>(f, i, ct), nghArr, i, v->getOutDegree(), false);
#else
    decode(sparseTCount<F>(f, i, ct), nghArr, i, v->getOutDegree(), false);
#endif
    return ct;
  }

  template <class V, class E, class F, class G>
  inline void copyOutNgh(V* v, long i, uintT o, F& f, G& g) {
    uchar *nghArr = v->getOutNeighbors();
#ifdef WEIGHTED
    decodeWgh(sparseTE<E, F, G>(f, g, i, o), nghArr, i, v->getOutDegree());
#else
    decode(sparseTE<E, F, G>(f, g, i, o), nghArr, i, v->getOutDegree());
#endif
  }

  // TODO(laxmand): Add support for weighted graphs. Measure how much slower
  // this version, which decodes, filters, and then recompresses is compared to
  // the version in byte.h which decodes and recompresses in one pass.
  template <class V, class P>
  inline size_t packOutNgh(V* v, long i, P &pred, bool* bits, uintE* tmp1, uintE* tmp2) {
    uchar *nghArr = v->getOutNeighbors();
    size_t original_deg = v->getOutDegree();
    // 1. Decode into tmp.
    auto f = [&] (const uintE& src, const uintE& ngh) {
      if (pred(src, ngh)) {
        return ngh;
      } else {
        return UINT_E_MAX;
      }
    };
    auto gen = [&] (const uintE& ngh, const uintE& offset, const Maybe<uintE>& val = Maybe<uintE>()) {
      tmp1[offset] = val.t;
    };
    copyOutNgh<V, uintE>(v, i, (uintT)0, f, gen);

    size_t new_deg = pbbs::filterf(tmp1, tmp2, original_deg, [] (uintE v) { return v != UINT_E_MAX; });
    if (new_deg < original_deg) {
      sequentialCompressEdgeSet(nghArr, 0, new_deg, i, tmp2);
      v->setOutDegree(new_deg);
    }
    return new_deg;
  }
}

struct compressedSymmetricVertex {
  uchar* neighbors;
  uintT degree;
  uchar* getInNeighbors() { return neighbors; }
  const uchar* getInNeighbors() const { return neighbors; }
  uchar* getOutNeighbors() { return neighbors; }
  const uchar* getOutNeighbors() const { return neighbors; }
  intT getInNeighbor(intT j) const { return -1; } //should not be called
  intT getOutNeighbor(intT j) const { return -1; } //should not be called
  uintT getInDegree() const { return degree; }
  uintT getOutDegree() const { return degree; }
  void setInNeighbors(uchar* _i) { neighbors = _i; }
  void setOutNeighbors(uchar* _i) { neighbors = _i; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
  void flipEdges() {}
  void del() {}


  template<class VS, class F, class G>
  inline void decodeInNghBreakEarly(long i, VS& vertexSubset, F &f, G &g, bool parallel = 0) {
    decode_compressed::decodeInNghBreakEarly<compressedSymmetricVertex, F, G, VS>(this, i, vertexSubset, f, g, parallel);
  }

  template<class F, class G>
  inline void decodeOutNgh(long i, F &f, G &g) {
    decode_compressed::decodeOutNgh<compressedSymmetricVertex, F, G>(this, i, f, g);
  }

  template <class F, class G>
  inline void decodeOutNghSparse(long i, uintT o, F &f, G &g) {
    decode_compressed::decodeOutNghSparse<compressedSymmetricVertex, F, G>(this, i, o, f, g);
  }

  template <class F, class G>
  inline size_t decodeOutNghSparseSeq(long i, uintT o, F &f, G &g) {
    return decode_compressed::decodeOutNghSparseSeq<compressedSymmetricVertex, F, G>(this, i, o, f, g);
  }

  template <class E, class F, class G>
  inline void copyOutNgh(long i, uintT o, F& f, G& g) {
    decode_compressed::copyOutNgh<compressedSymmetricVertex, E, F, G>(this, i, o, f, g);
  }

  template <class F>
  inline size_t countOutNgh(long i, F &f) {
    return decode_compressed::countOutNgh<compressedSymmetricVertex, F>(this, i, f);
  }

  template <class P>
  inline size_t packOutNgh(long i, P &pred, bool* bits, uintE* tmp1, uintE* tmp2) {
    return decode_compressed::packOutNgh<compressedSymmetricVertex, P>(this, i, pred, bits, tmp1, tmp2);
  }

};

struct compressedAsymmetricVertex {
  uchar* inNeighbors;
  uchar* outNeighbors;
  uintT outDegree;
  uintT inDegree;
  uchar* getInNeighbors() { return inNeighbors; }
  const uchar* getInNeighbors() const { return inNeighbors; }
  uchar* getOutNeighbors() { return outNeighbors; }
  const uchar* getOutNeighbors() const { return outNeighbors; }
  intT getInNeighbor(intT j) const { return -1; } //should not be called
  intT getOutNeighbor(intT j) const { return -1; } //should not be called
  uintT getInDegree() const { return inDegree; }
  uintT getOutDegree() const { return outDegree; }
  void setInNeighbors(uchar* _i) { inNeighbors = _i; }
  void setOutNeighbors(uchar* _i) { outNeighbors = _i; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
  void flipEdges() { swap(inNeighbors,outNeighbors);
    swap(inDegree,outDegree); }
  void del() {}

  template<class VS, class F, class G>
  inline void decodeInNghBreakEarly(long i, VS& vertexSubset, F &f, G &g, bool parallel = 0) {
    decode_compressed::decodeInNghBreakEarly<compressedAsymmetricVertex, F, G, VS>(this, i, vertexSubset, f, g, parallel);
  }

  template<class F, class G>
  inline void decodeOutNgh(long i, F &f, G &g) {
    decode_compressed::decodeOutNgh<compressedAsymmetricVertex, F, G>(this, i, f, g);
  }

  template <class F, class G>
  inline void decodeOutNghSparse(long i, uintT o, F &f, G &g) {
    decode_compressed::decodeOutNghSparse<compressedAsymmetricVertex, F, G>(this, i, o, f, g);
  }

  template <class F, class G>
  inline size_t decodeOutNghSparseSeq(long i, uintT o, F &f, G &g) {
    return decode_compressed::decodeOutNghSparseSeq<compressedAsymmetricVertex, F, G>(this, i, o, f, g);
  }

  template <class E, class F, class G>
  inline void copyOutNgh(long i, uintT o, F& f, G& g) {
    decode_compressed::copyOutNgh<compressedAsymmetricVertex, E, F, G>(this, i, o, f, g);
  }

  template <class F>
  inline size_t countOutNgh(long i, F &f) {
    return decode_compressed::countOutNgh<compressedAsymmetricVertex, F>(this, i, f);
  }

  template <class P>
  inline size_t packOutNgh(long i, P &pred, bool* bits, uintE* tmp1, uintE* tmp2) {
    return decode_compressed::packOutNgh<compressedAsymmetricVertex, P>(this, i, pred, bits, tmp1, tmp2);
  }

};

#endif
