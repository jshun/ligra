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
 * Temporal path: Fastest (smallest elapsed time).
 */
struct Fastest_F {
  intE* EarliestArrivalTime;
  intE* Fastest;
  int* Visited;
  uintE QuerySourceVertexId;  // source vertex id for temporal query
  time_t QueryStartTime;      // start time of interval for temporal query
  time_t QueryEndTime;        // end time of interval for temporal query

  Fastest_F(intE* _EarliestArrivalTime, intE* _Fastest, int* _Visited,
            uintE _QuerySourceVertexId, time_t _QueryStartTime,
            time_t _QueryEndTime)
      : EarliestArrivalTime(_EarliestArrivalTime),
        Fastest(_Fastest),
        Visited(_Visited),
        QuerySourceVertexId(_QuerySourceVertexId),
        QueryStartTime(_QueryStartTime),
        QueryEndTime(_QueryEndTime) {}

  // update
  //
  // From "Path Problems in Temporal Graphs", Lemma 9:
  //
  // "Let P be the set of temporal paths from s to d with the same starting time
  // t. Then, P ∈ P is a fastest path from s to d starting at t if P is an
  // earliest-arrival path from s to d starting at t."
  //
  // In other words, every fastest path is also an earliest arrival path.
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

    intE duration = edge_end_time - edge_start_time;
    intE newDuration = Fastest[s] + duration;

    // Temporal predicate constraint: next edge in the path needs to start
    // after previous one.
    if (edge_end_time <= QueryEndTime &&
        edge_start_time >= EarliestArrivalTime[s]) {
      // Update fastest path if constraint holds and this path has smaller
      // duration.
      if (edge_end_time < EarliestArrivalTime[d] && Fastest[d] > newDuration) {
        EarliestArrivalTime[d] = edge_end_time;
        Fastest[d] = newDuration;

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

    intE duration = edge_end_time - edge_start_time;
    intE newDuration = Fastest[s] + duration;

    return (writeMin(&EarliestArrivalTime[d], edge_end_time) &&
            writeMin(&Fastest[d], newDuration) && CAS(&Visited[d], 0, 1));
  }

  // cond function always returns true
  inline bool cond(uintE d) { return cond_true(d); }
};

// reset visited vertices
struct Fastest_Vertex_F {
  int* Visited;
  Fastest_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator()(uintE i) {
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, long source_vertex_id, time_t start_time,
             time_t end_time) {
  long n = GA.n;

  // initialize Fastest to "infinity"
  intE* Fastest = newA(intE, n);
  { parallel_for(long i = 0; i < n; i++) Fastest[i] = INT_MAX; }
  Fastest[source_vertex_id] = 0;

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
      { parallel_for(long i = 0; i < n; i++) Fastest[i] = -(INT_E_MAX / 2); }
      break;
    }
    vertexSubset output =
        edgeMap(GA, Frontier,
                Fastest_F(EarliestArrivalTime, Fastest, Visited,
                          source_vertex_id, start_time, end_time),
                GA.m / 20, dense_forward);
    vertexMap(output, Fastest_Vertex_F(Visited));
    Frontier.del();
    Frontier = output;
    round++;
  }

  // Print duration of fastest path from source vertex to all other vertices.
  std::cerr << "Fastest duration paths:\n";
  std::cerr << "Query: [" << start_time << ", " << end_time << "]\n";
  for (long i = 0; i < n; i++) {
    std::cerr << source_vertex_id << " -> " << i << " = " << Fastest[i]
              << std::endl;
  }

  Frontier.del();
  free(Visited);
  free(EarliestArrivalTime);
  free(Fastest);
}
/**** LIGRA Fastest ****/

int parallel_main(int argc, char* argv[]) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " input_file start_vertex start_time end_time" << std::endl;
    std::cerr << " input_file    - input temporal graph file (Ligra format)"
              << std::endl;
    // See temporal paths paper algo defintion:
    std::cerr << " source_vertex - vertex id of origin " << std::endl;
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
  t0.start();
  std::cerr << std::endl << "Reading input file..." << std::endl;
  graph<asymmetricVertex> G = readGraph<asymmetricVertex>(
      argv[1], false /* compressed */, false /* symmetric*/, false /* binary */,
      false /* mmap */);
  t0.stop();

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
