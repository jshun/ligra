#define WEIGHTED_TEMPORAL 1

#include <cstdlib>
#include <iostream>
#include <vector>

#include "gettime.h"
#include "ligra_app.h"

typedef double fType;

// Global timers.
timer t0;

/**** LIGRA app start ****/
/**
 * TemporalCentrality: Betweeness using Fastest temporal paths.
 *
 * See: Ligra's Betweeness Centrality app (apps/BC.C) and temporal Fastest
 * (Fastest.C).
 */
struct BC_Fastest_F {
  intE* EarliestArrivalTime;
  intE* Fastest;
  fType* NumPaths;
  bool* Visited;
  // Temporal version.
  time_t QueryStartTime;
  time_t QueryEndTime;

  BC_Fastest_F(intE* _EarliestArrivalTime, intE* _Fastest, fType* _NumPaths,
               bool* _Visited, time_t _QueryStartTime, time_t _QueryEndTime)
      : EarliestArrivalTime(_EarliestArrivalTime),
        Fastest(_Fastest),
        NumPaths(_NumPaths),
        Visited(_Visited),
        QueryStartTime(_QueryStartTime),
        QueryEndTime(_QueryEndTime) {}

  // update function for forward phase
  //
  // From "Path Problems in Temporal Graphs", Section 6.1 - Temporal Closeness
  // Centrality": closeness centrality in the temporal setting can use different
  // variations of minimal temporal paths, including both shortest and fastest.
  // A similar definition makes sense for betweeness centrality as well.
  //
  // Here we have an implementation of betweeness centrality using *fastest*
  // temporal paths. See also Shortest.C for original implementation of temporal
  // shortest paths.
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

    if (edge_end_time > QueryEndTime) {
      return false;
    }

    // Temporal predicate constraint: next edge in the path needs to start
    // after previous one.
    if (edge_start_time < EarliestArrivalTime[s]) {
      return false;
    }

    // Does not arrive earlier.
    if (edge_end_time >= EarliestArrivalTime[d]) {
      return false;
    }

    // Is not faster.
    intE duration = edge_end_time - edge_start_time;
    intE newDuration = Fastest[s] + duration;
    if (newDuration >= Fastest[d]) {
      return false;
    }

    // At this point we know that the path arrives earlier *and* is shorter.
    EarliestArrivalTime[d] = edge_end_time;
    Fastest[d] = newDuration;

    // Update number of paths that cross by this vertex.
    fType oldV = NumPaths[d];
    NumPaths[d] += NumPaths[s];
    return oldV == 0.0;
  }

  // atomic version of update, basically an add
  inline bool updateAtomic(uintE s, uintE d, intE edge_weight,
                           intE edge_start_time, intE edge_end_time) {
    // Edge starts after end of query interval.
    if (edge_start_time >= QueryEndTime) {
      return false;
    }

    // Edge ends before start of the query interval.
    if (edge_end_time <= QueryStartTime) {
      return false;
    }

    if (edge_end_time > QueryEndTime) {
      return false;
    }

    // Temporal predicate constraint: next edge in the path needs to start
    // after previous one.
    if (edge_start_time < EarliestArrivalTime[s]) {
      return false;
    }

    // Does not arrive earlier.
    if (edge_end_time >= EarliestArrivalTime[d]) {
      return false;
    }

    // Update number of shortests paths that cross by this vertex.
    volatile fType oldV, newV;
    volatile intE duration, newDuration;
    do {
      duration = edge_end_time - edge_start_time;
      newDuration = Fastest[s] + duration;
      oldV = NumPaths[d];
      newV = oldV + NumPaths[s];
    } while (!(writeMin(&EarliestArrivalTime[d], edge_end_time) &&
               writeMin(&Fastest[d], newDuration) &&
               CAS(&NumPaths[d], oldV, newV)));
    return oldV == 0.0;
  }

  // condition: check if visited
  inline bool cond(uintE d) { return Visited[d] == 0; }
};

// Used in second phase of algorithm. See:
// https://iss.oden.utexas.edu/?p=projects/galois/analytics/dist-bc
struct BC_Fastest_Back_F {
  fType* Dependencies;
  bool* Visited;
  // Temporal version.
  time_t QueryStartTime;
  time_t QueryEndTime;

  BC_Fastest_Back_F(fType* _Dependencies, bool* _Visited,
                    time_t _QueryStartTime, time_t _QueryEndTime)
      : Dependencies(_Dependencies),
        Visited(_Visited),
        QueryStartTime(_QueryStartTime),
        QueryEndTime(_QueryEndTime) {}

  // update function for backwards phase
  inline bool update(uintE s, uintE d, intE edge_weight, intE edge_start_time,
                     intE edge_end_time) {
    // Update number of paths that cross by this vertex.
    fType oldV = Dependencies[d];
    Dependencies[d] += Dependencies[s];
    return oldV == 0.0;
  }

  // atomic version of update
  inline bool updateAtomic(uintE s, uintE d, intE edge_weight,
                           intE edge_start_time, intE edge_end_time) {
    // Update number of paths that cross by this vertex.
    volatile fType oldV, newV;
    do {
      oldV = Dependencies[d];
      newV = oldV + Dependencies[s];
    } while (!CAS(&Dependencies[d], oldV, newV));
    return oldV == 0.0;
  }

  // condition: check if visited
  inline bool cond(uintE d) { return Visited[d] == 0; }
};

// vertex map function to mark visited vertexSubset
struct BC_Fastest_Vertex_F {
  bool* Visited;
  BC_Fastest_Vertex_F(bool* _Visited) : Visited(_Visited) {}
  inline bool operator()(uintE i) {
    Visited[i] = 1;
    return 1;
  }
};

