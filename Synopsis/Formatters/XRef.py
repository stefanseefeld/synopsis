# $Id: XRef.py,v 1.1 2002/10/28 17:38:57 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2002 Stephen Davies
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
# $Log: XRef.py,v $
# Revision 1.1  2002/10/28 17:38:57  chalky
# Created module to handle xref data
#
#

import string, cPickle

class CrossReferencer:
    """Handle cross-reference files"""
    def __init__(self):
	self.__data = {}
	self.__index = {}
	self.__file_map = {}
	self.__file_info = []

    def load(self, filename):
	"""Loads data from the given filename"""
	f = open(filename, 'rb')
	self.__data, self.__index = cPickle.load(f)
	f.close()
	# Split the data into multiple files based on size
	page = 0
	count = 0
	self.__file_info.append([])
	names = self.__data.keys()
	names.sort()
	for name in names:
	    if count > 1000:
		count = 0
		page = page + 1
		self.__file_info.append([])
	    self.__file_info[page].append(name)
	    self.__file_map[name] = page
	    target_data = self.__data[name]
	    count = count + len(target_data[0]) + len(target_data[1]) + len(target_data[2])

    def get_info(self, name):
	"""Retrieves the info for the give name. The info is returned as a
	3-tuple of [list of definitions], [list of calls], [list of
	other references]. The element of each list is another 3-tuple of
	(file, line, scope of reference). The last element is the scope in
	which the reference was made, which is often a function scope, denoted
	as starting with a backtick, eg: Builder::`add_operation(std::string).
	Returns None if there is no xref info for the given name.
	"""
	if not self.__data.has_key(name): return None
	return self.__data[name]

    def get_possible_names(self, name):
	"""Returns a list of possible scoped names that end with the given
	name. These scoped names can be passed to get_info(). Returns None if
	there is no scoped name ending with the given name."""
	if not self.__index.has_key(name): return None
	return self.__index[name]

    def get_page_for(self, name):
	"""Returns the number of the page that the xref info for the given
	name is on, or None if not found."""
	if not self.__file_map.has_key(name): return None
	return self.__file_map[name]

    def get_page_info(self):
	"""Returns a list of pages, each consisting of a list of names on that
	page. This method is intended to be used by whatever generates the
	files..."""
	return self.__file_info
    
