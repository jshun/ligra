#pragma once

#include "ligra.h"
#include "histogram.h"

// edgeMapInduced
// Version of edgeMapSparse that maps over the one-hop frontier and returns it as
// a sparse array, without filtering.
template <class E, class vertex, class VS, class F>
inline vertexSubsetData<E> edgeMapInduced(graph<vertex>& GA, VS& V, F& f) {
  vertex *G = GA.V;
  uintT m = V.size();
  V.toSparse();
  auto degrees = array_imap<uintT>(m);
  granular_for(i, 0, m, (m > 2000), {
    vertex v = G[V.vtx(i)];
    uintE degree = v.getOutDegree();
    degrees[i] = degree;
  });
  long outEdgeCount = pbbs::scan_add(degrees, degrees);
  if (outEdgeCount == 0) {
    return vertexSubsetData<E>(GA.n);
  }
  typedef tuple<uintE, E> VE;
  VE* outEdges = pbbs::new_array_no_init<VE>(outEdgeCount);

  auto gen = [&] (const uintE& ngh, const uintE& offset, const Maybe<E>& val = Maybe<E>()) {
    outEdges[offset] = make_tuple(ngh, val.t);
  };

  parallel_for (size_t i = 0; i < m; i++) {
    uintT o = degrees[i];
    auto v = V.vtx(i);
    G[v].template copyOutNgh<E>(v, o, f, gen);
  }
  auto vs = vertexSubsetData<E>(GA.n, outEdgeCount, outEdges);
  return vs;
}

template <class V, class vertex>
struct EdgeMap {
  using K = uintE; // keys are always uintE's (vertex-identifiers)
  using KV = tuple<K, V>;
  graph<vertex>& G;
  pbbs::hist_table<K, V> ht;

  EdgeMap(graph<vertex>& _G, KV _empty, size_t ht_size=numeric_limits<size_t>::max()) : G(_G) {
    if (ht_size == numeric_limits<size_t>::max()) {
      ht_size = 1L << pbbs::log2_up(G.m/20);
    } else { ht_size = 1L << pbbs::log2_up(ht_size); }
    ht = pbbs::hist_table<K, V>(_empty, ht_size);
  }

  // map_f: (uintE v, uintE ngh) -> E
  // reduce_f: (E, tuple(uintE ngh, E ngh_val)) -> E
  // apply_f: (uintE ngh, E reduced_val) -> O
  template <class O, class M, class Map, class Reduce, class Apply, class VS>
  inline vertexSubsetData<O> edgeMapReduce(VS& vs, Map& map_f, Reduce& reduce_f, Apply& apply_f) {
    size_t m = vs.size();
    if (m == 0) {
      return vertexSubsetData<O>(vs.numNonzeros());
    }

    auto oneHop = edgeMapInduced<M, vertex, VS, Map>(G, vs, map_f);
    oneHop.toSparse();

    auto get_elm = make_in_imap<tuple<K, M> >(oneHop.size(), [&] (size_t i) { return oneHop.vtxAndData(i); });
    auto get_key = make_in_imap<uintE>(oneHop.size(), [&] (size_t i) -> uintE { return oneHop.vtx(i); });

    auto q = [&] (sequentialHT<K, V>& S, tuple<K, M> v) -> void { S.template insertF<M>(v, reduce_f); };
    auto res = pbbs::histogram_reduce<tuple<K, M>, tuple<K, O> >(get_elm, get_key, oneHop.size(), q, apply_f, ht);
    oneHop.del();
    return vertexSubsetData<O>(vs.numNonzeros(), res.first, res.second);
  }

  template <class O, class Apply, class VS>
  inline vertexSubsetData<O> edgeMapCount(VS& vs, Apply& apply_f) {
    auto map_f = [] (const uintE& i, const uintE& j) { return pbbs::empty(); };
    auto reduce_f = [&] (const uintE& cur, const tuple<uintE, pbbs::empty>& r) { return cur + 1; };
    return edgeMapReduce<O, pbbs::empty>(vs, map_f, reduce_f, apply_f);
  }

  ~EdgeMap() {
    ht.del();
  }
};
