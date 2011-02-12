CFLAGS=  -O3 -g -Wall -mssse3 -mfpmath=sse -I../lib
LIBS = -lm -lpthread -lgcc_s -lfftw3f -L../lib -lspect -lmpg123

TAGLIB_CFLAGS = `taglib-config --cflags`
TAGLIB_LIB = `taglib-config --libs`_c

FFTW_LIB = -lfftw3f

CC=gcc-4.4
CFLAGS += $(TAGLIB_CFLAGS)
LIBS+=$(TAGLIB_LIB)

SPECTGEN_OBJS=spectgen.o
SPECTGEN_EXE=spectgen

INSTALL_DIR = /usr/local/bin

default: spectgen

spectgen: $(SPECTGEN_OBJS)
	$(CC) -o $(SPECTGEN_EXE) $(LIBS) $(SPECTGEN_OBJS)

clean:
	rm -f $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS) $(TEST_KMEANS_OBJS) $(GABOR_OBJS)
	rm -f $(CLUSTER_EXE) $(TEST_KMEANS_EXE) $(GABOR_EXE)

#install:

#uninstall:

