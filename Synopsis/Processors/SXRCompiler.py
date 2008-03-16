#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import SXR
from Synopsis.Processor import *
from Synopsis.QualifiedName import *
from xml.dom.minidom import parse
import os.path

class SXRCompiler(Processor):
    """This class compiles symbol references stored in sxr files into a single symbol table."""

    prefix = Parameter('', 'where to look for sxr files')
    no_locals = Parameter(True, '')

    def process(self, ir, **kwds):
      
        self.set_parameters(kwds)
        if not self.prefix: raise MissingArgument('prefix')
        self.ir = self.merge_input(ir)

        def prefix(filename):
            "Map filename to sxr filename."

            # Even though filenames shouldn't be absolute, we protect ourselves
            # against accidents.
            if os.path.isabs(filename):
                filename = os.path.splitdrive(filename)[1][1:]
            return os.path.join(self.prefix, filename) + '.sxr'

        for f in self.ir.files.values():
            if f.annotations['primary'] and os.path.exists(prefix(f.name)):
                self.compile(prefix(f.name), f.annotations['language'])

        self.ir.sxr.generate_index()

        return self.ir

    def compile(self, filename, language):

        if language == 'Python':
            QName = lambda name: QualifiedPythonName(str(name).split('.'))
        else:
            QName = lambda name: QualifiedCxxName(str(name).split('::'))


        if self.verbose: print "SXRCompiler: Reading", filename
        try:
            document = parse(filename)
        except:
            if self.debug:
                print 'Error parsing', filename
                raise
            else:
                raise InternalError('parsing %s'%filename)
        sxr = document.documentElement
        filename = sxr.getAttribute('filename')
        lines = sxr.getElementsByTagName('line')
        for lineno, line in enumerate(lines):
            for a in line.getElementsByTagName('a'):
                if a.getAttribute('continuation') == 'true':
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
                entry = self.ir.sxr.setdefault(qname, SXR.Entry())
                if type == 'definition':
                    entry.definitions.append((filename, lineno + 1, origin))
                elif type == 'call':
                    entry.calls.append((filename, lineno + 1, origin))
                elif type == 'reference':
                    entry.references.append((filename, lineno + 1, origin))
                else:
                    print 'Warning: Unknown sxr type in %s:%d : %s'%(filename, lineno + 1, type)
 

