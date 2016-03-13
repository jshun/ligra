---
id: running_code
sectionid: docs
layout: docs
title: Running Code
prev: graph.html
next: examples.html
redirect_from: "docs/index.html"
---

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

``` bash
$ numactl -i all ./BFS -s <input file>
```

Running code on compressed graphs (Ligra+) 
-----------
In order to run applications with compression enabled, the input graph
must first be compressed using the encoder program provided. The encoder 
program takes as input a file in the format described in the next section, 
as well as an output file name. For symmetric graphs, the flag "-s" should 
be passed before the filenames, and for weighted graphs, the flag "-w" 
should be passed before the filenames. For example:

``` bash
$ ./encoder -s ../inputs/rMatGraph_J_5_100 inputs/rMatGraph_J_5_100.compressed
$ ./encoder -s -w ../inputs/rMatGraph_WJ_5_100 inputs/rMatGraph_WJ_5_100.compressed
```
 
After compressing the graphs, the applications can be run in the same
manner as on uncompressed graphs, but with an additional "-c"
flag. For example:

``` bash
$ ./BFS -s -c ../inputs/rMatGraph_J_5_100.compressed
$ ./BellmanFord -s -c ../inputs/rMatGraph_WJ_5_100.compressed
``` 

Make sure that the compression method used for compilation of the
applications is consistent with the method used to compress the graph
with the encoder program.
