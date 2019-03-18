#pragma once
#include "concurrent_stack.h"
#include "utilities.h"
#include <atomic>
#include "memory_size.h"

struct mem_pool {
  concurrent_stack<void*>* buckets;
  static constexpr size_t header_size = 64;
  static constexpr size_t log_base = 20;
  static constexpr size_t num_buckets = 20;
  static constexpr size_t small_size_tag = 100;
  std::atomic<long> allocated{0};
  std::atomic<long> used{0};
  size_t mem_size{getMemorySize()};
  
  mem_pool() {
    buckets = new concurrent_stack<void*>[num_buckets];
  };

  void* add_header(void* a) {
    return (void*) ((char*) a + header_size);}

  void* sub_header(void* a) {
    return (void*) ((char*) a - header_size);}

  void* alloc(size_t s) {
    size_t log_size = pbbs::log2_up((size_t) s + header_size);
    if (log_size < 20) {
      void* a = (void*) aligned_alloc(header_size, s + header_size);
      *((size_t*) a) = small_size_tag;
      return add_header(a);
    }
    size_t bucket = log_size - log_base;
    maybe<void*> r = buckets[bucket].pop();
    size_t n = ((size_t) 1) << log_size;
    used += n;
    if (r) {
      //if (n > 10000000) cout << "alloc: " << add_header(*r) << ", " << n << endl;
      return add_header(*r);
    }
    else {
      void* a = (void*) aligned_alloc(header_size, n);
      //if (n > 10000000) cout << "alloc: " << add_header(a) << ", " << n << endl;
      allocated += n;
      if (a == NULL) std::cout << "alloc failed" << std::endl;
      // a hack to make sure pages are touched in parallel
      // not the right choice if you want processor local allocations
      size_t stride = (1 << 21); // 2 Mbytes in a huge page
      auto touch_f = [&] (size_t i) {((bool*) a)[i*stride] = 0;};
      par_for(0, n/stride, touch_f, 1);
      *((size_t*) a) = bucket;
      return add_header(a);
    }
  }

  void afree(void* a) {
    //cout << "free: " << a << endl;
    void* b = sub_header(a);
    size_t bucket = *((size_t*) b);
    if (bucket == small_size_tag) free(b);
    else if (bucket >= num_buckets) {
      std::cout << "corrupted header in free" << std::endl;
      abort();
    } else {
      size_t n = ((size_t) 1) << (bucket+log_base);
      //if (n > 10000000) cout << "free: " << a << ", " << n << endl;
      used -= n;
      if (n > mem_size/64) { // fix to 64
	free(b);
	allocated -= n;
      } else {
	buckets[bucket].push(b);
      }
    }
  }

  void clear() {
    for (size_t i = 0; i < num_buckets; i++) {
      size_t n = ((size_t) 1) << (i+log_base);
      maybe<void*> r = buckets[i].pop();
      while (r) {
	allocated -= n;
	free(*r);
	r = buckets[i].pop();
      }
    }
  }
};

static mem_pool my_mem_pool;

