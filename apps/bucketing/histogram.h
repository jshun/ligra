// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2010-2016 Guy Blelloch and the PBBS team
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
#include <math.h>
#include <stdio.h>
#include <cstdint>

#include "counting_sort.h"
#include "sequentialHT.h"
#include "sequence.h"

namespace pbbso {

  // Tunable parameters
  constexpr const size_t _hist_max_buckets = 2048;
  constexpr const size_t _hist_seq_threshold = 3500;

  template <class K, class V>
  struct hist_table {
    using KV = tuple<K, V>;
    KV empty;
    KV* table;
    size_t size;
    hist_table(KV _empty, size_t _size) : empty(_empty), size(_size) {
      table = newA(KV, size);
      parallel_for (size_t i=0; i<size; i++) { table[i] = empty; }
    }
    hist_table() {}

    //size needs to be a power of 2
    void resize(size_t req_size) {
      if (req_size > size) {
        free(table);
        table = newA(KV, req_size);
        size = req_size;
        parallel_for (size_t i=0; i<size; i++) { table[i] = empty; }
        cout << "resized to: " << size << endl;
      }
    }

    void del() {
      if (table) {
        free(table);
      }
    }
  };

  template <class E, class O, class K, class V, class A, class Reduce, class Apply>
  inline pair<size_t, O*> seq_histogram_reduce(A& get_elm, size_t n, Reduce& reduce_f, Apply& apply_f, hist_table<K, V>& ht) {
    typedef tuple<K, V> KV;
    long size = 1 << pbbso::log2_up(n+1);
    ht.resize(size);
    sequentialHT<K, V> S(ht.table, size, 1.0f, ht.empty);
    for (size_t i=0; i<n; i++) {
      E a = get_elm(i);
      reduce_f(S, a);
    }
    O* out = newA(O, n);
    size_t k = S.compactInto(apply_f, out);
    return make_pair(k, out);
  }

