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
                  'Class template',
                  'Class',
                  'Typedef',
                  'Struct',
                  'Enum',
                  'Union',
                  'Group',
                  'Member function template',
                  'Member function',
                  'Function template',
                  'Function')

def _compare(a, b):
    """Compare two section names."""

    ai, bi = 0, 0
    an, bn = a, b
    for i in range(1,4):
        access = _access_specs[i]
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


class DeclarationSorter(Parametrized, ASG.Visitor):
    """Sort declarations by type and accessibility."""

    struct_as_class = Parameter(False, '') 

    def __init__(self, declarations = None, **args):

        super(DeclarationSorter, self).__init__(**args)
        if not declarations: # This is a prototype
            return
        self.__sections = {}
        for d in declarations:
            d.accept(self)
        self.__sorted_keys = self.__sections.keys()
        self.__sorted_keys.sort(_compare)

    def __iter__(self): return iter(self.__sorted_keys)
    def __getitem__(self, i): return self.__sections[i]
    def get(self, *args): return self.__sections.get(*args)
    def has_key(self, k): return self.__sections.haskey(k)
    def keys(self): return self.__sorted_keys
    def values(self): return self.__sections.values()

    def _section_of(self, decl, name = None):
        """Generate a section name for the given declaration."""

        section = name or decl.type.capitalize()
        if self.struct_as_class and section == 'Struct':
            section = 'Class'
        if decl.accessibility != ASG.DEFAULT:
            section = _access_specs[decl.accessibility] + section
        return section


    def _add_declaration(self, decl, section):
        "Adds the given declaration with given name and section."

        if section not in self.__sections:
            self.__sections[section] = [decl]
        else:
            self.__sections[section].append(decl)


    def visit_declaration(self, decl):
        self._add_declaration(decl, self._section_of(decl))

    def visit_builtin(self, decl): pass
    def visit_macro(self, decl): pass
    def visit_forward(self, decl):
        if decl.template:
            self.visit_class_template(decl)
        else:
            self.visit_declaration(decl)
    def visit_group(self, group):
        section = group.name[-1]
        self._add_declaration(group, section)
        for d in group.declarations:
            self._add_declaration(d, section)
    def visit_scope(self, decl):
        self.visit_declaration(decl)
    def visit_class_template(self, decl):
        self._add_declaration(decl, self._section_of(decl, 'Class template'))
    def visit_function(self, decl):
        self.visit_declaration(decl)
    def visit_function_template(self, decl):
        self._add_declaration(decl, self._section_of(decl, 'Function template'))
