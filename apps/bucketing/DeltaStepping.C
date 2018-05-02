#define WEIGHTED 1
#include <cmath>
#include "ligra.h"
#include "index_map.h"
#include "bucket.h"

constexpr uintE TOP_BIT = ((uintE)INT_E_MAX) + 1;
constexpr uintE VAL_MASK = INT_E_MAX;

struct Visit_F {
  array_imap<uintE> dists;
  Visit_F(array_imap<uintE>& _dists) : dists(_dists) { }

  inline Maybe<uintE> update(const uintE& s, const uintE& d, const intE& w) {
    uintE oval = dists.s[d];
    uintE dist = oval | TOP_BIT, n_dist = (dists.s[s] | TOP_BIT) + w;
    if (n_dist < dist) {
      if (!(oval & TOP_BIT)) { // First visitor
        dists[d] = n_dist;
        return Maybe<uintE>(oval);
      }
      dists[d] = n_dist;
    }
    return Maybe<uintE>();
  }

  inline Maybe<uintE> updateAtomic(const uintE& s, const uintE& d, const intE& w) {
    uintE oval = dists.s[d];
    uintE dist = oval | TOP_BIT;
    uintE n_dist = (dists.s[s] | TOP_BIT) + w;
    if (n_dist < dist) {
      if (!(oval & TOP_BIT) && CAS(&(dists[d]), oval, n_dist)) { // First visitor
        return Maybe<uintE>(oval);
      }
      writeMin(&(dists[d]), n_dist);
    }
    return Maybe<uintE>();
  }

  inline bool cond(const uintE& d) const { return true; }
};

template <class vertex>
void DeltaStepping(graph<vertex>& G, uintE src, uintE delta, size_t num_buckets=128) {
  auto V = G.V; size_t n = G.n, m = G.m;
  auto dists = array_imap<uintE>(n, [&] (size_t i) { return INT_E_MAX; });
  dists[src] = 0;

  auto get_bkt = [&] (const uintE& dist) -> const uintE {
    return (dist == INT_E_MAX) ? UINT_E_MAX : (dist / delta); };
  auto get_ring = [&] (const size_t& v) -> const uintE {
    auto d = dists[v];
    return (d == INT_E_MAX) ? UINT_E_MAX : (d / delta); };
  auto b = make_buckets(n, get_ring, increasing, num_buckets);

  auto apply_f = [&] (const uintE v, uintE& oldDist) -> void {
    uintE newDist = dists.s[v] & VAL_MASK;
    dists.s[v] = newDist; // Remove the TOP_BIT in the distance.
    // Compute the previous bucket and new bucket for the vertex.
    uintE prev_bkt = get_bkt(oldDist), new_bkt = get_bkt(newDist);
    bucket_dest dest = b.get_bucket(prev_bkt, new_bkt);
    oldDist = dest; // write back
  };

  auto bkt = b.next_bucket();
  while (bkt.id != b.null_bkt) {
    auto active = bkt.identifiers;
    // The output of the edgeMap is a vertexSubsetData<uintE> where the value
    // stored with each vertex is its original distance in this round
    auto res = edgeMapData<uintE>(G, active, Visit_F(dists), G.m/20, sparse_no_filter | dense_forward);
    vertexMap(res, apply_f);
    if (res.dense()) {
      b.update_buckets(res.get_fn_repr(), n);
    } else {
      b.update_buckets(res.get_fn_repr(), res.size());
    }
    res.del(); active.del();
    bkt = b.next_bucket();
  }
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  uintE src = P.getOptionLongValue("-src",0);
  uintE delta = P.getOptionLongValue("-delta",1);
  size_t num_buckets = P.getOptionLongValue("-nb", 128);
  if (num_buckets != (1 << pbbs::log2_up(num_buckets))) {
    cout << "Please specify a number of buckets that is a power of two" << endl;
    exit(-1);
  }
  cout << "### Application: Delta-Stepping" << endl;
  cout << "### Graph: " << P.getArgument(0) << endl;
  cout << "### Workers: " << getWorkers() << endl;
  cout << "### Buckets: " << num_buckets << endl;
  cout << "### n: " << GA.n << endl;
  cout << "### m: " << GA.m << endl;
  cout << "### delta = " << delta << endl;
  DeltaStepping(GA, src, delta, num_buckets);
}
