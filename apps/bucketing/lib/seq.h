#pragma once

#include "utilities.h"

#define hello foo

#ifdef CONCEPTS
template<typename T>
concept bool Seq =
  requires(T t, size_t u) {
  typename T::value_type;
  { t.size() } -> size_t;
  { t.slice() };
  { t[u] };
};

template<typename T>
concept bool Range =
  Seq<T> && requires(T t, size_t u) {
  { t[u] } -> typename T::value_type&;
  typename T::iterator;
};
#define SEQ Seq
#define RANGE Range
#else
#define SEQ typename
#define RANGE typename
#endif

namespace pbbs {

  template <typename Iterator>
  struct range {
  public:
    using value_type = typename std::iterator_traits<Iterator>::value_type;
    using iterator = Iterator;
    range() {};
    range(iterator s, iterator e) : s(s), e(e) {};
    value_type& operator[] (const size_t i) const {return s[i];}
    range slice(size_t ss, size_t ee) const {
      return range(s + ss, s + ee); }
    range slice() const {return range(s,e);};
    size_t size() const { return e - s;}
    iterator begin() const {return s;}
    iterator end() const {return e;}
  private:
    iterator s;
    iterator e;
  };

  template <class Iter>
  range<Iter> make_range(Iter s, Iter e) {
    return range<Iter>(s,e);
  }

  template <typename T, typename F>
  struct delayed_sequence {
    using value_type = T;
    delayed_sequence(size_t n, F _f) : f(_f), s(0), e(n) {};
    delayed_sequence(size_t n, value_type v) : f([&] (size_t i) {return v;}), s(0), e(n) {};
    delayed_sequence(size_t s, size_t e, F _f) : f(_f), s(s), e(e) {};
    const value_type operator[] (size_t i) const {return (f)(i+s);}
    delayed_sequence<T,F> slice(size_t ss, size_t ee) const {
      return delayed_sequence<T,F>(s+ss,s+ee,f); }
    delayed_sequence<T,F> slice() const {
      return delayed_sequence<T,F>(s,e,f); }
    size_t size() const { return e - s;}
  private:
    const F f;
    const size_t s, e;
  };

  // used so second template argument can be inferred
  template <class T, class F>
  delayed_sequence<T,F> delayed_seq (size_t n, F f) {
    return delayed_sequence<T,F>(n,f);
  }

  constexpr bool check_copy = true;

  template <typename T>
  struct sequence {
  public:
    using value_type = T;
    //using iterator = T*;

    sequence() : s(NULL), n(0) {}

    // copy constructor
    sequence(const sequence& a) : n(a.n) {
      copy_here(a.s, a.n);
      if (check_copy)
	cout << "copy constructor: len: " << a.n << " sizeof: " << sizeof(T) << endl;
    }

    // move constructor
    sequence(sequence&& a)
      : s(a.s), n(a.n) {a.s = NULL; a.n = 0;}

    // copy assignment
    sequence& operator = (const sequence& a) {
      if (this != &a) {clear(); copy_here(a.s, a.n);}
      if (check_copy)
	cout << "copy assignment: len: " << a.n << " sizeof: " << sizeof(T) << endl;
      return *this;
    }

    // move assignment
    sequence& operator = (sequence&& a) {
      if (this != &a) {
	clear(); s = a.s; n = a.n; a.s = NULL; a.n = 0;}
      return *this;
    }

    sequence(const size_t n)
      : s(pbbs::new_array<T>(n)), n(n) {
      //if (n > 1000000000) cout << "make empty: " << s << endl;
    };

    sequence(value_type* a, const size_t n) : s(a), n(n) {
      //cout << "dangerous" << endl;
    };

    static sequence<T> no_init(const size_t n) {
      sequence<T> r;
      r.s = pbbs::new_array_no_init<T>(n);
      //if (n > 1000000000) cout << "make no init: " << r.s << endl;
      r.n = n;
      return r;
    };

    sequence(const size_t n, value_type v)
      : s(pbbs::new_array_no_init<T>(n, true)), n(n) {
      //if (n > 1000000000) cout << "make const: " << s << endl;
      auto f = [=] (size_t i) {new ((void*) (s+i)) value_type(v);};
      par_for(0, n, f);
    };

    template <typename Func>
    sequence(const size_t n, Func f)
      : s(pbbs::new_array_no_init<T>(n)), n(n) {
      //if (n > 1000000000) cout << "make func: " << s << endl;
      par_for(0, n, [&] (size_t i) {
	  new ((void*) (s+i)) value_type(f(i));}, 1000);
    };

    template <typename Iter>
    sequence(range<Iter> a) {
      copy_here(a.begin(), a.size());
    }

    template <class F>
    sequence(delayed_sequence<T,F> a) {
      copy_here(a, a.size());
    }

    ~sequence() { clear();}

    value_type& operator[] (const size_t i) const {return s[i];}

    range<value_type*> slice(size_t ss, size_t ee) const {
      return range<value_type*>(s + ss, s + ee);
    }

    range<std::reverse_iterator<value_type*>>
    rslice(size_t ss, size_t ee) const {
      auto i = std::make_reverse_iterator(s+n);
      return range<decltype(i)>(i + ss, i + ee);
    }

    range<std::reverse_iterator<value_type*>>
    rslice() const {return rslice(0, n);};

    range<value_type*> slice() const {
      return range<value_type*>(s, s + n);
    }

    void swap(sequence& b) {
      std::swap(s, b.s); std::swap(n, b.n);
    }

    size_t size() const { return n;}
    value_type* begin() const {return s;}
    value_type* end() const {return s + n;}

    // gives up ownership of space
    value_type* to_array() {
      value_type* r = s;  s = NULL; n = 0; return r;}

    void clear() {
      if (s != NULL) {
	//if (n > 1000000000) cout << "delete: " << s << endl;
        pbbs::delete_array<T>(s, n);
        s = NULL; n = 0;
      }
    }

  private:
    template <class Seq>
    void copy_here(Seq const &a, size_t an) {
      n = an;
      s = pbbs::new_array_no_init<T>(n, true);
      //cout << "s = " << s << " n = " << n << endl;
      //if (n > 0) { cout << "Yikes, copy: " << s << endl;}
      par_for(0, n, [&] (size_t i) {
	      pbbs::assign_uninitialized(s[i], a[i]);});
    }

    T *s;
    size_t n;
  };

  template <class Iter>
  bool slice_eq(range<Iter> a, range<Iter> b) {
    return a.begin() == b.begin();}

  template <class SeqA, class SeqB>
  bool slice_eq(SeqA a, SeqB b) { return false;}

}


