#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Processor import Processor
import AST
from getoptions import get_options

import sys

def error(msg):
   """Write an error message and exit."""
   sys.stderr.write(msg)
   sys.stderr.write('\n')
   sys.exit(-1)

def process(argv = sys.argv, **commands):
   """Accept a set of commands and process according to command line options.
   The typical call will start with the name of the processor to be executed,
   followed by a set of parameters, followed by non-parameter arguments.
   All parameters are either of the form 'name=value', or '--name=value'.
   The first form expects 'value' to be valid python, the second a string.
   The remaining non-parameter arguments are associated with the 'input'
   parameter.
   Once this initialization is done, the named command's 'process' method
   is executed.
   """

   #first make sure the function was called with the correct argument types
   for c in commands:
      if not isinstance(commands[c], Processor):
         error("command '%s' isn't a valid processor"%c)

   if len(argv) < 2:
      error("Usage : %s <command> [args] [input files]"%argv[0])

   elif argv[1] == '--help':
      print "Usage: %s --help"%argv[0]
      print "   or: %s <command> --help"%argv[0]
      print "   or: %s <command> [parameters]"%argv[0]
      print ""
      print "Available commands:"
      for c in commands:
         print "   %s"%c
      sys.exit(0)

   command = argv[1]
   args = argv[2:]

   if '--help' in args:
      print "Parameters for command '%s'"%command
      parameters = commands[command].get_parameters()
      tab = max(map(lambda x:len(x), parameters.keys()))
      for p in parameters:
         print "   %-*s     %s"%(tab, p, parameters[p].doc)
      sys.exit(0)

   props = {}
   # process all option arguments...
   for o, a in get_options(args): props[o] = a

   # ...and keep remaining (non-option) arguments as 'input'
   if args: props['input'] = args

   if command in commands:
      ast = AST.AST()
      try:
         commands[command].process(ast, **props)
      except KeyError, e:
         error('missing argument "%s"'%e)
      except Processor.Error, e:
         error(str(e))
      except KeyboardInterrupt, e:
         print 'KeyboardInterrupt'
   else:
      error('no command "%s"'%command)

