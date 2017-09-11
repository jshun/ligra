// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011-2016 Guy Blelloch and the PBBS team
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

#include <iostream>
#include <tuple>

#include "index_map.h"

namespace pbbs {
  using namespace std;

  constexpr const size_t _log_block_size = 12;
  constexpr const size_t _block_size = (1 << _log_block_size);

  inline size_t num_blocks(size_t n, size_t block_size) {
    return (1 + ((n)-1)/(block_size));}

  template <class F>
  void sliced_for(size_t n, size_t block_size, const F& f) {
    size_t l = num_blocks(n, block_size);
    parallel_for_1 (size_t i = 0; i < l; i++) {
      size_t s = i * block_size;
      size_t e = min(s + block_size, n);
      f(i, s, e);
    }
  }

  template <class Index_Map, class F>
  auto reduce_serial(Index_Map A, const F& f) -> typename Index_Map::T {
    typename Index_Map::T r = A[0];
    for (size_t j=1; j < A.size(); j++) {
      r = f(r,A[j]);
    }
    return r;
  }

  template <class Index_Map, class F>
  auto reduce(Index_Map A, const F& f, flags fl = no_flag)
    -> typename Index_Map::T
  {
    using T = typename Index_Map::T;
    size_t n = A.size();
    size_t l = num_blocks(n, _block_size);
    if (l <= 1 || (fl & fl_sequential))
      return reduce_serial(A, f);
    array_imap<T> Sums(l);
    sliced_for (n, _block_size,
		[&] (size_t i, size_t s, size_t e)
		{ Sums[i] = reduce_serial(A.cut(s,e), f);});
    T r = reduce_serial(Sums, f);
    return r;
  }

  // sums I with + (can be any type with + defined)
  template <class Index_Map>
  auto reduce_add(Index_Map I, flags fl = no_flag)
    -> typename Index_Map::T
  {
    using T = typename Index_Map::T;
    auto add = [] (T x, T y) {return x + y;};
    return reduce(I, add, fl);
  }

  const flags fl_scan_inclusive = (1 << 4);

  // serial scan with combining function f and start value zero
  // fl_scan_inclusive indicates it includes the current location
  template <class Imap_In, class Imap_Out, class F>
  auto scan_serial(Imap_In In, Imap_Out Out,
		   const F& f, typename Imap_In::T zero,
		   flags fl = no_flag)  -> typename Imap_In::T
  {
    using T = typename Imap_In::T;
    T r = zero;
    size_t n = In.size();
    bool inclusive = fl & fl_scan_inclusive;
    if (inclusive) {
      for (size_t i = 0; i < n; i++) {
	r = f(r,In[i]);
	Out.update(i,r);
      }
    } else {
      for (size_t i = 0; i < n; i++) {
	T t = In[i];
	Out.update(i,r);
	r = f(r,t);
      }
    }
    return r;
  }

  // parallel version of scan_serial -- see comments above
  template <class Imap_In, class Imap_Out, class F>
  auto scan(Imap_In In, Imap_Out Out,
	    const F& f, typename Imap_In::T zero,
	    flags fl = no_flag)  -> typename Imap_In::T
  {
    using T = typename Imap_In::T;
    size_t n = In.size();
    size_t l = num_blocks(n,_block_size);
    if (l <= 2 || fl & fl_sequential)
      return scan_serial(In, Out, f, zero, fl);
    array_imap<T> Sums(l);
    sliced_for (n, _block_size,
		[&] (size_t i, size_t s, size_t e)
		{ Sums[i] = reduce_serial(In.cut(s,e),f);});
    T total = scan_serial(Sums, Sums, f, zero, 0);
    sliced_for (n, _block_size,
		[&] (size_t i, size_t s, size_t e)
		{ scan_serial(In.cut(s,e), Out.cut(s,e), f, Sums[i], fl);});
    return total;
  }

  template <class Imap_In, class Imap_Out>
  auto scan_add(Imap_In In, Imap_Out Out, flags fl = no_flag)
    -> typename Imap_In::T
  {
    using T = typename Imap_In::T;
    auto add = [] (T x, T y) {return x + y;};
    return scan(In, Out, add, (T) 0, fl);
  }

  template <class Index_Map>
  size_t sum_flags_serial(Index_Map I) {
    size_t r = 0;
    for (size_t j=0; j < I.size(); j++) r += I[j];
    return r;
  }

  template <class Imap_In, class Imap_Fl>
  auto pack_serial(Imap_In In, Imap_Fl Fl)
    -> array_imap<typename Imap_In::T> {
    using T = typename Imap_In::T;
    size_t n = In.size();
    size_t m = sum_flags_serial(Fl);
    T* Out = new_array_no_init<T>(m);
    size_t k = 0;
    for (size_t i=0; i < n; i++)
      if (Fl[i]) assign_uninitialized(Out[k++], In[i]);
    return make_array_imap(Out,m);
  }

  template <class Imap_In, class Imap_Fl>
  void pack_serial_at(Imap_In In, typename Imap_In::T* Out, Imap_Fl Fl) {
    size_t k = 0;
    for (size_t i=0; i < In.size(); i++)
      if (Fl[i]) Out[k++] = In[i];
  }

  template <class Imap_In, class Imap_Fl>
  auto pack(Imap_In In, Imap_Fl Fl, flags fl = no_flag)
    -> array_imap<typename Imap_In::T>
  {
    using T = typename Imap_In::T;
    size_t n = In.size();
    size_t l = num_blocks(n,_block_size);
    if (l <= 1 || fl & fl_sequential)
      return pack_serial(In, Fl);
    array_imap<size_t> Sums(l);
    sliced_for (n, _block_size,
		[&] (size_t i, size_t s, size_t e)
		{ Sums[i] = sum_flags_serial(Fl.cut(s,e));});
    size_t m = scan_add(Sums, Sums);
    T* Out = new_array_no_init<T>(m);
    sliced_for (n, _block_size,
		[&] (size_t i, size_t s, size_t e)
		{ pack_serial_at(In.cut(s,e),
				 Out + Sums[i],
				 Fl.cut(s,e));});
    return make_array_imap(Out,m);
  }

