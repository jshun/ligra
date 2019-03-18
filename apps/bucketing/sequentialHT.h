// This code is based on the paper "Phase-Concurrent Hash Tables for
// Determinism" by Julian Shun and Guy Blelloch from SPAA 2014.
// Copyright (c) 2014 Julian Shun and Guy Blelloch
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights (to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANBILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
#pragma once

#include "parallel.h"
#include "utils.h"
#include "maybe.h"
using namespace std;

template <class K, class V>
class sequentialHT {
 typedef tuple<K,V> T;

 public:
  size_t m;
  intT mask;
  T empty;
  K max_key;
  T* table;

  inline size_t toRange(size_t h) {return h & mask;}
  inline size_t firstIndex(K v) {return toRange(pbbso::hash64(v));}
  inline size_t incrementIndex(size_t h) {return toRange(h+1);}

  sequentialHT(T* _table, size_t size, float loadFactor, tuple<K, V> _empty) :
    m((size_t) 1 << pbbso::log2_up((size_t)(loadFactor*size))),
    mask(m-1), table(_table), empty(_empty) { max_key = get<0>(empty); }

  // m must be a power of two
  sequentialHT(T* _table, size_t _m, tuple<K, V> _empty) :
    m((size_t)_m), mask(m-1), table(_table), empty(_empty) { max_key = get<0>(empty); }

  template <class M, class F>
  inline void insertF(tuple<K, M>& v, F& f) {
    K vKey = get<0>(v);
    size_t h = firstIndex(vKey);
    while (1) {
      auto k = get<0>(table[h]);
      if (k == max_key) {
        get<0>(table[h]) = vKey;
        V cur = get<1>(table[h]);
        get<1>(table[h]) = f(cur, v);
        return;
      } else if (k == vKey) {
        V cur = get<1>(table[h]);
        get<1>(table[h]) = f(cur, v);
        return;
      }
      h = incrementIndex(h);
    }
  }

  // V must support ++
  inline void insertAdd(K& vKey) {
    size_t h = firstIndex(vKey);
    while (1) {
      auto k = get<0>(table[h]);
      if (k == max_key) {
        table[h] = make_tuple(vKey, 1);
        return;
      } else if (k == vKey) {
        get<1>(table[h])++;
        return;
      }
      h = incrementIndex(h);
    }
  }

  // V must support ++, T<1> must be numeric
  inline void insertAdd(T& v) {
    const K& vKey = get<0>(v);
    size_t h = firstIndex(vKey);
    while (1) {
      auto k = get<0>(table[h]);
      if (k == max_key) {
        table[h] = make_tuple(vKey, 1);
        return;
      } else if (k == vKey) {
        get<1>(table[h]) += get<1>(v);
        return;
      }
      h = incrementIndex(h);
    }
  }

  inline T find(K& v) {
    size_t h = firstIndex(v);
    T c = table[h];
    while (1) {
      if (get<0>(c) == max_key) {
        return empty;
      } else if (get<0>(c) == v) {
      	return c;
      }
      h = incrementIndex(h);
      c = table[h];
    }
  }

  // F : KV -> E
  template <class E, class F>
  inline size_t compactInto(F& f, E* Out) {
    size_t k = 0;
    for (size_t i=0; i < m; i++) {
      auto kv = table[i]; auto key = get<0>(kv);
      if (key != max_key) {
        table[i] = empty;
        Maybe<E> value = f(kv);
        if (isSome(value)) {
          Out[k++] = getT(value);
        }
      }
    }
    return k;
  }

};

