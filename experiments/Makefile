CFLAGS=  -O3 -g -Wall -mssse3 -mfpmath=sse -I../utils
LIBS = -lm -lpthread -lgcc_s -lfftw3f

TAGLIB_CFLAGS = `taglib-config --cflags`
TAGLIB_LIB = `taglib-config --libs`_c

FFTW_LIB = -lfftw3f

CFLAGS += $(TAGLIB_CFLAGS)

KMEANS_OBJS = kmeans.o
HIST_OPS_OBJS = hist_ops.o
CLUSTER_OBJS = hist_cluster.o
GABOR_OBJS = gabor.o
EXT_OBJS = ../utils/hist.o ../utils/hist_dist.o ../utils/spect.o
TEST_KMEANS_OBJS = test_kmeans.o
GABOR_EXE = gabor

CLUSTER_EXE = cluster
TEST_KMEANS_EXE = test_kmeans

INSTALL_DIR = /usr/local/bin

default: test gabor

cluster: $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS) $(EXT_OBJS)
	$(CC)  -o $(CLUSTER_EXE)  $(LIBS) $(TAGLIB_LIB) $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS) $(EXT_OBJS)

test: $(KMEANS_OBJS) $(TEST_KMEANS_OBJS)
	$(CC) -o $(TEST_KMEANS_EXE) -lm $(KMEANS_OBJS) $(TEST_KMEANS_OBJS)

gabor: $(GABOR_OBJS)
	$(CC) -o $(GABOR_EXE) -lm -std=c99 $(TAGLIB_LIB) $(GABOR_OBJS)

clean:
	rm -f $(KMEANS_OBJS) $(HIST_OPS_OBJS) $(CLUSTER_OBJS) $(TEST_KMEANS_OBJS) $(GABOR_OBJS)
	rm -f $(CLUSTER_EXE) $(TEST_KMEANS_EXE) $(GABOR_EXE)

#install:

#uninstall:

