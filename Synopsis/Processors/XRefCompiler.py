# $Id: XRefCompiler.py,v 1.5 2003/11/11 02:57:57 stefan Exp $
#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis.Core import AST, Type, Util

import string, cPickle, urllib

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

   xref_path = Parameter('./%s-xref', '')
   xref_file = Parameter('compiled.xref', '')
   no_locals = Parameter(True, '')

   def process(self, ast, **kwds):
      
      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      filenames = map(lambda x: x[0], 
                      filter(lambda x: x[1].is_main(), ast.files().items()))
      filenames = map(lambda x:self.xref_path%x, filenames)
      self.do_compile(filenames, self.xref_file, self.no_locals)

      return self.output_and_return_ast()

   def do_compile(self, input_files, output_file, no_locals):
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
            print "Error opening %s: %s"%(file, e)
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

      if self.verbose: print "XRefCompiler: Writing",output_file
      f = open(output_file, 'wb')
      cPickle.dump(file_data, f)
      f.close()

linkerOperation = XRefCompiler
