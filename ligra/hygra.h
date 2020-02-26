// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of 
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#pragma once
#include "hypergraphIO.h"
#include "ligra.h"
#include "histogram.h"
using namespace std;

//*****START FRAMEWORK*****

#define hyperedgeSubset vertexSubset
#define hyperedgeSubsetData vertexSubsetData
#define hyperedgeMap vertexMap
#define hyperedgeFilter vertexFilter
#define vertexProp(H,args...) edgeMap(H,FROM_V,args)
#define hyperedgeProp(H,args...) edgeMap(H,FROM_H,args)
#define hyperedgeFilterNgh packEdges
#define vertexFilterNgh packEdges

#define GRAIN_SIZE 1024

template <class data, class vertex, class VS, class F>
  vertexSubsetData<data> edgeMapDense(vertex* G, long nTo, VS& vertexSubset, F &f, const flags fl) {
  using D = tuple<bool, data>;
  //long n = GA.nv;
  //vertex *G = GA.V;
  if(fl & edge_parallel) {
    if (should_output(fl)) {
      D* next = newA(D, nTo);
      auto g = get_emdense_gen<data>(next);
      std::function<void(intT,intT)> recursive_lambda =
	[&]
	(intT start, intT end){
	if ((start == end-1) || (G[end].getInNeighbors()-G[start].getInNeighbors() < GRAIN_SIZE*sizeof(intE))){ 
	  for (intT v = start; v < end; v++){
	    std::get<0>(next[v]) = 0;
	    if (f.cond(v)) {
	      G[v].decodeInNghBreakEarly(v, vertexSubset, f, g, fl & dense_parallel);
	    }
	  }
	} else {
	  cilk_spawn recursive_lambda(start, start + ((end-start) >> 1));
	  recursive_lambda(start + ((end-start)>>1), end);
	}
      }; 
      recursive_lambda(0,nTo);
      return vertexSubsetData<data>(nTo, next);
    } else {
      auto g = get_emdense_nooutput_gen<data>();
      std::function<void(intT,intT)> recursive_lambda =
	[&]
	(intT start, intT end){
	if ((start == end-1) || (G[end].getInNeighbors()-G[start].getInNeighbors() < GRAIN_SIZE*sizeof(intE))){
	  for (intT v = start; v < end; v++){
	    if (f.cond(v)) {
	      G[v].decodeInNghBreakEarly(v, vertexSubset, f, g, fl & dense_parallel);
	    }
	  } 
	} else {
	  cilk_spawn recursive_lambda(start, start + ((end-start) >> 1));
	  recursive_lambda(start + ((end-start)>>1), end);
	}
      }; 
      recursive_lambda(0,nTo);
      return vertexSubsetData<data>(nTo);
    }
  }
  else {
    if (should_output(fl)) {
      D* next = newA(D, nTo);
      auto g = get_emdense_gen<data>(next);
      parallel_for (long v=0; v<nTo; v++) {
        std::get<0>(next[v]) = 0;
        if (f.cond(v)) {
          G[v].decodeInNghBreakEarly(v, vertexSubset, f, g, fl & dense_parallel);
        }
      }
      return vertexSubsetData<data>(nTo, next);
    } else {
      auto g = get_emdense_nooutput_gen<data>();
      parallel_for (long v=0; v<nTo; v++) {
        if (f.cond(v)) {
          G[v].decodeInNghBreakEarly(v, vertexSubset, f, g, fl & dense_parallel);
        }
      }
      return vertexSubsetData<data>(nTo);
    }
  }
}

