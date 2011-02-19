

MEMORY_KB := `cat /proc/meminfo | grep "MemTotal:" | perl -e '$$x = <stdin>; if($$x =~ /(\d+)/){$$x=$$1;} print "$$x";'`

TAGLIB_CFLAGS = `taglib-config --cflags`
TAGLIB_LIB = `taglib-config --libs`_c
CC=gcc-4.4

CFLAGS=  -O3 -g -Wall -mssse3 -mfpmath=sse -DMEMORY_SIZE=$(MEMORY_KB) $(TAGLIB_CFLAGS)
CFLAGS += -fPIC -g
LIBS = -lm -lpthread -lgcc_s -lfftw3f $(TAGLIB_LIB)

OBJS = file_vecio.o hist.o hist_dist.o spect_bounds.o spect.o plot.o

VERSION_MAJOR=0
VERSION_MINOR=0.1
LIB_NAME=spect
LIB_FULL_NAME=lib$(LIB_NAME).so.$(VERSION_MAJOR).$(VERSION_MINOR)
LIB_SONAME=lib$(LIB_NAME).so.$(VERSION_MAJOR)

# FIXME: Make this compatible for 32 bit system or non suse systems
INSTALL_DIR = /usr/local/lib

all: libspect
clean: libspect_clean
install: libspect_install
uninstall: libspect_uninstall


libspect: $(OBJS)
	$(CC) -shared $(LIBS) -Wl,-soname,$(LIB_SONAME) -o $(LIB_FULL_NAME) $(OBJS)
	ln -s $(LIB_FULL_NAME) libspect.so.0
	ln -s libspect.so.0 libspect.so

libspect_clean:
	rm -f file_vecio.o hist.o hist_dist.o spect_bounds.o spect.o plot.o
	rm -f libspect.so*

libspect_install:
	cp --target-dir $(INSTALL_DIR) $(LIB_FULL_NAME)
	/sbin/ldconfig $(INSTALL_DIR)

libspect_uninstall:
	rm -f $(INSTALL_DIR)/libspect*

