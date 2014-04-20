AUTOMAKE_OPTIONS=foreign no-dependencies

INCLUDES = -I$(top_srcdir)/src \
	$(all_includes)



noinst_LIBRARIES = libalst.a

libalst_a_SOURCES = debug.c debug.h alarm.c alarm.h statistic.h statistic.c \
                    log.c log.h

alst_CFLAGS = -DDEBUG

