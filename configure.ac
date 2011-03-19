dnl Process this file with autoconf to produce a configure script.

dnl define package name and version 
AC_INIT(libmysqlnb, 0.1, christian.betz@gmail.com)

dnl initialize automake
AM_INIT_AUTOMAKE

dnl Check for mysql_config
AC_ARG_WITH(mysql-config,
[  --with-mysql-config=PATH  the path to mysql-config if not in $PATH], [
	MYSQLCONFIG=$withval
], [
	MYSQLCONFIG=mysql-config-ac-arg-not-found
])

if test "x$MYSQLCONFIG" = "xmysql-config-ac-arg-not-found"; then
	AC_PATH_PROG( MYSQLCONFIG, mysql_config, mysql_config-not-found,
	              $PATH:/usr/bin:/usr/local/bin)
fi

AC_SUBST(MYSQLCONFIG)
if test "x$MYSQLCONFIG" = "xmysql_config-not-found"; then
	AC_MSG_ERROR([mysql_config wasn not found please install http://www.mysql.com ])
else
	MYSQL_CFLAGS=`$MYSQLCONFIG --cflags`
	MYSQL_INCLUDES=`$MYSQLCONFIG --include`
	MYSQL_LIBS=`$MYSQLCONFIG --libs_r`
fi
AC_SUBST(MYSQL_CFLAGS)
AC_SUBST(MYSQL_INCLUDES)
AC_SUBST(MYSQL_LIBS)

dnl Tell it we want to use the C compiler (duh)
AC_PROG_CC

dnl Per-target/Per-library CFLAGS/LIBS
AM_PROG_CC_C_O

dnl Yes, we want to use libtool
AC_PROG_LIBTOOL

dnl Support dynamically loading libs 
LT_INIT([dlopen])

dnl Tell it where the Makefiles are
AC_CONFIG_FILES([Makefile
                 libsrc/Makefile
                 src/Makefile
                 doc/Makefile])

dnl Suggested by autotools
AC_CONFIG_MACRO_DIR([m4])

dnl Finally dump the output
AC_OUTPUT
