# $Id: process.py,v 1.1 2003/11/11 02:56:17 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Processor import Processor
from Core import AST

import sys

def error(msg):
   """Write an error message and exit."""
   sys.stderr.write(msg)
   sys.stderr.write('\n')
   sys.exit(-1)

def process(**commands):
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

   if len(sys.argv) < 2:
      error("Usage : %s <command> [args] [input files]'%sys.argv[0]")

   elif sys.argv[1] == '--help':
      print "Usage: %s --help"%sys.argv[0]
      print "   or: %s <command> --help"%sys.argv[0]
      print "   or: %s <command> [parameters]"%sys.argv[0]
      print ""
      print "Available commands:"
      for c in commands:
         print "   %s"%c
      sys.exit(0)

   command = sys.argv[1]
   args = sys.argv[2:]

   if '--help' in args:
      print "Parameters for command '%s'"%command
      parameters = commands[command].get_parameters()
      tab = max(map(lambda x:len(x), parameters.keys()))
      for p in parameters:
         print "   %-*s     %s"%(tab, p, parameters[p].doc)
      sys.exit(0)

   props = {}
   # process all option arguments (i.e. those containing a '='
   while args:
      arg = args[0]
      if arg.find('=') == -1 and not arg.startswith('--'):
         break
      attribute = arg.split('=', 1)
      if len(attribute) == 2:
         name, value = attribute
         if name.startswith('--'):
            props[name[2:]] = value # it's a string
         else:
            try:
               props[name] = eval(value) # it's a python expression
            except:
               error("""an error occured trying to evaluate the value of \'%s\' (\'%s\')
to pass this as a string, please use %s="'%s'" """%(name, value, name, value))
      else:
         name = attribute[0]
         if name.startswith('--'):
            props[name[2:]] = True # flag the attribute as 'set'
         else:
            # the nearest thing to 'no python expression'
            # is None...
            props[name] = None
      args = args[1:]

   # remaining arguments are mapped to the 'input' value,
   # if that is not yet defined
   if args and 'input' not in props:
      props['input'] = args

   if command in commands:
      ast = AST.AST()
      try:
         commands[command].process(ast, **props)
      except KeyError, e:
         error('missing argument "%s"'%e)
   else:
      error('no command "%s"'%command)

