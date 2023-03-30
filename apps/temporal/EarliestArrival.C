#define WEIGHTED_TEMPORAL 1

#include <cstdlib>
#include <iostream>
#include <vector>

#include "gettime.h"
#include "ligra_app.h"

// Global timers.
timer t0;

/**** LIGRA app start ****/
/**
 * Temporal shortest path: earliest arrival time, AKA "foremost".
 *
 * See also: Ligra's Bellman-Ford shortest path implementation.
 */
struct EarliestArrival_F {
  intE* EarliestArrivalTime;
  int* Visited;
  uintE QuerySourceVertexId;  // source vertex id for temporal query
  time_t QueryStartTime;      // start time of interval for temporal query
  time_t QueryEndTime;        // end time of interval for temporal query

  EarliestArrival_F(intE* _EarliestArrivalTime, int* _Visited,
                    uintE _QuerySourceVertexId, time_t _QueryStartTime,
                    time_t _QueryEndTime)
      : EarliestArrivalTime(_EarliestArrivalTime),
        Visited(_Visited),
        QuerySourceVertexId(_QuerySourceVertexId),
        QueryStartTime(_QueryStartTime),
        QueryEndTime(_QueryEndTime) {}

  // update
  //
  // See Algo 1 from "Path Problems in Temporal Graphs".
  inline bool update(uintE s, uintE d, intE edge_weight, intE edge_start_time,
                     intE edge_end_time) {
    // Edge starts after end of query interval.
    if (edge_start_time >= QueryEndTime) {
      return 0;
    }

    // Update EarliestArrivalTime path if found one with earlier arrival that
    // meets temporal path constraints.
    if (edge_end_time <= QueryEndTime &&
        // Temporal path constraint: "succeeds".
        edge_start_time >= EarliestArrivalTime[s]) {
      // Path with earlier arrival.
      if (edge_end_time < EarliestArrivalTime[d]) {
        EarliestArrivalTime[d] = edge_end_time;

        // Mark visited.
        //
        // NOTE: here we're assuming only one edge from u to v.
        if (Visited[d] == 0) {
          Visited[d] = 1;
          return 1;
        }
      }
    }
    return 0;
  }

  // atomic version of update
  inline bool updateAtomic(uintE s, uintE d, intE edge_weight,
                           intE edge_start_time, intE edge_end_time) {
    if (edge_start_time >= QueryEndTime) {
      return 0;
    }

    if (edge_end_time > QueryEndTime) {
      return 0;
    }

    if (edge_start_time < EarliestArrivalTime[s]) {
      return 0;
    }

    if (edge_end_time >= EarliestArrivalTime[d]) {
      return 0;
    }

    return (writeMin(&EarliestArrivalTime[d], edge_end_time) &&
            CAS(&Visited[d], 0, 1));
  }

  // cond function always returns true
  inline bool cond(uintE d) { return cond_true(d); }
};

// reset visited vertices
struct EarliestArrival_Vertex_F {
  int* Visited;
  EarliestArrival_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator()(uintE i) {
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, long source_vertex_id, time_t start_time,
             time_t end_time) {
  long n = GA.n;

  // initialize EarliestArrivalTime to "infinity" for all but the source vertex.
  intE* EarliestArrivalTime = newA(intE, n);
  { parallel_for(long i = 0; i < n; i++) EarliestArrivalTime[i] = INT_MAX; }
  EarliestArrivalTime[source_vertex_id] = start_time;

  int* Visited = newA(int, n);
  { parallel_for(long i = 0; i < n; i++) Visited[i] = 0; }

  vertexSubset Frontier(n, source_vertex_id);  // initial frontier

  while (!Frontier.isEmpty()) {
    vertexSubset output =
        edgeMap(GA, Frontier,
                EarliestArrival_F(EarliestArrivalTime, Visited,
                                  source_vertex_id, start_time, end_time),
                GA.m / 20, dense_forward);
    vertexMap(output, EarliestArrival_Vertex_F(Visited));
    Frontier.del();
    Frontier = output;
  }

  /*
    // Print earliest arrival times from source vertex to all other vertices.
    std::cerr << "Earliest Arrival times:\n";
    std::cerr << "Query: [" << start_time << ", " << end_time << "]\n";
    for (long i = 0; i < n; i++) {
      std::cerr << source_vertex_id << " -> " << i << " = "
                << EarliestArrivalTime[i] << std::endl;
    }
  */

  Frontier.del();
  free(Visited);
  free(EarliestArrivalTime);
}
/**** LIGRA app end ****/

int parallel_main(int argc, char* argv[]) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " input_file source_vertex start_time end_time" << std::endl;
    std::cerr << " input_file    - input temporal graph file (Ligra format)"
              << std::endl;
    // See temporal paths paper Algo 1:
    std::cerr << " source_vertex - vertex id of origin" << std::endl;
    std::cerr << " start_time    - start time of temporal path query"
              << std::endl;
    std::cerr << " end_time      - end time of temporal path query"
              << std::endl;

    return 0;
  }

  std::cerr << std::endl << "Reading input file..." << std::endl;
  t0.start();
  graph<asymmetricVertex> G = readGraph<asymmetricVertex>(
      argv[1], false /* compressed */, false /* symmetric*/, false /* binary */,
      false /* mmap */);
  t0.stop();

  /*
    // Print all edges
    for (size_t i = 0; i < G.n; i++) {
      asymmetricVertex u = G.V[i];
      auto d = u.getOutDegree();
      for (size_t j = 0; j < d; j++) {
        std::cerr << "u=" << i << ", v=" << u.getOutNeighbor(j)
                  << ", wgh=" << u.getOutWeight(j)
                  << ", startTime=" << u.getOutStartTime(j)
                  << ", endTime=" << u.getOutEndTime(j) << std::endl;
      }
    }
  */

  // Args needed for temporal path (earliest arrival time).
  uint64_t source_vertex_id = strtoull(argv[2], NULL, 10);
  time_t start_time = (time_t)strtoll(argv[3], NULL, 10);
  time_t end_time = (time_t)strtoll(argv[4], NULL, 10);

  std::cerr << std::endl;
  std::cerr << "num_vertices=" << G.n << std::endl;
  std::cerr << "num_edges=" << G.m << std::endl;
  std::cerr << "source_vertex=" << source_vertex_id << std::endl;
  std::cerr << "start_time=" << start_time << std::endl;
  std::cerr << "end_time=" << end_time << std::endl;
  t0.reportTotal("creation_time-ligra_csr");
  std::cerr << std::endl;

  startTime();
  Compute(G, source_vertex_id, start_time, end_time);
  nextTime("runtime");

  G.del();

  return 0;
}
