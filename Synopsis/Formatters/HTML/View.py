# $Id: View.py,v 1.16 2003/01/16 12:46:46 chalky Exp $
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
# Revision 1.16  2003/01/16 12:46:46  chalky
# Renamed FilePages to FileSource, FileTree to FileListing. Added FileIndexer
# (used to be part of FileTree) and FileDetails.
#
# Revision 1.15  2002/11/16 04:12:33  chalky
# Added strategies for page formatting, and added one to allow template HTML
# files to be used.
#
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

class PageFormat:
    """Default and base class for formatting a page layout. The PageFormat
    class basically defines the HTML used at the start and end of the page.
    The default creates an XHTML compliant header and footer with a proper
    title, and link to the stylesheet."""
    def __init__(self):
	self.__stylesheet = config.stylesheet
	self.__prefix = ''

    def set_prefix(self, prefix):
	"""Sets the prefix to use to correctly reference files in the document
	root directory."""
	self.__prefix = prefix

    def stylesheet(self):
	"""Returns the relative filename of the stylesheet to use. The
	stylesheet specified in the user's config is copied into the output
	directory. If this page is not in the same directory, the url returned
	from this function will have the appropriate number of '..'s added."""
	return self.__prefix + self.__stylesheet

    def prefix(self):
	"""Returns the prefix to use to correctly reference files in the
	document root directory. This will only ever not be '' if you are using the
	NestedFileLayout, in which case it will be '' or '../' or '../../' etc
	as appropraite."""
	return self.__prefix

    def page_header(self, os, title, body, headextra):
	"""Called to output the page header to the given output stream.
	@param os a file-like object (use os.write())
	@param title the title of this page
	@param body the body tag, which may contain extra parameters such as
	onLoad scripts, and may also be empty eg: for the frames index
	@param headextra extra html to put in the head section, such as
	scripts
	"""
	os.write('<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01//EN" "http://www.w3.org/TR/html4/strict.dtd">')
	os.write("<html>\n<head>\n")
	os.write(entity('title','Synopsis - '+ title) + '\n')
	ss = self.stylesheet()
	if ss: os.write(solotag('link', type='text/css', rel='stylesheet', href=ss) + '\n')
	os.write(headextra)
	os.write("</head>\n%s\n"%body)

    def page_footer(self, os, body):
	"""Called to output the page footer to the given output stream.
	@param os a file-like object (use os.write())
	@param body the close body tag, which may be empty eg: for the frames
	index
	"""
	os.write("\n%s\n</html>\n"%body)

class TemplatePageFormat (PageFormat):
    """PageFormat subclass that uses a template file to define the HTML header
    and footer for each page."""
    def __init__(self):
	PageFormat.__init__(self)
	self.__file = ''
	self.__re_body = re.compile('<body(?P<params>([ \t\n]+[-a-zA-Z0-9]+=("[^"]*"|\'[^\']*\'|[^ \t\n>]*))*)>', re.I)
	self.__re_closebody = re.compile('</body>', re.I)
	self.__re_closehead = re.compile('</head>', re.I)
	self.__title_tag = '@TITLE@'
	self.__content_tag = '@CONTENT@'
	if hasattr(config.obj, 'TemplatePageFormat'):
	    myconfig = config.obj.TemplatePageFormat
	    if hasattr(myconfig, 'file'):
		self.__file = myconfig.file
	    if hasattr(myconfig, 'copy_files'):
		for file in myconfig.copy_files:
		    dest = os.path.join(config.basename, file[1])
		    config.files.copyFile(file[0], dest)
	self.load_file()

    def load_file(self):
	"""Loads and parses the template file"""
	f = open(self.__file, 'rt')
	text = f.read(1024*64) # arbitrary max limit of 64kb
	f.close()
	# Find the content tag
	content_index = text.find(self.__content_tag)
	if content_index == -1:
	    print "Fatal: content tag '%s' not found in template file!"%self.__content_tag
	    raise SystemError, "Content tag not found"
	header = text[:content_index]
	# Find the title (doesn't matter if not found)
	self.__title_index = text.find(self.__title_tag)
	if self.__title_index:
	    # Remove the title tag
	    header = header[:self.__title_index] + \
		     header[self.__title_index+len(self.__title_tag):]
	# Find the close head tag
	mo = self.__re_closehead.search(header)
	if mo: self.__headextra_index = mo.start()
	else: self.__headextra_index = -1
	# Find the body tag
	mo = self.__re_body.search(header)
	if not mo:
	    print "Fatal: body tag not found in template file!"
	    print "(if you are sure there is one, this may be a bug in Synopsis)"
	    raise SystemError, "Body tag not found"
	if mo.group('params'): self.__body_params = mo.group('params')
	else: self.__body_params = ''
	self.__body_index = mo.start()
	header = header[:mo.start()] + header[mo.end():]
	# Store the header
	self.__header = header
	footer = text[content_index+len(self.__content_tag):]
	# Find the close body tag
	mo = self.__re_closebody.search(footer)
	if not mo:
	    print "Fatal: close body tag not found in template file"
	    raise SystemError, "Close body tag not found"
	self.__closebody_index = mo.start()
	footer = footer[:mo.start()] + footer[mo.end():]
	self.__footer = footer

    def write(self, os, text):
	"""Writes the text to the output stream, replaceing @PREFIX@ with the
	prefix for this file"""
	sections = string.split(text, '@PREFIX@')
	os.write(string.join(sections, self.prefix()))

    def page_header(self, os, title, body, headextra):
	"""Formats the header using the template file"""
	if not body: return PageFormat.page_header(self, os, title, body, headextra)
	header = self.__header
	index = 0
	if self.__title_index != -1:
	    self.write(os, header[:self.__title_index])
	    self.write(os, title)
	    index = self.__title_index
	if self.__headextra_index != -1:
	    self.write(os, header[index:self.__headextra_index])
	    self.write(os, headextra)
	    index = self.__headextra_index
	self.write(os, header[index:self.__body_index])
	if body:
	    if body[-1] == '>':
		self.write(os, body[:-1]+self.__body_params+body[-1])
	    else:
		# Hmmmm... Should not happen, perhaps use regex?
		self.write(os, body)
	self.write(os, header[self.__body_index:])

    def page_footer(self, os, body):
	"""Formats the footer using the template file"""
	if not body: return PageFormat.page_footer(self, os, body)
	footer = self.__footer
	self.write(os, footer[:self.__closebody_index])
	self.write(os, body)
	self.write(os, footer[self.__closebody_index:])

class Page:
    """Base class for a Page. The base class provides a common interface, and
    also handles common operations such as opening the file, and delegating
    the page formatting to a strategy class.
    @see PageFormat"""
    def __init__(self, manager):
	"""Constructor, loads the formatting class.
	@see PageFormat"""
	self.manager = manager
	self.__os = None
	format_class = PageFormat
	if config.page_format:
	    format_class = Util.import_object(config.page_format, basePackage = 'Synopsis.Formatter.HTML.Page.')
	self.__format = format_class()

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
	prefix = rel(self.filename(), '')
	self.__format.set_prefix(prefix)
	self.__format.page_header(self.__os, self.title(), body, headextra)
    
    def end_file(self, body='</body>'):
	"""Close the file using given close body tag. The default is
	just a close body tag, but if you specify '' then nothing will be
	written (useful for a frames page)"""
	self.__format.page_footer(self.__os, body)
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
