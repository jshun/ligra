#pragma once

#include <tuple>

using namespace std;

template <class T>
struct Maybe {
  // pays an extra byte for NONE/SOME
  // avoid Maybe<T>*---this is basically free otherwise
  bool exists;
  T t;
  Maybe(const T& _t) : t(_t), exists(true) {}
  Maybe() : exists(false) {}
};

inline const Maybe<tuple<uintE, uintE> > wrap(const uintE& l, const uintE& r) {
  auto t = Maybe<tuple<uintE, uintE> >(make_tuple(l, r));
  t.exists = (l != UINT_E_MAX) && (r != UINT_E_MAX);
  return t;
}

template <class L, class R>
inline const Maybe<tuple<L, R> > wrap(const L& l, const Maybe<R>& r) {
  auto t = Maybe<tuple<L, R> >(make_tuple(l, getT(r)));
  t.exists = r.exists;
  return t;
}

template <class L, class R>
inline Maybe<tuple<L, R> > wrap(const Maybe<L>& l, const R& r) {
  auto t = Maybe<tuple<L, R> >(make_tuple(getT(l), r));
  t.exists = l.exists;
  return t;
}

template <class L, class R>
inline Maybe<tuple<L, R> > wrap(const Maybe<L>& l, const Maybe<R>& r) {
  auto t = Maybe<tuple<L, R> >(make_tuple(getT(l), getT(r)));
  t.exists = l.exists && r.exists;
  return t;
}

template <class T>
inline bool isSome(const Maybe<T>& m) {
  return m.exists;
}

template <class T>
inline T getT(const Maybe<T>& m) {
  return m.t;
}
