#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

class QualifiedName(tuple):

    sep = ''

    def __getitem__(self, i):
        """If i is a slice, make sure a QualifiedName is returned."""
        
        result = super(QualifiedName, self).__getitem__(i)
        if type(i) is slice:
            # Wrap the result
            return type(self)(result)
        else:
            return result

    def __getslice__(self, begin, end):
        """This method exists because python < 3.0 still uses __getslice__
        for builtin types. (See http://bugs.python.org/issue2041)"""

        return self.__getitem__(slice(begin, end, None))

    def __add__(self, other):
        """Overload self + other to preserve the type."""

        return type(self)(tuple(self) + other)

    def prune(self, other):
        """Return a copy of other with any prefix it shares with self removed.
   
        e.g. ('A', 'B', 'C', 'D').prune(('A', 'B', 'D')) -> ('C', 'D')"""

        target = list(other)
        i = 0
        while (len(target) > 1 and i < len(self) and target[0] == self[i]):
            del target[0]
            i += 1
        return type(other)(target)

class QualifiedCxxName(QualifiedName):

    sep = '::'

    def __str__(self):
        return '::'.join(self)
        
class QualifiedPythonName(QualifiedName):

    sep = '.'

    def __str__(self):
        return '.'.join(self)
        
