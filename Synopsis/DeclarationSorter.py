#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parametrized, Parameter
from Synopsis import ASG

# The names of the access specifiers
_access_specs = {ASG.DEFAULT: '',
                 ASG.PUBLIC: 'Public ',
                 ASG.PROTECTED: 'Protected ',
                 ASG.PRIVATE: 'Private '}

# The predefined order for section names
_section_order = ('Namespace',
                  'Module',
                  'Class',
                  'Typedef',
                  'Struct',
                  'Enum',
                  'Union',
                  'Group',
                  'Member function',
                  'Function')

def _compare_sections(a, b):
    """Compare two section names."""

    ai, bi = 0, 0
    an, bn = a, b
    for i in range(1,4):
        access = _axs_str[i]
        len_access = len(access)
        is_a = (a[:len_access] == access)
        is_b = (b[:len_access] == access)
        if is_a:
            if not is_b: return -1 # a before b
            # Both matched
            an = a[len_access:]
            bn = b[len_access:]
            break
        if is_b:
            return 1 # b before a
        # Neither matched
    # Same access, sort by predefined order 
    for section in _section_order:
        if an == section: return -1 # a before b
        if bn == section: return 1 # b before a
    return 0


class DeclarationSorter(Parametrized):
    """Sort declarations by type and accessibility."""

    struct_as_class = Parameter(False, '') 

    def __init__(self, **args):

        super(DeclarationSorter, self).__init__(**args)
        self.__sections = {}


    def __getitem__(self, i): return self.__sections[i]
    def get(self, *args): return self.__sections.get(*args)
    def has_key(self, k): return self.__sections.haskey(k)
    def items(self): return self.__sections.items()
    def iteritems(self): return self.__sections.iteritems()
    def iterkeys(self): return self.__sections.iterkeys()
    def itervalues(self): return self.__sections.itervalues()
    def keys(self): return self.__sections.keys()
    def values(self): return self.__sections.values()

    def sort(self, declarations):

        self.__sections.clear()
        for d in declarations:
            if isinstance(d, (ASG.Forward, ASG.Builtin)):
                continue
            elif isinstance(d, ASG.Group):
                section = d.name[-1]
                self._add_declaration(d, section)
                for c in d.declarations:
                    self._add_declaration(c, section)
            else:
                self._add_declaration(d, self._section_of(d))


    def _section_of(self, declaration):
        """Generate a section name for the given declaration."""

        section = declaration.type.capitalize()
        if self.struct_as_class and section == 'Struct':
            section = 'Class'
        if declaration.accessibility != ASG.DEFAULT:
            section = _access_specs[declaration.accessibility] + section
        return section


    def _add_declaration(self, declaration, section):
        "Adds the given declaration with given name and section."

        if section not in self.__sections:
            self.__sections[section] = [declaration]
        else:
            self.__sections[section].append(declaration)

