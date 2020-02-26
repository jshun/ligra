#include <iostream>
#include <stdlib.h>
#include <string>
#include "parallel.h"
#include "gettime.h"
#include "utils.h"
#include "graph.h"
#include "hypergraphIO.h"
#include "parseCommandLine.h"
#include "vertex.h"
using namespace std;

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," <inFile>");
  char* iFile = P.getArgument(0);

  hypergraph<symmetricVertex> G =
    readHypergraphFromFile<symmetricVertex>(iFile,1,0);

  long n = G.nv;
  cout << "left side\n";
  for(long i=0;i<n;i++) {
    symmetricVertex v = G.V[i];
    cout << i << ": ";
    for(long j=0;j<v.getOutDegree();j++)
      cout << v.getOutNeighbor(j) << " ";
    cout << endl;
  }

  cout << "right side\n";
  long nh = G.nh;
  for(long i=0;i<nh;i++) {
    symmetricVertex v = G.H[i];
    cout << i << ": ";
    for(long j=0;j<v.getInDegree();j++)
      cout << v.getInNeighbor(j) << " ";
    cout << endl;
  }

}
