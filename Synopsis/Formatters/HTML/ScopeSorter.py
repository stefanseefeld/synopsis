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
	    name, section = decl.name(), self._section_of(decl)
	    if name[:-1] != scopename: continue
	    if not self.__section_dict.has_key(section):
		self.__section_dict[section] = []
		self.__sections.append(section)
	    self.__section_dict[section].append(decl)
	    self.__children.append(decl)
	    self.__child_dict[tuple(name)] = decl
	self._sort_sections()
    def _section_of(self, decl):
	_axs_str = ('','Public ','Protected ','Private ')
	section = string.capitalize(decl.type())
	if decl.accessibility != AST.DEFAULT:
	    section = _axs_str[decl.accessibility()]+section
	return section
    def _sort_sections(self): pass
    def sort_section_names(self):
	core.sort(self.__sections)
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


