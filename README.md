Ligra: A Lightweight Graph Processing Framework for Shared Memory
======================

Organization
--------

The code for Ligra, Ligra+, and Hygra is located in the ligra/
directory.  The code for the applications is in the apps/ directory,
which is where compilation should be performed.  Example inputs are
provided in the inputs/ directory. Graph and hypergraph utilities are
provided in the utils/ directory.

Compilation
--------

Compilation is done from within the apps/ directory. The compiled code
will work on both uncompressed and compressed graphs and hypergraphs.

Compilers

* g++ &gt;= 5.3.0 with support for Cilk Plus
* g++ &gt;= 5.3.0 with OpenMP
* Intel icpc compiler

To compile with g++ using Cilk Plus, define the environment variable
CILK. To compile with icpc, define the environment variable MKLROOT
and make sure CILK is not defined.  To compile with OpenMP, define the
environment variable OPENMP and make sure CILK and MKLROOT are not
defined.  Using Cilk Plus seems to give the best parallel performance in
our experience.  To compile with g++ with no parallel support, make
sure CILK, MKLROOT and OPENMP are not defined.

Note: OpenMP support in Ligra has not been thoroughly tested. If you
experience any errors, please send an email to [Julian
Shun](mailto:jshun@mit.edu).

For processing compressed graph and hypergraph files, there are three
compression schemes currently implemented that can be used---byte
codes, byte codes with run-length encoding and nibble codes. By
default, the code is compiled for byte codes with run-length
encoding. To use byte codes instead, define the environment variable
BYTE, and to use nibble codes instead, define the environment variable
NIBBLE. Parallel decoding within a vertex can be enabled by defining
the environment variable PD (by default, a vertex's edge list is
decoded sequentially).

After the appropriate environment variables are set, to compile,
simply run

```
$ make -j  #compiles with all threads
```

The following commands cleans the directory:
```
$ make clean #removes all executables
$ make cleansrc #removes all executables and linked files from the ligra/ directory
```

Running code in Ligra
-------
The applications take the input graph as input as well as an optional
flag "-s" to indicate a symmetric graph.  Symmetric graphs should be
called with the "-s" flag for better performance. For example:

```
$ ./BFS -s ../inputs/rMatGraph_J_5_100
$ ./BellmanFord -s ../inputs/rMatGraph_WJ_5_100
``` 

For BFS, BC and BellmanFord, one can also pass the "-r" flag followed
by an integer to indicate the source vertex.  rMat graphs along with
other graphs can be generated with the graph generators in the utils/
directory.  By default, the applications are run four times, with
times reported for the last three runs. This can be changed by passing
the flag "-rounds" followed by an integer indicating the number of
timed runs.

On NUMA machines, adding the command "numactl -i all " when running
the program may improve performance for large graphs. For example:

```
$ numactl -i all ./BFS -s <input file>
```


Running code in Hygra
-------
The hypergraph applications are located in the apps/hyper/
directory. The applications take the input hypergraph as input as well
as an optional flag "-s" to indicate a symmetric hypergraph.
Symmetric hypergraphs should be called with the "-s" flag for better
performance. For example:

```
$ ./HyperBFS -s ../inputs/test
$ ./HyperSSSP -s ../inputs/test-wgh
``` 

For traversal algorithms, one can also pass the "-r" flag followed by
an integer to indicate the source vertex.  Random hypergraphs can be
generated with the hypergraph generator in the utils/ directory.


Running code on compressed graphs and hypergraphs (Ligra+) 
-----------
When using Ligra+, graphs and hypergraphs must first be compressed using the encoder
program provided (encoder and hypergraphEncoder). The encoder program takes as input a file in the
format described in the next section, as well as an output file
name. For symmetric graphs and hypergraphs, the flag "-s" should be passed before the
filenames, and for weighted graphs and hypergraphs, the flag "-w" should be passed
before the filenames. For example:

```
$ ./encoder -s ../inputs/rMatGraph_J_5_100 ../inputs/rMatGraph_J_5_100.compressed
$ ./encoder -s -w ../inputs/rMatGraph_WJ_5_100 ../inputs/rMatGraph_WJ_5_100.compressed
$ ./hypergraphEncoder -s ../inputs/test ../inputs/test.compressed
$ ./hypergraphEncoder -s -w ../inputs/test-wgh ../inputs/test-wgh.compressed
```
 
