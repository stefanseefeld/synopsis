#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import *
from Synopsis.QualifiedName import *
from xml.dom.minidom import parse

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
        if not self.output: raise MissingArgument('output')
        self.ir = self.merge_input(ir)

        def prefix(filename):
            "Map filename to xref filename."

            # Even though filenames shouldn't be absolute, we protect ourselves
            # against accidents.
            if os.path.isabs(filename):
                filename = os.path.splitdrive(filename)[1][1:]
            return os.path.join(self.prefix, filename) + '.sxr'

        self._data = {}
        self._index = {}

        for f in self.ir.files.values():
            if f.annotations['primary'] and os.path.exists(prefix(f.name)):
                self.compile(prefix(f.name), f.annotations['language'])

        # Sort the data
        for target, target_data in self._data.items():
            target_data[1].sort()
            target_data[2].sort()
  
            name = target[-1]
            self._index.setdefault(name,[]).append(target)
            # If it's a function name, also add without the parameters
            paren = name.find('(')
            if paren != -1:
                self._index.setdefault(name[:paren],[]).append(target)

        if self.verbose: print 'XRefCompiler: Writing', self.output
        f = open(self.output, 'wb')
        cPickle.dump((self._data, self._index), f)
        f.close()

        return self.ir

    def compile(self, filename, language):

        if language == 'Python':
            QName = lambda name: QualifiedPythonName(str(name).split('.'))
        else:
            QName = lambda name: QualifiedCxxName(str(name).split('::'))


        if self.verbose: print "XRefCompiler: Reading", filename
        try:
            document = parse(filename)
        except:
            raise InternalError('parsing %s'%filename)
        sxr = document.documentElement
        filename = sxr.getAttribute('filename')
        lines = sxr.getElementsByTagName('line')
        for lineno, line in enumerate(lines):
            for a in line.getElementsByTagName('a'):
                if a.getAttribute('continuation') == 'true':
                    print 'continuing'
                    continue
                qname = QName(a.getAttribute('href'))
                origin = QName(a.getAttribute('from'))
                type = str(a.getAttribute('type'))
                if self.no_locals:
                    bad = False
                    for i in range(len(qname)):
                        if len(qname[i]) > 0 and qname[i][0] == '`':
                            # Don't store local function variables
                            bad = True
                            break
                    if bad: continue
  
                    for i in range(len(origin)):
                        if len(origin[i]) > 0 and origin[i][0] == '`':
                            # Function scope, truncate here
                            origin = origin[:i] + (origin[i][1:],)
                            break
                target_data = self._data.setdefault(qname, [[],[],[]])
                if type == 'definition':
                    target_data[0].append((filename, lineno + 1, origin))
                elif type == 'CALL':
                    target_data[1].append((filename, lineno + 1, origin))
                elif type == 'REF':
                    target_data[2].append((filename, lineno + 1, origin))
                else:
                    print 'Warning: Unknown type:', type
 

