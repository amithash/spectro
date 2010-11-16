CFLAGS=  -O3 -g -Wall -mssse3 -mfpmath=sse -I../utils
LIBS = -lm -lpthread -lgcc_s -lfftw3f

TAGLIB_CFLAGS = `taglib-config --cflags`
TAGLIB_LIB = `taglib-config --libs`_c

FFTW_LIB = -lfftw3f

CFLAGS += $(TAGLIB_CFLAGS)

KMEANS_OBJS = kmeans.o
HIST_OPS_OBJS = hist_ops.o
CLUSTER_OBJS = cluster.o
EXT_OBJS = ../utils/hist.o ../utils/hist_dist.o ../utils/spect.o ../utils/ceps.o

CLUSTER_EXE = cluster

INSTALL_DIR = /usr/local/bin

all: cluster

cluster: $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS) $(EXT_OBJS)
	$(CC)  -o $(CLUSTER_EXE)  $(LIBS) $(TAGLIB_LIB) $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS) $(EXT_OBJS)

clean:
	rm -f $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS)
	rm -f $(CLUSTER_EXE)

#install:

#uninstall:

