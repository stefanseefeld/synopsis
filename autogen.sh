#!/bin/sh
#

conf() 
{
  (cd $1
   echo "Generating $1/configure..."
   autoconf
  )
}

conf Synopsis/Parsers/Cpp
conf Synopsis/Parsers/C
conf Synopsis/Parsers/Cxx
conf Synopsis/Parsers/Cxx/gc
