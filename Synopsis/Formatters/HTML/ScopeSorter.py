# $Id: ScopeSorter.py,v 1.9 2003/11/11 06:01:13 stefan Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stephen Davies
# Copyright (C) 2000, 2001 Stefan Seefeld
#
# Synopsis is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.
#
# $Log: ScopeSorter.py,v $
# Revision 1.9  2003/11/11 06:01:13  stefan
# adjust to directory/package layout changes
#
# Revision 1.8  2002/10/28 06:12:31  chalky
# Add structs_as_classes option
#
# Revision 1.7  2001/07/15 08:28:43  chalky
# Added 'Inheritance' page Part
#
# Revision 1.6  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.5  2001/06/11 10:37:49  chalky
# Better grouping support
#
# Revision 1.4  2001/02/12 04:08:09  chalky
# Added config options to HTML and Linker. Config demo has doxy and synopsis styles.
#
# Revision 1.3  2001/02/06 05:13:05  chalky
# Fixes
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

"""Scope sorting class module.
This module contains the class for sorting Scopes.
"""
# System modules
import string

# Synopsis modules
from Synopsis import AST

# HTML modules
import core

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

class ScopeSorter:
    """A class that takes a scope and sorts its children by type. To use it
    call set_scope, then access the sorted list by the other methods."""
    def __init__(self, scope=None):
	"Optional scope starts using that AST.Scope"
	self.__scope = None
	if scope: self.set_scope(scope)
	self.__structs_as_classes = core.config.structs_as_classes
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
	    if isinstance(decl, AST.Forward): continue
	    if isinstance(decl, AST.Group) and decl.__class__.__name__ == 'Group':
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
	if self.__structs_as_classes and section == 'Struct':
	    section = 'Class'
	if decl.accessibility != AST.DEFAULT:
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
	    core.sort(names)
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


