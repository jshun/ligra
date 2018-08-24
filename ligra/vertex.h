#ifndef VERTEX_H
#define VERTEX_H
#include "vertexSubset.h"
using namespace std;

namespace decode_uncompressed {

  // Used by edgeMapDense. Callers ensure cond(v_id). For each vertex, decode
  // its in-edges, and check to see whether this neighbor is in the current
  // frontier, calling update if it is. If processing the edges sequentially,
  // break once !cond(v_id).
  template <class vertex, class F, class G, class VS>
  inline void decodeInNghBreakEarly(vertex* v, long v_id, VS& vertexSubset, F &f, G &g, bool parallel = 0) {
    uintE d = v->getInDegree();
    if (!parallel || d < 1000) {
      for (size_t j=0; j<d; j++) {
        uintE ngh = v->getInNeighbor(j);
        if (vertexSubset.isIn(ngh)) {
#ifndef WEIGHTED
          auto m = f.update(ngh, v_id);
#else
          auto m = f.update(ngh, v_id, v->getInWeight(j));
#endif
          g(v_id, m);
        }
        if(!f.cond(v_id)) break;
      }
    } else {
      parallel_for(size_t j=0; j<d; j++) {
        uintE ngh = v->getInNeighbor(j);
        if (vertexSubset.isIn(ngh)) {
#ifndef WEIGHTED
          auto m = f.updateAtomic(ngh, v_id);
#else
          auto m = f.updateAtomic(ngh, v_id, v->getInWeight(j));
#endif
          g(v_id, m);
        }
      }
    }
  }

  // Used by edgeMapDenseForward. For each out-neighbor satisfying cond, call
  // updateAtomic.
  template <class V, class F, class G>
  inline void decodeOutNgh(V* v, long i, F &f, G &g) {
    uintE d = v->getOutDegree();
    granular_for(j, 0, d, (d > 1000), {
      uintE ngh = v->getOutNeighbor(j);
      if (f.cond(ngh)) {
#ifndef WEIGHTED
      auto m = f.updateAtomic(i,ngh);
#else
      auto m = f.updateAtomic(i,ngh,v->getOutWeight(j));
#endif
        g(ngh, m);
      }
    });
  }

  // Used by edgeMapSparse. For each out-neighbor satisfying cond, call
  // updateAtomic.
  template <class V, class F, class G>
  inline void decodeOutNghSparse(V* v, long i, uintT o, F &f, G &g) {
    uintE d = v->getOutDegree();
    granular_for(j, 0, d, (d > 1000), {
      uintE ngh = v->getOutNeighbor(j);
      if (f.cond(ngh)) {
#ifndef WEIGHTED
        auto m = f.updateAtomic(i, ngh);
#else
        auto m = f.updateAtomic(i, ngh, v->getOutWeight(j));
#endif
        g(ngh, o+j, m);
      } else {
        g(ngh, o+j);
      }
    });
  }

  // Used by edgeMapSparse_no_filter. Sequentially decode the out-neighbors,
  // and compactly write all neighbors satisfying g().
  template <class V, class F, class G>
  inline size_t decodeOutNghSparseSeq(V* v, long i, uintT o, F &f, G &g) {
    uintE d = v->getOutDegree();
    size_t k = 0;
    for (size_t j=0; j<d; j++) {
      uintE ngh = v->getOutNeighbor(j);
      if (f.cond(ngh)) {
#ifndef WEIGHTED
        auto m = f.updateAtomic(i, ngh);
#else
        auto m = f.updateAtomic(i, ngh, v->getOutWeight(j));
#endif
        bool wrote = g(ngh, o+k, m);
        if (wrote) { k++; }
      }
    }
    return k;
  }

  // Decode the out-neighbors of v, and return the number of neighbors
  // that satisfy f.
  template <class V, class F>
  inline size_t countOutNgh(V* v, long vtx_id, F& f) {
    uintE d = v->getOutDegree();
    if (d < 2000) {
      size_t ct = 0;
      for (size_t i=0; i<d; i++) {
        uintE ngh = v->getOutNeighbor(i);
#ifndef WEIGHTED
        if (f(vtx_id, ngh))
#else
        if (f(vtx_id, ngh, v->getOutWeight(i)))
#endif
          ct++;
      }
      return ct;
    } else {
      size_t b_size = 2000;
      size_t blocks = 1 + ((d-1)/b_size);
      auto cts = array_imap<uintE>(blocks, [&] (size_t i) { return 0; });
      parallel_for_1(size_t i=0; i<blocks; i++) {
        size_t s = b_size*i;
        size_t e = std::min(s + b_size, (size_t)d);
        uintE ct = 0;
        for (size_t j = s; j < e; j++) {
          uintE ngh = v->getOutNeighbor(j);
#ifndef WEIGHTED
          if (f(vtx_id, ngh))
#else
          if (f(vtx_id, ngh, v->getOutNeighbor(j)))
#endif
            ct++;
        }
        cts[i] = ct;
      }
      size_t count = 0;
      return pbbs::reduce_add(cts);
    }
  }

