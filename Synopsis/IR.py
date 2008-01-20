#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import ASG
import cPickle

class IR:
    """Top-level Internal Representation. This is essentially a dictionary
    of different representations such as Parse Tree, Abstract Semantic Graph, etc.
    """

    def __init__(self, files=None, declarations=None, types=None):
        """Constructor"""

        self.files = files or {}
        self.declarations = declarations or []
        self.types = types or ASG.Dictionary()

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

        self.types.merge(other.types)
        self.declarations.extend(other.declarations)
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


def load(filename):
    """Loads an IR object from the given filename"""

    file = open(filename, 'rb')
    unpickler = cPickle.Unpickler(file)
    ir = unpickler.load()
    file.close()
    return ir
