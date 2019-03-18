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
#include <math.h>
#include "utilities.h"
#include "sequence_ops.h"
#include "transpose.h"
#include <chrono>
#include <thread>

// TODO
// Make sure works for inplace or not with regards to move_uninitialized

namespace pbbs {

  // the following parameters can be tuned
  constexpr const size_t SEQ_THRESHOLD = 8192;
  constexpr const size_t BUCKET_FACTOR = 32;
  constexpr const size_t LOW_BUCKET_FACTOR = 16;

  // count number in each bucket
  template <typename s_size_t, typename InSeq, typename KeySeq>
  void seq_count_(InSeq In, KeySeq Keys,
		  s_size_t* counts, size_t num_buckets) {
    size_t n = In.size();
    // use local counts to avoid false sharing
    size_t local_counts[num_buckets];
    for (size_t i = 0; i < num_buckets; i++)
      local_counts[i] = 0;
    for (size_t j = 0; j < n; j++) {
      size_t k = Keys[j];
      if (k >= num_buckets) abort();
      local_counts[k]++;
    }
    for (size_t i = 0; i < num_buckets; i++)
      counts[i] = local_counts[i];
  }

  // write to destination, where offsets give start of each bucket
  template <typename s_size_t, typename InSeq, typename KeySeq>
  void seq_write_(InSeq In, typename InSeq::value_type* Out, KeySeq Keys,
		  s_size_t* offsets, size_t num_buckets) {
    // copy to local offsets to avoid false sharing
    size_t local_offsets[num_buckets];
    for (size_t i = 0; i < num_buckets; i++)
      local_offsets[i] = offsets[i];
    for (size_t j = 0; j < In.size(); j++) {
      size_t k = local_offsets[Keys[j]]++;
      move_uninitialized(Out[k], In[j]);
    }
  }

  // write to destination, where offsets give end of each bucket
  template <typename s_size_t, typename InSeq, typename KeySeq>
  void seq_write_down_(InSeq In, typename InSeq::value_type* Out, KeySeq Keys,
		       s_size_t* offsets, size_t num_buckets) {
    for (long j = In.size()-1; j >= 0; j--) {
      long k = --offsets[Keys[j]];
      move_uninitialized(Out[k], In[j]);
    }
  }

  // Sequential counting sort internal
  template <typename s_size_t, typename InS, typename OutS, typename KeyS>
  void seq_count_sort_(InS In, OutS Out, KeyS Keys,
		       s_size_t* counts, size_t num_buckets) {

    // count size of each bucket
    seq_count_(In, Keys, counts, num_buckets);

    // generate offsets for buckets
    size_t s = 0;
    for (size_t i = 0; i < num_buckets; i++) {
      s += counts[i];
      counts[i] = s;
    }

    // send to destination
    seq_write_down_(In, Out.begin(), Keys, counts, num_buckets);
  }

  // Sequential counting sort
  template <typename InS, typename OutS, typename KeyS>
  sequence<size_t> seq_count_sort(InS& In, OutS& Out, KeyS& Keys,
				  size_t num_buckets) {
    sequence<size_t> counts(num_buckets+1);
    seq_count_sort_(In.slice(), Out.slice(), Keys.slice(),
		    counts.begin(), num_buckets);
    counts[num_buckets] = In.size();
    return counts;
  }

