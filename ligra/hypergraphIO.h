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

#pragma once
#include "IO.h"
using namespace std;

template <class vertex>
hypergraph<vertex> readHypergraphFromFile(char* fname, bool isSymmetric, bool mmap) {
  words W;
  if (mmap) {
    _seq<char> S = mmapStringFromFile(fname);
    char *bytes = newA(char, S.n);
    // Cannot mutate the graph unless we copy.
    parallel_for(size_t i=0; i<S.n; i++) {
      bytes[i] = S.A[i];
    }
    if (munmap(S.A, S.n) == -1) {
      perror("munmap");
      exit(-1);
    }
    S.A = bytes;
    W = stringToWords(S.A, S.n);
  } else {
    _seq<char> S = readStringFromFile(fname);
    W = stringToWords(S.A, S.n);
  }
#ifndef WEIGHTED
  if (W.Strings[0] != (string) "AdjacencyHypergraph") {
#else
  if (W.Strings[0] != (string) "WeightedAdjacencyHypergraph") {
#endif
    cout << "Bad input file" << endl;
    abort();
  }

  long len = W.m -1;
  long nv = atol(W.Strings[1]);
  long mv = atol(W.Strings[2]);
  long nh = atol(W.Strings[3]);
  long mh = atol(W.Strings[4]);

#ifndef WEIGHTED
  if (len != nv + mv + nh + mh + 4) {
#else
    if (len != nv + 2*mv + nh + 2*mh + 4) {
#endif
    cout << "Bad input file" << endl;
    abort();
  }

  uintT* offsetsV = newA(uintT,nv);
  uintT* offsetsH = newA(uintT,nh);
#ifndef WEIGHTED
  uintE* edgesV = newA(uintE,mv);
  uintE* edgesH = newA(uintE,mh);
#else
  intE* edgesV = newA(intE,2*mv);
  intE* edgesH = newA(intE,2*mh);
#endif

  {parallel_for(long i=0; i < nv; i++) offsetsV[i] = atol(W.Strings[i + 5]);}
  {parallel_for(long i=0; i<mv; i++) {
#ifndef WEIGHTED
      edgesV[i] = atol(W.Strings[i+nv+5]);
      if(edgesV[i] < 0 || edgesV[i] >= nh) { cout << "edgesV out of range: nh = " << nh << " edge = " << edgesV[i] << endl; exit(0); }
#else
      edgesV[2*i] = atol(W.Strings[i+nv+5]);
      if(edgesV[2*i] < 0 || edgesV[2*i] >= nh) { cout << "edgesV out of range: nh = " << nh << " edge = " << edgesV[2*i] << endl; exit(0); }
      edgesV[2*i+1] = atol(W.Strings[i+nv+mv+5]);
#endif
    }}

#ifndef WEIGHTED
    {parallel_for(long i=0; i < nh; i++) offsetsH[i] = atol(W.Strings[i + nv + mv + 5]);}
#else
    {parallel_for(long i=0; i < nh; i++) offsetsH[i] = atol(W.Strings[i + nv + 2*mv + 5]);}
#endif
    {parallel_for(long i=0; i<mh; i++) {
#ifndef WEIGHTED
      edgesH[i] = atol(W.Strings[i+nv+mv+nh+5]);
      if(edgesH[i] < 0 || edgesH[i] >= nv) { cout << "edgesH out of range: nv = " << nv << " edge = " << edgesH[i] << endl; exit(0); }
#else
      edgesH[2*i] = atol(W.Strings[i+nv+2*mv+nh+5]);
      if(edgesH[2*i] < 0 || edgesH[2*i] >= nv) { cout << "edgesH out of range: nv = " << nv << " edge = " << edgesH[2*i] << endl; exit(0); }
      edgesH[2*i+1] = atol(W.Strings[i+nv+2*mv+nh+mh+5]);
#endif
    }}

  //W.del(); // to deal with performance bug in malloc

  vertex* v = newA(vertex,nv+1);
  vertex* h = newA(vertex,nh+1);

  //vertices
  {parallel_for (uintT i=0; i < nv; i++) {
    uintT o = offsetsV[i];
    uintT l = ((i == nv-1) ? mv : offsetsV[i+1])-offsetsV[i];
    v[i].setOutDegree(l);
#ifndef WEIGHTED
    v[i].setOutNeighbors(edgesV+o);
#else
    v[i].setOutNeighbors(edgesV+2*o);
#endif
    }}
#ifndef WEIGHTED
  v[nv].setOutNeighbors(edgesV+mv);
#else
  v[nv].setOutNeighbors(edgesV+2*mv);
#endif
  
  //hyperedges
  {parallel_for (uintT i=0; i < nh; i++) {
    uintT o = offsetsH[i];
    uintT l = ((i == nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
    h[i].setOutDegree(l);
#ifndef WEIGHTED
    h[i].setOutNeighbors(edgesH+o);
#else
    h[i].setOutNeighbors(edgesH+2*o);
#endif
    }}
#ifndef WEIGHTED
  h[nh].setOutNeighbors(edgesH+mh);
#else
  h[nh].setOutNeighbors(edgesH+2*mh);
#endif

  if(!isSymmetric) {
    //in-edges for vertices obtained from out-edges for hyperedges
    uintT* tOffsets = newA(uintT,nv);
    {parallel_for(long i=0;i<nv;i++) tOffsets[i] = INT_T_MAX;}
#ifndef WEIGHTED
    intPair* temp = newA(intPair,mh);
#else
    intTriple* temp = newA(intTriple,mh);
#endif
    {parallel_for(long i=0;i<nh;i++){
      uintT o = offsetsH[i];
      for(uintT j=0;j<h[i].getOutDegree();j++){
#ifndef WEIGHTED
	temp[o+j] = make_pair(h[i].getOutNeighbor(j),i);
#else
	temp[o+j] = make_pair(h[i].getOutNeighbor(j),make_pair(i,h[i].getOutWeight(j)));
#endif
      }
      }}
    free(offsetsH);

#ifndef WEIGHTED
#ifndef LOWMEM
    intSort::iSort(temp,mh,nv+1,getFirst<uintE>());
#else
    quickSort(temp,mh,pairFirstCmp<uintE>());
#endif
#else
#ifndef LOWMEM
    intSort::iSort(temp,mh,nv+1,getFirst<intPair>());
#else
    quickSort(temp,mh,pairFirstCmp<intPair>());
#endif
#endif

    tOffsets[temp[0].first] = 0;
#ifndef WEIGHTED
    uintE* inEdgesV = newA(uintE,mh);
    inEdgesV[0] = temp[0].second;
#else
    intE* inEdgesV = newA(intE,2*mh);
    inEdgesV[0] = temp[0].second.first;
    inEdgesV[1] = temp[0].second.second;
#endif
    {parallel_for(long i=1;i<mh;i++) {
#ifndef WEIGHTED
      inEdgesV[i] = temp[i].second;
#else
      inEdgesV[2*i] = temp[i].second.first;
      inEdgesV[2*i+1] = temp[i].second.second;
#endif
      if(temp[i].first != temp[i-1].first) {
	tOffsets[temp[i].first] = i;
      }
      }}

    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsets,tOffsets,nv,minF<uintT>(),(uintT)mh);

    {parallel_for(long i=0;i<nv;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == nv-1) ? mh : tOffsets[i+1])-tOffsets[i];
      v[i].setInDegree(l);
#ifndef WEIGHTED
      v[i].setInNeighbors(inEdgesV+o);
#else
      v[i].setInNeighbors(inEdgesV+2*o);
#endif
      }}

#ifndef WEIGHTED
  v[nv].setInNeighbors(inEdgesV+mv);
#else
  v[nv].setInNeighbors(inEdgesV+2*mv);
#endif

    free(tOffsets);

    //in-edges for hyperedges obtained from out-edges for vertices
    tOffsets = newA(uintT,nh);
    {parallel_for(long i=0;i<nh;i++) tOffsets[i] = INT_T_MAX;}
#ifndef WEIGHTED
    temp = newA(intPair,mv);
#else
    temp = newA(intTriple,mv);
#endif
    {parallel_for(long i=0;i<nv;i++){
      uintT o = offsetsV[i];
      for(uintT j=0;j<v[i].getOutDegree();j++){
#ifndef WEIGHTED
	temp[o+j] = make_pair(v[i].getOutNeighbor(j),i);
#else
	temp[o+j] = make_pair(v[i].getOutNeighbor(j),make_pair(i,v[i].getOutWeight(j)));
#endif
      }
      }}
    free(offsetsV);

#ifndef WEIGHTED
#ifndef LOWMEM
    intSort::iSort(temp,mv,nh+1,getFirst<uintE>());
#else
    quickSort(temp,mv,pairFirstCmp<uintE>());
#endif
#else
#ifndef LOWMEM
    intSort::iSort(temp,mv,nh+1,getFirst<intPair>());
#else
    quickSort(temp,mv,pairFirstCmp<intPair>());
#endif
#endif

    tOffsets[temp[0].first] = 0;
#ifndef WEIGHTED
    uintE* inEdgesH = newA(uintE,mv);
    inEdgesH[0] = temp[0].second;
#else
    intE* inEdgesH = newA(intE,2*mv);
    inEdgesH[0] = temp[0].second.first;
    inEdgesH[1] = temp[0].second.second;
#endif
    {parallel_for(long i=1;i<mv;i++) {
#ifndef WEIGHTED
      inEdgesH[i] = temp[i].second;
#else
      inEdgesH[2*i] = temp[i].second.first;
      inEdgesH[2*i+1] = temp[i].second.second;
#endif
      if(temp[i].first != temp[i-1].first) {
	tOffsets[temp[i].first] = i;
      }
      }}

    free(temp);

    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsets,tOffsets,nh,minF<uintT>(),(uintT)mv);

    {parallel_for(long i=0;i<nh;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == nh-1) ? mv : tOffsets[i+1])-tOffsets[i];
      h[i].setInDegree(l);
#ifndef WEIGHTED
      h[i].setInNeighbors(inEdgesH+o);
#else
      h[i].setInNeighbors(inEdgesH+2*o);
#endif
      }}