template <class data, class vertex, class VS, class F>
  vertexSubsetData<data> edgeMapDenseForward(vertex *G, long nFrom, long nTo, VS& vertexSubset, F &f, const flags fl) {
  using D = tuple<bool, data>;
  //long n = GA.nv;
  //vertex *G = GA.V;
  if (should_output(fl)) {
    D* next = newA(D, nTo);
    auto g = get_emdense_forward_gen<data>(next);
    parallel_for(long i=0;i<nTo;i++) { std::get<0>(next[i]) = 0; }
    parallel_for (long i=0; i<nFrom; i++) {
      if (vertexSubset.isIn(i)) {
        G[i].decodeOutNgh(i, f, g);
      }
    }
    return vertexSubsetData<data>(nTo, next);
  } else {
    auto g = get_emdense_forward_nooutput_gen<data>();
    parallel_for (long i=0; i<nFrom; i++) {
      if (vertexSubset.isIn(i)) {
        G[i].decodeOutNgh(i, f, g);
      }
    }
    return vertexSubsetData<data>(nTo);
  }
}

template <class data, class vertex, class VS, class F>
  vertexSubsetData<data> edgeMapSparse(hypergraph<vertex>& GA, long nTo, vertex* frontierVertices, VS& indices, uintT* degrees, uintT m, F &f, const flags fl) {
  using S = tuple<uintE, data>;
  //long n = indices.n;
  S* outEdges;
  long outEdgeCount = 0;

  if (should_output(fl)) {
    uintT* offsets = degrees;
    outEdgeCount = sequence::plusScan(offsets, offsets, m);
    outEdges = newA(S, outEdgeCount);
    auto g = get_emsparse_gen<data>(outEdges);
    if(fl & edge_parallel) {
      std::function<void(intT,intT)> recursive_lambda =
	[&]
	(intT start, intT end){
	if ((start == end-1) || (offsets[end]-offsets[start] < GRAIN_SIZE*sizeof(intE))){ 
	  for (intT i = start; i < end; i++){
	    uintT v = indices.vtx(i), o = offsets[i];
	    vertex vert = frontierVertices[i];
	    vert.decodeOutNghSparse(v, o, f, g);
	  }
	}
	else {
	  cilk_spawn recursive_lambda(start, start + ((end-start) >> 1));
	  recursive_lambda(start + ((end-start)>>1), end);
	}
      }; 
      recursive_lambda(0,m);
    } else {
      parallel_for (size_t i = 0; i < m; i++) {
	uintT v = indices.vtx(i), o = offsets[i];
	vertex vert = frontierVertices[i];
	vert.decodeOutNghSparse(v, o, f, g);
      }
    }
  } else {
    auto g = get_emsparse_nooutput_gen<data>();
    parallel_for (size_t i = 0; i < m; i++) {
      uintT v = indices.vtx(i);
      vertex vert = frontierVertices[i];
      vert.decodeOutNghSparse(v, 0, f, g);
    }
  }

  if (should_output(fl)) {
    S* nextIndices = newA(S, outEdgeCount);
    if (fl & remove_duplicates) {
      if (GA.flags == NULL) {
	GA.initFlags();
      }
      auto get_key = [&] (size_t i) -> uintE& { return std::get<0>(outEdges[i]); };
      remDuplicates(get_key, GA.flags, outEdgeCount, nTo);
    }
    auto p = [] (tuple<uintE, data>& v) { return std::get<0>(v) != UINT_E_MAX; };
    size_t nextM = pbbs::filterf(outEdges, nextIndices, outEdgeCount, p);
    free(outEdges);
    return vertexSubsetData<data>(nTo, nextM, nextIndices);
  } else {
    return vertexSubsetData<data>(nTo);
  }
}

