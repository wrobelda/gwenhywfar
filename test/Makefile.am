AM_CPPFLAGS= -I$(gwenhywfar_symlinkdir) \
 $(LIBGNUTLS_CFLAGS) $(GPG_ERROR_CFLAGS) \
 -I$(top_srcdir)/src/base \
 -I$(top_srcdir)/src/crypt \
 -I$(top_srcdir)/src/io \
 -I$(top_srcdir)/src/ipc \
 -I$(top_srcdir)/src/net2 \
 -I$(top_srcdir)/src/os \
 -I$(top_srcdir)/src/parser @all_includes@

noinst_HEADERS=\
  testthread.h \
  testthread_p.h


noinst_PROGRAMS=gwentest
gwentest_SOURCES=gwentest.c testthread.c
gwentest_LDADD=$(top_builddir)/src/libgwenhywfar.la \
  $(LIBGCRYPT_LIBS) $(LIBGNUTLS_LIBS) $(GPG_ERROR_LIBS) \
  $(top_builddir)/src/base/libbase.la

EXTRA_DIST = README.MSVC gwentest.vcproj testdialog.xml gwentest-run.sh

# "make installcheck" runs the gwentest commands that "gwentest --list"
# shows as needing no arguments, or only optional ones, and as neither
# interactive nor tools. They run after installation, because some of
# them load the installed plugins, and one at a time in the order of the
# list, because some of them read the files that earlier ones write. Each
# command is one test of the Automake test harness, which writes its log
# to gwentest-logs/.
.NOTPARALLEL:

TESTS =
# "make check" has no tests here, so it writes no summary.
TEST_SUITE_LOG =
TEST_EXTENSIONS = .gwt
GWT_LOG_COMPILER = $(SHELL) $(srcdir)/gwentest-run.sh
AM_GWT_LOG_FLAGS = $(abs_builddir)/gwentest$(EXEEXT)

installcheck-local:
	rm -rf gwentest-logs
	$(MKDIR_P) gwentest-logs/run
	./gwentest$(EXEEXT) --list >gwentest-logs/list
	tests=`tr -d '\r' <gwentest-logs/list | \
	  $(AWK) -F '\t' '$$2 != "required" && $$3 != "interactive" && \
	    $$3 != "tool" { print "gwentest-logs/" $$1 ".gwt" }'` && \
	for t in $$tests; do : >$$t; done && \
	$(MAKE) $(AM_MAKEFLAGS) check-TESTS TESTS="$$tests" \
	  TEST_SUITE_LOG=gwentest-logs/test-suite.log

clean-local:
	rm -rf gwentest-logs

sources:
