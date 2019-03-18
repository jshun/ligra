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
#include "utilities.h"
#include "random.h"
#include "counting_sort.h"

namespace pbbs {

  // inplace sequential version
  template <typename Seq>
  void seq_random_shuffle_(Seq A, random r = random()) {
    size_t n = A.size();
    // the Knuth shuffle
    if (n < 2) return;
    for (size_t i=n-1; i > 0; i--)
      std::swap(A[i],A[r.ith_rand(i)%(i+1)]);
  }

  template <typename Seq>
  void random_shuffle_(Seq const &In,
		       range<typename Seq::value_type*> Out,
		       random r = random()) {
    size_t n = In.size();
    if (n < SEQ_THRESHOLD) {
      if (In.begin() != Out.begin()) 
	par_for(0,n,[&] (size_t i) {
	    assign_uninitialized(Out[i],In[i]);});
      seq_random_shuffle_(Out, r);
      return;
    }

    size_t bits = 10;
    if (n < (1 << 27))
      bits = (log2_up(n) - 7)/2;
    else bits = (log2_up(n) - 17);
    size_t num_buckets = (1<<bits);
    size_t mask = num_buckets - 1;
    auto rand_pos = [&] (size_t i) -> size_t {
      return r.ith_rand(i) & mask;};

    auto get_pos = delayed_seq<size_t>(n, rand_pos);

    // first randomly sorts based on random values [0,num_buckets)
    sequence<size_t> bucket_offsets = count_sort(In, Out, get_pos, num_buckets);
    
    // now sequentially randomly shuffle within each bucket
    auto bucket_f = [&] (size_t i) {
      size_t start = bucket_offsets[i];
      size_t end = bucket_offsets[i+1];
      seq_random_shuffle_(Out.slice(start,end), r.fork(i));
    };
    par_for(0, num_buckets, bucket_f, 1);
  }

  template <typename Seq>
  sequence<typename Seq::value_type>
  random_shuffle(Seq const &In, random r = random()) {
    using T = typename Seq::value_type;
    sequence<T> Out = sequence<T>::no_init(In.size());
    random_shuffle_(In, Out.slice(), r);
    return Out;
  }

  template <class intT>
  sequence<intT>
  random_permutation(size_t n, random r = random()) {
    sequence<intT> id(n, [&] (size_t i) { return i; });
    return pbbs::random_shuffle(id, r);
  }
}