template <class data, class vertex, class VS, class F>
  vertexSubsetData<data> edgeMapSparse_no_filter(hypergraph<vertex>& GA, long nTo,
						 vertex* frontierVertices, VS& indices, uintT* offsets, uintT m, F& f,
						 const flags fl) {
  using S = tuple<uintE, data>;
  //long n = indices.n;
  long outEdgeCount = sequence::plusScan(offsets, offsets, m);
  S* outEdges = newA(S, outEdgeCount);

  auto g = get_emsparse_no_filter_gen<data>(outEdges);

  // binary-search into scan to map workers->chunks
  size_t b_size = 10000;
  size_t n_blocks = nblocks(outEdgeCount, b_size);

  uintE* cts = newA(uintE, n_blocks+1);
  size_t* block_offs = newA(size_t, n_blocks+1);

  auto offsets_m = make_in_imap<uintT>(m, [&] (size_t i) { return offsets[i]; });
  auto lt = [] (const uintT& l, const uintT& r) { return l < r; };
  parallel_for(size_t i=0; i<n_blocks; i++) {
    size_t s_val = i*b_size;
    block_offs[i] = pbbs::binary_search(offsets_m, s_val, lt);
  }
  block_offs[n_blocks] = m;
  parallel_for (size_t i=0; i<n_blocks; i++) {
    if ((i == n_blocks-1) || block_offs[i] != block_offs[i+1]) {
      // start and end are offsets in [m]
      size_t start = block_offs[i];
      size_t end = block_offs[i+1];
      uintT start_o = offsets[start];
      uintT k = start_o;
      for (size_t j=start; j<end; j++) {
        uintE v = indices.vtx(j);
        size_t num_in = frontierVertices[j].decodeOutNghSparseSeq(v, k, f, g);
        k += num_in;
      }
      cts[i] = (k - start_o);
    } else {
      cts[i] = 0;
    }
  }

  long outSize = sequence::plusScan(cts, cts, n_blocks);
  cts[n_blocks] = outSize;

  S* out = newA(S, outSize);

  parallel_for (size_t i=0; i<n_blocks; i++) {
    if ((i == n_blocks-1) || block_offs[i] != block_offs[i+1]) {
      size_t start = block_offs[i];
      size_t start_o = offsets[start];
      size_t out_off = cts[i];
      size_t block_size = cts[i+1] - out_off;
      for (size_t j=0; j<block_size; j++) {
        out[out_off + j] = outEdges[start_o + j];
      }
    }
  }
  free(outEdges); free(cts); free(block_offs);

  if (fl & remove_duplicates) {
    if (GA.flags == NULL) {
      GA.initFlags();
    }
    auto get_key = [&] (size_t i) -> uintE& { return std::get<0>(out[i]); };
    remDuplicates(get_key, GA.flags, outSize, nTo);
    S* nextIndices = newA(S, outSize);
    auto p = [] (tuple<uintE, data>& v) { return std::get<0>(v) != UINT_E_MAX; };
    size_t nextM = pbbs::filterf(out, nextIndices, outSize, p);
    free(out);
    return vertexSubsetData<data>(nTo, nextM, nextIndices);
  }
  return vertexSubsetData<data>(nTo, outSize, out);
}

// Packs out the adjacency lists of all vertex in vs. A neighbor, ngh, is kept
// in the new adjacency list if p(ngh) is true.
// Weighted graphs are not yet supported, but this should be easy to do.
template <class vertex, class P>
  vertexSubsetData<uintE> packEdges(vertex* G, vertexSubset& vs, P& p, const flags& fl=0) {
  using S = tuple<uintE, uintE>;
  vs.toSparse();
  long m = vs.numNonzeros(); long n = vs.numRows();
  if (vs.size() == 0) {
    return vertexSubsetData<uintE>(n);
  }
  auto degrees = array_imap<uintT>(m);
  granular_for(i, 0, m, (m > 2000), {
      uintE v = vs.vtx(i);
      degrees[i] = G[v].getOutDegree();
    });
  long outEdgeCount = pbbs::scan_add(degrees, degrees);
  S* outV;
  if (should_output(fl)) {
    outV = newA(S, vs.size());
  }

  bool* bits = newA(bool, outEdgeCount);
  uintE* tmp1 = newA(uintE, outEdgeCount);
  uintE* tmp2 = newA(uintE, outEdgeCount);
  if (should_output(fl)) {
    parallel_for (size_t i=0; i<m; i++) {
      uintE v = vs.vtx(i);
      size_t offset = degrees[i];
      auto bitsOff = &(bits[offset]); auto tmp1Off = &(tmp1[offset]);
      auto tmp2Off = &(tmp2[offset]);
      size_t ct = G[v].packOutNgh(v, p, bitsOff, tmp1Off, tmp2Off);
      outV[i] = make_tuple(v, ct);
    }
  } else {
    parallel_for (size_t i=0; i<m; i++) {
      uintE v = vs.vtx(i);
      size_t offset = degrees[i];
      auto bitsOff = &(bits[offset]); auto tmp1Off = &(tmp1[offset]);
      auto tmp2Off = &(tmp2[offset]);
      size_t ct = G[v].packOutNgh(v, p, bitsOff, tmp1Off, tmp2Off);
    }
  }
  free(bits); free(tmp1); free(tmp2);
  if (should_output(fl)) {
    return vertexSubsetData<uintE>(n, m, outV);
  } else {
    return vertexSubsetData<uintE>(n);
  }
}

