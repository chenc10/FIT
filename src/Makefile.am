AUTOMAKE_OPTIONS=foreign
bin_PROGRAMS=TPTD

TPTD_SOURCES=tptd.c \
			 hash.c \
             libnids.c \
             util.c \
			 udp.c \
			 ip_options.c \
			 checksum.c \
			 scan.c \
			 control.c
INCLUDES = $(all_includes) -I/usr/local/include/glib-2.0 -I/usr/local/lib/glib-2.0/include

AM_LDFLAGS= -g -rdynamic
CFLAGS=@CFLAGS@ -DLIBNET_VER=@LIBNET_VER@ -DHAVE_ICMPHDR=@ICMPHEADER@ -DHAVE_TCP_STATES=@TCPSTATES@ -DHAVE_BSD_UDPHDR=@HAVE_BSD_UDPHDR@  -fopenmp 

TPTD_LDADD =  $(top_builddir)/src/wblist/libwblist.a $(top_builddir)/src/ipfragment/libipfragment.a \
              $(top_builddir)/src/fft/libfft.a $(top_builddir)/src/alst/libalst.a -lpcap -lnet -lm -lmysqlclient -ldl
SUBDIRS = wblist fft alst ipfragment




