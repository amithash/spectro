CFLAGS=  -O3 -g -Wall -mssse3 -mfpmath=sse -I../lib
LIBS = -lm -lpthread -lgcc_s -lfftw3f -L../lib -lspect

TAGLIB_CFLAGS = `taglib-config --cflags`
TAGLIB_LIB = `taglib-config --libs`_c

FFTW_LIB = -lfftw3f

CC=gcc-4.4
CFLAGS += $(TAGLIB_CFLAGS)
LIBS+=$(TAGLIB_LIB)

KMEANS_OBJS = kmeans.o
HIST_OPS_OBJS = hist_ops.o
CLUSTER_OBJS = hist_cluster.o
GABOR_OBJS = gabor.o
TEST_KMEANS_OBJS = test_kmeans.o
GABOR_EXE = gabor

CLUSTER_EXE = cluster
TEST_KMEANS_EXE = test_kmeans

INSTALL_DIR = /usr/local/bin

default: gabor test cluster

cluster: $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS)
	$(CC)  -o $(CLUSTER_EXE)  -lm $(LIBS) $(TAGLIB_LIB) $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS)

test: $(KMEANS_OBJS) $(TEST_KMEANS_OBJS)
	$(CC) -o $(TEST_KMEANS_EXE) -lm $(KMEANS_OBJS) $(TEST_KMEANS_OBJS)

gabor: $(GABOR_OBJS)
	$(CC) -o $(GABOR_EXE) -lm -std=c99 $(LIBS) $(GABOR_OBJS)

clean:
	rm -f $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS) $(TEST_KMEANS_OBJS) $(GABOR_OBJS)
	rm -f $(CLUSTER_EXE) $(TEST_KMEANS_EXE) $(GABOR_EXE)

#install:

#uninstall:

