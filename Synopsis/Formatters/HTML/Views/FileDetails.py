# $Id: FileDetails.py,v 1.2 2003/01/16 13:31:33 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000-2003 Stephen Davies
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
# $Log: FileDetails.py,v $
# Revision 1.2  2003/01/16 13:31:33  chalky
# Quote the scope name
#
# Revision 1.1  2003/01/16 12:46:46  chalky
# Renamed FilePages to FileSource, FileTree to FileListing. Added FileIndexer
# (used to be part of FileTree) and FileDetails.
#
# Revision 1.18  2003/01/02 07:00:58  chalky
# Only use Core.FileTree, refactored FileTree to be quicker and better.
#
# Revision 1.17  2002/12/12 17:25:33  chalky
# Implemented Include support for C++ parser. A few other minor fixes.
#
# Revision 1.16  2002/11/02 06:37:37  chalky
# Allow non-frames output, some refactoring of page layout, new modules.
#
# Revision 1.15  2002/11/01 07:21:15  chalky
# More HTML formatting fixes eg: ampersands and stuff
#
# Revision 1.14  2002/10/29 12:43:56  chalky
# Added flexible TOC support to link to things other than ScopePages
#

# System modules
import os

# Synopsis modules
from Synopsis.Core import AST, Util

# HTML modules
import Page
import core
from core import config
from Tags import *

class FileDetails (Page.Page):
    """A page that creates an index of files, and an index for each file.
    First the index of files is created, intended for the top-left frame.
    Second a page is created for each file, listing the major declarations for
    that file, eg: classes, global functions, namespaces, etc."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
        self.__filename = ''
        self.__title = ''
	self.__link_source = ('FileSource' in config.pages)

    def filename(self):
        """since FileTree generates a whole file hierarchy, this method returns the current filename,
        which may change over the lifetime of this object"""
        return self.__filename
    def title(self):
        """since FileTree generates a while file hierarchy, this method returns the current title,
        which may change over the lifetime of this object"""
        return self.__title

    def register_filenames(self, start):
        """Registers a page for each file indexed"""
	for filename, file in config.ast.files().items():
	    if file.is_main():
                filename = config.files.nameOfFileDetails(filename)
		self.manager.register_filename(filename, self, file)
    
    def process(self, start):
	"""Creates a page for each file using process_scope"""
	for filename, file in config.ast.files().items():
	    if file.is_main():
		self.process_scope(filename, file)

    def process_scope(self, filename, file):
	"""Creates a page for the given file. The page is just an index,
	containing a list of declarations."""
	toc = config.toc

        # set up filename and title for the current page
	self.__filename = config.files.nameOfFileDetails(filename)
	# (get rid of ../'s in the filename)
	name = string.split(filename, os.sep)
	while len(name) and name[0] == '..': del name[0]
        self.__title = string.join(name, os.sep)+' Details'

	self.start_file()
	self.write(self.manager.formatHeader(self.filename()))
	self.write(entity('h1', string.join(name, os.sep))+'<br>')
	if self.__link_source:
	    link = rel(self.filename(),
		config.files.nameOfFileSource(filename))
	    self.write(href(link, '[File Source]', target="main")+'<br>')

	# Print list of includes
	try:
	    sourcefile = config.ast.files()[filename]
	    includes = sourcefile.includes()
	    # Only show files from the project
	    includes = filter(lambda x: x.target().is_main(), includes)
	    self.write('<h2>Includes from this file:</h2>')
	    if not len(includes):
		self.write('No includes.<br>')
	    for include in includes:
		target_filename = include.target().filename()
		if include.is_next(): idesc = 'include_next '
		else: idesc = 'include '
		if include.is_macro(): idesc = idesc + 'from macro '
		link = rel(self.filename(), config.files.nameOfFileDetails(target_filename))
		self.write(idesc + href(link, target_filename)+'<br>')
	except:
	    pass

	self.write('<h2>Declarations in this file:</h2>')
	# Sort items (by name)
	items = map(lambda decl: (decl.type(), decl.name(), decl), file.declarations())
	items.sort()
	curr_scope = None
	curr_type = None
	comma = 0
	for decl_type, name, decl in items:
	    # Check scope and type to see if they've changed since the last
	    # declaration, thereby forming sections of scope and type
	    decl_scope = name[:-1]
	    if decl_scope != curr_scope or decl_type != curr_type:
		curr_scope = decl_scope
		curr_type = decl_type
		if curr_scope is not None:
		    self.write('\n</div>')
		if len(curr_type) and curr_type[-1] == 's': plural = 'es'
		else: plural = 's'
		if len(curr_scope):
		    self.write('<h3>%s%s in %s</h3>\n<div>'%(
			curr_type.capitalize(), plural,
			anglebrackets(Util.ccolonName(curr_scope))))
		else:
		    self.write('<h3>%s%s</h3>\n<div>'%(curr_type.capitalize(),plural))
		comma = 0
	    # Format this declaration
	    entry = config.toc[name]
	    
	    label = anglebrackets(Util.ccolonName(name, curr_scope))
	    label = replace_spaces(label)
	    if entry:
		link = rel(self.filename(), entry.link)
		text = ' ' + href(link, label)
	    else:
		text = ' ' + label
	    if comma: self.write(',')
	    self.write(text)
	    comma = 1
	
	# Close open DIV
	if curr_scope is not None:
	    self.write('\n</div>')
	self.end_file()
	
htmlPageClass = FileDetails
