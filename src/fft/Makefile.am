AUTOMAKE_OPTIONS=foreign no-dependencies

INCLUDES = -I$(top_srcdir)/src -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include

noinst_LIBRARIES = libfft.a

libfft_a_SOURCES = fudp.c fudp.h fft.c fft.h mytcp.c mytcp.h gre.h gre.c

AM_LDFLAGS= -DHAVE_TCP_STATES=1