  // Decode the out-neighbors of v. Apply f(src, ngh) and store the result
  // using g.
  template <class V, class E, class F, class G>
  inline void copyOutNgh(V* v, long src, uintT o, F& f, G& g) {
    uintE d = v->getOutDegree();
    granular_for(j, 0, d, (d > 1000), {
      uintE ngh = v->getOutNeighbor(j);
#ifdef WEIGHTED
      E val = f(src, ngh, v->getOutWeight(j));
#else
      E val = f(src, ngh);
#endif
      g(ngh, o+j, val);
    });
  }

  // TODO(laxmand): Add support for weighted graphs.
  template <class V, class Pred>
  inline size_t packOutNgh(V* v, long vtx_id, Pred& p, bool* bits, uintE* tmp) {
    uintE d = v->getOutDegree();
    if (d < 5000) {
      size_t k = 0;
      for (size_t i=0; i<d; i++) {
        uintE ngh = v->getOutNeighbor(i);
        if (p(vtx_id, ngh)) {
          v->setOutNeighbor(k, ngh);
          k++;
        }
      }
      v->setOutDegree(k);
      return k;
    } else {
      parallel_for(size_t i=0; i<d; i++) {
        uintE ngh = v->getOutNeighbor(i);
        tmp[i] = ngh;
        bits[i] = p(vtx_id, ngh);
      }
      size_t k = sequence::pack(tmp, v->getOutNeighbors(), bits, d);
      v->setOutDegree(k);
      return k;
    }
  }

}

struct symmetricVertex {
#ifndef WEIGHTED
  uintE* neighbors;
#else
  intE* neighbors;
#endif
  uintT degree;
  void del() {free(neighbors); }
#ifndef WEIGHTED
symmetricVertex(uintE* n, uintT d)
#else
symmetricVertex(intE* n, uintT d)
#endif
: neighbors(n), degree(d) {}
#ifndef WEIGHTED
  uintE* getInNeighbors () { return neighbors; }
  const uintE* getInNeighbors () const { return neighbors; }
  uintE* getOutNeighbors () { return neighbors; }
  const uintE* getOutNeighbors () const { return neighbors; }
  uintE getInNeighbor(uintT j) const { return neighbors[j]; }
  uintE getOutNeighbor(uintT j) const { return neighbors[j]; }

  void setInNeighbor(uintT j, uintE ngh) { neighbors[j] = ngh; }
  void setOutNeighbor(uintT j, uintE ngh) { neighbors[j] = ngh; }
  void setInNeighbors(uintE* _i) { neighbors = _i; }
  void setOutNeighbors(uintE* _i) { neighbors = _i; }
#else
  //weights are stored in the entry after the neighbor ID
  //so size of neighbor list is twice the degree
  intE* getInNeighbors () { return neighbors; }
  const intE* getInNeighbors () const { return neighbors; }
  intE* getOutNeighbors () { return neighbors; }
  const intE* getOutNeighbors () const { return neighbors; }
  intE getInNeighbor(intT j) const { return neighbors[2*j]; }
  intE getOutNeighbor(intT j) const { return neighbors[2*j]; }
  intE getInWeight(intT j) const { return neighbors[2*j+1]; }
  intE getOutWeight(intT j) const { return neighbors[2*j+1]; }
  void setInNeighbor(uintT j, uintE ngh) { neighbors[2*j] = ngh; }
  void setOutNeighbor(uintT j, uintE ngh) { neighbors[2*j] = ngh; }
  void setInWeight(uintT j, intE wgh) { neighbors[2*j+1] = wgh; }
  void setOutWeight(uintT j, intE wgh) { neighbors[2*j+1] = wgh; }
  void setInNeighbors(intE* _i) { neighbors = _i; }
  void setOutNeighbors(intE* _i) { neighbors = _i; }
#endif

  uintT getInDegree() const { return degree; }
  uintT getOutDegree() const { return degree; }
  void setInDegree(uintT _d) { degree = _d; }
  void setOutDegree(uintT _d) { degree = _d; }
  void flipEdges() {}

  template <class VS, class F, class G>
  inline void decodeInNghBreakEarly(long v_id, VS& vertexSubset, F &f, G &g, bool parallel = 0) {
    decode_uncompressed::decodeInNghBreakEarly<symmetricVertex, F, G, VS>(this, v_id, vertexSubset, f, g, parallel);
  }

  template <class F, class G>
  inline void decodeOutNgh(long i, F &f, G& g) {
     decode_uncompressed::decodeOutNgh<symmetricVertex, F, G>(this, i, f, g);
  }

  template <class F, class G>
  inline void decodeOutNghSparse(long i, uintT o, F &f, G &g) {
    decode_uncompressed::decodeOutNghSparse<symmetricVertex, F>(this, i, o, f, g);
  }

  template <class F, class G>
  inline size_t decodeOutNghSparseSeq(long i, uintT o, F &f, G &g) {
    return decode_uncompressed::decodeOutNghSparseSeq<symmetricVertex, F>(this, i, o, f, g);
  }

  template <class E, class F, class G>
  inline void copyOutNgh(long i, uintT o, F& f, G& g) {
    decode_uncompressed::copyOutNgh<symmetricVertex, E>(this, i, o, f, g);
  }

