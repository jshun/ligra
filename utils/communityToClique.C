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

#include "parseCommandLine.h"
#include "graphIO.h"
#include "parallel.h"

template <class intT>
graph<intT> cliquesFromHyperedges(hyperedgeArray<intT> EA) {
  long nv = EA.nv, nh = EA.nh, mv = EA.mv, mh = EA.mh;
  edge<intT>* E = newA(edge<intT>,mh);
  intT* offsetsH = newA(intT,nh);
  parallel_for(long i=0;i<mh;i++) E[i] = EA.HE[i];
  intSort::iSort(E,offsetsH,mh,nh,getuF<intT>()); 

  //degrees
  long* Degrees = newA(long,nh);
  parallel_for(long i=0;i<nh;i++) {
    intT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
    Degrees[i] = l*(l-1);
  }
  long numEdges = sequence::plusScan(Degrees,Degrees,nh);
  cout << nv << " " << nh << " " << mh << " " << numEdges << endl; 
  edge<intT> *edges = newA(edge<intT>,numEdges);

  parallel_for(long i=0;i<nh;i++) {
    intT o = offsetsH[i];
    intT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
    long edge_o = Degrees[i];
    for(intT j=0; j<l; j++) { //make clique
      for(intT k=j+1; k<l; k++) {
	edges[edge_o++] = edge<intT>(E[o+j].v,E[o+k].v);
	edges[edge_o++] = edge<intT>(E[o+k].v,E[o+j].v);
      }
    }
  }
  free(E);
  free(Degrees);
  free(offsetsH);

  intT* offsetsV = newA(intT,nv);
  intSort::iSort(edges,offsetsV,numEdges,nv,getuF<intT>());

  vertex<intT> *v = newA(vertex<intT>,nv);
  intT *edgesV = newA(intT,numEdges);
  parallel_for (intT i=0; i < nv; i++) {
    intT o = offsetsV[i];
    intT l = ((i == nv-1) ? numEdges : offsetsV[i+1])-offsetsV[i];
    v[i].degree = l;
    v[i].Neighbors = edgesV+o;
    for (intT j=0; j < l; j++) {
      v[i].Neighbors[j] = edges[o+j].v;
    }
  }
  free(offsetsV);
  free(edges);
  return graph<intT>(v,nv,numEdges,edgesV);
}

template <class intT>
wghGraph<intT> wghCliquesFromHyperedges(hyperedgeArray<intT> EA) {
  long nv = EA.nv, nh = EA.nh, mv = EA.mv, mh = EA.mh;
  edge<intT>* E = newA(edge<intT>,mh);
  intT* offsetsH = newA(intT,nh);
  parallel_for(long i=0;i<mh;i++) E[i] = EA.HE[i];
  intSort::iSort(E,offsetsH,mh,nh,getuF<intT>()); 

  //degrees
  long* Degrees = newA(long,nh);
  parallel_for(long i=0;i<nh;i++) {
    intT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
    Degrees[i] = l*(l-1);
  }
  long numEdges = sequence::plusScan(Degrees,Degrees,nh);
  cout << nv << " " << nh << " " << mv << " " << mh << " " << numEdges << endl; 
  wghEdge<intT> *edges = newA(wghEdge<intT>,numEdges);

  intT maxEdgeLen = log2(max(nv,nh));
  intT* Choices = newA(intT,maxEdgeLen);
  
  parallel_for (intT i=0;i<maxEdgeLen;i++){
    Choices[i] = i+1;
  }
  intT* OutWeights = newA(intT,nh); //each hyperedge has an outgoing weight
  parallel_for (long i=0;i<nh;i++) {
    OutWeights[i] = Choices[hashInt((uintT)i) % maxEdgeLen];
    //if(i%1000==0 && Weights[i] < 0) Weights[i]*=-1;
  }
  free(Choices);

  parallel_for(long i=0;i<nh;i++) {
    intT o = offsetsH[i];
    intT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
    long edge_o = Degrees[i];
    for(intT j=0; j<l; j++) { //make clique
      for(intT k=j+1; k<l; k++) {
	edges[edge_o++] = wghEdge<intT>(E[o+j].v,E[o+k].v,OutWeights[i]);
	edges[edge_o++] = wghEdge<intT>(E[o+k].v,E[o+j].v,OutWeights[i]);
      }
    }
  }
  free(E);
  free(Degrees);
  free(offsetsH);
  free(OutWeights);

  intT* offsetsV = newA(intT,nv);
  intSort::iSort(edges,offsetsV,numEdges,nv,getuFWgh<intT>());

  wghVertex<intT> *v = newA(wghVertex<intT>,nv);
  intT *edgesV = newA(intT,2*numEdges);
  intT *weightsV = edgesV + numEdges;
  parallel_for (intT i=0; i < nv; i++) {
    intT o = offsetsV[i];
    intT l = ((i == nv-1) ? numEdges : offsetsV[i+1])-offsetsV[i];
    v[i].degree = l;
    v[i].Neighbors = edgesV+o;
    v[i].nghWeights = weightsV+o;
    for (intT j=0; j < l; j++) {
      v[i].Neighbors[j] = edges[o+j].v;
      v[i].nghWeights[j] = edges[o+j].w;
    }
  }
  free(offsetsV);
  free(edges);
  return wghGraph<intT>(v,nv,numEdges,edgesV,weightsV);
}

//pass -w flag for weighted input
int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-w] <input hyperedge file> <output Ligra file>");
  char* iFile = P.getArgument(1);
  char* oFile = P.getArgument(0);
  bool wgh = P.getOption("-w");
  if(!wgh) {
    hyperedgeArray<uintT> G = readHyperedges<uintT>(iFile);
    writeGraphToFile(cliquesFromHyperedges(G),oFile);
  } else {
    hyperedgeArray<intT> G = readHyperedges<intT>(iFile);
    writeWghGraphToFile<intT>(wghCliquesFromHyperedges(G),oFile);
  }
}
