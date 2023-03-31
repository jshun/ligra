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
#ifndef LIGRA_APP_MAIN_H
#define LIGRA_APP_MAIN_H

#include "ligra_app.h"

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc, argv, " [-s] <inFile>");
  char* iFile = P.getArgument(0);
  bool symmetric = P.getOptionValue("-s");
  bool compressed = P.getOptionValue("-c");
  bool binary = P.getOptionValue("-b");
  bool mmap = P.getOptionValue("-m");
  // cout << "mmap = " << mmap << endl;
  long rounds = P.getOptionLongValue("-rounds", 3);

  std::cout << "Args:\nsymmetric: " << symmetric
            << ", compressed: " << compressed << ", binary: " << binary
            << ", mmap: " << mmap << ", rounds: " << rounds << std::endl;

  if (compressed) {
    if (symmetric) {
#ifndef HYPER
      graph<compressedSymmetricVertex> G =
          readCompressedGraph<compressedSymmetricVertex>(
              iFile, symmetric, mmap);  // symmetric graph
#else
      hypergraph<compressedSymmetricVertex> G =
          readCompressedHypergraph<compressedSymmetricVertex>(
              iFile, symmetric, mmap);  // symmetric graph
#endif
      Compute(G, P);
      for (int r = 0; r < rounds; r++) {
        startTime();
        Compute(G, P);
        nextTime("Running time");
      }
      G.del();
    } else {
#ifndef HYPER
      graph<compressedAsymmetricVertex> G =
          readCompressedGraph<compressedAsymmetricVertex>(
              iFile, symmetric, mmap);  // asymmetric graph
#else
      hypergraph<compressedAsymmetricVertex> G =
          readCompressedHypergraph<compressedAsymmetricVertex>(
              iFile, symmetric, mmap);  // asymmetric graph
#endif
      Compute(G, P);
      if (G.transposed) G.transpose();
      for (int r = 0; r < rounds; r++) {
        startTime();
        Compute(G, P);
        nextTime("Running time");
        if (G.transposed) G.transpose();
      }
      G.del();
    }
  } else {
    if (symmetric) {
#ifndef HYPER
      graph<symmetricVertex> G = readGraph<symmetricVertex>(
          iFile, compressed, symmetric, binary, mmap);  // symmetric graph
#else
      hypergraph<symmetricVertex> G = readHypergraph<symmetricVertex>(
          iFile, compressed, symmetric, binary, mmap);  // symmetric graph
#endif
      Compute(G, P);
      for (int r = 0; r < rounds; r++) {
        startTime();
        Compute(G, P);
        nextTime("Running time");
      }
      G.del();
    } else {
#ifndef HYPER
      graph<asymmetricVertex> G = readGraph<asymmetricVertex>(
          iFile, compressed, symmetric, binary, mmap);  // asymmetric graph
#else
      hypergraph<asymmetricVertex> G = readHypergraph<asymmetricVertex>(
          iFile, compressed, symmetric, binary, mmap);  // asymmetric graph
#endif
      Compute(G, P);
      if (G.transposed) G.transpose();
      for (int r = 0; r < rounds; r++) {
        startTime();
        Compute(G, P);
        nextTime("Running time");
        if (G.transposed) G.transpose();
      }
      G.del();
    }
  }
}
#endif  // LIGRA_APP_MAIN_H
