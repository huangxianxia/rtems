## Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([rtems],[_RTEMS_VERSION],[https://devel.rtems.org/newticket])
AC_CONFIG_SRCDIR([c])
RTEMS_TOP([.])
RTEMS_SOURCE_TOP
RTEMS_BUILD_TOP

# Abort if trying to build inside of the source tree.
AS_IF([test -f aclocal/version.m4],[
  rm -f config.cache config.log confdefs.h
  AC_MSG_ERROR([***]
    [Attempt to build inside of the source tree]
    [Please use a separate build directory, instead] )
])

AC_CANONICAL_TARGET([])
AM_INIT_AUTOMAKE([no-define foreign dist-bzip2 no-dist-gzip subdir-objects 1.12.2])
AM_MAINTAINER_MODE

## These option are only in here to let --help report all supported
## options.
RTEMS_ENABLE_RTEMSBSP
RTEMS_ENABLE_MULTIPROCESSING
RTEMS_ENABLE_SMP
RTEMS_ENABLE_POSIX
RTEMS_ENABLE_NETWORKING
RTEMS_ENABLE_CXX
RTEMS_ENABLE_TESTS
RTEMS_ENABLE_RTEMS_DEBUG
RTEMS_ENABLE_PARAVIRT
RTEMS_ENABLE_DRVMGR

## Check if there are valid tools install and in the path.
RTEMS_CHECK_TOOL(RTEMS_CC,gcc)
test -z "$RTEMS_CC" && AC_MSG_ERROR([no acceptable cc found in \$PATH])

## NOTES:
## * tools/build are host-native tools to be installed on the host.
## * tools/cpu are host-native or host-cross-target-tools

RTEMS_BUILD_CONFIG_SUBDIRS([tools/build])
RTEMS_BUILD_CONFIG_SUBDIRS([tools/cpu])

# Some BSPs carelessly apply these tools as build-tools
AS_IF([test $host != $build],[
RTEMS_HOST_CONFIG_SUBDIRS([tools/build])
])

RTEMS_TARGET_CONFIG_SUBDIRS([c])

AC_CONFIG_FILES([Makefile])

AC_OUTPUT
