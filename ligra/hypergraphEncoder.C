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

void encodeHypergraphFromFile(char* iFile, bool isSymmetric, char* outFile, bool binary) {
  cout << "reading file..."<<endl;
  long nv,nh,mv,mh;
  uintE* edgesV, *edgesH; uintT* offsetsV, *offsetsH, *offsetsVSaved;
  if(binary) {
    char* config = (char*) ".config";
    char* vadj = (char*) ".vadj";
    char* vidx = (char*) ".vidx";
    char* hadj = (char*) ".hadj";
    char* hidx = (char*) ".hidx";
    char configFile[strlen(iFile)+strlen(config)+1];
    char vadjFile[strlen(iFile)+strlen(vadj)+1];
    char vidxFile[strlen(iFile)+strlen(vidx)+1];
    char hadjFile[strlen(iFile)+strlen(hadj)+1];
    char hidxFile[strlen(iFile)+strlen(hidx)+1];
    *configFile = *vadjFile = *vidxFile = *hadjFile = *hidxFile = '\0';
    strcat(configFile,iFile);
    strcat(vadjFile,iFile);
    strcat(vidxFile,iFile);
    strcat(hadjFile,iFile);
    strcat(hidxFile,iFile);
    strcat(configFile,config);
    strcat(vadjFile,vadj);
    strcat(vidxFile,vidx);
    strcat(hadjFile,hadj);
    strcat(hidxFile,hidx);

    ifstream in(configFile, ifstream::in);
    in >> nv; in >> mv; in >> nh; in >> mh;
    in.close();

    ifstream in2(vadjFile,ifstream::in | ios::binary); //stored as uints
    in2.seekg(0, ios::end);
    long size = in2.tellg();
    in2.seekg(0);
    if(mv != size/(sizeof(uint))) { cout << "size wrong\n"; exit(0); }

    char* s = (char *) malloc(size);
    in2.read(s,size);
    in2.close();
    edgesV = (uintE*) s;

    ifstream in3(vidxFile,ifstream::in | ios::binary); //stored as longs
    in3.seekg(0, ios::end);
    size = in3.tellg();
    in3.seekg(0);
    if(nv != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

    char* t = (char *) malloc(size+sizeof(intT));
    in3.read(t,size);
    in3.close();
    offsetsV = (uintT*) t;
    offsetsV[nv] = mv;

    offsetsVSaved = newA(uintT,nv+1);
    parallel_for(long i=0;i<nv+1;i++) offsetsVSaved[i] = offsetsV[i];

    ifstream in4(hadjFile,ifstream::in | ios::binary); //stored as uints
    in4.seekg(0, ios::end);
    size = in4.tellg();
    in4.seekg(0);
    if(mh != size/(sizeof(uint))) { cout << "size wrong\n"; exit(0); }

    char* s2 = (char *) malloc(size);
    in4.read(s2,size);
    in4.close();
    edgesH = (uintE*) s2;

    ifstream in5(hidxFile,ifstream::in | ios::binary); //stored as longs
    in5.seekg(0, ios::end);
    size = in5.tellg();
    in5.seekg(0);
    if(nh != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

    char* t2 = (char *) malloc(size+sizeof(intT));
    in5.read(t2,size);
    in5.close();
    offsetsH = (uintT*) t2;
    offsetsH[nh] = mh;

  } else {
    _seq<char> S = readStringFromFile(iFile);
    words W = stringToWords(S.A, S.n);
    if (W.Strings[0] != (string) "AdjacencyHypergraph") {
      cout << "Bad input file" << endl;
      abort();
    }
    long len = W.m -1;
    nv = atol(W.Strings[1]);
    mv = atol(W.Strings[2]);
    nh = atol(W.Strings[3]);
    mh = atol(W.Strings[4]);
    if (len != nv + mv + nh + mh + 4) {
      cout << "Bad input file" << endl;
      abort();
    }
    offsetsV = newA(uintT,nv+1);
    offsetsH = newA(uintT,nh+1);
    edgesV = newA(uintE,mv);
    edgesH = newA(uintE,mh);
    offsetsV[nv]=mv;
    offsetsH[nh]=mh;
    offsetsVSaved = newA(uintT,nv+1);
    offsetsVSaved[nv] = mv;
    {parallel_for(long i=0; i < nv; i++) offsetsV[i] = offsetsVSaved[i] = atol(W.Strings[i + 5]);}
    {parallel_for(long i=0; i<mv; i++) {
	edgesV[i] = atol(W.Strings[i+nv+5]);
      }}
    {parallel_for(long i=0; i < nh; i++) offsetsH[i] = atol(W.Strings[i + nv + mv + 5]);}
    {parallel_for(long i=0; i<mh; i++) {
	edgesH[i] = atol(W.Strings[i+nv+mv+nh+5]);
      }}

    W.del();
  }

  long* sizesV = newA(long,3);
  long* sizesH = newA(long,3);
  sizesV[0] = nv; sizesH[0] = nh;

  uintE* DegreesV = newA(uintE,nv);
  uintT* DegreesVT = newA(uintT,nv+1);

  /* Steps : 
     1. Sort within each in-edge/out-edge segment 
     2. sequentially compress edges using difference coding  
  */
  {parallel_for (long i=0; i < nv; i++) {
      uintT o = offsetsV[i];
      intT d = offsetsV[i+1] - o;
      if(d < 0 || d > nh) { 
	cout << "degree out of bounds: vertex "<<i<< " has degree "<< d<<endl; 
	abort(); }
      DegreesV[i] = DegreesVT[i] = d;
      if(d > 0) {
	quickSort(edgesV+o, d, less<intE>());
	uintT k = 0;
	uintE lastRead = UINT_E_MAX;
	//remove duplicate edges
	for(long j=0;j<d;j++) {
	  uintE e = edgesV[o+j];
	if(e != lastRead) 
	  edgesV[o+k++] = lastRead = e;
      }
      DegreesV[i] = DegreesVT[i] = k;
    }
   }}

  DegreesVT[nv] = 0;
  //compute new m after duplicate edge removal
  mv = sequence::plusScan(DegreesVT,DegreesVT,nv+1);
  sizesV[1] = mv; 

  uintE* DegreesH = newA(uintE,nh);
  uintT* DegreesHT = newA(uintT,nh+1);

  {parallel_for (long i=0; i < nh; i++) {
      uintT o = offsetsH[i];
      intT d = offsetsH[i+1] - o;
      if(d < 0 || d > nv) { 
	cout << "degree out of bounds: vertex "<<i<< " has degree "<< d<<endl; 
	abort(); }
      DegreesH[i] = DegreesHT[i] = d;
      if(d > 0) {
	quickSort(edgesH+o, d, less<intE>());
	uintT k = 0;
	uintE lastRead = UINT_E_MAX;
	//remove self-edges and duplicate edges
	for(long j=0;j<d;j++) {
	  uintE e = edgesH[o+j];
	if(e != lastRead) 
	  edgesH[o+k++] = lastRead = e;
      }
      DegreesH[i] = DegreesHT[i] = k;
    }
   }}

  DegreesHT[nh] = 0;
  //compute new m after duplicate edge removal
  mh = sequence::plusScan(DegreesHT,DegreesHT,nh+1);
  sizesH[1] = mh; 

  if (!isSymmetric) {
    //edges of V
    uintT* tOffsetsV = newA(uintT,nv+1);
    {parallel_for(long i=0;i<nv;i++) tOffsetsV[i] = UINT_T_MAX;}
    uintE* inEdgesV = newA(uintE,mh);
    intPair* temp = newA(intPair,mh);
    // Create mv many new intPairs.
    {parallel_for(long i=0;i<nh;i++){
	uintT o = DegreesHT[i];
	intT d = DegreesHT[i+1] - o;
      for(long j=0;j<d;j++){
  	temp[o+j] = make_pair(edgesH[offsetsH[i]+j],i);
      }
    }}
    cout << "out edgesV: ";
    gapCost(offsetsV,edgesV,nv,mv,DegreesV);

    cout << "out edgesV: ";
    logCost(offsetsV,edgesV,nv,mv,DegreesV);


    // Compress the out-edges.
    uintE *nEdgesV = parallelCompressEdges(edgesV, offsetsV, nv, mv, DegreesV);
    long totalSpaceV = sizesV[2] = offsetsV[nv];

    cout << "writing out edgesV..."<<endl;
    //write to binary file

    ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizesV,sizeof(long)*3); //write nv, mv and space for edges
    out.write((char*)offsetsV,sizeof(uintT)*(nv+1)); //write offsets
    out.write((char*)DegreesV,sizeof(uintE)*nv);
    out.write((char*)nEdgesV,totalSpaceV); //write edges

    free(offsetsV);
    free(sizesV);


   /* From here on-out, we use temp, and tOffsets to guarantee
       stability as after compression offsets is no longer a reliable source
       of degree information. */

    // Use pairBothCmp because when encoding we need monotonicity
    // within an edge list.
    quickSort(temp,mh,pairBothCmp<uintE>());
    
    tOffsetsV[temp[0].first] = 0; tOffsetsV[nv] = mh; inEdgesV[0] = temp[0].second;
    {parallel_for(long i=1;i<mh;i++) {
      inEdgesV[i] = temp[i].second;
      if(temp[i].first != temp[i-1].first) {
      	tOffsetsV[temp[i].first] = i;
      }
    }}
    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsetsV,tOffsetsV,nv,minF<uintT>(),(uintT) mh);

    parallel_for(long i=0;i<nv;i++) 
      DegreesV[i] = tOffsetsV[i+1]-tOffsetsV[i];

    cout << "in edgesV: ";
    gapCost(tOffsetsV,inEdgesV,nv,mh,DegreesV);
    cout << "in edgesV: ";
    logCost(tOffsetsV,inEdgesV,nv,mh,DegreesV);

    cout << "compressing in edgesV..."<<endl;
    uintE *ninEdgesV = parallelCompressEdges(inEdgesV, tOffsetsV, nv, mh, DegreesV);
    long tTotalSpaceV[0];
    tTotalSpaceV[0] = tOffsetsV[nv];
    free(inEdgesV);
    cout << "writing in edgesV..."<<endl;
    //write data for in-edges
    out.write((char*)tTotalSpaceV,sizeof(long)); //space for in-edges
    out.write((char*)tOffsetsV,sizeof(uintT)*(nv+1)); //write offsets
    out.write((char*)DegreesV,sizeof(uintE)*nv); //write degrees
    out.write((char*)ninEdgesV,tTotalSpaceV[0]); //write edges

    free(tOffsetsV);
    free(ninEdgesV);
    free(DegreesV);

    //edges of H
    uintT* tOffsetsH = newA(uintT,nh+1);
    {parallel_for(long i=0;i<nh;i++) tOffsetsH[i] = UINT_T_MAX;}
    uintE* inEdgesH = newA(uintE,mv);
    temp = newA(intPair,mv);
    // Create mh many new intPairs.
    {parallel_for(long i=0;i<nv;i++){
	uintT o = DegreesVT[i];
	intT d = DegreesVT[i+1] - o;
      for(long j=0;j<d;j++){
  	temp[o+j] = make_pair(edgesV[offsetsVSaved[i]+j],i);
      }
    }}

    free(DegreesVT); free(offsetsVSaved);
    free(edgesV); 
    cout << "out edgesH: ";
    gapCost(offsetsH,edgesH,nh,mh,DegreesH);

    cout << "out edgesH: ";
    logCost(offsetsH,edgesH,nh,mh,DegreesH);
    cout << "compressing out edges of H..."<<endl;
    // Compress the out-edges.
    uintE *nEdgesH = parallelCompressEdges(edgesH, offsetsH, nh, mh, DegreesH);
    long totalSpaceH = sizesH[2] = offsetsH[nh];
    free(edgesH);

    //edgesH = nEdgesH;
    cout << "writing out edgesH..."<<endl;
    //write to binary file
    //ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizesH,sizeof(long)*3); //write nh, mh and space for edges
    out.write((char*)offsetsH,sizeof(uintT)*(nh+1)); //write offsets
    out.write((char*)DegreesH,sizeof(uintE)*nh);
    out.write((char*)nEdgesH,totalSpaceH); //write edges

    free(offsetsH);
    free(sizesH);

   /* From here on-out, we use temp, and tOffsets to guarantee
       stability as after compression offsets is no longer a reliable source
       of degree information. */

    // Use pairBothCmp because when encoding we need monotonicity
    // within an edge list.
    quickSort(temp,mv,pairBothCmp<uintE>());

    tOffsetsH[temp[0].first] = 0; tOffsetsH[nh] = mv; inEdgesH[0] = temp[0].second;
    {parallel_for(long i=1;i<mv;i++) {
      inEdgesH[i] = temp[i].second;
      if(temp[i].first != temp[i-1].first) {
      	tOffsetsH[temp[i].first] = i;
      }
    }}
    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsetsH,tOffsetsH,nh,minF<uintT>(),(uintT) mv);

    parallel_for(long i=0;i<nh;i++) 
      DegreesH[i] = tOffsetsH[i+1]-tOffsetsH[i];

    cout << "in edgesH: ";
    gapCost(tOffsetsH,inEdgesH,nh,mh,DegreesH);
    cout << "in edgesH: ";
    logCost(tOffsetsH,inEdgesH,nh,mh,DegreesH);

    cout << "compressing in edgesH..."<<endl;
    uintE *ninEdgesH = parallelCompressEdges(inEdgesH, tOffsetsH, nh, mv, DegreesH);
    long tTotalSpaceH[0];
    tTotalSpaceH[0] = tOffsetsH[nh];
    free(inEdgesH);
    cout << "writing in edgesH..."<<endl;
    //write data for in-edges
    out.write((char*)tTotalSpaceH,sizeof(long)); //space for in-edges
    out.write((char*)tOffsetsH,sizeof(uintT)*(nh+1)); //write offsets
    out.write((char*)DegreesH,sizeof(uintE)*nh); //write degrees
    out.write((char*)ninEdgesH,tTotalSpaceH[0]); //write edges
    out.close();
    free(tOffsetsH);
    free(ninEdgesH);
    free(DegreesH);
    free(DegreesHT);
  }
  else {
    free(offsetsVSaved);
    gapCost(offsetsV,edgesV,nv,mv,DegreesV);
    logCost(offsetsV,edgesV,nv,mv,DegreesV);
    gapCost(offsetsH,edgesH,nh,mh,DegreesH);
    logCost(offsetsH,edgesH,nh,mh,DegreesH);

    cout << "compressing..."<<endl;
    uintE *nEdgesV = parallelCompressEdges(edgesV, offsetsV, nv, mv, DegreesV);
    uintE *nEdgesH = parallelCompressEdges(edgesH, offsetsH, nh, mh, DegreesH);
    long totalSpaceV = sizesV[2] = offsetsV[nv];
    long totalSpaceH = sizesH[2] = offsetsH[nh];
    free(edgesV); free(edgesH);
    cout << "writing edges..."<<endl;
    ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizesV,sizeof(long)*3); //write n, m and isSymmetric
    out.write((char*)offsetsV,sizeof(uintT)*(nv+1)); //write offsets
    out.write((char*)DegreesV,sizeof(uintE)*nv); //write degrees
    out.write((char*)nEdgesV,totalSpaceV); //write edges
    out.write((char*)sizesH,sizeof(long)*3); //write n, m and isSymmetric
    out.write((char*)offsetsH,sizeof(uintT)*(nh+1)); //write offsets
    out.write((char*)DegreesH,sizeof(uintE)*nh); //write degrees
    out.write((char*)nEdgesH,totalSpaceH); //write edges
    out.close();
    free(sizesV); free(sizesH);
    free(offsetsV); free(offsetsH);
    free(nEdgesV); free(nEdgesH);
    free(DegreesV); free(DegreesH);
    free(DegreesVT); free(DegreesHT);
  }
}

void encodeWeightedHypergraphFromFile(char* iFile, bool isSymmetric, char* outFile, bool binary) {
  cout << "reading file..."<<endl;
  long nv,nh,mv,mh;
  intEPair* edgesV, *edgesH; uintT* offsetsV, *offsetsH, *offsetsVSaved;
  if(binary) {
    char* config = (char*) ".config";
    char* vadj = (char*) ".vadj";
    char* vidx = (char*) ".vidx";
    char* hadj = (char*) ".hadj";
    char* hidx = (char*) ".hidx";
    char configFile[strlen(iFile)+strlen(config)+1];
    char vadjFile[strlen(iFile)+strlen(vadj)+1];
    char vidxFile[strlen(iFile)+strlen(vidx)+1];
    char hadjFile[strlen(iFile)+strlen(hadj)+1];
    char hidxFile[strlen(iFile)+strlen(hidx)+1];
    *configFile = *vadjFile = *vidxFile = *hadjFile = *hidxFile = '\0';
    strcat(configFile,iFile);
    strcat(vadjFile,iFile);
    strcat(vidxFile,iFile);
    strcat(hadjFile,iFile);
    strcat(hidxFile,iFile);
    strcat(configFile,config);
    strcat(vadjFile,vadj);
    strcat(vidxFile,vidx);
    strcat(hadjFile,hadj);
    strcat(hidxFile,hidx);

    ifstream in(configFile, ifstream::in);
    in >> nv; in >> mv; in >> nh; in >> mh;
    in.close();

    ifstream in2(vadjFile,ifstream::in | ios::binary); //stored as uints
    in2.seekg(0, ios::end);
    long size = in2.tellg();
    in2.seekg(0);
    if(mv != size/(2*sizeof(uint))) { cout << "size wrong\n"; exit(0); }

    char* s = (char *) malloc(size);
    in2.read(s,size);
    in2.close();
    uintE* edges2 = (uintE*) s;
    edgesV = newA(intEPair,mv);
    parallel_for(long i=0;i<mv;i++) {
      edgesV[i].first = edges2[i];
      edgesV[i].second = 1; //default weight
    }
    free(edges2);

    //edgesV = (uintE*) s;

    ifstream in3(vidxFile,ifstream::in | ios::binary); //stored as longs
    in3.seekg(0, ios::end);
    size = in3.tellg();
    in3.seekg(0);
    if(nv != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

    char* t = (char *) malloc(size+sizeof(intT));
    in3.read(t,size);
    in3.close();
    offsetsV = (uintT*) t;
    offsetsV[nv] = mv;

    offsetsVSaved = newA(uintT,nv+1);
    parallel_for(long i=0;i<nv+1;i++) offsetsVSaved[i] = offsetsV[i];

    ifstream in4(hadjFile,ifstream::in | ios::binary); //stored as uints
    in4.seekg(0, ios::end);
    size = in4.tellg();
    in4.seekg(0);
    if(mh != size/(2*sizeof(uint))) { cout << "size wrong\n"; exit(0); }

    char* s2 = (char *) malloc(size);
    in4.read(s2,size);
    in4.close();
    edges2 = (uintE*) s2;
    parallel_for(long i=0;i<mv;i++) {
      edgesH[i].first = edges2[i];
      edgesH[i].second = 1; //default weight
    }
    free(edges2);

    //edgesH = (uintE*) s2;

    ifstream in5(hidxFile,ifstream::in | ios::binary); //stored as longs
    in5.seekg(0, ios::end);
    size = in5.tellg();
    in5.seekg(0);
    if(nh != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

    char* t2 = (char *) malloc(size+sizeof(intT));
    in5.read(t2,size);
    in5.close();
    offsetsH = (uintT*) t2;
    offsetsH[nh] = mh;
  } else {
    _seq<char> S = readStringFromFile(iFile);
    words W = stringToWords(S.A, S.n);
    if (W.Strings[0] != (string) "WeightedAdjacencyHypergraph") {
      cout << "Bad input file" << endl;
      abort();
    }
    long len = W.m -1;
    nv = atol(W.Strings[1]);
    mv = atol(W.Strings[2]);
    nh = atol(W.Strings[3]);
    mh = atol(W.Strings[4]);
    if (len != nv + 2*mv + nh + 2*mh + 4) {
      cout << "Bad input file" << endl;
      abort();
    }
    offsetsV = newA(uintT,nv+1);
    offsetsH = newA(uintT,nh+1);
    edgesV = newA(intEPair,mv);
    edgesH = newA(intEPair,mh);
    offsetsV[nv]=mv;
    offsetsH[nh]=mh;
    offsetsVSaved = newA(uintT,nv+1);
    offsetsVSaved[nv] = mv;
    {parallel_for(long i=0; i < nv; i++) offsetsV[i] = offsetsVSaved[i] = atol(W.Strings[i + 5]);}
    {parallel_for(long i=0; i<mv; i++) {
	edgesV[i] = make_pair(atol(W.Strings[i+nv+5]),atol(W.Strings[i+nv+mv+5]));
      }}
    {parallel_for(long i=0; i < nh; i++) offsetsH[i] = atol(W.Strings[i + nv + 2*mv + 5]);}
    {parallel_for(long i=0; i<mh; i++) {
	edgesH[i] = make_pair(atol(W.Strings[i+nv+2*mv+nh+5]),atol(W.Strings[i+nv+2*mv+nh+mh+5]));
      }}

    W.del();
  }

  long* sizesV = newA(long,3);
  long* sizesH = newA(long,3);
  sizesV[0] = nv; sizesH[0] = nh;

  uintE* DegreesV = newA(uintE,nv);
  uintT* DegreesVT = newA(uintT,nv+1);

  /* Steps : 
     1. Sort within each in-edge/out-edge segment 
     2. sequentially compress edges using difference coding  
  */
  {parallel_for (long i=0; i < nv; i++) {
      uintT o = offsetsV[i];
      intT d = offsetsV[i+1] - o;
      if(d < 0 || d > nh) { 
	cout << "degree out of bounds: vertex "<<i<< " has degree "<< d<<endl; 
	abort(); }
      DegreesV[i] = DegreesVT[i] = d;
      if(d > 0) {
	quickSort(edgesV+o, d, pairFirstCmp<intE>());
	uintT k = 0;
	uintE lastRead = UINT_E_MAX;
	//remove duplicate edges
	for(long j=0;j<d;j++) {
	  intEPair e = edgesV[o+j];
	  if(e.first != lastRead){
	    lastRead = e.first;
	    edgesV[o+k++] = e;
	  }
      }
      DegreesV[i] = DegreesVT[i] = k;
    }
   }}

  DegreesVT[nv] = 0;
  //compute new m after duplicate edge removal
  mv = sequence::plusScan(DegreesVT,DegreesVT,nv+1);
  sizesV[1] = mv; 

  uintE* DegreesH = newA(uintE,nh);
  uintT* DegreesHT = newA(uintT,nh+1);

  {parallel_for (long i=0; i < nh; i++) {
      uintT o = offsetsH[i];
      intT d = offsetsH[i+1] - o;
      if(d < 0 || d > nv) { 
	cout << "degree out of bounds: vertex "<<i<< " has degree "<< d<<endl; 
	abort(); }
      DegreesH[i] = DegreesHT[i] = d;
      if(d > 0) {
	quickSort(edgesH+o, d, pairFirstCmp<intE>());
	uintT k = 0;
	uintE lastRead = UINT_E_MAX;
	//remove self-edges and duplicate edges
	for(long j=0;j<d;j++) {
	  intEPair e = edgesH[o+j];
	  if(e.first != lastRead) {
	    lastRead = e.first;
	    edgesH[o+k++] = e;
	  }
      }
      DegreesH[i] = DegreesHT[i] = k;
    }
   }}

  DegreesHT[nh] = 0;
  //compute new m after duplicate edge removal
  mh = sequence::plusScan(DegreesHT,DegreesHT,nh+1);
  sizesH[1] = mh; 

  if (!isSymmetric) {
    //edges of V
    uintT* tOffsetsV = newA(uintT,nv+1);
    {parallel_for(long i=0;i<nv;i++) tOffsetsV[i] = UINT_T_MAX;}
    intEPair* inEdgesV = newA(intEPair,mh);
    intTriple2* temp = newA(intTriple2,mh);
    // Create mv many new intPairs.
    {parallel_for(long i=0;i<nh;i++){
	uintT o = DegreesHT[i];
	intT d = DegreesHT[i+1] - o;
      for(long j=0;j<d;j++){
  	temp[o+j] = make_pair(make_pair(edgesH[offsetsH[i]+j].first,i),edgesH[offsetsH[i]+j].second);
      }
    }}

    // Compress the out-edges.
    uchar *nEdgesV = parallelCompressWeightedEdges(edgesV, offsetsV, nv, mv, DegreesV);
    long totalSpaceV = sizesV[2] = offsetsV[nv];

    cout << "writing out edgesV..."<<endl;
    //write to binary file

    ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizesV,sizeof(long)*3); //write nv, mv and space for edges
    out.write((char*)offsetsV,sizeof(uintT)*(nv+1)); //write offsets
    out.write((char*)DegreesV,sizeof(uintE)*nv);
    out.write((char*)nEdgesV,totalSpaceV); //write edges

    free(offsetsV);
    free(sizesV);


   /* From here on-out, we use temp, and tOffsets to guarantee
       stability as after compression offsets is no longer a reliable source
       of degree information. */

    // Use pairBothCmp because when encoding we need monotonicity
    // within an edge list.
    quickSort(temp,mh,tripleCmp());
    
    tOffsetsV[temp[0].first.first] = 0; tOffsetsV[nv] = mh; 
    inEdgesV[0] = make_pair(temp[0].first.second,temp[0].second);
    {parallel_for(long i=1;i<mh;i++) {
	inEdgesV[i] = make_pair(temp[i].first.second,temp[i].second);
      if(temp[i].first.first != temp[i-1].first.first) {
      	tOffsetsV[temp[i].first.first] = i;
      }
    }}
    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsetsV,tOffsetsV,nv,minF<uintT>(),(uintT) mh);

    parallel_for(long i=0;i<nv;i++) 
      DegreesV[i] = tOffsetsV[i+1]-tOffsetsV[i];

    cout << "compressing in edgesV..."<<endl;
    uchar *ninEdgesV = parallelCompressWeightedEdges(inEdgesV, tOffsetsV, nv, mh, DegreesV);
    long tTotalSpaceV[0];
    tTotalSpaceV[0] = tOffsetsV[nv];
    free(inEdgesV);
    cout << "writing in edgesV..."<<endl;
    //write data for in-edges
    out.write((char*)tTotalSpaceV,sizeof(long)); //space for in-edges
    out.write((char*)tOffsetsV,sizeof(uintT)*(nv+1)); //write offsets
    out.write((char*)DegreesV,sizeof(uintE)*nv); //write degrees
    out.write((char*)ninEdgesV,tTotalSpaceV[0]); //write edges

    free(tOffsetsV);
    free(ninEdgesV);
    free(DegreesV);

    //edges of H
    uintT* tOffsetsH = newA(uintT,nh+1);
    {parallel_for(long i=0;i<nh;i++) tOffsetsH[i] = UINT_T_MAX;}
    intEPair* inEdgesH = newA(intEPair,mv);
    temp = newA(intTriple2,mv);
    // Create mh many new intPairs.
    {parallel_for(long i=0;i<nv;i++){
	uintT o = DegreesVT[i];
	intT d = DegreesVT[i+1] - o;
	for(long j=0;j<d;j++){
	  temp[o+j] = make_pair(make_pair(edgesV[offsetsVSaved[i]+j].first,i),edgesV[offsetsVSaved[i]+j].second);
      }
    }}

    free(DegreesVT); free(offsetsVSaved);
    free(edgesV); 

    cout << "compressing out edges of H..."<<endl;
    // Compress the out-edges.
    uchar *nEdgesH = parallelCompressWeightedEdges(edgesH, offsetsH, nh, mh, DegreesH);
    long totalSpaceH = sizesH[2] = offsetsH[nh];
    free(edgesH);

    //edgesH = nEdgesH;
    cout << "writing out edgesH..."<<endl;
    //write to binary file
    //ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizesH,sizeof(long)*3); //write nh, mh and space for edges
    out.write((char*)offsetsH,sizeof(uintT)*(nh+1)); //write offsets
    out.write((char*)DegreesH,sizeof(uintE)*nh);
    out.write((char*)nEdgesH,totalSpaceH); //write edges

    free(offsetsH);
    free(sizesH);

   /* From here on-out, we use temp, and tOffsets to guarantee
       stability as after compression offsets is no longer a reliable source
       of degree information. */

    // Use pairBothCmp because when encoding we need monotonicity
    // within an edge list.
    quickSort(temp,mh,tripleCmp());
    
    tOffsetsH[temp[0].first.first] = 0; tOffsetsH[nh] = mv; 
    inEdgesH[0] = make_pair(temp[0].first.second,temp[0].second);
    {parallel_for(long i=1;i<mv;i++) {
	inEdgesH[i] = make_pair(temp[i].first.second,temp[i].second);
      if(temp[i].first.first != temp[i-1].first.first) {
      	tOffsetsH[temp[i].first.first] = i;
      }
    }}
    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsetsH,tOffsetsH,nh,minF<uintT>(),(uintT) mv);

    parallel_for(long i=0;i<nh;i++) 
      DegreesH[i] = tOffsetsH[i+1]-tOffsetsH[i];
    cout << "compressing in edgesH..."<<endl;
    uchar *ninEdgesH = parallelCompressWeightedEdges(inEdgesH, tOffsetsH, nh, mv, DegreesH);
    long tTotalSpaceH[0];
    tTotalSpaceH[0] = tOffsetsH[nh];
    free(inEdgesH);
    cout << "writing in edgesH..."<<endl;
    //write data for in-edges
    out.write((char*)tTotalSpaceH,sizeof(long)); //space for in-edges
    out.write((char*)tOffsetsH,sizeof(uintT)*(nh+1)); //write offsets
    out.write((char*)DegreesH,sizeof(uintE)*nh); //write degrees
    out.write((char*)ninEdgesH,tTotalSpaceH[0]); //write edges
    out.close();
    free(tOffsetsH);
    free(ninEdgesH);
    free(DegreesH);
    free(DegreesHT);
  }
  else {
    free(offsetsVSaved);
    cout << "compressing..."<<endl;
    uchar *nEdgesV = parallelCompressWeightedEdges(edgesV, offsetsV, nv, mv, DegreesV);
    uchar *nEdgesH = parallelCompressWeightedEdges(edgesH, offsetsH, nh, mh, DegreesH);
    long totalSpaceV = sizesV[2] = offsetsV[nv];
    long totalSpaceH = sizesH[2] = offsetsH[nh];
    free(edgesV); free(edgesH);
    cout << "writing edges..."<<endl;
    ofstream out(outFile, ofstream::out | ios::binary);
    out.write((char*)sizesV,sizeof(long)*3); //write n, m and isSymmetric
    out.write((char*)offsetsV,sizeof(uintT)*(nv+1)); //write offsets
    out.write((char*)DegreesV,sizeof(uintE)*nv); //write degrees
    out.write((char*)nEdgesV,totalSpaceV); //write edges
    out.write((char*)sizesH,sizeof(long)*3); //write n, m and isSymmetric
    out.write((char*)offsetsH,sizeof(uintT)*(nh+1)); //write offsets
    out.write((char*)DegreesH,sizeof(uintE)*nh); //write degrees
    out.write((char*)nEdgesH,totalSpaceH); //write edges
    out.close();
    free(sizesV); free(sizesH);
    free(offsetsV); free(offsetsH);
    free(nEdgesV); free(nEdgesH);
    free(DegreesV); free(DegreesH);
    free(DegreesVT); free(DegreesHT);
  }
}


int parallel_main(int argc, char* argv[]) {  
  commandLine P(argc,argv," [-b] [-s] [-w] <inFile> <outFile>");
  char* iFile = P.getArgument(1);
  char* outFile = P.getArgument(0);
  bool binary = P.getOptionValue("-b");
  bool symmetric = P.getOptionValue("-s");
  bool weighted = P.getOptionValue("-w");
  if(!weighted) encodeHypergraphFromFile(iFile,symmetric,outFile,binary);
  else encodeWeightedHypergraphFromFile(iFile,symmetric,outFile,binary);
}
