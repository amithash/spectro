CFLAGS=  -O3 
LIBS = -lm -lpthread


SPECT_OBJS = spect.o
SPECT2CSV_OBJS = spect2csv.o
HIST_OBJS = hist.o
SPECT2HIST_OBJS = spect2hist.o
GENSPECTDB_OBJS = genspectdb.o
HIST2PGM_OBJS = hist2pgm.o
HIST_DIST_OBJS = hist_dist.o
GETSIMILAR_OBJS = getsimilar.o

all: spect2csv genspectdb spect2hist hist2pgm getsimilar
#getsimilar

spect2csv: $(SPECT_OBJS) $(SPECT2CSV_OBJS)
	$(CC)  -o spect2csv  $(LIBS) $(SPECT_OBJS) $(SPECT2CSV_OBJS)

genspectdb: $(SPECT_OBJS) $(GENSPECTDB_OBJS)
	$(CC)  -o genspectdb  $(LIBS) $(SPECT_OBJS) $(GENSPECTDB_OBJS)

spect2hist: $(SPECT_OBJS) $(HIST_OBJS) $(SPECT2HIST_OBJS)
	$(CC)  -o spect2hist  $(LIBS) $(SPECT_OBJS) $(HIST_OBJS) $(SPECT2HIST_OBJS)

hist2pgm: $(SPECT_OBJS) $(HIST_OBJS) $(HIST2PGM_OBJS)
	$(CC)  -o hist2pgm  $(LIBS) $(SPECT_OBJS) $(HIST_OBJS) $(HIST2PGM_OBJS)

getsimilar: $(SPECT_OBJS) $(HIST_OBJS) $(HIST_DIST_OBJS) $(GETSIMILAR_OBJS)
	$(CC)  -o getsimilar  $(LIBS) $(SPECT_OBJS) $(HIST_OBJS) $(HIST_DIST_OBJS) $(GETSIMILAR_OBJS)

clean:
	rm -f spect2csv $(SPECT2CSV_OBJS) $(SPECT_OBJS)
	rm -f genspectdb $(GENSPECTDB_OBJS)
	rm -f spect2hist $(SPECT2HIST_OBJS) $(HIST_OBJS)
	rm -f hist2pgm $(HIST2PGM_OBJS)
	rm -f getsimilar $(HIST_DIST_OBJS) $(GETSIMILAR_OBJS)

