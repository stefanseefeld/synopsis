# $Id: XRef.py,v 1.1 2002/10/28 17:39:36 chalky Exp $
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
# $Log: XRef.py,v $
# Revision 1.1  2002/10/28 17:39:36  chalky
# Cross referencing support
#

# System modules
import os

# Synopsis modules
from Synopsis.Core import AST, Util

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
		link = config.files.nameOfScopedSpecial('page', string.split(file, os.sep))
		link = link + "#%d"%line
		self.write('<li><a href="%s">%s:%s</a>: %s</li>\n'%(link, file, line, string.join(scope, '::')))
	    self.write('</ul></li>\n')
	if target_data[1]:
	    self.write('<li>Called from:<ul>\n')
	    for file, line, scope in target_data[1]:
		link = config.files.nameOfScopedSpecial('page', string.split(file, os.sep))
		link = link + "#%d"%line
		self.write('<li><a href="%s">%s:%s</a>: %s</li>\n'%(link, file, line, string.join(scope, '::')))
	    self.write('</ul></li>\n')
	if target_data[2]:
	    self.write('<li>Referenced from:<ul>\n')
	    for file, line, scope in target_data[2]:
		link = config.files.nameOfScopedSpecial('page', string.split(file, os.sep))
		link = link + "#%d"%line
		self.write('<li><a href="%s">%s:%s</a>: %s</li>\n'%(link, file, line, string.join(scope, '::')))
	    self.write('</ul></li>\n')
	
	self.write('</ul><hr>\n')

htmlPageClass = XRefPages
