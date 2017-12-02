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
else ifdef STREAMVBYTE
CODE = -DSTREAMVBYTE
else ifdef STREAMVEC
CODE = -DSTREAMVEC
else ifdef STREAMCASE
CODE = -DSTREAMCASE
else ifdef BP
CODE = -DBP
else
CODE = -DBYTERLE
endif

ifdef DEBUG
CODE += -g
endif

ifdef LOWMEM
MEM = -DLOWMEM
endif

#compilers
ifdef CILK
PCC = g++
PCFLAGS = -std=c++14 -fcilkplus -lcilkrts -O2 -DCILK $(INTT) $(INTE) $(CODE) $(PD) $(MEM)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -std=c++14 -O3 -DCILKP $(INTT) $(INTE) $(CODE) $(PD) $(MEM)

else ifdef OPENMP
PCC = g++
PCFLAGS = -std=c++14 -fopenmp -O3 -DOPENMP $(INTT) $(INTE) $(CODE) $(PD) $(MEM)

else
PCC = g++
PCFLAGS = -std=c++14 -O2 $(INTT) $(INTE) $(CODE) $(PD) $(MEM)
endif

COMMON= ligra.h graph.h compressedVertex.h vertex.h utils.h IO.h parallel.h gettime.h index_map.h maybe.h sequence.h edgeMap_utils.h binary_search.h quickSort.h blockRadixSort.h transpose.h parseCommandLine.h byte.h byteRLE.h nibble.h streamvbyte.h byte-pd.h byteRLE-pd.h nibble-pd.h streamvbyte_vec.h streamvbyte_256.h vertexSubset.h streamvbyte_shuffle_tables.h encoder.C decoder.C
ALL= decoder encoder BFS BC BellmanFord Components Radii PageRank PageRankDelta BFSCC BFS-Bitvector KCore MIS Triangle CF decoderSpeed
all: $(ALL)

% : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -o $@ $<

$(COMMON):
	ln -s ../ligra/$@ .

.PHONY : clean

clean :
	rm -f *.o $(ALL)
#	git rm -f *.h $(COMMON)
#	git rm -f encoder.C 
#	git rm -f decoder.C
cleansrc :
	rm -f *.o $(ALL)
	rm $(COMMON)
