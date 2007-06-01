#!/bin/sh
#

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

conf src
conf src/Synopsis/gc
conf Synopsis/Parsers/Cpp/ucpp
conf_with_header Synopsis/Parsers/IDL
conf Synopsis/Parsers/C
conf Synopsis/Parsers/Cxx
conf tests
conf doc
conf sandbox
