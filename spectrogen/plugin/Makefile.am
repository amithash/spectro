
plugin_LTLIBRARIES = libspectgen.la

libspectgen_la_SOURCES = gstfftwspectrum.c gstfftwunspectrum.c \
			gstspectrumeq.c gstspectgen.c spectrum.c 

libspectgen_la_CFLAGS = $(GST_BASE_CFLAGS) $(GST_CFLAGS) $(FFTW_CFLAGS)
libspectgen_la_LIBADD = $(GST_BASE_LIBS) $(GST_LIBS) $(FFTW_LIBS)
libspectgen_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstfftwspectrum.h gstfftwunspectrum.h \
	         gstspectrumeq.h gstspectgen.h spectrum.h
