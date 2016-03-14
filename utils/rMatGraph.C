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

double hashDouble(intT i) {
  return ((double) (hashInt((uintT)i))/((double) UINT_T_MAX));}

template <class intT>
struct rMat {
  double a, ab, abc;
  intT n; 
  uintT h;
  rMat(intT _n, intT _seed, 
       double _a, double _b, double _c) {
    n = _n; a = _a; ab = _a + _b; abc = _a+_b+_c;
    h = hashInt((uintT)_seed);
    if(abc > 1) { cout << "in rMat: a + b + c add to more than 1\n"; abort();}
    if((1 << log2Up(n)) != n) { cout << "in rMat: n not a power of 2"; abort(); } 
  }

  edge<intT> rMatRec(intT nn, intT randStart, intT randStride) {
    if (nn==1) return edge<intT>(0,0);
    else {
      edge<intT> x = rMatRec(nn/2, randStart + randStride, randStride);
      double r = hashDouble(randStart); 
      if (r < a) return x;
      else if (r < ab) return edge<intT>(x.u,x.v+nn/2);
      else if (r < abc) return edge<intT>(x.u+nn/2, x.v);
      else return edge<intT>(x.u+nn/2, x.v+nn/2);
    }
  }

  edge<intT> operator() (intT i) {
    uintT randStart = hashInt((uintT)(2*i)*h);
    uintT randStride = hashInt((uintT)(2*i+1)*h);
    return rMatRec(n, randStart, randStride);
  }
};

template <class intT>
edgeArray<intT> edgeRmat(intT n, intT m, intT seed, 
		   float a, float b, float c) {
  intT nn = (1 << log2Up(n));
  rMat<intT> g(nn,seed,a,b,c);
  edge<intT>* E = newA(edge<intT>,m);
  parallel_for (intT i = 0; i < m; i++) 
    E[i] = g(i);
  return edgeArray<intT>(E,nn,nn,m);
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-s] [-m <numedges>] [-r <intseed>] [-a <a>] [-b <b>] [-c <c>] n <outFile>");
  pair<intT,char*> in = P.sizeAndFileName();
  uintT n = in.first;
  char* fname = in.second;
  double a = P.getOptionDoubleValue("-a",.5);
  double b = P.getOptionDoubleValue("-b",.1);
  double c = P.getOptionDoubleValue("-c", b);
  uintT m = P.getOptionLongValue("-m", 10*n);
  intT seed = P.getOptionLongValue("-r", 1);
  bool sym = P.getOptionValue("-s");
  edgeArray<uintT> EA = edgeRmat<uintT>(n, m, seed, a, b, c);
  graph<uintT> G = graphFromEdges<uintT>(EA, sym);
  EA.del();
  writeGraphToFile<uintT>(G, fname);
  G.del();
}
