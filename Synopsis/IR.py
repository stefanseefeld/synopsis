#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import ASG
import SXR
import cPickle

class IR(object):
    """Top-level Internal Representation. This is essentially a dictionary
    of different representations such as Parse Tree, Abstract Semantic Graph, etc.
    """

    def __init__(self, files=None, asg=None, sxr=None):
        """Constructor"""

        self.files = files or {}
        """A dictionary mapping filenames to `SourceFile.SourceFile` instances."""
        self.asg = asg or ASG.ASG()
        """The Abstract Semantic Graph."""
        self.sxr = sxr or SXR.SXR()
        """The Source Cross-Reference SymbolTable."""

    def copy(self):
        """Make a shallow copy of this IR."""

        return type(self)(self.files.copy(),
                          self.asg.copy(),
                          self.sxr)

    def save(self, filename):
        """Saves an IR object to the given filename"""

        file = open(filename, 'wb')
        pickler = cPickle.Pickler(file, 1)
        pickler.dump(self)
        file.close()

    def merge(self, other):
        """Merges another IR. Files and declarations are appended to those in
        this IR, and types are merged by overwriting existing types -
        Unduplicator is responsible for sorting out the mess this may cause :)"""

        #merge files
        replacement = {}
        for filename, file in other.files.iteritems():
            if not self.files.has_key(filename):
                self.files[filename] = file
                continue
            myfile = self.files[filename]
            replacement[file] = myfile
            # the 'main' flag dominates...
            if not myfile.annotations['primary']:
                myfile.annotations['primary'] = file.annotations['primary']
            myfile.declarations.extend(file.declarations)
            myfile.includes.extend(file.includes)
        # fix dangling inclusions of 'old' files
        for r in replacement:
            for f in self.files.values():
                for i in f.includes:
                    if i.target == r:
                        i.target = replacement[r]

        # merge ASG
        self.asg.merge(other.asg)

        # merge SXR
        self.sxr.merge(other.sxr)


def load(filename):
    """Loads an IR object from the given filename"""

    file = open(filename, 'rb')
    unpickler = cPickle.Unpickler(file)
    ir = unpickler.load()
    file.close()
    return ir
