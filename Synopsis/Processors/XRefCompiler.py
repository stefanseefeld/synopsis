# $Id: XRefCompiler.py,v 1.9 2003/12/08 03:34:13 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST, Type, Util

import os.path, string, cPickle, urllib

class XRefCompiler(Processor):
   """This class compiles a set of text-based xref files from the C++ parser
   into a cPickled data structure with a name index.
   
   The format of the data structure is:
   <pre>
   (data, index) = cPickle.load()
   data = dict<scoped targetname, target_data>
   index = dict<name, list<scoped targetname>>
   target_data = (definitions = list<target_info>,
   func calls = list<target_info>,
   references = list<target_info>)
   target_info = (filename, int(line number), scoped context name)
   </pre>
   The scoped targetnames in the index are guaranteed to exist in the data
   dictionary.
   """

   prefix = Parameter('', 'where to look for xref files')
   no_locals = Parameter(True, '')

   def process(self, ast, **kwds):
      
      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      filenames = map(lambda x: x[0], 
                      filter(lambda x: x[1].is_main(), ast.files().items()))
      filenames = map(lambda x:os.path.join(self.prefix, x), filenames)
      self.do_compile(filenames, self.output, self.no_locals)

      return self.ast

   def do_compile(self, input_files, output, no_locals):

      if not output:
         if self.verbose: print "XRefCompiler: no output given"
         return

      # Init data structures
      data = {}
      index = {}
      file_data = (data, index)

      # Read input files
      for file in input_files:
         if self.verbose: print "XRefCompiler: Reading",file
         try:
            f = open(file, 'rt')
         except IOError, e:
            if self.verbose: print "Error opening %s: %s. Skipping."%(file, e)
            continue

         lines = f.readlines()
         f.close()

         for line in lines:
            target, file, line, scope, context = string.split(line)
            target = tuple(map(intern, string.split(urllib.unquote(target), '\t')))
            scope = map(intern, string.split(urllib.unquote(scope), '\t'))
            line = int(line)
            file = intern(file)

            if no_locals:
               bad = 0
               for i in range(len(target)):
                  if len(target[i]) > 0 and target[i][0] == '`':
                     # Don't store local function variables
                     bad = 1
                     break
               if bad: continue

               for i in range(len(scope)):
                  if len(scope[i]) > 0 and scope[i][0] == '`':
                     # Function scope, truncate here
                     del scope[i+1:]
                     scope[i] = scope[i][1:]
                     break
            scope = tuple(scope)

            target_data = data.setdefault(target, [[],[],[]])
            if context == 'DEF':
               target_data[0].append( (file, line, scope) )
            elif context == 'CALL':
               target_data[1].append( (file, line, scope) )
            elif context == 'REF':
               target_data[2].append( (file, line, scope) )
            else:
               print "Warning: Unknown context:",context

      # Sort the data
      for target, target_data in data.items():
         target_data[1].sort()
         target_data[2].sort()

         name = target[-1]
         index.setdefault(name,[]).append(target)
         # If it's a function name, also add without the parameters
         paren = name.find('(')
         if paren != -1:
            index.setdefault(name[:paren],[]).append(target)

      if self.verbose: print "XRefCompiler: Writing",output
      f = open(output, 'wb')
      cPickle.dump(file_data, f)
      f.close()
