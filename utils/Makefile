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
PCFLAGS = -std=c++14 $(GXX_FLAGS) -fcilkplus -lcilkrts -O3 -DCILK $(INTT) $(INTE)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef CILKDBG
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -fcilkplus -lcilkrts -ggdb -DCILK $(INTT) $(INTE)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -std=c++14 $(GXX_FLAGS) -O3 -DCILKP $(INTT) $(INTE)

else ifdef OPENMP
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -fopenmp -march=native -O3 -DOPENMP $(INTT) $(INTE)

else
PCC = $(GXX)
PCFLAGS = -std=c++14 $(GXX_FLAGS) -O3 $(INTT) $(INTE)
endif

COMMON = utils.h parseCommandLine.h parallel.h quickSort.h blockRadixSort.h transpose.h
LOCAL_COMMON = graphIO.h
GENERATORS = rMatGraph gridGraph randLocalGraph SNAPtoAdj wghSNAPtoAdj wghTemporalSNAPtoAdj adjGraphAddWeights adjToBinary communityToHyperAdj hyperAdjToBinary adjHypergraphAddWeights randHypergraph KONECTtoHyperAdj KONECTtoClique communityToClique communityToMESH KONECTtoMESH

.PHONY: all clean
all: $(GENERATORS)

$(COMMON):
	ln -s ../ligra/$@ .

% : %.C $(COMMON) $(LOCAL_COMMON)
	$(PCC) $(PCFLAGS) -o $@ $<

clean :
	rm -f *.o $(GENERATORS)

cleansrc :
	make -s clean
	rm -f $(COMMON)