  template <class F>
  inline size_t countOutNgh(long i, F &f) {
    return decode_uncompressed::countOutNgh<symmetricVertex, F>(this, i, f);
  }

  template <class F>
  inline size_t packOutNgh(long i, F &f, bool* bits, uintE* tmp1, uintE* tmp2) {
    return decode_uncompressed::packOutNgh<symmetricVertex, F>(this, i, f, bits, tmp1);
  }

};

struct asymmetricVertex {
#ifndef WEIGHTED
  uintE* inNeighbors, *outNeighbors;
#else
  intE* inNeighbors, *outNeighbors;
#endif
  uintT outDegree;
  uintT inDegree;
  void del() {free(inNeighbors); free(outNeighbors);}
#ifndef WEIGHTED
asymmetricVertex(uintE* iN, uintE* oN, uintT id, uintT od)
#else
asymmetricVertex(intE* iN, intE* oN, uintT id, uintT od)
#endif
: inNeighbors(iN), outNeighbors(oN), inDegree(id), outDegree(od) {}
#ifndef WEIGHTED
  uintE* getInNeighbors () { return inNeighbors; }
  const uintE* getInNeighbors () const { return inNeighbors; }
  uintE* getOutNeighbors () { return outNeighbors; }
  const uintE* getOutNeighbors () const { return outNeighbors; }
  uintE getInNeighbor(uintT j) const { return inNeighbors[j]; }
  uintE getOutNeighbor(uintT j) const { return outNeighbors[j]; }
  void setInNeighbor(uintT j, uintE ngh) { inNeighbors[j] = ngh; }
  void setOutNeighbor(uintT j, uintE ngh) { outNeighbors[j] = ngh; }
  void setInNeighbors(uintE* _i) { inNeighbors = _i; }
  void setOutNeighbors(uintE* _i) { outNeighbors = _i; }
#else
  intE* getInNeighbors () { return inNeighbors; }
  const intE* getInNeighbors () const { return inNeighbors; }
  intE* getOutNeighbors () { return outNeighbors; }
  const intE* getOutNeighbors () const { return outNeighbors; }
  intE getInNeighbor(uintT j) const { return inNeighbors[2*j]; }
  intE getOutNeighbor(uintT j) const { return outNeighbors[2*j]; }
  intE getInWeight(uintT j) const { return inNeighbors[2*j+1]; }
  intE getOutWeight(uintT j) const { return outNeighbors[2*j+1]; }
  void setInNeighbor(uintT j, uintE ngh) { inNeighbors[2*j] = ngh; }
  void setOutNeighbor(uintT j, uintE ngh) { outNeighbors[2*j] = ngh; }
  void setInWeight(uintT j, uintE wgh) { inNeighbors[2*j+1] = wgh; }
  void setOutWeight(uintT j, uintE wgh) { outNeighbors[2*j+1] = wgh; }
  void setInNeighbors(intE* _i) { inNeighbors = _i; }
  void setOutNeighbors(intE* _i) { outNeighbors = _i; }
#endif

  uintT getInDegree() const { return inDegree; }
  uintT getOutDegree() const { return outDegree; }
  void setInDegree(uintT _d) { inDegree = _d; }
  void setOutDegree(uintT _d) { outDegree = _d; }
  void flipEdges() { swap(inNeighbors,outNeighbors); swap(inDegree,outDegree); }

  template <class VS, class F, class G>
  inline void decodeInNghBreakEarly(long v_id, VS& vertexSubset, F &f, G &g, bool parallel = 0) {
    decode_uncompressed::decodeInNghBreakEarly<asymmetricVertex, F, G, VS>(this, v_id, vertexSubset, f, g, parallel);
  }

  template <class F, class G>
  inline void decodeOutNgh(long i, F &f, G &g) {
    decode_uncompressed::decodeOutNgh<asymmetricVertex, F, G>(this, i, f, g);
  }

  template <class F, class G>
  inline void decodeOutNghSparse(long i, uintT o, F &f, G &g) {
    decode_uncompressed::decodeOutNghSparse<asymmetricVertex, F>(this, i, o, f, g);
  }

  template <class F, class G>
  inline size_t decodeOutNghSparseSeq(long i, uintT o, F &f, G &g) {
    return decode_uncompressed::decodeOutNghSparseSeq<asymmetricVertex, F>(this, i, o, f, g);
  }

  template <class E, class F, class G>
  inline void copyOutNgh(long i, uintT o, F& f, G& g) {
    decode_uncompressed::copyOutNgh<asymmetricVertex, E>(this, i, o, f, g);
  }

  template <class F>
  inline size_t countOutNgh(long i, F &f) {
    return decode_uncompressed::countOutNgh<asymmetricVertex, F>(this, i, f);
  }

  template <class F>
  inline size_t packOutNgh(long i, F &f, bool* bits, uintE* tmp1, uintE* tmp2) {
    return decode_uncompressed::packOutNgh<asymmetricVertex, F>(this, i, f, bits, tmp1);
  }

};

#endif