After compressing the inputs, the applications can be run in the same
manner as on uncompressed inputs, but with an additional "-c"
flag. For example:

```
$ ./BFS -s -c ../inputs/rMatGraph_J_5_100.compressed
$ ./BellmanFord -s -c ../inputs/rMatGraph_WJ_5_100.compressed
$ ./HyperBFS -s -c ../inputs/test.compressed
$ ./HyperSSSP -s -c ../inputs/test-wgh.compressed
``` 

Make sure that the compression method used for compilation of the
applications is consistent with the method used to compress the input
with the encoder program.


Input Format for Ligra applications
-----------
The input format of unweighted graphs should be in one of two
formats (the Ligra+ encoder currently only supports the first format).

1) The adjacency graph format from the Problem Based Benchmark Suite
 (http://www.cs.cmu.edu/~pbbs/benchmarks/graphIO.html). The adjacency
 graph format starts with a sequence of offsets one for each vertex,
 followed by a sequence of directed edges ordered by their source
 vertex. The offset for a vertex i refers to the location of the start
 of a contiguous block of out edges for vertex i in the sequence of
 edges. The block continues until the offset of the next vertex, or
 the end if i is the last vertex. All vertices and offsets are 0 based
 and represented in decimal. The specific format is as follows:

AdjacencyGraph  
&lt;n>  
&lt;m>  
&lt;o0>  
&lt;o1>  
...  
&lt;o(n-1)>  
&lt;e0>  
&lt;e1>  
...  
&lt;e(m-1)>  

This file is represented as plain text.

2) In binary format. This requires three files NAME.config, NAME.adj,
and NAME.idx, where NAME is chosen by the user. The .config file
stores the number of vertices in the graph in text format. The .idx
file stores in binary the offsets for the vertices in the CSR format
(the &lt;o>'s above). The .adj file stores in binary the edge targets in
the CSR format (the &lt;e>'s above).

Weighted graphs: For format (1), the weights are listed at the end of
the file (after &lt;e(m-1)>), and the first line of the file should
store the string "WeightedAdjacencyGraph". For format (2), the weights
are stored after all of the edge targets in the .adj file.

By default, format (1) is used. To run an input with format (2), pass
the "-b" flag as a command line argument.

By default the offsets are stored as 32-bit integers, and to represent
them as 64-bit integers, compile with the variable LONG defined. By
default the vertex IDs (edge values) are stored as 32-bit integers,
and to represent them as 64-bit integers, compile with the variable
EDGELONG defined.

Input Format for Hygra applications
-----------
The input can be in either adjacency hypergraph format or binary format, similar to graphs.

1) The adjacency hypergraph format starts with a sequence of offsets
 one for each vertex, followed by a sequence of incident hyperedges
 (the vertex is an incoming member of the hyperedge) ordered by
 vertex, followed by a sequence of offsets one for each hyperedge, and
 finally a sequence of incident vertices (the vertex is an outgoing
 member of the hyperedge) ordered by hyperedge.  All vertices,
 hyperedges, and offsets are 0 based and represented in decimal. For a
 graph with *nv* vertices, *mv* incident hyperedges for the vertices,
 *nh* hyperedges, and *mh* incident vertices for the hyperedges, the
 specific format is as follows:

AdjacencyHypergraph  
&lt;nv>		     
&lt;mv>		     
&lt;nh>		     
&lt;mh>		     
&lt;ov0>  
&lt;ov1>  
...  
&lt;ov(nv-1)>  
&lt;ev0>  
&lt;ev1>  
...  
&lt;ev(mv-1)>  
&lt;oh0>  
&lt;oh1>  
...  
&lt;oh(nh-1)>  
&lt;eh0>  
&lt;eh1>  
...  
&lt;eh(mh-1)>  

This file is represented as plain text.

2) In binary format. This requires five files NAME.config, NAME.vadj,
NAME.vidx, NAME.hadj, NAME.hidx, where NAME is chosen by the user. The
.config file stores *nv*, *mv*, *nh*, and *mh* in text format. The
.vidx and .hidx files store in binary the offsets for the vertices and
hyperedges (the &lt;ov>'s and &lt;oh>'s above). The .vadj and .hadj
files stores in binary the neighbors (the &lt;ev>'s and &lt;eh>'s
above).

Weighted hypergraphs: For format (1), the weights are listed as
another sequence following the sequence of neighbors for vertices or
hyperedges file (i.e., after &lt;ev(mv-1)> and &lt;eh(mh-1)>), and the
first line of the file should store the string
"WeightedAdjacencyHypergraph". For format (2), the weights are stored
after all of the edge targets in the .vadj and .hadj files.


