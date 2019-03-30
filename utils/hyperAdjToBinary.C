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

// Converts a Ligra hypergraph in adjacency graph format into binary format

#include "parseCommandLine.h"
#include "graphIO.h"
#include "parallel.h"
#include <iostream>
#include <sstream>
using namespace benchIO;
using namespace std;

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-w] <inFile>");
  char* iFile = P.getArgument(0);
  // char* idxFile = P.getArgument(2);
  // char* adjFile = P.getArgument(1);
  // char* configFile = P.getArgument(0);
  bool weighted = P.getOptionValue("-w");

  char* configS = (char*) ".config";
  char* vadjS = (char*) ".vadj";
  char* vidxS = (char*) ".vidx";
  char* hadjS = (char*) ".hadj";
  char* hidxS = (char*) ".hidx";
  char configFile[strlen(iFile)+strlen(configS)+1];
  char vadjFile[strlen(iFile)+strlen(vadjS)+1];
  char vidxFile[strlen(iFile)+strlen(vidxS)+1];
  char hadjFile[strlen(iFile)+strlen(hadjS)+1];
  char hidxFile[strlen(iFile)+strlen(hidxS)+1];
  *configFile = *vadjFile = *vidxFile = *hadjFile = *hidxFile = '\0';
  strcat(configFile,iFile);
  strcat(vadjFile,iFile);
  strcat(vidxFile,iFile);
  strcat(hadjFile,iFile);
  strcat(hidxFile,iFile);
  strcat(configFile,configS);
  strcat(vadjFile,vadjS);
  strcat(vidxFile,vidxS);
  strcat(hadjFile,hadjS);
  strcat(hidxFile,hidxS);

  ofstream vidx(vidxFile, ofstream::out | ios::binary);
  ofstream vadj(vadjFile, ofstream::out | ios::binary);
  ofstream hidx(hidxFile, ofstream::out | ios::binary);
  ofstream hadj(hadjFile, ofstream::out | ios::binary);
  ofstream config(configFile, ofstream::out);
  if(!weighted) {
    hypergraph<uintT> G = readHypergraphFromFile<uintT>(iFile);
    config << G.nv << " " << G.mv << " " << G.nh << " " << G.mh;
    uintT* In = G.allocatedInplace;
    uintT* offsetsV = In+4;
    uintT* edgesV = In+4+G.nv;
    uintT* offsetsH = In+4+G.nv+G.mv;
    uintT* edgesH = In+4+G.nv+G.mv+G.nh;
    vidx.write((char*)offsetsV,sizeof(uintT)*G.nv);
    if(sizeof(uintE) != sizeof(uintT)) {
      uintE* E = newA(uintE,G.mv);
      parallel_for(long i=0;i<G.mv;i++) E[i] = edgesV[i];
      vadj.write((char*)E,sizeof(uintE)*G.mv);
      free(E);
    } else {
      vadj.write((char*)edgesV,sizeof(uintT)*G.mv);
    }
    hidx.write((char*)offsetsH,sizeof(uintT)*G.nh);
    if(sizeof(uintE) != sizeof(uintT)) {
      uintE* E = newA(uintE,G.mh);
      parallel_for(long i=0;i<G.mh;i++) E[i] = edgesH[i];
      hadj.write((char*)E,sizeof(uintE)*G.mh);
      free(E);
    } else {
      hadj.write((char*)edgesH,sizeof(uintT)*G.mh);
    }
    G.del();
  }
  else {
    wghHypergraph<uintT> G = readWghHypergraphFromFile<uintT>(iFile);
    config << G.nv << " " << G.mv << " " << G.nh << " " << G.mh;
    uintT* In = G.allocatedInplace;
    uintT* offsetsV = In+4;
    uintT* edgesV = In+4+G.nv;
    uintT* offsetsH = In+4+G.nv+2*G.mv;
    uintT* edgesH = In+4+G.nv+2*G.mv+G.nh;
    vidx.write((char*)offsetsV,sizeof(uintT)*G.nv);
    if(sizeof(uintE) != sizeof(uintT)) {
      intE* E = newA(intE,2*G.mv);
      parallel_for(long i=0;i<2*G.mv;i++) E[i] = edgesV[i];
      vadj.write((char*)E,sizeof(intE)*2*G.mv);
      free(E);
    } else {
      vadj.write((char*)edgesV,sizeof(uintT)*2*G.mv);  //edges and weights
    }
    hidx.write((char*)offsetsH,sizeof(uintT)*G.nh);
    if(sizeof(uintE) != sizeof(uintT)) {
      intE* E = newA(intE,2*G.mh);
      parallel_for(long i=0;i<2*G.mh;i++) E[i] = edgesH[i];
      hadj.write((char*)E,sizeof(intE)*2*G.mh);
      free(E);
    } else {
      hadj.write((char*)edgesH,sizeof(uintT)*2*G.mh);  //edges and weights
    }
    G.del();
  }
  config.close();
  vidx.close();
  vadj.close();
  hidx.close();
  hadj.close();
}