#ifndef WEIGHTED
  h[nh].setInNeighbors(inEdgesH+mh);
#else
  h[nh].setInNeighbors(inEdgesH+2*mh);
#endif

    free(tOffsets);

    Uncompressed_Memhypergraph<vertex>* mem =
      new Uncompressed_Memhypergraph<vertex>(v,h,nv,mv,nh,mh,edgesV,edgesH,inEdgesV,inEdgesH);
    return hypergraph<vertex>(v,h,nv,mv,nh,mh,mem);
  }
  else {
    free(offsetsV); free(offsetsH);
    Uncompressed_Memhypergraph<vertex>* mem =
      new Uncompressed_Memhypergraph<vertex>(v,h,nv,mv,nh,mh,edgesV,edgesH);
    return hypergraph<vertex>(v,h,nv,mv,nh,mh,mem);
  }
}

template <class vertex>
hypergraph<vertex> readHypergraphFromBinary(char* iFile, bool isSymmetric) {
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
  long nv, mv, nh, mh;
  in >> nv; in >> mv; in >> nh; in >> mh;
  in.close();

  ifstream in2(vadjFile,ifstream::in | ios::binary); //stored as uints
  in2.seekg(0, ios::end);
  long size = in2.tellg();
  in2.seekg(0);
#ifdef WEIGHTED
  if(mv != size/(2*sizeof(uint))) { cout << "size wrong\n"; exit(0); }
#else
  if(mv != size/(sizeof(uint))) { cout << "size wrong\n"; exit(0); }
#endif
  char* s = (char *) malloc(size);
  in2.read(s,size);
  in2.close();
  uintE* edgesV = (uintE*) s;

  ifstream in3(vidxFile,ifstream::in | ios::binary); //stored as longs
  in3.seekg(0, ios::end);
  size = in3.tellg();
  in3.seekg(0);
  if(nv != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

  char* t = (char *) malloc(size);
  in3.read(t,size);
  in3.close();
  uintT* offsetsV = (uintT*) t;

  ifstream in4(hadjFile,ifstream::in | ios::binary); //stored as uints
  in4.seekg(0, ios::end);
  size = in4.tellg();
  in4.seekg(0);
#ifdef WEIGHTED
  if(mh != size/(2*sizeof(uint))) { cout << "size wrong\n"; exit(0); }
#else
  if(mh != size/(sizeof(uint))) { cout << "size wrong\n"; exit(0); }
#endif
  char* s2 = (char *) malloc(size);
  in4.read(s2,size);
  in4.close();
  uintE* edgesH = (uintE*) s2;

  ifstream in5(hidxFile,ifstream::in | ios::binary); //stored as longs
  in5.seekg(0, ios::end);
  size = in5.tellg();
  in5.seekg(0);
  if(nh != size/sizeof(intT)) { cout << "File size wrong\n"; abort(); }

  char* t2 = (char *) malloc(size);
  in5.read(t2,size);
  in5.close();
  uintT* offsetsH = (uintT*) t2;

  vertex* v = newA(vertex,nv+1);
  vertex* h = newA(vertex,nh+1);
#ifdef WEIGHTED
  intE* edgesAndWeightsV = newA(intE,2*mv);
  {parallel_for(long i=0;i<mv;i++) {
    edgesAndWeightsV[2*i] = edgesV[i];
    edgesAndWeightsV[2*i+1] = edgesV[i+mh];
    }}
  intE* edgesAndWeightsH = newA(intE,2*mh);
  {parallel_for(long i=0;i<mh;i++) {
    edgesAndWeightsH[2*i] = edgesH[i];
    edgesAndWeightsH[2*i+1] = edgesH[i+mh];
    }}
#endif
  
  //vertices
  {parallel_for(long i=0;i<nv;i++) {
    uintT o = offsetsV[i];
    uintT l = ((i==nv-1) ? mv : offsetsV[i+1])-offsetsV[i];
      v[i].setOutDegree(l);
#ifndef WEIGHTED
      v[i].setOutNeighbors((uintE*)edgesV+o);
#else
      v[i].setOutNeighbors(edgesAndWeightsV+2*o);
#endif
    }}
#ifndef WEIGHTED
  v[nv].setOutNeighbors((uintE*)edgesV+mv);
#else
  v[nv].setOutNeighbors(edgesAndWeightsV+2*mv);
#endif
  
  //hyperedges
  {parallel_for(long i=0;i<nh;i++) {
    uintT o = offsetsH[i];
    uintT l = ((i==nh-1) ? mh : offsetsH[i+1])-offsetsH[i];
      h[i].setOutDegree(l);
#ifndef WEIGHTED
      h[i].setOutNeighbors((uintE*)edgesH+o);
#else
      h[i].setOutNeighbors(edgesAndWeightsH+2*o);
#endif
    }}
#ifndef WEIGHTED
  h[nh].setOutNeighbors((uintE*)edgesH+mh);
#else
  h[nh].setOutNeighbors(edgesAndWeightsH+2*mh);
#endif

  if(!isSymmetric) {
    //in-edges for vertices obtained from out-edges for hyperedges
    uintT* tOffsets = newA(uintT,nv);
    {parallel_for(long i=0;i<nv;i++) tOffsets[i] = INT_T_MAX;}
#ifndef WEIGHTED
    intPair* temp = newA(intPair,mh);
#else
    intTriple* temp = newA(intTriple,mh);
#endif

    {parallel_for(intT i=0;i<nh;i++){
      uintT o = offsetsH[i];
      for(uintT j=0;j<h[i].getOutDegree();j++){
#ifndef WEIGHTED
	temp[o+j] = make_pair(h[i].getOutNeighbor(j),i);
#else
	temp[o+j] = make_pair(h[i].getOutNeighbor(j),make_pair(i,h[i].getOutWeight(j)));
#endif
      }
      }}
    free(offsetsH);

#ifndef WEIGHTED
#ifndef LOWMEM
    intSort::iSort(temp,mh,nv+1,getFirst<uintE>());
#else
    quickSort(temp,mh,pairFirstCmp<uintE>());
#endif
#else
#ifndef LOWMEM
    intSort::iSort(temp,mh,nv+1,getFirst<intPair>());
#else
    quickSort(temp,mh,pairFirstCmp<intPair>());
#endif
#endif

    tOffsets[temp[0].first] = 0;
#ifndef WEIGHTED
    uintE* inEdgesV = newA(uintE,mh);
    inEdgesV[0] = temp[0].second;
#else
    intE* inEdgesV = newA(intE,2*mh);
    inEdgesV[0] = temp[0].second.first;
    inEdgesV[1] = temp[0].second.second;
#endif
    {parallel_for(long i=1;i<mh;i++) {
#ifndef WEIGHTED
      inEdgesV[i] = temp[i].second;
#else
      inEdgesV[2*i] = temp[i].second.first;
      inEdgesV[2*i+1] = temp[i].second.second;
#endif
      if(temp[i].first != temp[i-1].first) {
	tOffsets[temp[i].first] = i;
      }
      }}

    free(temp);
    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsets,tOffsets,nv,minF<uintT>(),(uintT)mh);
    {parallel_for(long i=0;i<nv;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == nv-1) ? mh : tOffsets[i+1])-tOffsets[i];
      v[i].setInDegree(l);
#ifndef WEIGHTED
      v[i].setInNeighbors((uintE*)inEdgesV+o);
#else
      v[i].setInNeighbors((intE*)(inEdgesV+2*o));
#endif
      }}
    free(tOffsets);
