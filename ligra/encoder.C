// This code is part of the project "Smaller and Faster: Parallel
// Processing of Compressed Graphs with Ligra+", presented at the IEEE
// Data Compression Conference, 2015.
// Copyright (c) 2015 Julian Shun, Laxman Dhulipala and Guy Blelloch
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

//include the code for the desired compression scheme
#ifndef PD 
#ifdef BYTE
#include "byte.h"
#elif defined NIBBLE
#include "nibble.h"
#else
#include "byteRLE.h"
#endif
#else //decode in parallel
#ifdef BYTE
#include "byte-pd.h"
#elif defined NIBBLE
#include "nibble-pd.h"
#else
#include "byteRLE-pd.h"
#endif
#endif

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include "parallel.h"
#include "quickSort.h"
#include "utils.h"
#include "graph.h"
#include "IO.h"
#include "parseCommandLine.h"
using namespace std;

typedef pair<pair<uintE,uintE> ,uintE > intTriple2;

struct tripleCmp {
  bool operator() (intTriple2 a, intTriple2 b) {
    intEPair aa = a.first, bb = b.first;
    if (aa.first != bb.first) return aa.first < bb.first;
    return aa.second < bb.second;
  }
};


void logCost(uintT* offsets, uintE* edges, long n, long m, uintE* Degrees){
  double* logs = newA(double,n);
  parallel_for(long i=0;i<n;i++) logs[i] = 0.0;
  parallel_for(long i=0;i<n;i++) {
    long o = offsets[i];
    for(long j=0;j<Degrees[i];j++) {
      logs[i] += log((double) abs(edges[o+j]-i) + 1);
    }
  }
  cout << "log cost = " << 
    sequence::plusReduce(logs,n)/(m*log(2.0)) << endl;
  free(logs);
}


void gapCost(uintT* offsets, uintE* edges, long n, long m, uintE* Degrees){
  double* logs = newA(double,n);
  parallel_for(long i=0;i<n;i++) logs[i] = 0.0;
  parallel_for(long i=0;i<n;i++) {
    long o = offsets[i];
    long d = Degrees[i];
    if(d > 0) {
      logs[i] += log((double) abs(i-edges[o]) + 1);
      for(long j=1;j<d;j++) {
	logs[i] += log((double) abs((long)edges[o+j]-(long)edges[o+j-1]) + 1);
      }
    }
  }
  cout << "log gap cost = " << 
    sequence::plusReduce(logs,n)/(m*log(2.0)) << endl;
  free(logs);
}

void encodeGraphFromFile(char* fname, bool isSymmetric, char* outFile, bool binary) {
  cout << "reading file..."<<endl;
  long n,m;
  uintE* edges; uintT* offsets;
  if(binary) {
    char* config = (char*) ".config";
    char* adj = (char*) ".adj";
    char* idx = (char*) ".idx";
    char configFile[strlen(fname)+strlen(config)+1];
    char adjFile[strlen(fname)+strlen(adj)+1];
    char idxFile[strlen(fname)+strlen(idx)+1];
    *configFile = *adjFile = *idxFile = '\0'; 
    strcat(configFile,fname);
    strcat(adjFile,fname);
    strcat(idxFile,fname);
    strcat(configFile,config);
    strcat(adjFile,adj);
    strcat(idxFile,idx);

    ifstream in(configFile, ifstream::in);
    //long n;
    in >> n;
    in.close();

    ifstream in2(adjFile,ifstream::in | ios::binary); //stored as uints
    in2.seekg(0, ios::end);
    long size = in2.tellg();
    in2.seekg(0);
    m = size/sizeof(uint);
    char* s = (char *) malloc(size);
    in2.read(s,size);
    in2.close();
    edges = (uintE*) s;

    ifstream in3(idxFile,ifstream::in | ios::binary); //stored as longs
    in3.seekg(0, ios::end);
    size = in3.tellg();
    in3.seekg(0);
    if(n != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

    char* t = (char *) malloc(size+sizeof(uintT));
    in3.read(t,size);
    in3.close();
    offsets = (uintT*) t;
    offsets[n] = m;
  } else {

    _seq<char> S = readStringFromFile(fname);
    words W = stringToWords(S.A, S.n);
    if (W.Strings[0] != (string) "AdjacencyGraph") {
      cout << "Bad input file" << endl;
      abort();
    }

    long len = W.m -1;
    n = atol(W.Strings[1]);
    m = atol(W.Strings[2]);
    if (len != n + m + 2) {
      cout << "Bad input file" << endl;
      abort();
    }
    offsets = newA(uintT,n+1);
    edges = newA(uintE,m);

    offsets[n] = m;
    {parallel_for(long i=0; i < n; i++) offsets[i] = atol(W.Strings[i + 3]);}
    {parallel_for(long i=0; i<m; i++) {
	edges[i] = atol(W.Strings[i+n+3]);
	if(atol(W.Strings[i+n+3]) < 0 || atol(W.Strings[i+n+3]) >= n) 
	  { cout << "Out of bounds: edge at index "<<
	      i<< " is "<<atol(W.Strings[i+n+3])<<endl;
	    abort();}
      }
    }
    W.del();
  }

  long* sizes = newA(long,3);
  sizes[0] = n; 

  uintE* Degrees = newA(uintE,n);
  uintT* DegreesT = newA(uintT,n+1);

  /* Steps : 
      1. Sort within each in-edge/out-edge segment 
      2. sequentially compress edges using difference coding  
  */
  {parallel_for (long i=0; i < n; i++) {
      uintT o = offsets[i];
      intT d = offsets[i+1] - o;
    if(d < 0 || d > n) { 
      cout << "degree out of bounds: vertex "<<i<< " has degree "<< d<<endl; 
      abort(); }
    Degrees[i] = DegreesT[i] = d;
    if(d > 0) {
      quickSort(edges+o, d, less<intE>());
      uintT k = 0;
      uintE lastRead = UINT_E_MAX;
      //remove self-edges and duplicate edges
      for(long j=0;j<d;j++) {
	uintE e = edges[o+j];
	if(e != i && e != lastRead) 
	  edges[o+k++] = lastRead = e;
      }
      Degrees[i] = DegreesT[i] = k;
    }
   }}

  DegreesT[n] = 0;
  //compute new m after duplicate edge removal
  m = sequence::plusScan(DegreesT,DegreesT,n+1);
  sizes[1] = m; 

  if (!isSymmetric) {
    uintT* tOffsets = newA(uintT,n+1);
    {parallel_for(long i=0;i<n;i++) tOffsets[i] = UINT_T_MAX;}
    uintE* inEdges = newA(uintE,m);
    intPair* temp = newA(intPair,m);
    // Create m many new intPairs.
    {parallel_for(long i=0;i<n;i++){
	uintT o = DegreesT[i];
	intT d = DegreesT[i+1] - o;
      for(long j=0;j<d;j++){
  	temp[o+j] = make_pair(edges[offsets[i]+j],i);
      }
    }}
    cout << "out edges: ";
    gapCost(offsets,edges,n,m,Degrees);

    cout << "out edges: ";
    logCost(offsets,edges,n,m,Degrees);
    cout << "compressing out edges..."<<endl;
    // Compress the out-edges.
    uintE *nEdges = parallelCompressEdges(edges, offsets, n, m, Degrees);
    long totalSpace = sizes[2] = offsets[n];
    free(edges);

    edges = nEdges;
    cout << "writing out edges..."<<endl;
    //write to binary file
    ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizes,sizeof(long)*3); //write n, m and isSymmetric
    out.write((char*)offsets,sizeof(uintT)*(n+1)); //write offsets
    out.write((char*)Degrees,sizeof(uintE)*n);
    out.write((char*)nEdges,totalSpace); //write edges

    free(offsets);

   /* From here on-out, we use temp, and tOffsets to guarantee
       stability as after compression offsets is no longer a reliable source
       of degree information. */

    // Use pairBothCmp because when encoding we need monotonicity
    // within an edge list.
    quickSort(temp,m,pairBothCmp<uintE>());
    
    tOffsets[temp[0].first] = 0; tOffsets[n] = m; inEdges[0] = temp[0].second;
    {parallel_for(long i=1;i<m;i++) {
      inEdges[i] = temp[i].second;
      if(temp[i].first != temp[i-1].first) {
      	tOffsets[temp[i].first] = i;
      }
    }}
    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsets,tOffsets,n,minF<uintT>(),(uintT) m);

    parallel_for(long i=0;i<n;i++) 
      Degrees[i] = tOffsets[i+1]-tOffsets[i];

    cout << "in edges: ";
    gapCost(tOffsets,inEdges,n,m,Degrees);
    cout << "in edges: ";
    logCost(tOffsets,inEdges,n,m,Degrees);

    cout << "compressing in edges..."<<endl;
    uintE *ninEdges = parallelCompressEdges(inEdges, tOffsets, n, m, Degrees);
    long tTotalSpace[0];
    tTotalSpace[0] = tOffsets[n];
    free(inEdges);
    inEdges = ninEdges;
    cout << "writing in edges..."<<endl;
    //write data for in-edges
    out.write((char*)tTotalSpace,sizeof(long)); //space for in-edges
    out.write((char*)tOffsets,sizeof(uintT)*(n+1)); //write offsets
    out.write((char*)Degrees,sizeof(uintE)*n); //write degrees
    out.write((char*)inEdges,tTotalSpace[0]); //write edges
    out.close();
    free(sizes);
    free(tOffsets);
    free(inEdges);
    free(Degrees);
    free(DegreesT);
  }
  else {
    gapCost(offsets,edges,n,m,Degrees);
    logCost(offsets,edges,n,m,Degrees);
    cout << "compressing..."<<endl;
    uintE *nEdges = parallelCompressEdges(edges, offsets, n, m, Degrees);
    long totalSpace = sizes[2] = offsets[n];
    free(edges);
    cout << "writing edges..."<<endl;
    ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizes,sizeof(long)*3); //write n, m and isSymmetric
    out.write((char*)offsets,sizeof(uintT)*(n+1)); //write offsets
    out.write((char*)Degrees,sizeof(uintE)*n); //write degrees
    out.write((char*)nEdges,totalSpace); //write edges
    out.close();
    free(sizes);
    free(offsets);
    free(nEdges);
    free(Degrees);
    free(DegreesT);
  }
}


