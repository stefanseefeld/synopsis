# $Id: getoptions.py,v 1.1 2003/12/11 04:38:59 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from __future__ import generators

import sys

def getoptions(args):
   """provide an iterator over the options in args.
   All found options are stripped, such that args will
   contain the remainder, i.e. non-option arguments.
   The required format is '--option[=[arg]]' or 'option=arg'.
   In the former case the optional argument is interpreted as
   a string (only '--option' sets the value to True, '--option='
   sets it to the empty string), in the latter case the argument
   is evaluated as a python expression.
   """

   while args:
      arg = args[0]
      if arg.find('=') == -1 and not arg.startswith('--'):
         return # we are done
      attribute = arg.split('=', 1)
      if len(attribute) == 2:
         name, value = attribute
         if name.startswith('--'):
            name = name[2:] # value is a string
         else:
            try:
               value = eval(value) # it's a python expression
            except:
               sys.stderr.write("""an error occured trying to evaluate the value of \'%s\' (\'%s\')
to pass this as a string, please use %s="'%s'" \n"""%(name, value, name, value))
               sys.exit(-1)
      else:
         name, value = attribute[0][2:], True # flag the attribute as 'set'

      args[:] = args[1:]
      yield name, value