#ifndef WEIGHTED
      v[nv].setInNeighbors((uintE*)inEdgesV+mv);
#else
      v[nv].setInNeighbors((intE*)(inEdgesV+2*mv));
#endif

    //in-edges for hyperedges obtained from out-edges for vertices
    tOffsets = newA(uintT,nh);
    {parallel_for(long i=0;i<nh;i++) tOffsets[i] = INT_T_MAX;}
#ifndef WEIGHTED
    temp = newA(intPair,mv);
#else
    temp = newA(intTriple,mv);
#endif
    {parallel_for(intT i=0;i<nv;i++){
      uintT o = offsetsV[i];
      for(uintT j=0;j<v[i].getOutDegree();j++){
#ifndef WEIGHTED
	temp[o+j] = make_pair(v[i].getOutNeighbor(j),i);
#else
	temp[o+j] = make_pair(v[i].getOutNeighbor(j),make_pair(i,v[i].getOutWeight(j)));
#endif
      }
      }}
    free(offsetsV);
#ifndef WEIGHTED
#ifndef LOWMEM
    intSort::iSort(temp,mv,nh+1,getFirst<uintE>());
#else
    quickSort(temp,mv,pairFirstCmp<uintE>());
#endif
#else
#ifndef LOWMEM
    intSort::iSort(temp,mv,nh+1,getFirst<intPair>());
