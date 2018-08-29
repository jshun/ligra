// Contains helper functions and special cases of edgeMap. Most of these
// functions are specialized for the type of data written per vertex using tools
// from type_traits.

#pragma once

#include <type_traits>

#include "binary_search.h"

// Standard version of edgeMapDense.
template <typename data, typename std::enable_if<
  std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emdense_gen(tuple<bool, data>* next) {
  return [next] (uintE ngh, bool m=false) {
    if (m) next[ngh] = make_tuple(1, pbbs::empty()); };
}

template <typename data, typename std::enable_if<
  !std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emdense_gen(tuple<bool, data>* next) {
  return [next] (uintE ngh, Maybe<data> m=Maybe<data>()) {
    if (m.exists) next[ngh] = make_tuple(1, m.t); };
}

// Standard version of edgeMapDenseForward.
template <typename data, typename std::enable_if<
  std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emdense_forward_gen(tuple<bool, data>* next) {
  return [next] (uintE ngh, bool m=false) {
    if (m) next[ngh] = make_tuple(1, pbbs::empty()); };
}

template <typename data, typename std::enable_if<
  !std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emdense_forward_gen(tuple<bool, data>* next) {
  return [next] (uintE ngh, Maybe<data> m=Maybe<data>()) {
    if (m.exists) next[ngh] = make_tuple(1, m.t); };
}

// Standard version of edgeMapSparse.
template <typename data, typename std::enable_if<
  std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emsparse_gen(tuple<uintE, data>* outEdges) {
  return [outEdges] (uintE ngh, uintT offset, bool m=false) {
    if (m) {
      outEdges[offset] = make_tuple(ngh, pbbs::empty());
    } else {
      outEdges[offset] = make_tuple(UINT_E_MAX, pbbs::empty());
    }
  };
}

template <typename data, typename std::enable_if<
  !std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emsparse_gen(tuple<uintE, data>* outEdges) {
  return [outEdges] (uintE ngh, uintT offset, Maybe<data> m=Maybe<data>()) {
    if (m.exists) {
      outEdges[offset] = make_tuple(ngh, m.t);
    } else {
      std::get<0>(outEdges[offset]) = UINT_E_MAX;
    }
  };
}

// edgeMapSparse_no_filter
// Version of edgeMapSparse that binary-searches and packs out blocks of the
// next frontier.
template <typename data, typename std::enable_if<
  std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emsparse_no_filter_gen(tuple<uintE, data>* outEdges) {
  return [outEdges] (uintE ngh, uintT offset, bool m=false) {
    if (m) {
      outEdges[offset] = make_tuple(ngh, pbbs::empty());
      return true;
    }
    return false;
  };
}

template <typename data, typename std::enable_if<
  !std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emsparse_no_filter_gen(tuple<uintE, data>* outEdges) {
  return [outEdges] (uintE ngh, uintT offset, Maybe<data> m=Maybe<data>()) {
    if (m.exists) {
      outEdges[offset] = make_tuple(ngh, m.t);
      return true;
    }
    return false;
  };
}




// Gen-functions that produce no output
template <typename data, typename std::enable_if<
  std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emsparse_nooutput_gen() {
  return [&] (uintE ngh, uintT offset, bool m=false) { };
}

template <typename data, typename std::enable_if<
  !std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emsparse_nooutput_gen() {
  return [&] (uintE ngh, uintT offset, Maybe<data> m=Maybe<data>()) { };
}

template <typename data, typename std::enable_if<
  std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emdense_nooutput_gen() {
  return [&] (uintE ngh, bool m=false) { };
}

template <typename data, typename std::enable_if<
  !std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emdense_nooutput_gen() {
  return [&] (uintE ngh, Maybe<data> m=Maybe<data>()) { };
}

template <typename data, typename std::enable_if<
  std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emdense_forward_nooutput_gen() {
  return [&] (uintE ngh, bool m=false) { };
}

template <typename data, typename std::enable_if<
  !std::is_same<data, pbbs::empty>::value, int>::type=0 >
auto get_emdense_forward_nooutput_gen() {
  return [&] (uintE ngh, Maybe<data> m=Maybe<data>()) { };
}
