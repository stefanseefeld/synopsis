#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import *
from Synopsis import AST, Type, Util

import os.path, cPickle, urllib

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

    def process(self, ir, **kwds):
      
        self.set_parameters(kwds)
        if not self.prefix: raise MissingArgument('prefix')
        self.ir = self.merge_input(ir)

        def prefix(filename):
            "Map filename to xref filename."

            # Even though filenames shouldn't be absolute, we protect ourselves
            # against accidents.
            if os.path.isabs(filename):
                filename = os.path.splitdrive(filename)[1][1:]
            return os.path.join(self.prefix, filename)

        filenames = [prefix(x[0]) for x in ir.files.items()
                     if x[1].annotations['primary']]
        self.do_compile(filenames, self.output, self.no_locals)

        return self.ir

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
                target, file, line, scope, context = line.split()
                target = tuple([intern(t) for t in urllib.unquote(target).split('\t')])
                scope = [intern(s) for s in urllib.unquote(scope).split('\t')]
                if scope == ['','']:
                    scope = ()
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
