#!/bin/sh
#

record_revision()
{
  if test -n "`svn info 2> /dev/null`"
  then
    echo "Recording current revision..."
    rev=`svn info | awk '/Revision:/ {print $2}'`
    echo $rev > revision
  fi
}

conf() 
{
  (cd $1
   echo "Generating $1/configure..."
   aclocal -I `echo /$1 | sed 's,/[^\\/]*,../,g'`config
   autoconf
  )
}

conf_with_header()
{
  (cd $1
   echo "Generating $1/configure..."
   aclocal -I `echo /$1 | sed 's,/[^\\/]*,../,g'`config
   autoconf
   autoheader
  )
}

record_revision
conf src
conf src/Synopsis/gc
conf src/Synopsis/gc/libatomic_ops-1.2
conf Synopsis/Parsers/Cpp
conf_with_header Synopsis/Parsers/IDL
conf Synopsis/Parsers/C
conf Synopsis/Parsers/Cxx
conf tests
conf doc
conf sandbox
