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
void xToCSVString(char* s, edge<intT> a) {
  int l = xToStringLen(a.u);
  xToString(s, a.u);
  s[l] = ',';
  xToString(s+l+1, a.v);
}

template <class intT>
_seq<char> arrayToCSVString(edge<intT>* A, long n) {
  long* L = newA(long,n);
  {parallel_for(long i=0; i < n; i++) L[i] = xToStringLen(A[i])+1;}
  long m = sequence::scan(L,L,n,addF<long>(),(long) 0);
  char* B = newA(char,m);
  parallel_for(long j=0; j < m; j++) 
    B[j] = 0;
  parallel_for(long i=0; i < n-1; i++) {
    xToCSVString(B + L[i],A[i]);
    B[L[i+1] - 1] = '\n';
  }
  xToCSVString(B + L[n-1],A[n-1]);
  B[m-1] = '\n';
  free(L);
  char* C = newA(char,m+1);
  long mm = sequence::filter(B,C,m,notZero());
  C[mm] = 0;
  free(B);
  return _seq<char>(C,mm);
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"<input hyperedge file> <output MESH file>");
  char* iFile = P.getArgument(1);
  char* oFile = P.getArgument(0);
  hyperedgeArray<uintT> G = readHyperedges<uintT>(iFile);
  ofstream file (oFile, ios::out | ios::binary);
  long BSIZE = 1000000;
  long offset = 0;
  while (offset < G.mh) {
    // Generates a string for a sequence of size at most BSIZE
    // and then wrties it to the output stream
    _seq<char> S = arrayToCSVString(G.HE+offset,min(BSIZE,G.mh-offset));
    file.write(S.A, S.n);
    S.del();
    offset += BSIZE;
  }

  //writeArrayToStream< edge<uintT> >(file,G.VE,G.mv);
  file.close();
}
