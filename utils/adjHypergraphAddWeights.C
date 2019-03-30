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

// Adds a random integer weight in [1...log(n)] to each hyperedge
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

  hypergraph<intT> G = readHypergraphFromFile<intT>(iFile);

  long nv = G.nv, mv = G.mv, nh = G.nh, mh = G.mh;

  intT* WeightsV = newA(intT,mv);
  intT* WeightsH = newA(intT,mh);
  intT* InWeights = newA(intT,nh); //each hyperedge has an incoming weight
  intT* OutWeights = newA(intT,nh); //each hyperedge has an outgoing weight
  intT maxEdgeLen = log2(max(nv,nh));
  intT* Choices = newA(intT,maxEdgeLen);
  
  parallel_for (intT i=0;i<maxEdgeLen;i++){
    Choices[i] = i+1;
  }

  parallel_for (long i=0;i<nh;i++) {
    InWeights[i] = Choices[hashInt((uintT)i) % maxEdgeLen];
    OutWeights[i] = Choices[hashInt((uintT)(i+nh)) % maxEdgeLen];
    //if(i%1000==0 && Weights[i] < 0) Weights[i]*=-1;
  }
  free(Choices);
  
  wghVertex<intT>* WV = newA(wghVertex<intT>,nv);
  wghVertex<intT>* WH = newA(wghVertex<intT>,nh);
  intT* Neighbors_startV = G.allocatedInplace+4+nv;
  intT* Neighbors_startH = G.allocatedInplace+4+nv+mv+nh;

  {parallel_for(long i=0;i<nv;i++){
    WV[i].Neighbors = G.V[i].Neighbors;
    WV[i].degree = G.V[i].degree;
    intT offset = G.V[i].Neighbors - Neighbors_startV;
    WV[i].nghWeights = WeightsV+offset;
    for(long j=0;j<WV[i].degree;j++) WV[i].nghWeights[j] = InWeights[WV[i].Neighbors[j]];
  }}

  {parallel_for(long i=0;i<nh;i++){
    WH[i].Neighbors = G.H[i].Neighbors;
    WH[i].degree = G.H[i].degree;
    intT offset = G.H[i].Neighbors - Neighbors_startH;
    WH[i].nghWeights = WeightsH+offset;
    for(long j=0;j<WH[i].degree;j++) WH[i].nghWeights[j] = OutWeights[i];
    }}

  free(InWeights); free(OutWeights);
  
  wghHypergraph<intT> WG(WV,WH,nv,mv,nh,mh,G.allocatedInplace);
  int r = writeWghHypergraphToFile<intT>(WG,oFile);

  WG.del();
  
  return r;
}