  // Parallel internal counting sort specialized to type for bucket counts
  template <typename s_size_t, 
	    typename InS, typename OutS, typename KeyS>
  sequence<size_t> count_sort_(InS& In, OutS& Out, KeyS& Keys,
			       size_t num_buckets,
			       float parallelism=1.0,
			       bool skip_if_in_one = false) {
    timer t("count sort", false);
    using T = typename InS::value_type;
    size_t n = In.size();
    size_t num_threads = num_workers();
    bool is_nested = parallelism < .5;

    // if not given, then use heuristic to choose num_blocks
    size_t sqrt = (size_t) ceil(pow(n,0.5));
    size_t num_blocks = 
      (size_t) (n < (1<<24)) ? sqrt/17 : ((n < (1<<28)) ? sqrt/9 : sqrt/5);
    if (is_nested) num_blocks = num_blocks / 6;
    else if (2*num_blocks < num_threads) num_blocks *= 2;
    if (sizeof(T) <= 4) num_blocks = num_blocks/2;
    
    size_t par_lower = 1 + round(num_threads * parallelism * 9);
    size_t size_lower = 1 + n * sizeof(T) / 700000;
    size_t bucket_upper = n * sizeof(T) / (4 * num_buckets * sizeof(s_size_t));
    num_blocks = std::min(bucket_upper, std::max(par_lower, size_lower));

    // if insufficient parallelism, sort sequentially
    if (n < SEQ_THRESHOLD || num_blocks == 1 || num_threads == 1) {
      return seq_count_sort(In,Out,Keys,num_buckets);}

    size_t block_size = ((n-1)/num_blocks) + 1;
    size_t m = num_blocks * num_buckets;
    
    s_size_t *counts = new_array_no_init<s_size_t>(m,1);
    t.next("head");

    // sort each block
    par_for(0, num_blocks,  [&] (size_t i) {
	s_size_t start = std::min(i * block_size, n);
	s_size_t end =  std::min(start + block_size, n);
	seq_count_(In.slice(start,end), Keys.slice(start,end),
		   counts + i*num_buckets, num_buckets);
      }, 1, is_nested);

    t.next("count");

    sequence<size_t> bucket_offsets = sequence<size_t>::no_init(num_buckets+1);
    par_for(0, num_buckets, [&] (size_t i) {
	size_t v = 0;
	for (size_t j= 0; j < num_blocks; j++) 
	  v += counts[j*num_buckets + i];
	bucket_offsets[i] = v;
      }, 1 + 1024/num_blocks);
    bucket_offsets[num_buckets] = 0;

    // if all in one buckect, then no need to sort
    size_t num_non_zero = 0;
    for (size_t i =0 ; i < num_buckets; i++)
      num_non_zero += (bucket_offsets[i] > 0);
    if (skip_if_in_one && num_non_zero == 1)
      return sequence<size_t>(0);

    size_t total = scan_inplace(bucket_offsets.slice(), addm<size_t>());
    if (total != n) abort();
    
    sequence<s_size_t> dest_offsets = sequence<s_size_t>::no_init(num_blocks*num_buckets);
    par_for(0, num_buckets, [&] (size_t i) {
	size_t v = bucket_offsets[i];
	size_t start = i * num_blocks;
	for (size_t j= 0; j < num_blocks; j++) {
	  dest_offsets[start+j] = v;
	  v += counts[j*num_buckets + i];
	}
      }, 1 + 1024/num_blocks);

    s_size_t *counts2 = new_array_no_init<s_size_t>(m,1);
    
    par_for(0, num_blocks, [&] (size_t i) {
	size_t start = i * num_buckets;
	for (size_t j= 0; j < num_buckets; j++)
	  counts2[start+j] = dest_offsets[j*num_blocks + i];
      }, 1 + 1024/num_buckets);
    //par_for(0, n, [&] (size_t i) {Out[i] = In[0];}, 100000);
    
    t.next("buckets");
    
    // transpose<s_size_t>(counts, dest_offsets.begin()).trans(num_blocks,
    // 							    num_buckets);
    // size_t sum = scan_inplace(dest_offsets, addm<s_size_t>());
    // if (sum != n) abort();
    // transpose<s_size_t>(dest_offsets.begin(), counts).trans(num_buckets,
    // 							    num_blocks);
    //if (n > 1000000000) t.next("scan");

    par_for(0, num_blocks,  [&] (size_t i) {
	s_size_t start = std::min(i * block_size, n);
	s_size_t end =  std::min(start + block_size, n);
	seq_write_(In.slice(start,end), Out.begin(),
		   Keys.slice(start,end),
		   counts2 + i*num_buckets, num_buckets);
      }, 1, is_nested);

    //if (n > 1000000000) t.next("move");
    
    // for (s_size_t i=0; i < num_buckets; i++) {
    //   bucket_offsets[i] = dest_offsets[i*num_blocks];
    //   //cout << i << ", " << bucket_offsets[i] << endl;
    // }
    // // last element is the total size n
    // bucket_offsets[num_buckets] = n;

    t.next("transpose");
    free_array(counts);
    free_array(counts2);
    //std::this_thread::sleep_for(std::chrono::seconds(1));
    return bucket_offsets;
  }

  // Parallel version
  template <typename InS, typename KeyS>
  sequence<size_t> count_sort(InS const &In,
			      range<typename InS::value_type*> Out,
			      KeyS const &Keys,
			      size_t num_buckets,
			      float parallelism = 1.0,
			      bool skip_if_in_one=false) {
    size_t n = In.size();
    size_t max32 = ((size_t) 1) << 32;
    if (n < max32 && num_buckets < max32)
      // use 4-byte counters when larger ones not needed
      return count_sort_<uint32_t>(In, Out, Keys, num_buckets,
				   parallelism, skip_if_in_one);
    return count_sort_<size_t>(In, Out, Keys, num_buckets,
			       parallelism, skip_if_in_one);
  }
}