#define FROM_V 1
#define FROM_H 0
// Decides on sparse or dense base on number of nonzeros in the active vertices.
// Takes as input a flag (fromV) to determine whether mapping from vertices (fromV=1) or hyperedges (fromV=0)
template <class data, class vertex, class VS, class F>
  vertexSubsetData<data> edgeMapData(hypergraph<vertex>& GA, bool fromV, VS &vs, F f,
				     intT threshold = -1, const flags& fl=0) {
  //nFrom is num vertices on incoming side and nTo is num vertices on outgoing side
  long nFrom = fromV ? GA.nv : GA.nh, nTo = fromV ? GA.nh : GA.nv;
  long numEdges = fromV ? GA.mv : GA.mh, m = vs.numNonzeros();
  if(threshold == -1) threshold = numEdges/20; //default threshold
  vertex *G = fromV ? GA.V : GA.H;
  if (nFrom != vs.numRows()) {
    cout << "edgeMap: Sizes Don't match" << endl;
    abort();
  }
  if (m == 0) return vertexSubsetData<data>(nTo);
  uintT* degrees;
  vertex* frontierVertices;
  uintT outDegrees = 0;
  if(threshold > 0) { //compute sum of out-degrees if threshold > 0 
    vs.toSparse();
    degrees = newA(uintT, m+1);
    frontierVertices = newA(vertex,m);
    {parallel_for (size_t i=0; i < m; i++) {
	uintE v_id = vs.vtx(i);
	vertex v = G[v_id];
	degrees[i] = v.getOutDegree();
	frontierVertices[i] = v;
      }}
    outDegrees = sequence::plusReduce(degrees, m);
    degrees[m] = outDegrees; //boundary case for sparse edge_parallel
    if (outDegrees == 0) return vertexSubsetData<data>(nTo);
  }
  if (m + outDegrees > threshold) {
    vs.toDense();
    return (fl & dense_forward) ?
      edgeMapDenseForward<data, vertex, VS, F>(G, nFrom, nTo, vs, f, fl) :
      edgeMapDense<data, vertex, VS, F>(fromV ? GA.H : GA.V, nTo, vs, f, fl);
  } else {
    auto vs_out =
      (should_output(fl) && fl & sparse_no_filter) ? // only call snof when we output
      edgeMapSparse_no_filter<data, vertex, VS, F>(GA, nTo, frontierVertices, vs, degrees, vs.numNonzeros(), f, fl) :
      edgeMapSparse<data, vertex, VS, F>(GA, nTo, frontierVertices, vs, degrees, vs.numNonzeros(), f, fl);
    free(degrees); free(frontierVertices);
    return vs_out;
  }
}

// Regular edgeMap, where no extra data is stored per vertex.
template <class vertex, class VS, class F>
  vertexSubset edgeMap(hypergraph<vertex> &GA, bool fromV, VS& vs, F f,
		       intT threshold = -1, const flags& fl=0) {
  return edgeMapData<pbbs::empty>(GA, fromV, vs, f, threshold, fl);
}

// edgeMapInduced
// Version of edgeMapSparse that maps over the one-hop frontier and returns it as
// a sparse array, without filtering.
template <class E, class vertex, class VS, class F>
  inline vertexSubsetData<E> edgeMapInduced(vertex* G, long nTo, VS& V, F& f) {
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
    return vertexSubsetData<E>(nTo);
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
  auto vs = vertexSubsetData<E>(nTo, outEdgeCount, outEdges);
  return vs;
}

template <class V, class vertex>
  struct HypergraphProp {
    using K = uintE; // keys are always uintE's (vertex-identifiers)
    using KV = tuple<K, V>;
    hypergraph<vertex>& GA;
    pbbs::hist_table<K, V> ht;

  HypergraphProp(hypergraph<vertex>& _GA, KV _empty, size_t ht_size=numeric_limits<size_t>::max()) : GA(_GA) {
    if (ht_size == numeric_limits<size_t>::max()) {
      ht_size = 1L << pbbs::log2_up(GA.mv/20);
    } else { ht_size = 1L << pbbs::log2_up(ht_size); }
    ht = pbbs::hist_table<K, V>(_empty, ht_size);
  }

    // map_f: (uintE v, uintE ngh) -> E
    // reduce_f: (E, tuple(uintE ngh, E ngh_val)) -> E
    // apply_f: (uintE ngh, E reduced_val) -> O
    template <class O, class M, class Map, class Reduce, class Apply, class VS>
      inline vertexSubsetData<O> edgeMapReduce(VS& vs, bool fromV, Map& map_f, Reduce& reduce_f, Apply& apply_f) {
      long nTo = fromV ? GA.nh : GA.nv;
      vertex* G = fromV ? GA.V : GA.H;
      size_t m = vs.size();
      if (m == 0) {
	return vertexSubsetData<O>(nTo);
      }

      auto oneHop = edgeMapInduced<M, vertex, VS, Map>(G, nTo, vs, map_f);
      oneHop.toSparse();

      auto get_elm = make_in_imap<tuple<K, M> >(oneHop.size(), [&] (size_t i) { return oneHop.vtxAndData(i); });
      auto get_key = make_in_imap<uintE>(oneHop.size(), [&] (size_t i) -> uintE { return oneHop.vtx(i); });

      auto q = [&] (sequentialHT<K, V>& S, tuple<K, M> v) -> void { S.template insertF<M>(v, reduce_f); };
      auto res = pbbs::histogram_reduce<tuple<K, M>, tuple<K, O> >(get_elm, get_key, oneHop.size(), q, apply_f, ht);
      oneHop.del();
      return vertexSubsetData<O>(nTo, res.first, res.second);
    }

    template <class O, class Apply, class VS>
      inline vertexSubsetData<O> hyperedgePropCount(VS& vs, Apply& apply_f) {
      auto map_f = [] (const uintE& i, const uintE& j) { return pbbs::empty(); };
      auto reduce_f = [&] (const uintE& cur, const tuple<uintE, pbbs::empty>& r) { return cur + 1; };
      return edgeMapReduce<O, pbbs::empty>(vs, FROM_H, map_f, reduce_f, apply_f);
    }

    template <class O, class Apply, class VS>
      inline vertexSubsetData<O> vertexPropCount(VS& vs, Apply& apply_f) {
      auto map_f = [] (const uintE& i, const uintE& j) { return pbbs::empty(); };
      auto reduce_f = [&] (const uintE& cur, const tuple<uintE, pbbs::empty>& r) { return cur + 1; };
      return edgeMapReduce<O, pbbs::empty>(vs, FROM_V, map_f, reduce_f, apply_f);
    }

    ~HypergraphProp() {
      ht.del();
    }
  };
