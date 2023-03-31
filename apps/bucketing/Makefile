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

#compiler
GXX=g++  # clang++

# ---- with local cilkrts build ----
ifdef CILKPLUS_HOME
CILK=1
CILKPLUS=${CILKPLUS_HOME}
GXX=clang++ -isystem $(CILKPLUS)/include/ -I$(CILKPLUS)/include/ -L$(CILKPLUS)
endif
# ---- with local cilkrts build ----

ifdef CILK
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -fcilkplus -lcilkrts -O3 -DCILK $(INTT) $(INTE) $(CODE) $(PD) $(FLAGS)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -std=c++14 $(GXX_FLAGS) -O3 -DCILKP $(INTT) $(INTE) $(CODE) $(PD)

else ifdef OPENMP
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -fopenmp -march=native -O3 -DOPENMP $(INTT) $(INTE) $(CODE) $(PD)

else
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -O3 $(INTT) $(INTE) $(CODE) $(PD)
endif

COMMON= ligra.h ligra_app.h ligra_app_main.h edgeMap_utils.h index_map.h sequence.h maybe.h binary_search.h graph.h compressedVertex.h vertex.h utils.h IO.h parallel.h gettime.h quickSort.h blockRadixSort.h transpose.h parseCommandLine.h byte.h byteRLE.h nibble.h byte-pd.h byteRLE-pd.h nibble-pd.h vertexSubset.h encoder.C bucket.h counting_sort.h dyn_arr.h edgeMapReduce.h histogram.h sequentialHT.h

ALL= DeltaStepping KCore KCoreSerial SetCover

all: $(ALL)

% : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -o $@ $<

$(COMMON):
	ln -s ../../ligra/$@ .

.PHONY : clean

clean :
	rm -f *.o $(ALL)

cleansrc :
	rm -f *.o $(ALL)
	rm $(COMMON)