#else
    quickSort(temp,mv,pairFirstCmp<intPair>());
#endif
#endif
    tOffsets[temp[0].first] = 0;
#ifndef WEIGHTED
    uintE* inEdgesH = newA(uintE,mv);
    inEdgesH[0] = temp[0].second;
#else
    intE* inEdgesH = newA(intE,2*mv);
    inEdgesH[0] = temp[0].second.first;
    inEdgesH[1] = temp[0].second.second;
#endif
    {parallel_for(long i=1;i<mv;i++) {
#ifndef WEIGHTED
      inEdgesH[i] = temp[i].second;
#else
      inEdgesH[2*i] = temp[i].second.first;
      inEdgesH[2*i+1] = temp[i].second.second;
#endif
      if(temp[i].first != temp[i-1].first) {
	tOffsets[temp[i].first] = i;
      }
      }}
    free(temp);
    //fill in offsets of degree 0 vertices by taking closest non-zero
    //offset to the right
    sequence::scanIBack(tOffsets,tOffsets,nh,minF<uintT>(),(uintT)mv);
    {parallel_for(long i=0;i<nh;i++){
      uintT o = tOffsets[i];
      uintT l = ((i == nh-1) ? mv : tOffsets[i+1])-tOffsets[i];
      h[i].setInDegree(l);
#ifndef WEIGHTED
      h[i].setInNeighbors((uintE*)inEdgesH+o);
#else
      h[i].setInNeighbors((intE*)(inEdgesH+2*o));
#endif
      }}
    free(tOffsets);
