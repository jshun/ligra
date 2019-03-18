#include "ligra.h"
#include "index_map.h"
#include "bucket.h"
#include "edgeMapReduce.h"

#include "lib/random_shuffle.h"
#include "lib/random.h"

constexpr uintE TOP_BIT = ((uintE)INT_E_MAX) + 1;
constexpr uintE COVERED = ((uintE)INT_E_MAX) - 1;
constexpr double epsilon = 0.01;
const double x = 1.0/log(1.0 + epsilon);
auto max_f = [] (uintE x, uintE y) { return std::max(x,y); };

struct Visit_Elms {
  uintE* elms;
  uintE* perm;
  Visit_Elms(uintE* _elms, uintE* _perm) : elms(_elms), perm(_perm) {}
  inline bool updateAtomic(const uintE& s, const uintE& d) {
    uintE p_s = perm[s];
    writeMin(&(elms[d]), p_s); // writes perm[s], not s.
    return false;
  }
  inline bool update(const uintE& s, const uintE& d) { return updateAtomic(s, d); }
  inline bool cond(const uintE& d) const { return elms[d] != COVERED; }
};

template <class vertex>
dyn_arr<uintE> SetCover(graph<vertex>& G, size_t num_buckets=128) {
  timer t; t.start();
  auto Elms = array_imap<uintE>(G.n, [&] (size_t i) { return UINT_E_MAX; });

  auto get_bucket_clamped = [&] (size_t deg) -> uintE { return (deg == 0) ? UINT_E_MAX : (uintE)floor(x * log((double) deg)); };
  auto D = array_imap<uintE>(G.n, [&] (size_t i) { return get_bucket_clamped(G.V[i].getOutDegree()); });
  auto B = make_buckets(G.n, D, decreasing, strictly_decreasing, num_buckets);

  auto perm = array_imap<uintE>(G.n);

  auto r = pbbs::random();
  size_t rounds = 0;
  dyn_arr<uintE> cover = dyn_arr<uintE>();
  while (true) {
    auto bkt = B.next_bucket();
    auto active = bkt.identifiers; size_t cur_bkt = bkt.id;
    if (cur_bkt == B.null_bkt) { break; }

    // 1. sets -> elements (Pack out sets and update their degree)
    auto pack_predicate = [&] (const uintE& u, const uintE& ngh) { return Elms[ngh] != COVERED; };
    auto pack_apply = [&] (uintE v, size_t ct) { D[v] = get_bucket_clamped(ct); };
    auto packed_vtxs = edgeMapFilter(G, active, pack_predicate, pack_edges);
    vertexMap(packed_vtxs, pack_apply);

    // Calculate the sets which still have sufficient degree (degree >= threshold)
    size_t threshold = ceil(pow(1.0+epsilon,cur_bkt));
    auto above_threshold = [&] (const uintE& v, const uintE& deg) { return deg >= threshold; };
    auto still_active = vertexFilter2<uintE>(packed_vtxs, above_threshold);
    packed_vtxs.del();

    still_active.toSparse();
//    cout << "perm of size = " << still_active.size() << endl;
    auto P = pbbs::random_permutation<uintE>(still_active.size(), r);
//    cout << "generated = " << still_active.size() << endl;
//    for (size_t i=0; i<still_active.size(); i++) {
//      cout << "P[i] = " << P[i] << endl;
//    }
    parallel_for(size_t i=0; i<still_active.size(); i++) {
      uintE v = still_active.vtx(i);
      perm[v] = P[i];
//      cout << "v = " << v << endl;
    }

    // 2. sets -> elements (writeMin to acquire neighboring elements)
    edgeMap(G, still_active, Visit_Elms(Elms.s, perm.s), -1, no_output | dense_forward);

    // 3. sets -> elements (count and add to cover if enough elms were won)
    const size_t low_threshold = std::max((size_t)ceil(pow(1.0+epsilon,cur_bkt-1)), (size_t)1);
    auto won_ngh_f = [&] (const uintE& u, const uintE& v) -> bool { return Elms[v] == perm[u]; };
    auto threshold_f = [&] (const uintE& v, const uintE& numWon) {
      if (numWon >= low_threshold) D[v] = UINT_E_MAX;
    };
    auto activeAndCts = edgeMapFilter(G, still_active, won_ngh_f);
    vertexMap(activeAndCts, threshold_f);
    auto inCover = vertexFilter2(activeAndCts, [&] (const uintE& v, const uintE& numWon) {
        return numWon >= low_threshold; });
    cover.copyInF([&] (uintE i) { return inCover.vtx(i); }, inCover.size());
    inCover.del(); activeAndCts.del();

    // 4. sets -> elements (Sets that joined the cover mark their neighboring
    // elements as covered. Sets that didn't reset any acquired elements)
    auto reset_f = [&] (const uintE& u, const uintE& v) -> bool {
      if (Elms[v] == perm[u]) {
        if (D[u] == UINT_E_MAX) Elms[v] = COVERED;
        else Elms[v] = UINT_E_MAX;
      } return false;
    };
    edgeMap(G, still_active, EdgeMap_F<decltype(reset_f)>(reset_f), -1, no_output | dense_forward);

    // Rebucket the active sets. Ignore those that joined the cover.
    active.toSparse();
    auto f = [&] (size_t i) -> Maybe<tuple<uintE, uintE> > {
      const uintE v = active.vtx(i);
      const uintE v_bkt = D[v];
      uintE bkt = UINT_E_MAX;
      if (!(v_bkt == UINT_E_MAX))
        bkt = B.get_bucket(v_bkt);
      return Maybe<tuple<uintE, uintE> >(make_tuple(v, bkt));
    };
    B.update_buckets(f, active.size());
    active.del(); still_active.del();
    rounds++;
  }
  t.stop(); t.reportTotal("Running time: ");

  auto elm_cov = make_in_imap<uintE>(G.n, [&] (uintE v) { return (uintE)(Elms[v] == COVERED); });
  size_t elms_cov = pbbso::reduce_add(elm_cov);
  cout << "|V| = " << G.n << " |E| = " << G.m << endl;
  cout << "|cover|: " << cover.size << endl;
  cout << "Rounds: " << rounds << endl;
  cout << "Num_uncovered = " << (G.n - elms_cov) << endl;
  return cover;
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  size_t num_buckets = P.getOptionLongValue("-nb", 128);
  cout << "### Application: set-cover" << endl;
  cout << "### Graph: " << P.getArgument(0) << endl;
  cout << "### Workers: " << getWorkers() << endl;
  cout << "### Buckets: " << num_buckets << endl;
  cout << "### n: " << GA.n << endl;
  cout << "### m: " << GA.m << endl;
  auto cover = SetCover(GA, num_buckets);
  cover.del();
}
