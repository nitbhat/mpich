##
## Copyright (C) by Argonne National Laboratory
##     See COPYRIGHT in top-level directory
##

AM_CPPFLAGS = -I$(srcdir)/../include -I../include -I$(srcdir)/../dtpools/include @cuda_CPPFLAGS@ @ze_CPPFLAGS@ @hip_CPPFLAGS@

noinst_LTLIBRARIES = libmtest.la
libmtest_la_SOURCES = mtest.c mtest_common.c

noinst_LTLIBRARIES += libdtypes.la
libdtypes_la_SOURCES = dtypes.c

#---- CXX ---------------------------------------------------------------------
if HAS_CXX

## list a dummy library that we don't actually build in order to cause automake
## to emit a rule for building mtest_cxx.o from mtest_cxx.cxx
noinst_LTLIBRARIES += libmtest_cxx.la
libmtest_cxx_la_SOURCES = mtest_cxx.cxx mtest_common.c

endif

#---- F77 ---------------------------------------------------------------------
if HAS_F77

noinst_LTLIBRARIES += libmtest_f77.la
libmtest_f77_la_SOURCES = mtest_f77.f

endif

#---- F90 ---------------------------------------------------------------------
if HAS_F90

noinst_LTLIBRARIES += libmtest_f90.la
libmtest_f90_la_SOURCES = mtest_f90.f90

endif

#---- F08 ---------------------------------------------------------------------
if HAS_F08

noinst_LTLIBRARIES += libmtest_f08.la
libmtest_f08_la_SOURCES = mtest_f08.f90

endif

#---- single ---------------------------------------------------------------------
noinst_LTLIBRARIES += libmtest_single.la
libmtest_single_la_SOURCES = mtest_single.c
libmtest_single_la_LIBADD = libmtest.la

#---- thread ---------------------------------------------------------------------
noinst_LTLIBRARIES += libmtest_thread.la
libmtest_thread_la_SOURCES = mtest_thread.c
libmtest_thread_la_LIBADD = libmtest.la
