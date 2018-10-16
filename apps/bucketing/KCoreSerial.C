#include "ligra.h"

// Linear-work sequential implementation of the BZ coreness alg
// Modified code from https://github.com/athomo/kcore/blob/master/src/KCoreWG_BZ.java

struct decompress_f {
  intT* deg;
  intT* pos;
  intT* vert;
  intT* bin;
  decompress_f(intT* _deg, intT* _pos, intT* _vert, intT* _bin) : deg(_deg), pos(_pos), vert(_vert), bin(_bin) {}

  inline bool update (const uintE& v, const uintE& u) {
    if (deg[u] > deg[v]) {
      intT du = deg[u]; intT pu = pos[u];
      intT pw = bin[du]; intT w = vert[pw];
      if (u != w) {
        pos[u] = pw; vert[pu] = w;
        pos[w] = pu; vert[pw] = u;
      }
      bin[du]++;
      deg[u]--;
    }
    return false;
  }

  inline bool updateAtomic (uintE s, uintE d) {
    return update(s, d);
  }

  // Condition is checked in update.
  inline bool cond (uintE d) { return true; }
};

template <class vertex>
array_imap<intT> KCore(graph<vertex>& G, bool printCores=false) {
  size_t n = G.n, m = G.m;

  uintE md = 0;
  for(size_t v=0; v<n; v++) {
    md = max(md, (uintE)G.V[v].getOutDegree());
  }

  array_imap<intT> vert(n);
  array_imap<intT> pos(n);
  array_imap<intT> deg(n);
  array_imap<intT> bin(md+1);

  for (size_t d=0; d<=md; d++) {
    bin[d] = 0;
  }
  for (size_t v=0; v<n; v++) {
    intT d_v = G.V[v].getOutDegree();
    deg[v] = d_v;
    bin[d_v]++;
  }

  // Scan for positions.
  size_t start = 0;
  for (size_t d=0; d<=md; d++) {
    intT num = bin[d];
    bin[d] = start;
    start += num;
  }

  // Bin-sort.
  for (size_t v=0; v<n; v++) {
    pos[v] = bin[deg[v]];
    vert[pos[v]] = v;
    bin[deg[v]]++;
  }

  // Recover bin[].
  for (size_t d=md; d>=1; d--) {
    bin[d] = bin[d-1];
  }
  bin[0] = 0;

  auto df = decompress_f(deg.s, pos.s, vert.s, bin.s);

  // Main algorithm.
  auto vs = vertexSubset(n, (uintE)0);
  for (size_t i = 0; i < n; i++) {
    intT v = vert[i];  // Smallest degree vertex in the remaining graph.
    vs.s[0] = v;
    edgeMap(G, vs, df, -1, no_output | no_dense);
  }
  vs.del();

  if (printCores) {
    for (size_t i=0; i<n; i++) {
      cout << deg[i] << endl;
    }
  }
  size_t mc = 0;
  for (size_t i=0; i<n; i++) {
    mc = max(mc, (size_t)deg[i]);
  }
  cout << "### max core: " << mc << endl;

  return deg;
}

template <class vertex>
void Compute(graph<vertex>& GA, commandLine P) {
  bool printCores = P.getOptionValue("-p");
  cout << "### application: k-core-serial" << endl;
  cout << "### graph: " << P.getArgument(0) << endl;
  cout << "### workers: " << getWorkers() << endl;
  cout << "### n: " << GA.n << endl;
  cout << "### m: " << GA.m << endl;
  auto cores = KCore(GA, printCores);
}