// vertex map function (used on backwards phase) to mark visited vertexSubset
// and add to Dependencies score
struct BC_Fastest_Back_Vertex_F {
  bool* Visited;
  fType *Dependencies, *inverseNumPaths;
  BC_Fastest_Back_Vertex_F(bool* _Visited, fType* _Dependencies,
                           fType* _inverseNumPaths)
      : Visited(_Visited),
        Dependencies(_Dependencies),
        inverseNumPaths(_inverseNumPaths) {}
  inline bool operator()(uintE i) {
    Visited[i] = 1;
    Dependencies[i] += inverseNumPaths[i];
    return 1;
  }
};

// Two-phase algorithm. See:
// https://iss.oden.utexas.edu/?p=projects/galois/analytics/dist-bc
template <class vertex>
void Compute(graph<vertex>& GA, long start_vertex_id, time_t start_time,
             time_t end_time) {
  long n = GA.n;

  // initialize Fastest to "infinity"
  intE* Fastest = newA(intE, n);
  { parallel_for(long i = 0; i < n; i++) Fastest[i] = INT_MAX; }
  Fastest[start_vertex_id] = 0;

  // initialize EarliestArrivalTime to "infinity" for all but the source vertex.
  intE* EarliestArrivalTime = newA(intE, n);
  { parallel_for(long i = 0; i < n; i++) EarliestArrivalTime[i] = INT_MAX; }
  EarliestArrivalTime[start_vertex_id] = start_time;

  fType* NumPaths = newA(fType, n);
  { parallel_for(long i = 0; i < n; i++) NumPaths[i] = 0.0; }
  NumPaths[start_vertex_id] = 1.0;

  bool* Visited = newA(bool, n);
  { parallel_for(long i = 0; i < n; i++) Visited[i] = 0; }
  Visited[start_vertex_id] = 1;
  vertexSubset Frontier(n, start_vertex_id);

  vector<vertexSubset> Levels;
  Levels.push_back(Frontier);

  long round = 0;
  while (!Frontier.isEmpty()) {  // first phase
    round++;
    vertexSubset output =
        edgeMap(GA, Frontier,
                BC_Fastest_F(EarliestArrivalTime, Fastest, NumPaths, Visited,
                             start_time, end_time));
    vertexMap(output, BC_Fastest_Vertex_F(Visited));  // mark visited
    Levels.push_back(output);  // save frontier onto Levels
    Frontier = output;
  }

  fType* Dependencies = newA(fType, n);
  { parallel_for(long i = 0; i < n; i++) Dependencies[i] = 0.0; }

  // invert numpaths
  fType* inverseNumPaths = NumPaths;
  {
    parallel_for(long i = 0; i < n; i++) inverseNumPaths[i] =
        1 / inverseNumPaths[i];
  }

  Levels[round].del();
  // reuse Visited
  { parallel_for(long i = 0; i < n; i++) Visited[i] = 0; }
  Frontier = Levels[round - 1];
  vertexMap(Frontier,
            BC_Fastest_Back_Vertex_F(Visited, Dependencies, inverseNumPaths));

  // tranpose graph
  GA.transpose();
  for (long r = round - 2; r >= 0; r--) {  // backwards phase
    edgeMap(GA, Frontier,
            BC_Fastest_Back_F(Dependencies, Visited, start_time, end_time), -1,
            no_output);
    Frontier.del();
    Frontier = Levels[r];  // gets frontier from Levels array
    // vertex map to mark visited and update Dependencies scores
    vertexMap(Frontier,
              BC_Fastest_Back_Vertex_F(Visited, Dependencies, inverseNumPaths));
  }

  Frontier.del();

  // Update dependencies scores
  parallel_for(long i = 0; i < n; i++) {
    Dependencies[i] =
        (Dependencies[i] - inverseNumPaths[i]) / inverseNumPaths[i];
  }

  // Print number of paths and dependencies:
  std::cerr << "Query: [" << start_time << ", " << end_time << "]\n";
  for (long i = 0; i < n; i++) {
    std::cerr << "Dependencies[" << i << "] = " << Dependencies[i]
              << ", NumPaths[" << i << "] =" << NumPaths[i] << std::endl;
  }

  free(inverseNumPaths);
  free(Visited);
  free(Dependencies);
}
/**** LIGRA app end ****/

int parallel_main(int argc, char* argv[]) {
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0];
    std::cerr << " input_file start_time end_time" << std::endl;
    std::cerr << " input_file    - input temporal graph file (Ligra format)"
              << std::endl;
    std::cerr << " start_time    - start time of temporal centrality query"
              << std::endl;
    std::cerr << " end_time      - end time of temporal centrality query"
              << std::endl;

    return 0;
  }

  std::cerr << std::endl << "Reading input file..." << std::endl;
  graph<symmetricVertex> G = readGraph<symmetricVertex>(
      argv[1], false /* compressed */, true /* symmetric*/, false /* binary */,
      false /* mmap */);

  // Args needed for temporal centrality.
  time_t start_time = (time_t)strtoll(argv[2], NULL, 10);
  time_t end_time = (time_t)strtoll(argv[3], NULL, 10);

  std::cerr << std::endl;
  std::cerr << "num_vertices=" << G.n << std::endl;
  std::cerr << "num_edges=" << G.m << std::endl;
  std::cerr << "start_time=" << start_time << std::endl;
  std::cerr << "end_time=" << end_time << std::endl;
  std::cerr << std::endl;

  startTime();
  Compute(G, 0 /*start_vertex_id*/, start_time, end_time);
  nextTime("runtime");

  G.del();

  return 0;
}
