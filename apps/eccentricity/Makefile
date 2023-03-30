ifdef LONG
INTT = -DLONG
endif

ifdef EDGELONG
INTE = -DEDGELONG
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

GXX_FLAGS=#-Wall -Wextra -Werror

ifdef CILK
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -fcilkplus -lcilkrts -O3 -DCILK $(INTT) $(INTE) $(CODE) $(PD) $(MEM)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef CILKDBG
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -fcilkplus -lcilkrts -ggdb -DCILK $(INTT) $(INTE) $(CODE) $(PD) $(MEM)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -std=c++14 $(GXX_FLAGS) -O3 -DCILKP $(INTT) $(INTE) $(CODE) $(PD) $(MEM)

else ifdef OPENMP
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -fopenmp -march=native -O3 -DOPENMP $(INTT) $(INTE) $(CODE) $(PD) $(MEM)

else
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -O3 $(INTT) $(INTE) $(CODE) $(PD) $(MEM)
endif

COMMON= ligra.h ligra_app.h ligra_app_main.h graph.h compressedVertex.h vertex.h utils.h IO.h parallel.h gettime.h index_map.h maybe.h sequence.h edgeMap_utils.h binary_search.h quickSort.h parseCommandLine.h byte.h byteRLE.h nibble.h byte-pd.h byteRLE-pd.h nibble-pd.h vertexSubset.h encoder.C
INTSORT= blockRadixSort.h transpose.h
ALL= kBFS-1Phase-Ecc kBFS-Ecc FM-Ecc LogLog-Ecc kBFS-Exact RV CLRSTV TK Simple-Approx-Ecc

all: $(ALL)

% : %.C $(COMMON) $(INTSORT) CCBFS.h
	$(PCC) $(PCFLAGS) -o $@ $< 

$(COMMON):
	ln -s ../../ligra/$@ .

$(INTSORT):
	ln -s ../../utils/$@ .

.PHONY : clean

clean :
	rm -f *.o $(ALL)

cleansrc :
	rm -f *.o $(ALL)
	rm $(COMMON) $(INTSORT)
