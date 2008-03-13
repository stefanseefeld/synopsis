#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

class Entry(object):

    def __init__(self):
        """Represents a set of references found for a given symbol."""

        self.definitions = []
        "List of (file, line, scope) tuples."
        self.calls = []
        "List of (file, line, scope) tuples."
        self.references = []
        "List of (file, line, scope) tuples."


class SXR(dict):
    """Symboltable containing source code locations of symbol definitions,
    as well as different types of references."""

    def __init__(self):

        self._index = {}

    def index(self):

        return self._index
    
    def generate_index(self):
        """(Re-)generate an index after entries have been added."""

        # Sort the data
        for target, entry in self.items():
            entry.calls.sort()
            entry.references.sort()
  
            name = target[-1]
            self._index.setdefault(name,[]).append(target)
            # If it's a function name, also add without the parameters
            paren = name.find('(')
            if paren != -1:
                self._index.setdefault(name[:paren],[]).append(target)

    def merge(self, other):

        for k in other:
            e = other[k]
            entry = self.setdefault(k, Entry())
            # TODO: Should we try to eliminate duplicates here ?
            entry.definitions += e.definitions
            entry.calls += e.calls
            entry.references += e.references

        self.generate_index()

