#define HYPER 1
#include "hygra.h"
#include "index_map.h"
#include "bucket.h"
#include "edgeMapReduce.h"

//return true for neighbor the first time it's updated 
struct Remove_Hyperedge {
  uintE* Flags;
  Remove_Hyperedge(uintE* _Flags) : Flags(_Flags) {}
  inline bool update (uintE s, uintE d) {
    return Flags[d] = 1;
  }
  inline bool updateAtomic (uintE s, uintE d){
    return CAS(&Flags[d],(uintE)0,(uintE)1);
  }
  inline bool cond (uintE d) { return Flags[d] == 0; }
};

template <class vertex>
array_imap<uintE> KCore(hypergraph<vertex>& GA, size_t num_buckets=128) {
  const size_t nv = GA.nv, mv = GA.mv, nh = GA.nh;
  bool* active = newA(bool,nv);
  {parallel_for(long i=0;i<nv;i++) active[i] = 1;}
  vertexSubset Frontier(nv, nv, active);
  uintE* Degrees = newA(uintE,nv);
  {parallel_for(long i=0;i<nv;i++) {
      Degrees[i] = GA.V[i].getOutDegree();
    }}
  uintE* Flags = newA(uintE,nh);
  {parallel_for(long i=0;i<nh;i++) Flags[i] = 0;}
  auto D = array_imap<uintE>(GA.nv, [&] (size_t i) { return GA.V[i].getOutDegree(); });

  auto hp = HypergraphProp<uintE, vertex>(GA, make_tuple(UINT_E_MAX, 0), (size_t)GA.mv/5);
  auto b = make_buckets(nv, D, increasing, num_buckets);

  size_t finished = 0;
  while (finished != nv) {
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

    hyperedgeSubset FrontierH = vertexProp(GA,active,Remove_Hyperedge(Flags));
    //cout << "k="<<k<< " num active = " << active.numNonzeros() << " frontierH = " << FrontierH.numNonzeros() << endl;
    vertexSubsetData<uintE> moved = hp.template hyperedgePropCount<uintE>(FrontierH, apply_f);
    b.update_buckets(moved.get_fn_repr(), moved.size());
    moved.del(); active.del();
  }
  return D;
}

template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  size_t num_buckets = P.getOptionLongValue("-nb", 128);
  if (num_buckets != (1 << pbbs::log2_up(num_buckets))) {
    cout << "Number of buckets must be a power of two." << endl;
    exit(-1);
  }
  //cout << "### Application: k-core" << endl;
  //cout << "### Graph: " << P.getArgument(0) << endl;
  //cout << "### Workers: " << getWorkers() << endl;
  //cout << "### Buckets: " << num_buckets << endl;

  auto cores = KCore(GA, num_buckets);
  //cout << "### Max core: " << sequence::reduce(cores.s,GA.nv,maxF<uintE>()) << endl;
}
