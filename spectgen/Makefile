CFLAGS=  -O3 -g -Wall -mssse3 -mfpmath=sse -I../lib
LIBS = -lm -lpthread -lgcc_s -lfftw3f -L../lib -lspect -lmpg123

TAGLIB_CFLAGS = `taglib-config --cflags`
TAGLIB_LIB = `taglib-config --libs`_c

FFTW_LIB = -lfftw3f

CC=gcc-4.4
CFLAGS += $(TAGLIB_CFLAGS)
LIBS+=$(TAGLIB_LIB)

DECODER_OBJS=decoder.o decoder_backend.o decoder_backend_mpg123.o queue.o decode_app.o
DECODER_EXE=decoder
SPECTGEN_OBJS=decoder.o decoder_backend.o decoder_backend_mpg123.o queue.o spectgen_2.o
SPECTGEN_EXE=spectgen
SPECTGEN2_OBJS=spectgen.o
SPECTGEN2_EXE=spectgen2

INSTALL_DIR = /usr/local/bin

default: spectgen decoder spectgen2

spectgen2: $(SPECTGEN2_OBJS)
	$(CC) -o $(SPECTGEN2_EXE) $(LIBS) $(SPECTGEN2_OBJS)

spectgen: $(SPECTGEN_OBJS)
	$(CC) -o $(SPECTGEN_EXE) $(LIBS) $(SPECTGEN_OBJS)

decoder: $(DECODER_OBJS)
	$(CC) -o $(DECODER_EXE) $(LIBS) $(DECODER_OBJS)

clean:
	rm -f $(DECODER_OBJS) $(DECODER_EXE)
	rm -f $(SPECTGEN_OBJS) $(SPECTGEN_EXE)
	rm -f $(SPECTGEN2_OBJS) $(SPECTGEN2_EXE)

#install:

#uninstall:

