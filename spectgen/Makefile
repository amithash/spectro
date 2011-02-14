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
SPECTGEN_OBJS=decoder.o decoder_backend.o decoder_backend_mpg123.o queue.o spectgen.o
SPECTGEN_EXE=spectgen

INSTALL_DIR = /usr/local/bin

default: spectgen decoder

spectgen: $(SPECTGEN_OBJS)
	$(CC) -o $(SPECTGEN_EXE) $(LIBS) $(SPECTGEN_OBJS)

decoder: $(DECODER_OBJS)
	$(CC) -o $(DECODER_EXE) $(LIBS) $(DECODER_OBJS)

clean:
	rm -f $(DECODER_OBJS) $(DECODER_EXE)
	rm -f $(SPECTGEN_OBJS) $(SPECTGEN_EXE)

install:
	cp -f $(SPECTGEN_EXE) $(INSTALL_DIR)
	cp -f $(DECODER_EXE) $(INSTALL_DIR)

uninstall:
	rm -f $(INSTALL_DIR)/$(SPECTGEN_EXE)
	rm -f $(INSTALL_DIR)/$(DECODER_EXE)