#ifndef WEIGHTED
      h[nh].setInNeighbors((uintE*)inEdgesH+mh);
#else
      h[nh].setInNeighbors((intE*)(inEdgesH+2*mh));
#endif

#ifndef WEIGHTED
    Uncompressed_Memhypergraph<vertex>* mem =
      new Uncompressed_Memhypergraph<vertex>(v,h,nv,mv,nh,mh,edgesV,edgesH,inEdgesV,inEdgesH);
    return hypergraph<vertex>(v,h,nv,mv,nh,mh,mem);
#else
    Uncompressed_Memhypergraph<vertex>* mem =
      new Uncompressed_Memhypergraph<vertex>(v,h,nv,mv,nh,mh,edgesAndWeightsV,edgesAndWeightsH,inEdgesV,inEdgesH);
    return hypergraph<vertex>(v,h,nv,mv,nh,mh,mem);
#endif
  }
  free(offsetsV); free(offsetsH);
#ifndef WEIGHTED
  Uncompressed_Memhypergraph<vertex>* mem =
    new Uncompressed_Memhypergraph<vertex>(v,h,nv,mv,nh,mh,edgesV,edgesH);
  return hypergraph<vertex>(v,h,nv,mv,nh,mh,mem);
#else
  Uncompressed_Memhypergraph<vertex>* mem =
    new Uncompressed_Memhypergraph<vertex>(v,h,nv,mv,nh,mh,edgesAndWeightsV,edgesAndWeightsH);
  return hypergraph<vertex>(v,h,nv,mv,nh,mh,mem);
