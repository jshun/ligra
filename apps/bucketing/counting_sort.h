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
#include <stdio.h>
#include "utils.h"
#include "sequence.h"

namespace pbbso {

  // the following parameters can be tuned
  constexpr const size_t _cs_seq_threshold = 2048;
  constexpr const size_t _cs_max_blocks = 512;

  // Sequential internal version
  template <typename b_size_t, typename s_size_t, typename E, typename I, typename F>
  inline void _seq_count_sort(I& In, E* Out, F& get_key,
                       s_size_t start, s_size_t end,
            		       s_size_t* counts, s_size_t num_buckets) {

    s_size_t n = end-start;
    s_size_t* offsets = new_array_no_init<s_size_t>(num_buckets);
    b_size_t* tmp = new_array_no_init<b_size_t>(n);

    for (s_size_t i = 0; i < num_buckets; i++) {
      offsets[i] = 0;
    }
    for (s_size_t j = 0; j < n; j++) {
      s_size_t k = tmp[j] = get_key(j + start);
      offsets[k]++;
    }
    size_t s = 0;
    for (s_size_t i = 0; i < num_buckets; i++) {
      counts[i] = s;
      s += offsets[i];
      offsets[i] = s;
    }
    for (long j = ((long) n)-1; j >= 0; j--) {
      s_size_t k =  --offsets[tmp[j]];
      // needed for types with self defined assignment or initialization
      // otherwise equivalent to: Out[k+start] = In[j+start];
      move_uninitialized(Out[k+start], In[j+ start]);
    }
    free(offsets); free(tmp);
  }

  // Parallel internal version
  template <typename b_size_t, typename s_size_t, typename E, typename I, typename F>
  inline tuple<E*, s_size_t*, s_size_t> _count_sort(I& A, F& get_key, s_size_t n, s_size_t num_buckets) {
    // pad to 16 buckets to avoid false sharing (does not affect results)

    size_t sqrt = (size_t) ceil(pow(n,0.5));
    size_t num_blocks = (size_t) (n < 20000000) ? (sqrt/10) : sqrt;
    num_blocks = min(num_blocks, _cs_max_blocks);

    //if (num_blocks < num_threads)
    //  num_blocks = min((s_size_t) (n/(num_buckets*LOW_BUCKET_FACTOR*2) + 1),
    //num_threads);
    num_blocks = 1 << log2_up(num_blocks);

    // if insufficient parallelism, sort sequentially
    if (n < _cs_seq_threshold || num_blocks == 1) {
      s_size_t* counts = new_array_no_init<s_size_t>(num_buckets+1);
      E* B = new_array_no_init<E>(n);
      _seq_count_sort<b_size_t>(A, B, get_key, (s_size_t)0, n, counts, num_buckets);
      return make_tuple(B, counts, (s_size_t)1);
    }

    s_size_t block_size = ((n-1)/num_blocks) + 1;
    s_size_t m = num_blocks * num_buckets;

    E *B = new_array_no_init<E>(n); // warn: new_array will call cons on each item
    s_size_t *counts = new_array_no_init<s_size_t>(m, 1);

    // sort each block
    parallel_for_1 (s_size_t i = 0; i < num_blocks; ++i) {
      s_size_t start = std::min(i * block_size, n);
      s_size_t end =  std::min(start + block_size, n);
      _seq_count_sort<b_size_t>(A, B, get_key, start, end,
				counts + i*num_buckets, num_buckets);
    }

    return make_tuple(B, counts, num_blocks);
  }

  template <typename s_size_t, typename E, typename F>
    size_t* _count_sort_size(E* A, F& get_key, size_t n, size_t num_buckets) {
    if (num_buckets <= 256) {
      return _count_sort<uint8_t>(A, get_key, n, num_buckets);
    } else if  (num_buckets < (1 << 16)) {
      return _count_sort<uint16_t,s_size_t>(A, get_key, n, num_buckets);
    } else {
      return _count_sort<s_size_t,s_size_t>(A, get_key, n, num_buckets);
    }
  }

  // Parallel version
  template <typename E, typename F>
  size_t* count_sort(E* A, F& get_key, size_t n, size_t num_buckets) {
    size_t max32 = ((size_t) 1) << 32;
    if (n < max32 && num_buckets < max32)
      // use 4-byte counters when larger ones not needed
      return _count_sort_size<uint32_t>(A, get_key, n, num_buckets);
    return _count_sort_size<size_t>(A, get_key, n, num_buckets);
  }
}
