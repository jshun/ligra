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
PCFLAGS = -O3 -DCILKP $(INTT) $(INTE)

else ifdef OPENMP
PCC = g++
PCFLAGS = -std=c++14 -fopenmp -march=native -O3 -DOPENMP $(INTT) $(INTE)

else
PCC = g++
PCFLAGS = -std=c++14 -O3 $(INTT) $(INTE)
endif

COMMON= ligra.h graph.h compressedVertex.h vertex.h utils.h IO.h parallel.h gettime.h index_map.h maybe.h sequence.h edgeMap_utils.h binary_search.h quickSort.h parseCommandLine.h byte.h byteRLE.h nibble.h byte-pd.h byteRLE-pd.h nibble-pd.h vertexSubset.h encoder.C
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
