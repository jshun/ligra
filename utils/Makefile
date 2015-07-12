ifdef LONG
INTT = -DLONG
endif

ifdef EDGELONG
INTE = -DEDGELONG
endif

#compilers
ifdef CILK
PCC = g++
PCFLAGS = -fcilkplus -lcilkrts -O2 -DCILK $(INTT) $(INTE)
PLFLAGS = -fcilkplus -lcilkrts

else ifdef MKLROOT
PCC = icpc
PCFLAGS = -O3 -DCILKP $(INTT) $(INTE)

else ifdef OPENMP
PCC = g++
PCFLAGS = -fopenmp -O3 -DOPENMP $(INTT) $(INTE)

else
PCC = g++
PCFLAGS = -O2 $(INTT) $(INTE)
endif

COMMON = utils.h parseCommandLine.h parallel.h quickSort.h
LOCAL_COMMON = blockRadixSort.h transpose.h graphIO.h
GENERATORS = rMatGraph gridGraph randLocalGraph SNAPtoAdj adjGraphAddWeights adjToBinary adjWghToBinary

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