Utilities
---------

Several utilities are provided in the utils/ directory and can
be compiled using "make".

### Graph Generators

Three graph generators from the [PBBS
project](http://www.cs.cmu.edu/~pbbs) are provided. **rMatGraph**
generators an rMat graph (described by Chakrabarti, Zhan and Faloutsos
in *SDM '04*). The required parameters are the number of vertices and
the output file. By default the number of directed edges is set to 10
times the number of vertices, and can be changed by specifying the
"-m" flag followed by the number of edges. The default parameters are
(*a=.5*, *b=.1*, *c=.1* and *d=.3*), and can be changed by specifying
the "-a", "-b", and "-c" flags each followed by the desired
probability (*d=1-a-b-c*). The "-r" flag followed by an integer
specifies the random seed (default value of 1).  **randLocalGraph**
generators a random graph, and the required parameters are the number
of vertices (10 times the number of edges are generated by default,
and can be changed with the "-m" flag) and the output file.
**gridGraph** takes the same parameters generates a 2 or 3 dimensional
graph, specified by the "-d" flag (default value is 2). The grid
graphs are symmetrized, and the rMat and random graphs are not
symmetrized but can be symmetrized by passing the "-s" flag.  For
graphs that are symmetrized, the total number of edges can be up to
twice the number specified.

Examples:
```
$ ./rMatGraph 10000000 rMat_10000000
$ ./rMatGraph -a .57 -b .19 -c .19 10000000 rMat_10000000 #modify rMat parameters
$ ./randLocalGraph 10000000 rand_10000000
$ ./randLocalGraph -m 50000000 10000000 rand_10000000 #modify edge count
$ ./gridGraph 10000000 2Dgrid_10000000
$ ./gridGraph -d 3 10000000 3Dgrid_10000000 
```

### Graph Converters

**SNAPtoAdj** converts a graph in [SNAP
format](http://snap.stanford.edu/data/index.html) and converts it to Ligra's
adjacency graph format. The first required parameter is the input
(SNAP) file name and second required parameter is the output (Ligra)
file name. The "-s" flag may be used to symmetrize the input
file. This converter works for any format that lists the two endpoints
of each edge separated by white space per line, with lines starting
with '#' ignored.

**adjGraphAddWeights** adds random integer weights in the range
[1,...,*log<sub>2</sub>*(number of vertices)] to an unweighted Ligra
graph in adjacency graph format, and takes as input the input file
name followed by the output file name.

**adjToBinary** converts a Ligra graph in adjacency graph format to
binary format. The arguments are the adjacency graph file name
followed by the 3 binary file names (.idx, .adj and .config). For a
weighted graph, pass the "-w" flag before the file names.

Examples:
```
$ ./SNAPtoAdj SNAPfile LigraFile
$ ./adjGraphAddWeights unweightedLigraFile weightedLigraFile
$ ./adjToBinary rMatGraph_J_5_100 rMatGraph_J_5_100.idx rMatGraph_J_5_100.adj rMatGraph_J_5_100.config 
$ ./adjToBinary -w rMatGraph_WJ_5_100 rMatGraph_WJ_5_100.idx rMatGraph_WJ_5_100.adj rMatGraph_WJ_5_100.config 
```

### Random Hypergraph Generator

The random hypergraph generator **randHypergraph** takes as input the
number of vertices ('-nv'), number of hyperdges ('-nh') and
cardinality of each hyperedge ('-c'). It generates a symmetric
hypergraph where each hyperedge has the specified cardinality and
where member vertices uniformly at random.

Examples:
```
$ ./randHypergraph -nv 100000000 -nh 100000000 -c 10 randHypergraph_output 
```

### Hypergraph Converters

**communityToHyperAdj** converts a communities network in [SNAP
format](http://snap.stanford.edu/data/index.html) and converts it to
symmetric adjacency hypergraph format. The first required parameter is
the input (SNAP) file name and second required parameter is the output
(Ligra) file name.

**KONECTtoHyperAdj** converts a bipartite graph in [KONECT
format](http://konect.uni-koblenz.de/) (the out.* file) and converts
it to symmetric adjacency hypergraph format. The first required
parameter is the input (KONECT) file name and second required
parameter is the output (Ligra) file name.

**adjHypergraphAddWeights** adds random integer weights in the range
[1,...,*log<sub>2</sub>*(max(number of vertices, number of
hyperedges))] to an unweighted hypergraph in adjacency hypergraph
format, and takes as input the input file name followed by the output
file name.

**hyperAdjToBinary** converts an input in adjacency hypergraph format
to binary format. The argument is the adjacency hypergraph file
name. For a weighted hypergraph, pass the "-w" flag before the file
name. The program will generate 5 output files with the input file
name followed by each of the prefixes .config, .vadj, .vidx, .hadj,
and .hidx.

Examples:
```
$ ./communityToHyperAdj SNAPfile HygraFile
$ ./KONECTtoHyperAdj KONECTfile Hygrafile
$ ./adjHypergraphAddWeights unweightedHygraFile weightedHygraFile
$ ./hyperAdjToBinary test
$ ./hyperAdjToBinary -w test-wgh
```

Ligra Data Structure and Functions
---------
### Data Structure

**vertexSubset**: represents a subset of vertices in the
graph. Various constructors are given in ligra.h

### Functions

**edgeMap**: takes as input 3 required arguments and 3 optional arguments:
a graph *G*, vertexSubset *V*, struct *F*, threshold argument
(optional, default threshold is *m*/20), an option in {DENSE,
DENSE_FORWARD} (optional, default value is DENSE), and a boolean
indicating whether to remove duplicates (optional, default does not
remove duplicates). It returns as output a vertexSubset Out
(see section 4 of paper for how Out is computed).

The *F* struct contains three boolean functions: update, updateAtomic
and cond.  update and updateAtomic should take two integer arguments
(corresponding to source and destination vertex). In addition,
updateAtomic should be atomic with respect to the destination
vertex. cond takes one argument corresponding to a vertex.  For the
cond function which always returns true, cond_true can be called. 

```
struct F {
  inline bool update (intT s, intT d) {
  //fill in
  }
  inline bool updateAtomic (intT s, intT d){ 
  //fill in
  }
  inline bool cond (intT d) {
  //fill in 
  }
};
```

The threshold argument determines when edgeMap switches between
edgemapSparse and edgemapDense---for a threshold value *T*, edgeMap
calls edgemapSparse if the vertex subset size plus its number of
outgoing edges is less than *T*, and otherwise calls edgemapDense.

DENSE and is a read-based version where all vertices not satisfying
Cond loop over their incoming edges and DENSE_FORWARD is a write-based
version where each frontier vertex loops over its outgoing edges. This
optimization is described in Section 4 of the paper.

Note that duplicate removal can only be avoided if updateAtomic
returns true at most once for each vertex in a call to edgeMap.

**vertexMap**: takes as input 2 arguments: a vertexSubset *V* and a
function *F* which is applied to all vertices in *V*. It does not have
a return value.

**vertexFilter**: takes as input a vertexSubset *V* and a boolean
function *F* which is applied to all vertices in *V*. It returns a
vertexSubset containing all vertices *v* in *V* such that *F(v)*
returns true.

```
struct F {
  inline bool operator () (intT i) {
  //fill in
  }
};
```

To write your own Ligra code, it would be helpful to look at the code
for the provided applications as reference.

Currently the results of the computation are not used, but the code
can be easily modified to output the results to a file.

To develop a new implementation, simply include "ligra.h" in the
implementation files. When finished, one may add it to the ALL
variable in Makefile. The function that is passed to the Ligra/Ligra+
driver is the following Compute function, which is filled in by the
user. The first argument is the graph, and second argument is a
structure containing the command line arguments, which can be
extracted using routines in parseCommandLine.h, or manually from
P.argv and P.argc.

```
template<class vertex>
void Compute(graph<vertex>& GA, commandLine P){ 

}
```

For weighted graph applications, add "#define WEIGHTED 1" before
including ligra.h.

To write a parallel for loop in your code, simply use the parallel_for
construct in place of "for".

Graph Applications
---------
Implementation files are provided in the apps/ directory: **BFS.C**
(breadth-first search), **BFS-Bitvector.C** (breadth-first search with
a bitvector to mark visited vertices), **BC.C** (betweenness
centrality), **Radii.C** (graph eccentricity estimation),
**Components.C** (connected components), **BellmanFord.C**
(Bellman-Ford shortest paths), **PageRank.C**, **PageRankDelta.C**,
**BFSCC.C** (connected components based on BFS), **MIS.C** (maximal
independent set), **KCore.C** (K-core decomposition), **Triangle.C**
(triangle counting), and **CF.C** (collaborative filtering).


Eccentricity Estimation 
-------- 
Code for eccentricity estimation is available in the
apps/eccentricity/ directory: **kBFS-Ecc.C** (2 passes of multiple
BFS's), **kBFS-1Phase-Ecc.C** (1 pass of multiple BFS's), **FM-Ecc.C**
(estimation using Flajolet-Martin counters; an implementation of a
variant of HADI from *TKDD '11*), **LogLog-Ecc.C** (estimation using
LogLog counters; an implementation of a variant of HyperANF from *WWW
'11*), **RV.C** (a parallel implementation of the algorithm by Roditty
and Vassilevska Williams from *STOC '13*), **CLRSTV.C** (a parallel
implementation of a variant of the algorithm by Chechik, Larkin,
Roditty, Schoenebeck, Tarjan, and Vassilevska Williams from *SODA
'14*), **kBFS-Exact.C** (exact algorithm using multiple BFS's),
**TK.C** (a parallel implementation of the exact algorithm by Takes
and Kosters from *Algorithms '13*), **Simple-Approx-Ecc.C** (simple
2-approximation algorithm).  Follow the same instructions as above for
compilation, but from the apps/eccentricity/ directory.

For kBFS-Ecc.C, kBFS-1Phase-Ecc.C, FM-Ecc.C, LogLog-Ecc.C, and
kBFS-Exact.C, the "-r" flag followed by an integer indicates the
maximum number of words to associate with each vertex. For all
implementations, the "-s" flag should be used as the current
implementations are designed for undirected graphs.  To output the
eccentricity estimates to a file, use the "-out" flag followed by the
name of the output file. The file format is one integer per line, with
the eccentricity estimate for vertex *i* on line *i*.

Hypergraph Applications
---------
Implementation files are provided in the apps/ directory:
**HyperBFS.C** (hypertree), **HyperBPath.C** (hyperpaths),
**HyperBC.C** (betweenness centrality), **HyperCC.C** (connected
components), **HyperSSSP.C** (shortest paths), **HyperPageRank.C**
(PageRank), **HyperMIS.C** (maximal independent set), **HyperKCore.C**
(work-inefficient K-core decomposition), and
**HyperKCore-Efficient.C** (work-efficient K-core decomposition).

Resources  
-------- 
Julian Shun and Guy E. Blelloch. [*Ligra: A
Lightweight Graph Processing Framework for Shared
Memory*](https://people.csail.mit.edu/jshun/ligra.pdf). Proceedings of the
ACM SIGPLAN Symposium on Principles and Practice of Parallel
Programming (PPoPP), pp. 135-146, 2013.

Julian Shun, Laxman Dhulipala, and Guy E. Blelloch. [*Smaller and Faster:
Parallel Processing of Compressed Graphs with
Ligra+*](http://people.csail.mit.edu/jshun/ligra+.pdf). Proceedings of the
IEEE Data Compression Conference (DCC), pp. 403-412, 2015.

Julian Shun. [*An Evaluation of Parallel Eccentricity Estimation
Algorithms on Undirected Real-World
Graphs*](http://people.csail.mit.edu/jshun/kdd-final.pdf). Proceedings of
the ACM SIGKDD Conference on Knowledge Discovery and Data Mining
(KDD), pp. 1095-1104, 2015.

Julian Shun, Farbod Roosta-Khorasani, Kimon Fountoulakis, and Michael
W.  Mahoney. [*Parallel Local Graph
Clustering*](http://people.csail.mit.edu/jshun/local.pdf). Proceedings
of the International Conference on Very Large Data Bases (VLDB),
9(12), pp. 1041-1052, 2016.

Laxman Dhulipala, Guy E. Blelloch, and Julian Shun. [*Julienne: A
Framework for Parallel Graph Algorithms using Work-efficient
Bucketing*](https://people.csail.mit.edu/jshun/bucketing.pdf). Proceedings
of the ACM Symposium on Parallelism in Algorithms and Architectures
(SPAA), pp. 293-304, 2017.

Julian Shun. [*Practical Parallel Hypergraph
Algorithms*](https://people.csail.mit.edu/jshun/hygra.pdf). Proceedings
of the ACM SIGPLAN Symposium on Principles and Practice of Parallel
Programming (PPoPP), pp. 232-249, 2020.
