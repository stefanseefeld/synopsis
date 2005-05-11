#!/bin/sh
#

conf() 
{
  (cd $1
   echo "Generating $1/configure..."
   autoconf
  )
}

conf src
conf src/Synopsis/gc
conf Synopsis/Parsers/Cpp/ucpp
conf Synopsis/Parsers/C
conf Synopsis/Parsers/Cxx
conf tests
conf doc
conf contrib
