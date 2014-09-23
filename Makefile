ifdef LONG
INTT = -DLONG
endif

ifdef EDGELONG
INTE = -DEDGELONG
endif

# # no compare and swap!
# ifdef OPENMP
# PCC = g++
# PCFLAGS = -fopenmp -mcx16 -O3 -DOPENMP $(INTT) $(INTE)

ifdef CILK
PCC = g++
#-cilk
PCFLAGS = -fcilkplus -lcilkrts -O2 -DCILK $(INTT) $(INTE)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -O3 -DCILKP $(INTT) $(INTE)

else
PCC = g++
PCFLAGS = -O2 $(INTT) $(INTE)
endif

COMMON= ligra.h graph.h utils.h IO.h parallel.h gettime.h quickSort.h parseCommandLine.h

ALL= BFS BC Components Radii PageRank PageRankDelta BellmanFord BFSCC

all: $(ALL)

% : %.C $(COMMON)
	$(PCC) $(PCFLAGS) -o $@ $< 

.PHONY : clean

clean :
	rm -f *.o $(ALL)