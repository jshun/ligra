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
#define HYPER 1
#define WEIGHTED 1
#include "hygra.h"

template <class vertex>
void Compute(hypergraph<vertex>& GA, commandLine P) {
  long n = GA.nv;
  cout << "vertices\n";
  for(long i=0;i<n;i++) {
    vertex v = GA.V[i];
    cout << i << endl;
    for(long j=0;j<v.getOutDegree();j++)
      cout << "("<< v.getOutNeighbor(j) << " " << v.getOutWeight(j) << ") ";
    cout << endl;
    for(long j=0;j<v.getInDegree();j++)
      cout << "(" << v.getInNeighbor(j) << " " << v.getInWeight(j) << ") ";
    cout << endl;

  }

  cout << "hyperedges\n";
  long nh = GA.nh;
  for(long i=0;i<nh;i++) {
    vertex v = GA.H[i];
    cout << i << endl;
    for(long j=0;j<v.getOutDegree();j++)
      cout << "("<< v.getOutNeighbor(j) << " " << v.getOutWeight(j) << ") ";
    cout << endl;
    for(long j=0;j<v.getInDegree();j++)
      cout << "(" << v.getInNeighbor(j) << " "  << v.getInWeight(j) << ") ";
    cout << endl;
  }
}
