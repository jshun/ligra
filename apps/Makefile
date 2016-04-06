ifdef LONG
INTT = -DLONG
endif

ifdef EDGELONG
INTE = -DEDGELONG
endif

ifdef PD
PD = -DPD
endif

ifdef BYTE
CODE = -DBYTE
else ifdef NIBBLE
CODE = -DNIBBLE
else 
CODE = -DBYTERLE
endif

#compilers
ifdef CILK
PCC = g++
PCFLAGS = -fcilkplus -lcilkrts -O2 -DCILK $(INTT) $(INTE) $(CODE) $(PD)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -O3 -DCILKP $(INTT) $(INTE) $(CODE) $(PD)

else ifdef OPENMP
PCC = g++
PCFLAGS = -fopenmp -O3 -DOPENMP $(INTT) $(INTE) $(CODE) $(PD)

else
PCC = g++
PCFLAGS = -O2 $(INTT) $(INTE) $(CODE) $(PD)
endif

COMMON= ligra.h graph.h compressedVertex.h vertex.h utils.h IO.h parallel.h gettime.h quickSort.h blockRadixSort.h transpose.h parseCommandLine.h byte.h byteRLE.h nibble.h byte-pd.h byteRLE-pd.h nibble-pd.h vertexSubset.h encoder.C

ALL= encoder BFS BC BellmanFord Components Radii PageRank PageRankDelta BFSCC BFS-Bitvector KCore MIS Triangle CF

all: $(ALL)

% : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -o $@ $< 

$(COMMON):
	ln -s ../ligra/$@ .

.PHONY : clean

clean :
	rm -f *.o $(ALL)

cleansrc :
	rm -f *.o $(ALL)
	rm $(COMMON)