void encodeWeightedGraphFromFile
(char* fname, bool isSymmetric, char* outFile, bool binary) {
  cout << "reading file..."<<endl;
  long n,m;
  intEPair* edges; uintT* offsets;
  if(binary) {
    char* config = (char*) ".config";
    char* adj = (char*) ".adj";
    char* idx = (char*) ".idx";
    char configFile[strlen(fname)+strlen(config)+1];
    char adjFile[strlen(fname)+strlen(adj)+1];
    char idxFile[strlen(fname)+strlen(idx)+1];
    *configFile = *adjFile = *idxFile = '\0'; 
    strcat(configFile,fname);
    strcat(adjFile,fname);
    strcat(idxFile,fname);
    strcat(configFile,config);
    strcat(adjFile,adj);
    strcat(idxFile,idx);

    ifstream in(configFile, ifstream::in);
    //long n;
    in >> n;
    in.close();

    ifstream in2(adjFile,ifstream::in | ios::binary); //stored as uints
    in2.seekg(0, ios::end);
    long size = in2.tellg();
    in2.seekg(0);
    m = size/sizeof(uint);
    char* s = (char *) malloc(size);
    in2.read(s,size);
    in2.close();
    uintE* edges1 = (uintE*) s;
    edges = newA(intEPair,m);
    parallel_for(long i=0;i<m;i++) {
      edges[i].first = edges1[i];
      edges[i].second = 1; //default weight
    }
    free(edges1);

    ifstream in3(idxFile,ifstream::in | ios::binary); //stored as longs
    in3.seekg(0, ios::end);
    size = in3.tellg();
    in3.seekg(0);
    if(n != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

    char* t = (char *) malloc(size+sizeof(uintT));
    in3.read(t,size);
    in3.close();
    offsets = (uintT*) t;
    offsets[n] = m;
  } else {

    _seq<char> S = readStringFromFile(fname);
    words W = stringToWords(S.A, S.n);
    if (W.Strings[0] != (string) "WeightedAdjacencyGraph") {
      cout << "Bad input file" << endl;
      abort();
    }

    long len = W.m -1;
    n = atol(W.Strings[1]);
    m = atol(W.Strings[2]);
    if (len != n + 2*m + 2) {
      cout << "Bad input file" << endl;
      abort();
    }
    offsets = newA(uintT,n+1);
    edges = newA(intEPair,m);

    offsets[n] = m;
    {parallel_for(long i=0; i < n; i++) offsets[i] = atol(W.Strings[i + 3]);}
    {parallel_for(long i=0; i<m; i++) {
	edges[i].first = atol(W.Strings[i+n+3]);
	if(atol(W.Strings[i+n+3]) < 0 || atol(W.Strings[i+n+3]) >= n) 
	  { cout << "Out of bounds: edge at index "<<i
		 << " is "<<atol(W.Strings[i+n+3])<<endl; 
	    abort();}
	edges[i].second = atol(W.Strings[i+n+m+3]);
      }
    }

    W.del(); // to deal with performance bug in malloc
  }
  
  long* sizes = newA(long,3);
  sizes[0] = n; 

  uintE* Degrees = newA(uintE,n);
  uintT* DegreesT = newA(uintT,n+1);

  /* Steps : 
      1. Sort within each in-edge/out-edge segment 
      2. sequentially compress edges using difference coding  
  */

  {parallel_for (long i=0; i < n; i++) {
      uintT o = offsets[i];
      intT d = offsets[i+1] - o;
      if(d < 0 || d > n) { 
	cout << "degree out of bounds: vertex "<<
	  i<< " has degree "<< d<<endl; 
	abort(); }
      Degrees[i] = DegreesT[i] = d;
      if(d > 0) {
	quickSort(edges+o, d, pairFirstCmp<uintE>());
	uintT k = 0;
	uintE lastRead = UINT_E_MAX;
	//remove self-edges and duplicate edges
	for(long j=0;j<d;j++) {
	  intEPair e = edges[o+j];
	  if(e.first != i && e.first != lastRead) {
	    lastRead = e.first;
	    edges[o+k++] = e;
	  }}
	Degrees[i] = DegreesT[i] = k;
      }
    }}

  DegreesT[n] = 0;
  cout << "m = "<<m << endl;
  //compute new m after duplicate edge removal
  m = sequence::plusScan(DegreesT,DegreesT,n+1);
  sizes[1] = m; 

  cout << "new m = "<<m<<endl;

  if (!isSymmetric) {
    uintT* tOffsets = newA(uintT,n+1);
    {parallel_for(long i=0;i<n;i++) tOffsets[i] = UINT_T_MAX;}
    intEPair* inEdges = newA(intEPair,m);
    intTriple2* temp = newA(intTriple2,m);

    // Create m many new intPairs.
    {parallel_for(long i=0;i<n;i++){
	uintT o = DegreesT[i];
	intT d = DegreesT[i+1]-o;
      for(long j=0;j<d;j++){
  	temp[o+j] = make_pair(make_pair(edges[offsets[i]+j].first,i),edges[offsets[i]+j].second);
      }
    }}

    cout << "compressing out edges..."<<endl;
    // Compress the out-edges.
    uchar *nEdges = parallelCompressWeightedEdges(edges, offsets, n, m,Degrees);
    long totalSpace = sizes[2] = offsets[n];
    free(edges);
    cout<<"writing out edges..."<<endl;
    //write to binary file
    ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizes,sizeof(long)*3); //write n, m and isSymmetric
    out.write((char*)offsets,sizeof(uintT)*(n+1)); //write offsets
    out.write((char*)Degrees,sizeof(uintE)*n);
    out.write((char*)nEdges,totalSpace); //write edges

    free(offsets);

   /* From here on-out, we use temp, and tOffsets to guarantee
       stability as after compression offsets is no longer a reliable source
       of degree information. */

    // Use tripleCmp because when encoding we need monotonicity within
    // an edge list.

    quickSort(temp,m,tripleCmp());
 
    tOffsets[temp[0].first.first] = 0; tOffsets[n] = m; 
    inEdges[0] = make_pair(temp[0].first.second,temp[0].second);
    {parallel_for(long i=1;i<m;i++) {
	inEdges[i] = make_pair(temp[i].first.second,temp[i].second);
      if(temp[i].first.first != temp[i-1].first.first) {
      	tOffsets[temp[i].first.first] = i;
      }
    }}
    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsets,tOffsets,n,minF<uintT>(),(uintT) m);

    parallel_for(long i=0;i<n;i++) 
      Degrees[i] = tOffsets[i+1]-tOffsets[i];
    cout << "compressing in edges..."<<endl;
    uchar *ninEdges = parallelCompressWeightedEdges(inEdges, tOffsets, n, m,Degrees);
    long tTotalSpace[0];
    tTotalSpace[0] = tOffsets[n];
    free(inEdges);
    cout << "writing in edges..."<<endl;
    //write data for in-edges
    out.write((char*)tTotalSpace,sizeof(long)); //space for in-edges
    out.write((char*)tOffsets,sizeof(uintT)*(n+1)); //write offsets
    out.write((char*)Degrees,sizeof(uintE)*n); //write degrees
    out.write((char*)ninEdges,tTotalSpace[0]); //write edges
    
    out.close();
    free(sizes);
    free(tOffsets);
    free(ninEdges);
    free(Degrees);
    free(DegreesT);
  }
  else {
    uchar *nEdges = parallelCompressWeightedEdges(edges, offsets, n, m,Degrees);
    long totalSpace = sizes[2] = offsets[n];
    free(edges);

    ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizes,sizeof(long)*3); //write n, m and isSymmetric
    out.write((char*)offsets,sizeof(uintT)*(n+1)); //write offsets
    out.write((char*)Degrees,sizeof(uintE)*n); //write degrees
    out.write((char*)nEdges,totalSpace); //write edges
    out.close();

    free(sizes);
    free(offsets);
    free(nEdges);
    free(Degrees);
    free(DegreesT);
  }
}

int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv," [-b] [-s] [-w] <inFile> <outFile>");
  char* iFile = P.getArgument(1);
  char* outFile = P.getArgument(0);
  bool binary = P.getOptionValue("-b");
  bool symmetric = P.getOptionValue("-s");
  bool weighted = P.getOptionValue("-w");
  if(!weighted) encodeGraphFromFile(iFile,symmetric,outFile,binary);
  else encodeWeightedGraphFromFile(iFile,symmetric,outFile,binary);
}
