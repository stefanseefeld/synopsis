# $Id: XRef.py,v 1.2 2002/10/29 01:35:58 chalky Exp $
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
# Revision 1.2  2002/10/29 01:35:58  chalky
# Add descriptive comment and link to docs for scope in xref pages
#
# Revision 1.1  2002/10/28 17:39:36  chalky
# Cross referencing support
#

# System modules
import os

# Synopsis modules
from Synopsis.Core import AST, Type, Util

# HTML modules
import Page
from core import config
from Tags import *

class XRefPages (Page.Page):
    """A module for creating pages full of xref infos"""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.xref = config.xref
	self.__filename = None
	self.__title = None
	if hasattr(config.obj, 'XRefPages'):
	    if hasattr(config.obj.XRefPages, 'xref_file'):
		self.xref.load(config.obj.XRefPages.xref_file)

    def filename(self):
        """Returns the current filename,
        which may change over the lifetime of this object"""
        return self.__filename
    def title(self):
        """Returns the current title,
        which may change over the lifetime of this object"""
        return self.__title

    def process(self, start):
	"""Creates a page for every bunch of xref infos"""
	page_info = self.xref.get_page_info()
	if not page_info: return
	for i in range(len(page_info)):
	    self.__filename = config.files.nameOfSpecial('xref%d'%i)
	    self.__title = 'Cross Reference page #%d'%i

	    self.start_file()
	    self.write(self.manager.formatHeader(self.filename()))
	    self.write(entity('h1', self.__title))
	    self.write('<hr>')
	    for name in page_info[i]:
		self.process_name(name)
	    self.end_file()

    def register_filenames(self, start):
	"""Registers each page"""
	page_info = self.xref.get_page_info()
	if not page_info: return
	for i in range(len(page_info)):
	    filename = config.files.nameOfSpecial('xref%d'%i)
	    self.mananger.register_filename(filename, self, i)
    
    def process_link(self, file, line, scope):
	"""Outputs the info for one link"""
	# Make a link to the highlighted source
	file_link = config.files.nameOfScopedSpecial('page', string.split(file, os.sep))
	file_link = file_link + "#%d"%line
	# Try and make a descriptive
	desc = ''
	if config.types.has_key(scope):
	    type = config.types[scope]
	    if isinstance(type, Type.Declared):
		desc = ' ' + type.declaration().type()
	# Try and find a link to the scope
	scope_text = string.join(scope, '::')
	entry = config.toc[scope]
	if entry:
	    scope_text = href(entry.link, scope_text)
	# Output list element
	self.write('<li><a href="%s">%s:%s</a>: in%s %s</li>\n'%(
	    file_link, file, line, desc, scope_text))
     
    def process_name(self, name):
	"""Outputs the info for a given name"""

	target_data = self.xref.get_info(name)
	if not target_data: return

	jname = string.join(name, '::')
	self.write('<a name="%s">'%Util.quote(jname))
	self.write(entity('h2', jname) + '<ul>\n')
	
	if target_data[0]:
	    self.write('<li>Defined at:<ul>\n')
	    for file, line, scope in target_data[0]:
		self.process_link(file, line, scope)
	    self.write('</ul></li>\n')
	if target_data[1]:
	    self.write('<li>Called from:<ul>\n')
	    for file, line, scope in target_data[1]:
		self.process_link(file, line, scope)
	    self.write('</ul></li>\n')
	if target_data[2]:
	    self.write('<li>Referenced from:<ul>\n')
	    for file, line, scope in target_data[2]:
		self.process_link(file, line, scope)
	    self.write('</ul></li>\n')
	
	self.write('</ul><hr>\n')

htmlPageClass = XRefPages
