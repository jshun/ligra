// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
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
#include "parseCommandLine.h"
#include "graphIO.h"
#include "utils.h"
#include "parallel.h"
using namespace benchIO;
using namespace std;

struct edgeFirstCmp {
  bool operator() (edge<uintT> e1, edge<uintT> e2) {
    return e1.u < e2.u;
  }
};

struct edgeSecondCmp {
  bool operator() (edge<uintT> e1, edge<uintT> e2) {
    return e1.v < e2.v;
  }
};

template <class intT>
struct nonNeg {bool operator() (edge<intT> e) {return (e.u != UINT_T_MAX && e.v != UINT_T_MAX);}};

template <class intT>
hyperedgeArray<intT> hyperedgeRandom(long nv, long nh, long cardinality) {
  long numEdges = nh*cardinality;
  edge<intT> *VE = newA(edge<intT>,numEdges);
  edge<intT> *HE = newA(edge<intT>,numEdges);
  {parallel_for(long i=0;i<nh;i++) {
      for(long j=0;j<cardinality;j++) {
	ulong offset = i*cardinality+j;
	HE[offset] = edge<intT>(i,hashInt(offset)%nv);
	VE[offset] = edge<intT>(hashInt(offset)%nv,i);
      }
      //need to remove duplicates
      quickSort(VE+i*cardinality,cardinality,edgeFirstCmp());
      quickSort(HE+i*cardinality,cardinality,edgeSecondCmp());
      intT curr = HE[i*cardinality].v;
      for(long j=1;j<cardinality;j++) {
	ulong offset = i*cardinality+j;
	if(HE[offset].v == curr) {HE[offset].v = UINT_T_MAX; VE[offset].u = UINT_T_MAX; }
	else curr = HE[offset].v;
      }
    }}
  //filter out -1's
  edge<intT> *HE2 = newA(edge<intT>,numEdges);
  intT mh = sequence::filter(HE,HE2,numEdges,nonNeg<intT>());
  free(HE);
  edge<intT> *VE2 = newA(edge<intT>,numEdges);
  intT mv = sequence::filter(VE,VE2,numEdges,nonNeg<intT>());
  free(VE);
  //cout << mv << " " << mh << endl;
  return hyperedgeArray<intT>(VE2,HE2,nv,nh,mv,mh);
}

//Generates a symmetrized hypergraph with n vertices and m hyperedges,
//each hyperedge containing c random vertices. Duplicate vertices in a
//hyperedge are removed.
int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-nv <numvertices>] [-nh <numhyperedges>] [-c <cardinality>] <outFile>");
  char* fname = P.getArgument(0);
  
  long nv = P.getOptionLongValue("-nv",100);
  long nh = P.getOptionLongValue("-nh", 10*nv);  
  long cardinality = P.getOptionLongValue("-c", 3);

  hyperedgeArray<uintT> EA = hyperedgeRandom<uintT>(nv, nh, cardinality);
  hypergraph<uintT> G = hypergraphFromHyperedges<uintT>(EA);
  EA.del();
  writeHypergraphToFile<uintT>(G, fname);
  G.del();
}