  template <class Idx_Type, class Imap_Fl>
  array_imap<Idx_Type> pack_index(Imap_Fl Fl, flags fl = no_flag) {
    auto identity = [] (size_t i) {return (Idx_Type) i;};
    return pack(make_in_imap<Idx_Type>(Fl.size(),identity), Fl, fl);
  }

  template <class Idx_Type, class D, class F>
  array_imap<tuple<Idx_Type, D> > pack_index_and_data(F& f, size_t size, flags fl = no_flag) {
    auto identity = [&] (size_t i) {return make_tuple((Idx_Type)i, get<1>(f(i))); };
    auto flgs_in = make_in_imap<bool>(size, [&] (size_t i) { return get<0>(f(i)); });
    return pack(make_in_imap<tuple<Idx_Type, D> >(size,identity), flgs_in, fl);
  }

  template <class T, class PRED>
  size_t filter_serial(T* In, T* Out, size_t n, PRED p) {
    size_t k = 0;
    for (size_t i = 0; i < n; i++)
      if (p(In[i])) Out[k++] = In[i];
    return k;
  }

  // template <class T, class PRED>
  // size_t filter(T* In, T* Out, size_t n, PRED p, flags fl = no_flag) {
  //   if (n < _F_BSIZE || fl & fl_sequential)
  //     return filter_serial(In, Out, n, p);
  //   bool *Flags = new_array_no_init<bool>(n);
  //   parallel_for (size_t i=0; i < n; i++) Flags[i] = (bool) p(In[i]);
  //   size_t  m = pack(In, Out, Flags, n);
  //   free(Flags);
  //   return m;
  // }

  // // Avoids reallocating the bool array
  // template <class T, class PRED>
  // size_t filter(T* In, T* Out, bool* Flags, size_t n, PRED p,
  // 		flags fl = no_flag) {
  //   if (n < _F_BSIZE || fl & fl_sequential)
  //     return filter_serial(In, Out, n, p);
  //   parallel_for (size_t i=0; i < n; i++) Flags[i] = (bool) p(In[i]);
  //   size_t  m = pack(In, Out, Flags, n);
  //   return m;
  // }

  // template <class T, class PRED>
  // seq<T> filter(T* In, size_t n, PRED p, flags fl = no_flag) {
  //   bool *Fl = new_array_no_init<bool>(n);
  //   parallel_for (size_t i=0; i < n; i++) Fl[i] = (bool) p(In[i]);
  //   seq<T> R = pack(In, Fl, n);
  //   free(Fl);
  //   return R;
  // }

  // // g maps indices to input elements
  // // f maps indices to boolean flags
  // template <class T, class G, class F>
  // seq<T> filter(size_t n, G g, F f) {
  //   bool *Fl = new_array_no_init<bool>(n);
  //   parallel_for (size_t i=0; i < n; i++)
  //     Fl[i] = (bool) f(i);
  //   seq<T> R = pack<T>((T*) NULL, Fl, 0, n, g);
  //   free(Fl);
  //   return R;
  // }

   // Faster for a small number in output (about 40% or less)
   // Destroys the input.   Does not need a bool array.
   template <class T, class PRED>
   size_t filterf(T* In, T* Out, size_t n, PRED p) {
     size_t b = _F_BSIZE;
     if (n < b)
       return filter_serial(In, Out, n, p);
     size_t l = nblocks(n, b);
     b = nblocks(n, l);
     size_t *Sums = new_array_no_init<size_t>(l + 1);
     parallel_for_1 (size_t i = 0; i < l; i++) {
       size_t s = i * b;
       size_t e = min(s + b, n);
       size_t k = s;
       for (size_t j = s; j < e; j++)
   	if (p(In[j])) In[k++] = In[j];
       Sums[i] = k - s;
     }
     auto isums = array_imap<size_t>(Sums,l);
     size_t m = scan_add(isums, isums);
     Sums[l] = m;
     parallel_for_1 (size_t i = 0; i < l; i++) {
       T* I = In + i*b;
       T* O = Out + Sums[i];
       for (size_t j = 0; j < Sums[i+1]-Sums[i]; j++) {
        O[j] = I[j];
       }
     }
     free(Sums);
     return m;
   }

   template <class T, class PRED>
   size_t filterf_and_clear(T* In, T* Out, size_t n, PRED p, T& empty, size_t* Sums) {
     size_t b = _F_BSIZE;
     if (n < b)
       return filter_serial(In, Out, n, p);
     size_t l = nblocks(n, b);
     b = nblocks(n, l);
     parallel_for_1 (size_t i = 0; i < l; i++) {
       size_t s = i * b;
       size_t e = min(s + b, n);
       size_t k = s;
       for (size_t j = s; j < e; j++) {
        if (p(In[j])) {
          In[k] = In[j];
          if (j > k) {
            In[j] = empty;
          }
          k++;
        }
       }
       Sums[i] = k - s;
     }
     auto isums = array_imap<size_t>(Sums,l);
     size_t m = scan_add(isums, isums);
     Sums[l] = m;
     parallel_for_1 (size_t i = 0; i < l; i++) {
       T* I = In + i*b;
       T* O = Out + Sums[i];
       for (size_t j = 0; j < Sums[i+1]-Sums[i]; j++) {
        O[j] = I[j];
        I[j] = empty;
       }
     }
     return m;
   }

}

