#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Scope sorting class module.
This module contains the class for sorting Scopes.
"""

from Synopsis.Processor import Parametrized, Parameter
from Synopsis import ASG

import string

# The names of the access types
_axs_str = ('','Public ','Protected ','Private ')

# The predefined order for section names
_section_order = (
    'Namespace', 'Module', 'Class', 'Typedef', 'Struct', 'Enum', 'Union',
    'Group', 'Member function', 'Function'
)

# Compare two section names
def compare_sections(a, b):
   # Determine access of each name
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

class ScopeSorter(Parametrized):
   """A class that takes a scope and sorts its children by type. To use it
   call set_scope, then access the sorted list by the other methods."""

   struct_as_class = Parameter(False, '') 

   def __init__(self, scope=None):
      "Optional scope starts using that ASG.Scope"

      self.__scope = None
      if scope: self.set_scope(scope)

   def set_scope(self, scope):
      "Sort children of given scope"

      if scope is self.__scope: return
      self.__sections = []
      self.__section_dict = {}
      self.__children = []
      self.__child_dict = {}
      self.__scope = scope
      self.__sorted_sections = 0
      self.__sorted_secnames = 0
      scopename = scope.name()
      for decl in scope.declarations():
         if isinstance(decl, ASG.Forward): continue
         if isinstance(decl, ASG.Builtin): continue
         if isinstance(decl, ASG.Group) and decl.__class__.__name__ == 'Group':
            self._handle_group(decl)
            continue
         name, section = decl.name(), self._section_of(decl)
         if len(name) > 1 and name[:-1] != scopename: continue
         self._add_decl(decl, name, section)
      self._sort_sections()

   def _add_decl(self, decl, name, section):
      "Adds the given decl with given name and section to the internal data"

      if not self.__section_dict.has_key(section):
         self.__section_dict[section] = []
         self.__sections.append(section)
      self.__section_dict[section].append(decl)
      self.__children.append(decl)
      self.__child_dict[tuple(name)] = decl

   def _section_of(self, decl):

      section = string.capitalize(decl.type())
      if self.struct_as_class and section == 'Struct':
         section = 'Class'
      if decl.accessibility != ASG.DEFAULT:
         section = _axs_str[decl.accessibility()]+section
      return section

   def _sort_sections(self): pass
   def sort_section_names(self):
      """Sorts sections names if they need it"""

      if self.__sorted_secnames: return
      self.__sections.sort(compare_sections)
      self.__sorted_secnames = 1

   def _set_section_names(self, sections): self.__sections = sections
   def _handle_group(self, group):
      """Handles a group"""

      section = group.name()[-1]
      self._add_decl(group, group.name(), section)
      for decl in group.declarations():
         name = decl.name()
         self._add_decl(decl, name, section)

   def sort_sections(self):
      """Sorts the children of all sections, if they need it"""
      
      if self.__sorted_sections: return
      for children in self.__section_dict.values()+[self.__children]:
         dict = {}
         for child in children: dict[child.name()] = child
         names = dict.keys()
         names.sort()
         del children[:]
         for name in names: children.append(dict[name])
      self.__sorted_sections = 1

   def child(self, name):
      "Returns the child with the given name. Throws KeyError if not found."
      return self.__child_dict[name]

   def sections(self):
      "Returns a list of available section names"

      return self.__sections

   def children(self, section=None):
      "Returns list of children in given section, or all children"

      if section is None: return self.__children
      if self.__section_dict.has_key(section):
         return self.__section_dict[section]
      return {}


