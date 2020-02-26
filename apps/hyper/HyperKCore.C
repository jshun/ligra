// This code is part of the project "Ligra: A Lightweight Graph Processing
// Framework for Shared Memory", presented at Principles and Practice of 
// Parallel Programming, 2013.
// Copyright (c) 2013 Julian Shun and Guy Blelloch
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

// Parallel implementation of K-Core decomposition of a symmetric
// hypergraph.
#define HYPER 1
#include "hygra.h"

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

struct Update_Deg {
  intE* Degrees; uintE k;
  Update_Deg(intE* _Degrees, uintE _k) : Degrees(_Degrees), k(_k) {}
  inline bool update (uintE s, uintE d) {
    Degrees[d] -= 1;
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d){
    xadd(&Degrees[d],-1);
    return 1;
  }
  inline bool cond (uintE d) { return Degrees[d] >= k; }
};

template<class vertex>
struct Deg_LessThan_K {
  vertex* V;
  intE* Degrees;
  uintE k;
  Deg_LessThan_K(vertex* _V, intE* _Degrees, uintE _k) : 
    V(_V), k(_k), Degrees(_Degrees) {}
  inline bool operator () (uintE i) {
    if(Degrees[i] < k) { Degrees[i] = k-1; return true; }
    else return false;
  }
};

template<class vertex>
struct Deg_AtLeast_K {
  vertex* V;
  intE *Degrees;
  uintE k;
  Deg_AtLeast_K(vertex* _V, intE* _Degrees, uintE _k) : 
    V(_V), k(_k), Degrees(_Degrees) {}
  inline bool operator () (uintE i) {
    return Degrees[i] >= k;
  }
};

//assumes symmetric hypergraph
// 1) iterate over all remaining active vertices
// 2) for each active vertex, remove if induced degree < k. Any vertex removed has
//    core-number (k-1) (part of (k-1)-core, but not k-core)
// 3) stop once no vertices are removed. Vertices remaining are in the k-core.
template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  const long nv = GA.nv, nh = GA.nh;
  bool* active = newA(bool,nv);
  {parallel_for(long i=0;i<nv;i++) active[i] = 1;}
  vertexSubset Frontier(nv, nv, active);
  intE* Degrees = newA(intE,nv);
  {parallel_for(long i=0;i<nv;i++) {
      Degrees[i] = GA.V[i].getOutDegree();
    }}
  uintE* Flags = newA(uintE,nh);
  {parallel_for(long i=0;i<nh;i++) Flags[i] = 0;}
  long largestCore = -1;

  //auto hp = HypergraphProp<uintE, vertex>(GA, make_tuple(UINT_E_MAX, 0), (size_t)GA.mv/5);

  for (long k = 1;;k++) {
    while (true) {
      vertexSubset toRemove 
	= vertexFilter(Frontier,Deg_LessThan_K<vertex>(GA.V,Degrees,k));
      vertexSubset remaining = vertexFilter(Frontier,Deg_AtLeast_K<vertex>(GA.V,Degrees,k));
      Frontier.del();
      Frontier = remaining;
      if (0 == toRemove.numNonzeros()) { // fixed point. found k-core
	toRemove.del();
        break;
      }
      else {
	hyperedgeSubset FrontierH = vertexProp(GA,toRemove,Remove_Hyperedge(Flags));
	//cout << "k="<<k-1<< " num active = " << toRemove.numNonzeros() << " frontierH = " << FrontierH.numNonzeros() << endl;
	// auto apply_f = [&] (const tuple<uintE, uintE>& p) -> const Maybe<tuple<uintE, uintE> > {
	//   uintE v = std::get<0>(p), edgesRemoved = std::get<1>(p);
	//   uintE deg = Degrees[v];
	//   if (deg > k-1) {
	//     uintE new_deg = max(deg - edgesRemoved, (uintE) k-1);
	//     Degrees[v] = new_deg;
	//     //uintE bkt = b.get_bucket(deg, new_deg);
	//     return wrap(v, 0);
	//   }
	//   return Maybe<tuple<uintE, uintE> >();
	// };
	//vertexSubsetData<uintE> moved = hp.template edgeMapCount<uintE>(FrontierH, FROM_H, apply_f);
	//moved.del();
	hyperedgeProp(GA,FrontierH,Update_Deg(Degrees,k),-1,no_output);
	FrontierH.del();
	toRemove.del();
      }
    }
    if(Frontier.numNonzeros() == 0) { largestCore = k-1; break; }
  }
  //cout << "largestCore was " << largestCore << endl;
  //cout << "### Max core: " << sequence::reduce(Degrees,GA.nv,maxF<uintE>()) << endl;
  Frontier.del(); free(Degrees); free(Flags);
}
