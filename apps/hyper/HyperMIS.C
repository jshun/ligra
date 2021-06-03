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
//
// This is an implementation of the MIS algorithm on hyper graphs from
// "Parallel Search for Maximal Independence Given Minimal
// Dependence", Proceedings of the ACM-SIAM Symposium on Discrete
// Algorithms (SODA), 1990 by Paul Beame and Michael Luby. We choose a
// different sampling probability for better performance.
#define HYPER 1
#include "hygra.h"

//#define CHECK 1

struct MIS_Count_Neighbors {
  uintT* flags;
  intT* Degrees;
  long round;
  MIS_Count_Neighbors(uintT* _flags, intT* _Degrees, long _round) : flags(_flags), Degrees(_Degrees), round(_round) {}
  inline bool update (uintE s, uintE d) {
    xadd(&Degrees[s],(intT)1);
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) {
    xadd(&Degrees[s],(intT)1);
    return 1;
  }
  inline bool cond (uintE i) {return flags[i] == round;}
};

struct MIS_Reset_Neighbors {
  uintT* flags;
  uintT round;
  MIS_Reset_Neighbors(uintT* _flags, uintT _round) : flags(_flags), round(_round) {}
  inline bool update (uintE s, uintE d) {
    flags[d] = 0;
    return 1;
  }
  inline bool updateAtomic (uintE s, uintE d) {
    CAS(&flags[d],round,(uintT)0);
    return 1;
  }
  inline bool cond (uintE i) {return flags[i] == round;}
};

struct Random_Sample {
  uintT* flags;
  long offset, inverseProb, round;
  Random_Sample(uintT* _flags, long _round, long _offset, long _inverseProb) :
    flags(_flags), round(_round), offset(_offset), inverseProb(_inverseProb) {}
  inline void operator () (uintE i) {
    if(hashInt((ulong)(i+offset)) % inverseProb == 0) flags[i] = round;    
  }
};

struct MIS_Filter {
  uintT* flags;
  MIS_Filter(uintT* _flags) : flags(_flags) {}
  inline bool operator () (uintE i) {
    return flags[i] == 0;
  }
};

template <class vertex>
struct Check_Independence {
  vertex* H;
  intT* Degrees;
  Check_Independence(vertex* _H, intT* _Degrees) : H(_H), Degrees(_Degrees) {}
  inline bool operator () (uintE i) {
    return Degrees[i] == H[i].getOutDegree();
  }
};

template <class vertex>
struct Filter_Hyperedges {
  vertex* H;
  uintT* flags;
  Filter_Hyperedges(vertex* _H, uintT* _flags) : H(_H), flags(_flags) {}
  inline bool operator () (uintE i) {
    if(H[i].getOutDegree() == 0) return 0;
    else if(H[i].getOutDegree() == 1) {
      if(flags[H[i].getOutNeighbor(0)] == 0) flags[H[i].getOutNeighbor(0)] = 1;
      return 0;
    } else return 1;
  }
};

struct Degrees_Reset {
  intT* Degrees;
  Degrees_Reset(intT* _Degrees) : Degrees(_Degrees) {}
  inline void operator () (uintE i) { Degrees[i] = 0; }
};

//Takes a symmetric graph as input; make sure to pass "-s" flag.
//Mutates graph, so only run once (pass "-rounds 0").
template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  timer t;
  t.start();
  const intE nv = GA.nv, nh = GA.nh;
  uintT* flags = newA(uintT,nv); //undecided = 0, out = 1, in = anything else
  bool* frontier_data = newA(bool, nv);
  {parallel_for(long i=0;i<nv;i++) {
    flags[i] = 0;
    frontier_data[i] = 1;
  }}
  bool* frontierH = newA(bool, nh);
  intT* Degrees = newA(intT,nh);
  {parallel_for(long i=0;i<nh;i++) {frontierH[i] = 1; }}
  long round = 1;
  long inverseProb = 3; //to be set
  vertexSubset FrontierV(nv, frontier_data);
  hyperedgeSubset FrontierH(nh, frontierH);
  long numVerticesProcessed = 0;
  while (!FrontierV.isEmpty()) {
    round++;
    vertexMap(FrontierV,Random_Sample(flags,round,numVerticesProcessed,inverseProb));

    numVerticesProcessed += FrontierV.numNonzeros();
    hyperedgeMap(FrontierH,Degrees_Reset(Degrees));
    hyperedgeProp(GA, FrontierH, MIS_Count_Neighbors(flags,Degrees,round), -1, no_output);

    hyperedgeSubset fullEdges = hyperedgeFilter(FrontierH, Check_Independence<vertex>(GA.H,Degrees));
    hyperedgeProp(GA, fullEdges, MIS_Reset_Neighbors(flags,round), -1, no_output);
    fullEdges.del();

    //pack edges
    //this predicate was previously incorrect and has been updated. timing results may now differ from the reported times in the paper
    auto pack_predicate = [&] (const uintE& u, const uintE& ngh) { return flags[ngh] < 2; }; 
    hyperedgeFilterNgh(GA.H, FrontierH, pack_predicate, no_output);
    hyperedgeSubset remainingHyperedges = hyperedgeFilter(FrontierH, Filter_Hyperedges<vertex>(GA.H,flags));
    
    FrontierH.del();
    FrontierH = remainingHyperedges;
    vertexSubset output = vertexFilter(FrontierV, MIS_Filter(flags));
    FrontierV.del();
    FrontierV = output;
  }

// #ifdef CHECK
//   for(long i=0;i<nh;i++) {
//     vertex h = GA.H[i];
//     long inSet = 0;
//     for(long j=0;j<h.getInDegree();j++) if(flags[h.getInNeighbor(j)] > 1) inSet++;
//     if(h.getInDegree() > 0 && inSet == h.getInDegree()) {cout << "hyperedge " << i << " is violated" << endl; cout << inSet << " " << h.getInDegree() << " " << h.getOutDegree() << endl; exit(0);}
//   }

//   //does not currently work due to hypergraph mutation
//   for(long i=0;i<nv;i++) {
//     if(flags[i] == 1) //not in MIS, check if adding it would violate any hyperedge
//       {
// 	vertex v = GA.V[i];    
// 	bool wouldViolate = 0;
// 	for(long j=0;j<v.getOutDegree();j++) {
// 	  if(wouldViolate) break;
// 	  vertex h = GA.H[v.getOutNeighbor(j)];
// 	  long inSet = 0;
// 	  for(long k=0;k<h.getOutDegree();k++) {
// 	    if(flags[h.getOutNeighbor(k)] > 1) inSet++;
// 	  }
// 	  if(inSet == h.getOutDegree()-1) wouldViolate = 1;
	    
// 	}
// 	if(!wouldViolate) { cout << "vertex " << i << " is not in MIS but could be added" << endl; exit(0);}
//       }
//   }  
//   cout << "check complete\n";
// #endif

  // //Print vertices in MIS
  // cout << "MIS vertices: ";
  // for(long i=0;i<nv;i++) if(flags[i] > 1) cout << i << " ";
  // cout << endl;
  
  free(flags); free(Degrees);
  FrontierV.del(); FrontierH.del();
  t.reportTotal("MIS time");
}
