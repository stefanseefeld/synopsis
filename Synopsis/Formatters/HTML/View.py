# $Id: View.py,v 1.14 2002/11/01 03:39:21 chalky Exp $
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
# $Log: View.py,v $
# Revision 1.14  2002/11/01 03:39:21  chalky
# Cleaning up HTML after using 'htmltidy'
#
# Revision 1.13  2002/10/29 12:43:56  chalky
# Added flexible TOC support to link to things other than ScopePages
#
# Revision 1.12  2002/07/19 14:26:33  chalky
# Revert prefix in FileLayout but keep relative referencing elsewhere.
#
# Revision 1.10  2002/01/09 11:43:41  chalky
# Inheritance pics
#
# Revision 1.9  2002/01/09 10:16:35  chalky
# Centralized navigation, clicking links in (html) docs works.
#
# Revision 1.8  2001/11/09 15:35:04  chalky
# GUI shows HTML pages. just. Source window also scrolls to correct line.
#
# Revision 1.7  2001/07/05 05:39:58  stefan
# advanced a lot in the refactoring of the HTML module.
# Page now is a truely polymorphic (abstract) class. Some derived classes
# implement the 'filename()' method as a constant, some return a variable
# dependent on what the current scope is...
#
# Revision 1.6  2001/07/05 02:08:35  uid20151
# Changed the registration of pages to be part of a two-phase construction
#
# Revision 1.5  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.4  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.3  2001/02/05 05:26:24  chalky
# Graphs are separated. Misc changes
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
#

"""
Page base class, contains base functionality and common interface for all Pages.
"""

import os.path, cStringIO
from Synopsis.Core import Util

import core
from core import config
from Tags import *

class Page:
    """Base class for a Page"""
    def __init__(self, manager):
	"Constructor"
	self.manager = manager
	self.__os = None
	self.__filename = ''
	self.__title = ''

    def filename(self):
        "Polymorphic method returning the filename associated with the page"
        return ''
    def title(self):
        "Polymorphic method returning the title associated with the page"
        return ''

    def os(self):
	"Returns the output stream opened with start_file"
	return self.__os

    def write(self, str):
	"""Writes the given string to the currently opened file"""
	self.__os.write(str)

    def register(self):
	"""Registers this Page class with the PageManager. This method is
	abstract - derived Pages should implement it to call the appropriate
	methods in PageManager if they need to. This method is called after
	construction."""
	pass

    def register_filenames(self, start):
	"""Registers filenames for each file this Page will generate, given
	the starting Scope."""
	pass

    def get_toc(self, start):
	"""Retrieves the TOC for this page. This method assumes that the page
	generates info for the the whole AST, which could be the ScopePages,
	the FilePages (source code) or the XRefPages (cross reference info).
	The default implementation returns None. Start is the declaration to
	start processing from, which could be the global namespace."""
	pass
       
    def process(self, start):
	"""Process the given Scope recursively. This is the method which is
	called to actually create the files, so you probably want to override
	it ;)"""
	pass

    def process_scope(self, scope):
	"""Process just the given scope"""
	pass

    def open_file(self):
	"""Returns a new output stream. This template method is for internal
	use only, but may be overriden in derived classes.
	The default joins config.basename and self.filename()
	and uses Util.open()"""
	return Util.open(os.path.join(config.basename, self.filename()))

    def close_file(self):
	"""Closes the internal output stream. This template method is for
	internal use only, but may be overriden in derived classes."""
	self.__os.close()
	self.__os = None
	
    def start_file(self, body='<body>', headextra=''):
	"""Start a new file with given filename, title and body. This method
	opens a file for writing, and writes the html header crap at the top.
	You must specify a title, which is prepended with the project name.
	The body argument is optional, and it is preferred to use stylesheets
	for that sort of stuff. You may want to put an onLoad handler in it
	though in which case that's the place to do it. The opened file is
	stored and can be accessed using the os() method."""
	self.__os = self.open_file()
	self.write('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">')
	self.write("<html>\n<head>\n")
        self.write('<!-- ' + self.filename() + ' -->\n')
        self.write('<!-- this page was generated by ' + self.__class__.__name__ + ' -->\n')
	self.write(entity('title','Synopsis - '+ self.title()) + '\n')
	if len(config.stylesheet):
	    self.write(solotag('link', type='text/css', rel='stylesheet', href=rel(self.filename(), config.stylesheet)) + '\n')
	self.write(headextra)
	self.write("</head>\n%s\n"%body)
    
    def end_file(self, body='</body>'):
	"Close the file using given close body tag"
	self.write("\n%s\n</html>\n"%body)
	self.close_file()

    def reference(self, name, scope, label=None, **keys):
	"""Returns a reference to the given name. The name is a scoped name,
	and the optional label is an alternative name to use as the link text.
	The name is looked up in the TOC so the link may not be local. The
	optional keys are appended as attributes to the A tag."""
	if not label: label = anglebrackets(Util.ccolonName(name, scope))
	entry = config.toc[name]
	if entry: return apply(href, (rel(self.filename(), entry.link), label), keys)
	return label or ''


class BufferPage (Page):
    """A page that writes to a string buffer."""
    def _take_control(self):
	self.open_file = lambda s=self: BufferPage.open_file(s)
	self.close_file = lambda s=self: BufferPage.close_file(s)
	self.get_buffer = lambda s=self: BufferPage.get_buffer(s)
	
    def open_file(self):
	"Returns a new StringIO"
	return cStringIO.StringIO()

    def close_file(self):
	"Does nothing."
	pass

    def get_buffer(self):
	"""Returns the page as a string, then deletes the internal buffer"""
	page = self.os().getvalue()
	# NOW we do the close
	Page.close_file(self)
	return page