  // Issue: want to make the type that's count-sort'd independent of K,V
  // A : <E> elms
  // B : inmap<keys> (numeric so that we can hash)
  // E : type of intermediate elements (what we count sort)
  // Q : reduction (seqHT<uintE, E>&, E&) -> void
  // F : tuple<K, Elm> -> Maybe<uintE>
  template <class E, class O, class K, class V, class A, class B, class Reduce, class Apply>
  inline pair<size_t, O*> histogram_reduce(A& get_elm, B& get_key, size_t n, Reduce& reduce_f, Apply& apply_f, hist_table<K, V>& ht) {
    typedef tuple<K, V> KV;

    int nworkers = getWorkers();

    if (n < _hist_seq_threshold || nworkers == 1) {
      auto r = seq_histogram_reduce<E, O>(get_elm, n, reduce_f, apply_f, ht);
      return r;
    }

    size_t sqrt = (size_t) ceil(pow(n,0.5));
    size_t num_buckets = (size_t) (n < 20000000) ? (sqrt/10) : sqrt;
    num_buckets = std::max(1 << log2_up(num_buckets), 1);
    num_buckets = min(num_buckets, _hist_max_buckets);
    size_t bits = log2_up(num_buckets);

    // (1) count-sort based on bucket
    // TODO: add back contention detection
    size_t low_mask = ~((size_t)15);
    size_t bucket_mask = num_buckets - 1;
    auto get_bucket = [&] (uintE i) {
      return pbbso::hash64(get_key[i] & low_mask) & bucket_mask;
    };

    auto p = _count_sort<int16_t, size_t, E>(get_elm, get_bucket, n, (uintE)num_buckets);

    auto elms = std::get<0>(p); // count-sort'd
    // laid out as num_buckets (row), blocks (col)
    size_t* counts = std::get<1>(p);
    size_t num_blocks = std::get<2>(p);
    size_t block_size = ((n-1)/num_blocks) + 1;

#define S_STRIDE 1
    size_t* bkt_counts = new_array_no_init<size_t>(num_buckets*S_STRIDE);
    parallel_for (size_t i=0; i<num_buckets; i++) {
      bkt_counts[i*S_STRIDE] = 0;
      if (i == (num_buckets-1)) {
        for (size_t j=0; j<num_blocks; j++) {
          size_t start = std::min(j * block_size, n);
          size_t end =  std::min(start + block_size, n);
          bkt_counts[i*S_STRIDE] += (end - start) - counts[j*num_buckets+i];
        }
      } else {
        for (size_t j=0; j<num_blocks; j++) {
          bkt_counts[i*S_STRIDE] += counts[j*num_buckets+i+1] - counts[j*num_buckets+i];
        }
      }
    }

    array_imap<size_t> out_offs = array_imap<size_t>(num_buckets+1);
    array_imap<size_t> ht_offs = array_imap<size_t>(num_buckets+1);

    // (2) process each bucket, compute the size of each HT and scan (seq)
    ht_offs[0] = 0;
    for (size_t i=0; i<num_buckets; i++) {
      size_t size = bkt_counts[i*S_STRIDE];
      size_t ht_size = 0;
      if (size > 0) {
        ht_size = 1 << pbbso::log2_up((intT)(size + 1));
      }
      ht_offs[i+1] = ht_offs[i] + ht_size;
    }

    ht.resize(ht_offs[num_buckets]);
    O* out = newA(O, ht_offs[num_buckets]);

    // (3) insert elms into per-bucket hash table (par)
    parallel_for_1 (size_t i = 0; i < num_buckets; i++) {
      size_t ht_start = ht_offs[i];
      size_t ht_size = ht_offs[i+1] - ht_start;

      size_t k = 0;
      KV* table = ht.table;
      if (ht_size > 0) {
        KV* my_ht = &(table[ht_start]);
        sequentialHT<K, V> S(my_ht, ht_size, ht.empty);

        O* my_out = &(out[ht_start]);

        for (size_t j=0; j<num_blocks; j++) {
          size_t start = std::min(j * block_size, n);
          size_t end =  std::min(start + block_size, n);
          size_t ct = 0;
          size_t off = 0;
          if (i == (num_buckets-1)) {
            off = counts[j*num_buckets+i];
            ct = (end - start) - off;
          } else {
            off = counts[j*num_buckets+i];
            ct = counts[j*num_buckets+i+1] - off;
          }
          off += start;
          for (size_t k=0; k<ct; k++) {
            E a = elms[off + k];
            reduce_f(S, a);
          }
        }

        k = S.compactInto(apply_f, my_out);
      }
      out_offs[i] = k;
    }

    // (4) scan
    size_t ct = 0;
    for (size_t i=0; i<num_buckets; i++) {
      size_t s = ct;
      ct += out_offs[i];
      out_offs[i] = s;
    }
    out_offs[num_buckets] = ct;
    uintT num_distinct = ct;

    O* res = newA(O, ct);

    // (5) map compacted hts to output, clear hts
    parallel_for_1 (size_t i=0; i<num_buckets; i++) {
      size_t o = out_offs[i];
      size_t k = out_offs[(i+1)] - o;

      size_t ht_start = ht_offs[i];
      size_t ht_size = ht_offs[i+1] - ht_start;

      if (ht_size > 0) {
        O* my_out = &(out[ht_start]);

        for (size_t j=0; j<k; j++) {
          res[o+j] = my_out[j];
        }
      }
    }

    free(elms); free(counts); free(bkt_counts); free(out);
    return make_pair(num_distinct, res);
  }

  template <class E, class A, class F, class Timer>
  inline pair<size_t, tuple<E, E>*> histogram(A& get_elm, size_t n, F& f, hist_table<E, E>& ht, Timer& t) {
    auto q = [] (sequentialHT<E, E>& S, E v) -> void { S.insertAdd(v); };
    return histogram_reduce<E, tuple<E, E>>(get_elm, get_elm, n, q, f, ht, t);
  }

}
