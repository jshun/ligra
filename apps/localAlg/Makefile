ifdef LONG
INTT = -DLONG
endif

ifdef EDGELONG
INTE = -DEDGELONG
endif

#compilers
ifdef CILK
PCC = g++
PCFLAGS = -std=c++14 -fcilkplus -lcilkrts -O3 -DCILK $(INTT) $(INTE)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -std=c++14 -O3 -DCILKP $(INTT) $(INTE)

else ifdef OPENMP
PCC = g++
PCFLAGS = -std=c++14 -fopenmp -march=native -O3 -DOPENMP $(INTT) $(INTE)

else
PCC = g++
PCFLAGS = -std=c++14 -O3 $(INTT) $(INTE)
endif

COMMON= ligra.h graph.h compressedVertex.h vertex.h utils.h IO.h parallel.h gettime.h index_map.h maybe.h sequence.h edgeMap_utils.h binary_search.h quickSort.h parseCommandLine.h byte.h byteRLE.h nibble.h byte-pd.h byteRLE-pd.h nibble-pd.h vertexSubset.h encoder.C
LOCAL_COMMON = sweep.h sparseSet.h sampleSort.h
INTSORT= blockRadixSort.h transpose.h
SERIAL = ACL-Serial ACL-Serial-Naive ACL-Serial-Opt ACL-Serial-Opt-Naive HeatKernel-Serial HeatKernel-Randomized-Serial Nibble-Serial
PARALLEL = ACL-Sync-Local ACL-Sync-Local-Opt HeatKernel-Parallel HeatKernel-Randomized-Parallel Nibble-Parallel

all: $(SERIAL) $(PARALLEL)

ser: $(SERIAL)

par: $(PARALLEL)

% : %.C $(COMMON) $(LOCAL_COMMON) $(INTSORT)
	$(PCC) $(PCFLAGS) -o $@ $< 

$(COMMON):
	ln -s ../../ligra/$@ .

$(INTSORT):
	ln -s ../../utils/$@ .

.PHONY : clean

clean :
	rm -f *.o $(SERIAL) $(PARALLEL)

cleansrc :
	rm -f *.o $(SERIAL) $(PARALLEL)
	rm $(COMMON) $(INTSORT)
