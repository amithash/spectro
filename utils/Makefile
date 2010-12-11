
MEMORY_KB := `cat /proc/meminfo | grep "MemTotal:" | perl -e '$$x = <stdin>; if($$x =~ /(\d+)/){$$x=$$1;} print "$$x";'`

TAGLIB_CFLAGS = `taglib-config --cflags`
FFTW_LIB = -lfftw3f
TAGLIB_LIB = `taglib-config --libs`_c

CFLAGS=  -O3 -g -Wall -mssse3 -mfpmath=sse -I../lib $(TAGLIB_CFLAGS) -DMEMORY_SIZE=$(MEMORY_KB)
LIBS= -lm -lgcc_s $(TAGLIB_LIB) $(FFTW_LIB) -L../lib -lspect

SPECT2CSV_OBJS = spect2csv.o
SPECT2HIST_OBJS = spect2hist.o
GENSPECTDB_OBJS = genspectdb.o
HIST2PGM_OBJS = hist2pgm.o
GETSIMILAR_OBJS = getsimilar.o
BPM_OBJS = bpm.o
PLOTBPM_OBJS = plotbpm.o
PLOTHIST_OBJS = plothist.o
NORM_SPECT_OBJS = norm_spect.o

SPECT2CSV_EXE = spect2csv
GENSPECTDB_EXE = genspectdb
SPECT2HIST_EXE = spect2hist
HIST2PGM_EXE = hist2pgm
GETSIMILAR_EXE = getsimilar
PLOTBPM_EXE    = plotbpm
PLOTHIST_EXE = plothist
NORM_SPECT_EXE = normspectdb

INSTALL_DIR = /usr/local/bin


all: spect2csv genspectdb spect2hist hist2pgm getsimilar plotbpm plothist normspectdb

spect2csv: $(SPECT2CSV_OBJS)
	$(CC)  -o $(SPECT2CSV_EXE) $(LIBS) $(SPECT2CSV_OBJS)

genspectdb: $(GENSPECTDB_OBJS)
	$(CC)  -o $(GENSPECTDB_EXE)  $(LIBS) $(GENSPECTDB_OBJS)

normspectdb: $(NORM_SPECT_OBJS)
	$(CC)  -o $(NORM_SPECT_EXE)  $(LIBS) $(NORM_SPECT_OBJS)

spect2hist: $(SPECT2HIST_OBJS)
	$(CC)  -o $(SPECT2HIST_EXE)  $(LIBS) $(SPECT2HIST_OBJS)

hist2pgm: $(HIST2PGM_OBJS)
	$(CC)  -o $(HIST2PGM_EXE)  $(LIBS) $(HIST2PGM_OBJS)

getsimilar: $(GETSIMILAR_OBJS)
	$(CC)  -o $(GETSIMILAR_EXE)  $(LIBS) $(GETSIMILAR_OBJS)

plotbpm: $(BPM_OBJS) $(PLOTBPM_OBJS)
	$(CC) -o $(PLOTBPM_EXE) $(LIBS) $(PLOTBPM_OBJS) $(BPM_OBJS)

plothist: $(PLOTHIST_OBJS)
	$(CC) -o $(PLOTHIST_EXE) $(LIBS) $(PLOTHIST_OBJS)


clean:
	rm -f $(SPECT2CSV_EXE) $(GENSPECTDB_EXE) $(SPECT2HIST_EXE) $(HIST2PGM_EXE)
	rm -f $(GETSIMILAR_EXE) $(PLOTBPM_EXE) $(PLOTHIST_EXE) $(NORM_SPECT_EXE)
	rm -f $(SPECT2CSV_OBJS) $(GENSPECTDB_OBJS) $(SPECT2HIST_OBJS) $(BPM_OBJS)
	rm -f $(HIST2PGM_OBJS) $(GETSIMILAR_OBJS) $(PLOTBPM_OBJS)
	rm -f $(PLOTHIST_OBJS) $(NORM_SPECT_OBJS)

install:
	cp --target-directory $(INSTALL_DIR) $(SPECT2CSV_EXE) $(GENSPECTDB_EXE) $(SPECT2HIST_EXE) $(HIST2PGM_EXE) $(GETSIMILAR_EXE) $(NORM_SPECT_EXE)
uninstall:
	rm -f $(INSTALL_DIR)/$(SPECT2CSV_EXE)
	rm -f $(INSTALL_DIR)/$(GENSPECTDB_EXE)
	rm -f $(INSTALL_DIR)/$(SPECT2HIST_EXE)
	rm -f $(INSTALL_DIR)/$(HIST2PGM_EXE)
	rm -f $(INSTALL_DIR)/$(GETSIMILAR_EXE)
	rm -f $(INSTALL_DIR)/$(NORMSPECT_EXE)

