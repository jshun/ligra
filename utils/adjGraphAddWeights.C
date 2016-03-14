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

// Adds a random integer weight in [1...log(n)] to each edge
#include <math.h>
#include "graphIO.h"
#include "parseCommandLine.h"
#include "parallel.h"
using namespace benchIO;
using namespace std;

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"<inFile> <outFile>");
  pair<char*,char*> fnames = P.IOFileNames();
  char* iFile = fnames.first;
  char* oFile = fnames.second;

  graph<intT> G = readGraphFromFile<intT>(iFile);

  long m = G.m;
  long n = G.n;
  
  intT* Weights = newA(intT,m);

  intT maxEdgeLen = log2(n);
  intT* Choices = newA(intT,2*maxEdgeLen);
  
  parallel_for (intT i=0;i<maxEdgeLen;i++){
    Choices[2*i] = i+1;
    Choices[2*i+1] = i+1;
    //Choices[2*i+1] = -(i/10)-1;
  }

  parallel_for (long i=0;i<m;i++) {
    Weights[i] = Choices[hashInt((uintT)i) % (2*maxEdgeLen)];
    //if(i%1000==0 && Weights[i] < 0) Weights[i]*=-1;
  }
  free(Choices);
  
  wghVertex<intT>* WV = newA(wghVertex<intT>,n);
  intT* Neighbors_start = G.allocatedInplace+2+n;

  parallel_for(long i=0;i<n;i++){
    WV[i].Neighbors = G.V[i].Neighbors;
    WV[i].degree = G.V[i].degree;
    intT offset = G.V[i].Neighbors - Neighbors_start;
    WV[i].nghWeights = Weights+offset;
  }

  //symmetrize
  parallel_for(long i=0;i<n;i++){
    parallel_for(long j=0;j<WV[i].degree;j++){
      uintT ngh = WV[i].Neighbors[j];
      if(ngh > i) {
	for(long k=0;k<WV[ngh].degree;k++) {
	  uintT ngh_ngh = WV[ngh].Neighbors[k];
	  if(ngh_ngh == i) {
	    WV[i].nghWeights[j] = WV[ngh].nghWeights[k];
	    break;
	  }
	}
      }
    }
  }

  wghGraph<intT> WG(WV,n,m,(intT*)G.allocatedInplace,Weights);
  int r = writeWghGraphToFile<intT>(WG,oFile);

  WG.del();
  
  return r;
}
