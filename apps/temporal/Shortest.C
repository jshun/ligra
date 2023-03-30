#define WEIGHTED_TEMPORAL 1

#include <string.h>
#include <cstdlib>
#include <iostream>

#include "gettime.h"
#include "ligra_app.h"

// Global timers.
timer t0;

/**** LIGRA app start ****/
/**
 * Temporal path: Shortest distance (with temporal path constraints).
 */
struct Shortest_F {
  intE* EarliestArrivalTime;
  intE* Shortest;
  int* Visited;
  uintE QuerySourceVertexId;  // source vertex id for temporal query
  time_t QueryStartTime;      // start time of interval for temporal query
  time_t QueryEndTime;        // end time of interval for temporal query

  Shortest_F(intE* _EarliestArrivalTime, intE* _Shortest, int* _Visited,
             uintE _QuerySourceVertexId, time_t _QueryStartTime,
             time_t _QueryEndTime)
      : EarliestArrivalTime(_EarliestArrivalTime),
        Shortest(_Shortest),
        Visited(_Visited),
        QuerySourceVertexId(_QuerySourceVertexId),
        QueryStartTime(_QueryStartTime),
        QueryEndTime(_QueryEndTime) {}

  // update
  //
  // From "Path Problems in Temporal Graphs", Lemma 13:
  //
  // Given two temporal paths from s to d, P1 and P2, within [QueryStartTime,
  // QueryEndTime], if
  //
  // 1. dist(P1) <= dist(P2) and
  // 2. end(P1) <= end(P2)
  //
  // Then we can safely prune P2 in the computation of shortest paths from s to
  // any vertex that pass through d within [QueryStartTime, QueryEndTime].
  inline bool update(uintE s, uintE d, intE edge_weight, intE edge_start_time,
                     intE edge_end_time) {
    // Edge starts after end of query interval.
    if (edge_start_time >= QueryEndTime) {
      return false;
    }

    // Edge ends before start of the query interval.
    if (edge_end_time <= QueryStartTime) {
      return false;
    }

    intE newDist = Shortest[s] + edge_weight;

    // Temporal predicate constraint: next edge in the path needs to start
    // after previous one.
    bool is_start_time_later = edge_start_time >= EarliestArrivalTime[s];
    if (edge_end_time <= QueryEndTime && is_start_time_later) {
      // Update shortest path if temporal constraint holds *and* this path is
      // shortest.
      bool is_end_time_earlier = edge_end_time < EarliestArrivalTime[d];
      bool is_shorter = Shortest[d] > newDist;
      if (is_end_time_earlier && is_shorter) {
        EarliestArrivalTime[d] = edge_end_time;
        Shortest[d] = newDist;

        // Mark visited.
        //
        // NOTE: here we're assuming only one edge from u to v.
        if (Visited[d] == 0) {
          Visited[d] = 1;
          return 1;
        }
      }
    }

    return false;
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

    intE newDist = Shortest[s] + edge_weight;

    return (writeMin(&EarliestArrivalTime[d], edge_end_time) &&
            writeMin(&Shortest[d], newDist) && CAS(&Visited[d], 0, 1));
  }

  // cond function always returns true
  inline bool cond(uintE d) { return cond_true(d); }
};

// reset visited vertices
struct Shortest_Vertex_F {
  int* Visited;
  Shortest_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator()(uintE i) {
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, long source_vertex_id, time_t start_time,
             time_t end_time) {
  long n = GA.n;

  // initialize Shortest to "infinity"
  intE* Shortest = newA(intE, n);
  { parallel_for(long i = 0; i < n; i++) Shortest[i] = INT_MAX / 2; }
  Shortest[source_vertex_id] = 0;

  // initialize EarliestArrivalTime to "infinity" for all but the source vertex.
  intE* EarliestArrivalTime = newA(intE, n);
  { parallel_for(long i = 0; i < n; i++) EarliestArrivalTime[i] = INT_MAX; }
  EarliestArrivalTime[source_vertex_id] = start_time;

  int* Visited = newA(int, n);
  { parallel_for(long i = 0; i < n; i++) Visited[i] = 0; }

  vertexSubset Frontier(n, source_vertex_id);  // initial frontier

  long round = 0;
  while (!Frontier.isEmpty()) {
    if (round == n) {
      // negative weight cycle
      { parallel_for(long i = 0; i < n; i++) Shortest[i] = -(INT_E_MAX / 2); }
      break;
    }
    vertexSubset output =
        edgeMap(GA, Frontier,
                Shortest_F(EarliestArrivalTime, Shortest, Visited,
                           source_vertex_id, start_time, end_time),
                GA.m / 20, dense_forward);
    vertexMap(output, Shortest_Vertex_F(Visited));
    Frontier.del();
    Frontier = output;
    round++;
  }
  /*
    // Print shortest path lengths from source vertex to all other vertices.
    std::cerr << "Shortest length paths:\n";
    std::cerr << "Query: [" << start_time << ", " << end_time << "]\n";
    for (long i = 0; i < n; i++) {
      std::cerr << source_vertex_id << " -> " << i << " = " << Shortest[i]
                << std::endl;
    }
  */
  Frontier.del();
  free(Visited);
  free(EarliestArrivalTime);
  free(Shortest);
}
/**** LIGRA TemporalPath Shortest ****/

int parallel_main(int argc, char* argv[]) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " input_file start_vertex start_time end_time" << std::endl;
    std::cerr << " input_file    - input temporal graph file (Ligra format)"
              << std::endl;
    // See temporal paths paper for algo definition:
    std::cerr << " source_vertex - vertex id of origin" << std::endl;
    std::cerr << " start_time    - start time of temporal path query"
              << std::endl;
    std::cerr << " end_time      - end time of temporal path query"
              << std::endl;

    return 0;
  }

  // Args needed for temporal version.
  uint64_t source_vertex_id = strtoull(argv[2], NULL, 10);
  time_t start_time = (time_t)strtoll(argv[3], NULL, 10);
  time_t end_time = (time_t)strtoll(argv[4], NULL, 10);

  // Read Ligra adjacency weighted temporal graph.
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
