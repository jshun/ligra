#define WEIGHTED_TEMPORAL 1

#include <string.h>
#include <cstdlib>
#include <iostream>
#include <vector>

#include "gettime.h"
#include "ligra_app.h"

// Global timers.
timer t0;

/**** LIGRA app start ****/
/**
 * Temporal shortest path: latest departure time, AKA "reverse foremost".
 *
 * See also: Ligra's Bellman-Ford shortest path implementation.
 */
struct LatestDeparture_F {
  intE* LatestDepartureTime;
  int* Visited;
  uintE QueryTargetVertexId;  // target vertex id for temporal query
  time_t QueryStartTime;      // start time of interval for temporal query
  time_t QueryEndTime;        // end time of interval for temporal query

  LatestDeparture_F(intE* _LatestDepartureTime, int* _Visited,
                    uintE _QueryTargetVertexId, time_t _QueryStartTime,
                    time_t _QueryEndTime)
      : LatestDepartureTime(_LatestDepartureTime),
        Visited(_Visited),
        QueryTargetVertexId(_QueryTargetVertexId),
        QueryStartTime(_QueryStartTime),
        QueryEndTime(_QueryEndTime) {}

  // update
  //
  // See Algo 2 from "Path Problems in Temporal Graphs".
  inline bool update(uintE s, uintE d, intE edge_weight, intE edge_start_time,
                     intE edge_end_time) {
    // Edge starts after before start of query interval.
    if (edge_start_time < QueryStartTime) {
      return 0;
    }

    // Update LatestDepartureTime path if found one with later departure that
    // meets temporal path constraints.
    bool is_unset = LatestDepartureTime[s] < 0;
    if (edge_end_time <= LatestDepartureTime[s] || is_unset) {
      // Path with later departure.
      if (edge_start_time > LatestDepartureTime[d]) {
        LatestDepartureTime[d] = edge_start_time;

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
  //
  // See Algo 2 of "Path Problems in Temporal Graphs".
  inline bool updateAtomic(uintE s, uintE d, intE edge_weight,
                           intE edge_start_time, intE edge_end_time) {
    if (edge_start_time < QueryStartTime) {
      return 0;
    }

    bool is_unset = LatestDepartureTime[s] < 0;
    if (!is_unset && edge_end_time > LatestDepartureTime[s]) {
      return 0;
    }

    if (edge_start_time <= LatestDepartureTime[d]) {
      return 0;
    }

    return (writeMax(&LatestDepartureTime[d], edge_start_time) &&
            CAS(&Visited[d], 0, 1));
  }

  // cond function always returns true
  inline bool cond(uintE d) { return cond_true(d); }
};

// reset visited vertices
struct LatestDeparture_Vertex_F {
  int* Visited;
  LatestDeparture_Vertex_F(int* _Visited) : Visited(_Visited) {}
  inline bool operator()(uintE i) {
    Visited[i] = 0;
    return 1;
  }
};

template <class vertex>
void Compute(graph<vertex>& GA, long target_vertex_id, time_t start_time,
             time_t end_time) {
  long n = GA.n;

  // initialize LatestDepartureTime to *negative* "infinity" for all but the
  // target vertex.
  intE* LatestDepartureTime = newA(intE, n);
  {
    parallel_for(long i = 0; i < n; i++) LatestDepartureTime[i] =
        -(INT_MAX / 2);
  }
  LatestDepartureTime[target_vertex_id] = end_time;

  int* Visited = newA(int, n);
  { parallel_for(long i = 0; i < n; i++) Visited[i] = 0; }

  vertexSubset Frontier(n, target_vertex_id);  // initial frontier

  while (!Frontier.isEmpty()) {
    vertexSubset output =
        edgeMap(GA, Frontier,
                LatestDeparture_F(LatestDepartureTime, Visited,
                                  target_vertex_id, start_time, end_time),
                GA.m / 20, dense_forward);
    vertexMap(output, LatestDeparture_Vertex_F(Visited));
    Frontier.del();
    Frontier = output;
  }
  /*
    // Print latest departure times to target vertex from all other vertices.
    std::cerr << "Latest Departure times:\n";
    std::cerr << "Query: [" << start_time << ", " << end_time << "]\n";
    for (long i = 0; i < n; i++) {
      std::cerr << i << " -> " << target_vertex_id << " = "
                << LatestDepartureTime[i] << std::endl;
    }
  */
  Frontier.del();
  free(Visited);
  free(LatestDepartureTime);
}
/**** LIGRA app end ****/

int parallel_main(int argc, char* argv[]) {
  if (argc != 5) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " input_file target_vertex start_time end_time" << std::endl;
    std::cerr << " input_file    - input temporal graph file (Ligra format)"
              << std::endl;
    // See temporal paths paper for algo definition:
    std::cerr << " target_vertex - vertex id of destination" << std::endl;
    std::cerr << " start_time    - start time of temporal path query"
              << std::endl;
    std::cerr << " end_time      - end time of temporal path query"
              << std::endl;

    return 0;
  }

  // Args needed for temporal path (latest departure time).
  uint64_t target_vertex_id = strtoull(argv[2], NULL, 10);
  time_t start_time = (time_t)strtoll(argv[3], NULL, 10);
  time_t end_time = (time_t)strtoll(argv[4], NULL, 10);

  std::cerr << std::endl << "Reading input file..." << std::endl;
  t0.start();
  graph<asymmetricVertex> G = readGraph<asymmetricVertex>(
      argv[1], false /* compressed */, false /* symmetric*/, false /* binary */,
      false /* mmap */);
  // NOTE: The "latest departure time" algo actually only works correctly if
  // edges are reversed, which is a crucial detail that seems to have been left
  // out of the "Path Problems in Temporal Graphs".
  G.transpose();
  t0.stop();

  std::cerr << std::endl;
  std::cerr << "num_vertices=" << G.n << std::endl;
  std::cerr << "num_edges=" << G.m << std::endl;
  std::cerr << "target_vertex=" << target_vertex_id << std::endl;
  std::cerr << "start_time=" << start_time << std::endl;
  std::cerr << "end_time=" << end_time << std::endl;
  t0.reportTotal("creation_time-ligra_csr");
  std::cerr << std::endl;

  startTime();
  Compute(G, target_vertex_id, start_time, end_time);
  nextTime("runtime");

  G.del();

  return 0;
}
