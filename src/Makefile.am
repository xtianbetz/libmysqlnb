noinst_PROGRAMS = testselect
testselect_SOURCES = testselect.c
testselect_LDADD= $(top_srcdir)/libsrc/libmysqlnb.la \
                  $(MYSQL_LIBS)
testselect_CPPFLAGS = -I$(top_srcdir)/libsrc \
                       $(MYSQL_CFLAGS)
