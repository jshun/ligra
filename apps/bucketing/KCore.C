#include "ligra.h"
#include "index_map.h"
#include "bucket.h"
#include "edgeMapReduce.h"

template <class vertex>
array_imap<uintE> KCore(graph<vertex>& GA, size_t num_buckets=128) {
  const size_t n = GA.n; const size_t m = GA.m;
  auto D = array_imap<uintE>(n, [&] (size_t i) { return GA.V[i].getOutDegree(); });

  auto em = EdgeMap<uintE, vertex>(GA, make_tuple(UINT_E_MAX, 0), (size_t)GA.m/5);
  auto b = make_buckets(n, D, increasing, num_buckets);

  size_t finished = 0;
  while (finished != n) {
    auto bkt = b.next_bucket();
    auto active = bkt.identifiers;
    uintE k = bkt.id;
    finished += active.size();

    auto apply_f = [&] (const tuple<uintE, uintE>& p) -> const Maybe<tuple<uintE, uintE> > {
      uintE v = std::get<0>(p), edgesRemoved = std::get<1>(p);
      uintE deg = D.s[v];
      if (deg > k) {
        uintE new_deg = max(deg - edgesRemoved, k);
        D.s[v] = new_deg;
        uintE bkt = b.get_bucket(deg, new_deg);
        return wrap(v, bkt);
      }
      return Maybe<tuple<uintE, uintE> >();
    };

    vertexSubsetData<uintE> moved = em.template edgeMapCount<uintE>(active, apply_f);
    b.update_buckets(moved.get_fn_repr(), moved.size());
    moved.del(); active.del();
  }
  return D;
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  size_t num_buckets = P.getOptionLongValue("-nb", 128);
  if (num_buckets != (1 << pbbs::log2_up(num_buckets))) {
    cout << "Number of buckets must be a power of two." << endl;
    exit(-1);
  }
  cout << "### Application: k-core" << endl;
  cout << "### Graph: " << P.getArgument(0) << endl;
  cout << "### Workers: " << getWorkers() << endl;
  cout << "### Buckets: " << num_buckets << endl;
  cout << "### n: " << GA.n << endl;
  cout << "### m: " << GA.m << endl;

  auto cores = KCore(GA, num_buckets);
  uintE mc = 0;
  for (size_t i=0; i < GA.n; i++) { mc = std::max(mc, cores[i]); }
  cout << "### Max core: " << mc << endl;
}
