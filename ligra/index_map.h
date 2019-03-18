// An implementation of index_maps written by Guy Blelloch for pbbs.
//
// An index map is an alternative to iterators that can be better
// in certain applications, especially parallel code.
//
// Index maps have an associated length n (A.size()),
// and supply the functions:
//     A[i] that  "reads" the i'th index, and
//     A.update(i,v) that updates the i'th index with v.
// Both are only valid for i = [0,n).
// Input index maps only supply the first, output index maps only the
// second, and input/output index maps supply both.

// Index maps are useful when not just reading or writing contiguously in
// an arrary.    They can save time and memory by avoiding copying.
// Some applications include
//
// accessing every k'th element of an array A:
//     make_in_imap<T>(n, [&] (size_t i) {return A[i*k];})
// accessing fields of a structure:
//     make_in_imap<T>(n, [&] (size_t i) {return A[i].my_field;})
// applying a function to each element of an array:
//     make_in_imap<T>(n, [&] (size_t i) {return f(A[i]);})
// defining elements to be some function of i, e.g. its square:
//     make_in_imap<T>(n, [] (size_t i) {return i*i;})
// an arbitrary function can be applied on writing:
//     make_out_imap<T>(n, [&] (size_t i, T v) {A[i] = f(v);})
// or every other element can be written:
//     make_out_imap<T>(n, [&] (size_t i, T v) {A[i*n] = f(v);})
//
// Random access iterators can be converted into an index map:
//     make_iter_map(start, end)
// This creates an input/output map
//
// And the simplest usage is converting an array into an index map:
//     make_array_map(A, n)
// This also creates an input/output map

#pragma once

#include "utils.h"

template <typename E, typename F>
struct in_imap {
  using T = E;
  F f;
  size_t s, e;
  in_imap(size_t n, F f) : f(f), s(0), e(n) {};
  in_imap(size_t s, size_t e, F f) : f(f), s(s), e(e) {};
  inline T operator[] (const size_t i) {return f(i+s);}
  inline T operator() (const size_t i) {return f(i+s);}
  in_imap<T,F> cut(size_t ss, size_t ee) {return in_imap<T,F>(s+ss,s+ee,f); }
  in_imap<T,F> slice(size_t ss, size_t ee) {return in_imap<T,F>(s+ss,s+ee,f); }
  size_t size() { return e - s;}
};

// used so second template argument can be inferred
template <class E, class F>
in_imap<E,F> make_in_imap (size_t n, F f) {
  return in_imap<E,F>(n,f);
}

template <typename E, typename F>
struct out_imap {
  using T = E;
  F f;
  size_t s, e;
  out_imap(size_t n, F f) : f(f), s(0), e(n) {};
  out_imap(size_t s, size_t e, F f) : f(f), s(s), e(e) {};
  out_imap<T,F> cut(size_t ss, size_t ee) {return out_imap<T,F>(s+ss,s+ee,f); }
  void update(size_t i, const T& val) {f(i+s, val);}
  size_t size() { return e - s;}
};

// used so second template argument can be inferred
template <class E, class F>
out_imap<E,F> make_out_imap (size_t n, F f) {
  return out_imap<E,F>(n,f);
}

template <typename Iterator>
struct iter_imap {
  using T = typename std::iterator_traits<Iterator>::value_type;
  Iterator s;
  const Iterator e;
  iter_imap(const iter_imap& b) : s(b.s), e(b.e) {}
  iter_imap() {}
  iter_imap(Iterator s, Iterator e) : s(s), e(e) {};
  T& operator[] (const size_t i) const {return s[i];}
  iter_imap<Iterator> cut(size_t ss, size_t ee) {
    return iter_imap<Iterator>(s + ss, s + ee);}
  void update(size_t i, const T& val) {s[i] = val;}
  size_t size() { return e - s;}
};

// used so template argument can be inferred
template <class Iterator>
iter_imap<Iterator> make_iter_imap (Iterator s, Iterator e) {
  return iter_imap<Iterator>(s,e);
}

template <typename E>
struct array_imap {
  using T = E;
  E *s, *e;
  bool alloc;
  array_imap(const array_imap& b) : s(b.s), e(b.e), alloc(false) {}
  array_imap() : alloc(false) {}
  array_imap(E* s, size_t n, bool alloc=false)
    : s(s), e(s + n), alloc(alloc) {};
  array_imap(const size_t n) : s(pbbso::new_array_no_init<E>(n)), e(s + n), alloc(true) {};
  template <class F>
  array_imap(const size_t n, F f) : s(pbbso::new_array_no_init<E>(n)), e(s + n), alloc(true) {
    granular_for(i, 0, n, (n > 2000), { s[i] = f(i); });
  };
  ~array_imap() { if (alloc) { free(s);}}
  inline E& operator[] (const size_t i) const {return s[i];}
  inline E& operator() (const size_t i) const {return s[i];}
  inline array_imap<T> cut(size_t ss, size_t ee) {
    return array_imap<T>(s + ss, ee - ss);}
  inline void update(size_t i, const E& val) {s[i] = val;}
//  inline void del() { if (alloc) { alloc = false; free(s); }}
  inline size_t size() { return e - s;}
};

// used so template argument can be inferred
template <class E>
array_imap<E> make_array_imap (E* A, size_t n) {
  return array_imap<E>(A,n);
}
