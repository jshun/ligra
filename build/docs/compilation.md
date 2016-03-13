---
id: compilation
sectionid: docs
layout: docs
title: Compilation
next: introduction.html
redirect_from: "docs/index.html"
---

Compilation is done from within the apps/ directory. The code can be
compiled for either Ligra or Ligra+. There are two Makefiles provided
in the directory (Makefile.ligra and Makefile.ligra+), one for Ligra
and one for Ligra+. The file for the desired backend should be
linked/copied into a file named "Makefile". For example:

```
$ ln -s Makefile.ligra Makefile #if using Ligra
$ ln -s Makefile.ligra+ Makefile #if using Ligra+
```

Compilers

* Intel icpc compiler
* g++ &gt;= 4.8.0 with support for Cilk+, 
* OpenMP

To compile with g++ using Cilk, define the environment variable
CILK. To compile with icpc, define the environment variable MKLROOT
and make sure CILK is not defined.  To compile with OpenMP, define the
environment variable OPENMP and make sure CILK and MKLROOT are not
defined.  Using Cilk+ seems to give the best parallel performance in
our experience.  To compile with g++ with no parallel support, make
sure CILK, MKLROOT and OPENMP are not defined.

Note: OpenMP support in Ligra has not been thoroughly tested. If you
experience any errors, please send an email to [Julian
Shun](mailto:jshun@cs.cmu.edu). A known issue is that OpenMP will not
work correctly when using the experimental version of gcc 4.8.0.

If Ligra+ is used, there are three compression schemes currently
implemented that can be used---byte codes, byte codes with run-length
encoding and nibble codes. By default, the code is compiled for byte
codes with run-length encoding. To use byte codes instead, define the
environment variable BYTE, and to use nibble codes instead, define the
environment variable NIBBLE. Parallel decoding within a vertex can be
enabled by defining the environment variable PD (by default, a
vertex's edge list is decoded sequentially).

After the appropriate environment variables are set, to compile,
simply run

```
$ make -j 16  #compiles with 16 threads (thread count can be changed)
```

The following commands cleans the directory:
```
$ make clean #removes all executables
$ make cleansrc #removes all executables and linked files from the ligra/ or ligra+/ directory


Running code in Ligra
-------
The applications take the input graph as input as well as an optional
flag "-s" to indicate a symmetric graph.  Symmetric graphs should be
called with the "-s" flag for better performance. For example:

``` bash
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
