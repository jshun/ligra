// Converts a weighted temporal SNAP graph
//(http://snap.stanford.edu/data/index.html) to Ligra weighted
// temporal adjacency graph format. Each line should contain five values
//---the two endpoints followed by the weight, start time, and end time.
// To symmetrize the graph, pass the "-s" flag. For undirected graphs on
// SNAP, the "-s" flag must be passed since each edge appears in only one
// direction

#include "graphIO.h"
#include "parallel.h"
#include "parseCommandLine.h"

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc, argv, "[-s] <input SNAP file> <output Ligra file>");
  char* iFile = P.getArgument(1);
  char* oFile = P.getArgument(0);
  bool sym = P.getOption("-s");
  wghEdgeTemporalArray<uintT> G = readWghTemporalSNAP<uintT>(iFile);
  writeWghTemporalGraphToFile<uintT>(
      wghGraphTemporalFromWghTemporalEdges(G, sym), oFile);
}