#endif
}

template <class vertex>
hypergraph<vertex> readHypergraph(char* iFile, bool compressed, bool symmetric, bool binary, bool mmap) {
  if(binary) return readHypergraphFromBinary<vertex>(iFile,symmetric);
  else return readHypergraphFromFile<vertex>(iFile,symmetric,mmap);
}

template <class vertex>
hypergraph<vertex> readCompressedHypergraph(char* fname, bool isSymmetric, bool mmap) {
  char* s;
  if (mmap) {
    _seq<char> S = mmapStringFromFile(fname);
    // Cannot mutate graph unless we copy.
    char *bytes = newA(char, S.n);
    parallel_for(size_t i=0; i<S.n; i++) {
      bytes[i] = S.A[i];
    }
    if (munmap(S.A, S.n) == -1) {
      perror("munmap");
      exit(-1);
    }
    s = bytes;
  } else {
    ifstream in(fname,ifstream::in |ios::binary);
    in.seekg(0,ios::end);
    long size = in.tellg();
    in.seekg(0);
    cout << "size = " << size << endl;
    s = (char*) malloc(size);
    in.read(s,size);
    in.close();
  }

  long* sizesV = (long*) s;
  long nv = sizesV[0], mv = sizesV[1], totalSpaceV = sizesV[2];

  cout << "nv = "<<nv<<" mv = "<<mv<<" totalSpaceV = "<<totalSpaceV<<endl;
  cout << "reading file..."<<endl;

  uintT* offsetsV = (uintT*) (s+3*sizeof(long));
  long skip = 3*sizeof(long) + (nv+1)*sizeof(intT);
  uintE* DegreesV = (uintE*) (s+skip);
  skip+= nv*sizeof(intE);
  uchar* edgesV = (uchar*)(s+skip);
  skip += totalSpaceV;

  uintT* inOffsetsV;
  uchar* inEdgesV;
  uintE* inDegreesV;
  if(!isSymmetric){
    uchar* inData = (uchar*)(s + skip);
    sizesV = (long*) inData;
    long inTotalSpace = sizesV[0];
    cout << "inTotalSpaceV = "<<inTotalSpace<<endl;
    skip += sizeof(long);
    inOffsetsV = (uintT*) (s + skip);
    skip += (nv+1)*sizeof(uintT);
    inDegreesV = (uintE*)(s+skip);
    skip += nv*sizeof(uintE);
    inEdgesV = (uchar*)(s + skip);
    skip += inTotalSpace;
  } else {
    inOffsetsV = offsetsV;
    inEdgesV = edgesV;
    inDegreesV = DegreesV;
  }

  long* sizesH = (long*) (s+skip);
  long nh = sizesH[0], mh = sizesH[1], totalSpaceH = sizesH[2];

  cout << "nh = "<<nh<<" mh = "<<mh<<" totalSpaceH = "<<totalSpaceH<<endl;
  cout << "reading file..."<<endl;

  skip += 3*sizeof(long);
  uintT* offsetsH = (uintT*) (s+skip);
  skip += (nh+1)*sizeof(intT);
  uintE* DegreesH = (uintE*) (s+skip);
  skip+= nh*sizeof(intE);
  uchar* edgesH = (uchar*)(s+skip);
  skip += totalSpaceH;
  uintT* inOffsetsH;
  uchar* inEdgesH;
  uintE* inDegreesH;
  if(!isSymmetric){
    uchar* inData = (uchar*)(s + skip);
    sizesH = (long*) inData;
    long inTotalSpace = sizesH[0];
    cout << "inTotalSpaceH = "<<inTotalSpace<<endl;
    skip += sizeof(long);
    inOffsetsH = (uintT*) (s + skip);
    skip += (nh+1)*sizeof(uintT);
    inDegreesH = (uintE*)(s+skip);
    skip += nh*sizeof(uintE);
    inEdgesH = (uchar*)(s + skip);
  } else {
    inOffsetsH = offsetsH;
    inEdgesH = edgesH;
    inDegreesH = DegreesH;
  }


  vertex *V = newA(vertex,nv);
  parallel_for(long i=0;i<nv;i++) {
    long o = offsetsV[i];
    uintT d = DegreesV[i];
    V[i].setOutDegree(d);
    V[i].setOutNeighbors(edgesV+o);
  }

  if(sizeof(vertex) == sizeof(compressedAsymmetricVertex)){
    parallel_for(long i=0;i<nv;i++) {
      long o = inOffsetsV[i];
      uintT d = inDegreesV[i];
      V[i].setInDegree(d);
      V[i].setInNeighbors(inEdgesV+o);
    }
  }

  vertex *H = newA(vertex,nh);
  parallel_for(long i=0;i<nh;i++) {
    long o = offsetsH[i];
    uintT d = DegreesH[i];
    H[i].setOutDegree(d);
    H[i].setOutNeighbors(edgesH+o);
  }

  if(sizeof(vertex) == sizeof(compressedAsymmetricVertex)){
    parallel_for(long i=0;i<nh;i++) {
      long o = inOffsetsH[i];
      uintT d = inDegreesH[i];
      H[i].setInDegree(d);
      H[i].setInNeighbors(inEdgesH+o);
    }
  }

  cout << "creating hypergraph..."<<endl;

  Compressed_Memhypergraph<vertex>* mem =
    new Compressed_Memhypergraph<vertex>(V,H,s);
  hypergraph<vertex> G(V,H,nv,mv,nh,mh,mem);
  return G;
}
