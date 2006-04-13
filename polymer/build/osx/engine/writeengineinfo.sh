#!/bin/sh
UNAME=`uname -a`
CC="gcc"
CCVERSION=`$CC -dumpversion`
CCMACHINE=`$CC -dumpmachine`
echo "const char _engine_cflags[] = \"$OTHER_CFLAGS\";" > engineinfo.c
echo "const char _engine_libs[] = \"$LIBS\";" >> engineinfo.c
echo "const char _engine_uname[] = \"$UNAME\";" >> engineinfo.c
echo "const char _engine_compiler[] = \"$CC $CCVERSION $CCMACHINE\";" >> engineinfo.c
echo "const char _engine_date[] = __DATE__ \" \" __TIME__;" >> engineinfo.c
