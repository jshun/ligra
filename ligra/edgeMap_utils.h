// Contains helper functions and special cases of edgeMap. Most of these
// functions are specialized for the type of data written per vertex using tools
// from type_traits.

#pragma once

#include <type_traits>

#include "binary_search.h"

// For template meta-programming
enum class enabler_t {};

template<typename T>
using EnableIf = typename std::enable_if<T::value, enabler_t>::type;

template<typename T>
using DisableIf = typename std::enable_if<!T::value, enabler_t>::type;

template <typename data>
using EnableIfEmpty = EnableIf<std::is_same<data, pbbs::empty>>;

template <typename data>
using DisableIfEmpty = DisableIf<std::is_same<data, pbbs::empty>>;

// Standard version of edgeMapDense.
template <class data, EnableIfEmpty<data>...>
auto get_emdense_gen(tuple<bool, data>* next) {
  return [&] (uintE ngh, bool m=false) {
    if (m) next[ngh] = make_tuple(1, pbbs::empty()); };
}

template <class data, DisableIfEmpty<data>...>
auto get_emdense_gen(tuple<bool, data>* next) {
  return [&] (uintE ngh, Maybe<data> m=Maybe<data>()) {
    if (m.exists) next[ngh] = make_tuple(1, m.t); };
}

// Standard version of edgeMapDenseForward.
template <class data, EnableIfEmpty<data>...>
auto get_emdense_forward_gen(tuple<bool, data>* next) {
  return [&] (uintE ngh, bool m=false) {
    if (m) next[ngh] = make_tuple(1, pbbs::empty()); };
}

template <class data, DisableIfEmpty<data>...>
auto get_emdense_forward_gen(tuple<bool, data>* next) {
  return [&] (uintE ngh, Maybe<data> m=Maybe<data>()) {
    if (m.exists) next[ngh] = make_tuple(1, m.t); };
}

// Standard version of edgeMapSparse.
template <class data, EnableIfEmpty<data>...>
auto get_emsparse_gen(tuple<uintE, data>* outEdges) {
  return [&] (uintE ngh, uintE offset, bool m=false) {
    if (m) {
      outEdges[offset] = make_tuple(ngh, pbbs::empty());
    } else {
      outEdges[offset] = make_tuple(UINT_E_MAX, pbbs::empty());
    }
  };
}

template <class data, DisableIfEmpty<data>...>
auto get_emsparse_gen(tuple<uintE, data>* outEdges) {
  return [&] (uintE ngh, uintE offset, Maybe<data> m=Maybe<data>()) {
    if (m.exists) {
      outEdges[offset] = make_tuple(ngh, m.t);
    } else {
      std::get<0>(outEdges[offset]) = UINT_E_MAX;
    }
  };
}

// An edgeMapSparse that just maps over the out-edges.
template <class data, EnableIfEmpty<data>...>
auto get_emsparse_nooutput_gen() {
  return [&] (uintE ngh, uintE offset, bool m=false) { };
}

template <class data, DisableIfEmpty<data>...>
auto get_emsparse_nooutput_gen() {
  return [&] (uintE ngh, uintE offset, Maybe<data> m=Maybe<data>()) { };
}

// enabled only when <data != pbbs:empty>
template <class data, class vertex, class vs, class F>
void edgeMapSparseNoOutput(vertex* frontierVertices, vs &indices, uintT m, F &f) {
  auto g = get_emsparse_nooutput_gen<data>();
  parallel_for (size_t i = 0; i < m; i++) {
    uintT v = indices.vtx(i), o = 0;
    vertex vert = frontierVertices[i];
    vert.decodeOutNghSparse(v, o, f, g);
  }
}

// edgeMapSparse_no_filter
// Version of edgeMapSparse that binary-searches and packs out blocks of the
// next frontier.
template <class data, EnableIfEmpty<data>...>
auto get_emsparse_no_filter_gen(tuple<uintE, data>* outEdges) {
  return [&] (uintE ngh, uintE offset, bool m=false) {
    if (m) {
      outEdges[offset] = make_tuple(ngh, pbbs::empty());
      return true;
    }
    return false;
  };
}

template <class data, DisableIfEmpty<data>...>
auto get_emsparse_no_filter_gen(tuple<uintE, data>* outEdges) {
  return [&] (uintE ngh, uintE offset, Maybe<data> m=Maybe<data>()) {
    if (m.exists) {
      outEdges[offset] = make_tuple(ngh, m.t);
      return true;
    }
    return false;
  };
}

template <class data, class vertex, class vs, class F>
pair<size_t, tuple<uintE, data>*> edgeMapSparse_no_filter(vertex* frontierVertices, vs& indices,
      uintT* offsets, uintT m, F& f) {
  using S = tuple<uintE, data>;
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
  return make_pair(outSize, out);
}
