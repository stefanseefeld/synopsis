# $Id: Scope.py,v 1.14 2001/07/15 08:28:43 chalky Exp $
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
# $Log: Scope.py,v $
# Revision 1.14  2001/07/15 08:28:43  chalky
# Added 'Inheritance' page Part
#
# Revision 1.13  2001/07/15 06:41:57  chalky
# Factored summarizer and detailer into 'Parts', and added a separate one for
# the top of the page (Heading)
#
# Revision 1.12  2001/07/10 14:40:23  chalky
# Cache summarizer and detailer
#
# Revision 1.11  2001/07/05 05:39:58  stefan
# advanced a lot in the refactoring of the HTML module.
# Page now is a truely polymorphic (abstract) class. Some derived classes
# implement the 'filename()' method as a constant, some return a variable
# dependent on what the current scope is...
#
# Revision 1.10  2001/07/04 08:17:48  uid20151
# Comments
#
# Revision 1.9  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.8  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.7  2001/06/21 01:17:27  chalky
# Fixed some paths for the new dir structure
#
# Revision 1.6  2001/04/05 09:58:14  chalky
# More comments, and use config object exclusively with basePackage support
#
# Revision 1.5  2001/02/12 04:08:09  chalky
# Added config options to HTML and Linker. Config demo has doxy and synopsis styles.
#
# Revision 1.4  2001/02/06 15:00:42  stefan
# Dot.py now references the template, not the last parameter, when displaying a Parametrized; replaced logo with a simple link
#
# Revision 1.3  2001/02/05 07:58:39  chalky
# Cleaned up image copying for *JS. Added synopsis logo to ScopePages.
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#

# System modules
import time, os

# Synopsis modules
from Synopsis.Core import AST

# HTML modules
import Page
import core
import ASTFormatter
from core import config
from Tags import *


class ScopePages (Page.Page):
    """A module for creating a page for each Scope with summaries and
    details. This module is highly modular, using the classes from
    ASTFormatter to do the actual formatting. The classes to use may be
    controlled via the config script, resulting in a very configurable output.
    @see ASTFormatter The ASTFormatter module
    @see Config.Formatter.HTML.ScopePages Config for ScopePages
    """
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	share = config.datadir
	self.syn_logo = 'synopsis200.jpg'
	config.files.copyFile(os.path.join(share, 'synopsis200.jpg'), os.path.join(config.basename, self.syn_logo))
	self.__parts = []
	self._get_parts()

    def _get_parts(self):
	"Loads the list of parts from config"
	try:
	    parts = config.obj.ScopePages.parts
	except AttributeError:
	    parts = ['Heading', 'Summary', 'Detail']
	base = 'Synopsis.Formatter.HTML.ASTFormatter.'
	for part in parts:
	    obj = core.import_object(part, basePackage=base)(self)
	    self.__parts.append(obj)

    def filename(self):
        """since ScopePages generates a whole file hierarchy, this method returns the current filename,
        which may change over the lifetime of this object"""
        return self.__filename
    def title(self):
        """since ScopePages generates a while file hierarchy, this method returns the current title,
        which may change over the lifetime of this object"""
        return self.__title
    def scope(self):
        """return the current scope processed by this object"""
        return self.__scope

    def process(self, start):
	"""Creates a page for every Scope"""
	self.__namespaces = [start]
	while self.__namespaces:
	    ns = self.__namespaces.pop(0)
	    self.process_scope(ns)
    
    def process_scope(self, ns):
	"""Creates a page for the given scope"""
	details = {} # A hash of lists of detailed children by type
	sections = [] # a list of detailed sections
	
	# Open file and setup scopes
        self.__scope = ns.name()
	self.__filename = config.files.nameOfScope(self.__scope)
	self.__title = string.join(self.__scope)
	self.start_file()
	
	# Write heading
	self.write(self.manager.formatHeader(self.filename()))

	# Loop throught all the page Parts
	for part in self.__parts:
	    part.process(ns)
	self.end_file()
	
	# Queue child namespaces
	for child in config.sorter.children():
	    if isinstance(child, AST.Scope):
		self.__namespaces.append(child)
    
    def end_file(self):
	"""Overrides end_file to provide synopsis logo"""
	self.write('<hr>\n')
        now = time.strftime(r'%c', time.localtime(time.time()))
        logo = href('http://synopsis.sourceforge.net', 'synopsis')
        self.write(div('logo', 'Generated on ' + now + ' by \n<br>\n' + logo))
	Page.Page.end_file(self)
 
htmlPageClass = ScopePages
