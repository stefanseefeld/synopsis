# $Id: ScopeSorter.py,v 1.5 2001/06/11 10:37:49 chalky Exp $
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
from Synopsis.Core import AST

# HTML modules
import core


class ScopeSorter:
    """A class that takes a scope and sorts its children by type. To use it
    call set_scope, then access the sorted list by the other methods."""
    def __init__(self, scope=None):
	"Optional scope starts using that AST.Scope"
	if scope: self.set_scope(scope)
    def set_scope(self, scope):
	"Sort children of given scope"
	self.__sections = []
	self.__section_dict = {}
	self.__children = []
	self.__child_dict = {}
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
	_axs_str = ('','Public ','Protected ','Private ')
	section = string.capitalize(decl.type())
	if decl.accessibility != AST.DEFAULT:
	    section = _axs_str[decl.accessibility()]+section
	return section
    def _sort_sections(self): pass
    def sort_section_names(self):
	core.sort(self.__sections)
    def _set_section_names(self, sections): self.__sections = sections
    def _handle_group(self, group):
	"""Handles a group"""
	print "Handling group",group.name()
	section = group.name()[-1]
	self._add_decl(group, group.name(), section)
	for decl in group.declarations():
	    name = decl.name()
	    self._add_decl(decl, name, section)
    def sort_sections(self):
	for children in self.__section_dict.values()+[self.__children]:
	    dict = {}
	    for child in children: dict[child.name()] = child
	    names = dict.keys()
	    core.sort(names)
	    del children[:]
	    for name in names: children.append(dict[name])
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


