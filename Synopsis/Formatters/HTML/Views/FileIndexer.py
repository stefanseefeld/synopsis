# $Id: FileIndexer.py,v 1.3 2003/11/11 06:01:13 stefan Exp $
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
# $Log: FileIndexer.py,v $
# Revision 1.3  2003/11/11 06:01:13  stefan
# adjust to directory/package layout changes
#
# Revision 1.2  2003/01/20 06:43:02  chalky
# Refactored comment processing. Added AST.CommentTag. Linker now determines
# comment summary and extracts tags. Increased AST version number.
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
from Synopsis import AST, Util

# HTML modules
import Page
import core
from core import config
from Tags import *

class FileIndexer (Page.Page):
    """A page that creates an index of files, and an index for each file.
    First the index of files is created, intended for the top-left frame.
    Second a page is created for each file, listing the major declarations for
    that file, eg: classes, global functions, namespaces, etc."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
        self.__filename = ''
        self.__title = ''
	self.__link_source = ('FileSource' in config.pages)
	self.__link_details = ('FileDetails' in config.pages)

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
                filename = config.files.nameOfFileIndex(filename)
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
	self.__filename = config.files.nameOfFileIndex(filename)
	# (get rid of ../'s in the filename)
	name = string.split(filename, os.sep)
	while len(name) and name[0] == '..': del name[0]
        self.__title = string.join(name, os.sep)

	self.start_file()
	self.write(entity('b', string.join(name, os.sep))+'<br>')
	if self.__link_source:
	    link = rel(self.filename(),
		config.files.nameOfFileSource(filename))
	    self.write(href(link, '[File Source]', target="main")+'<br>')
	if self.__link_details:
	    link = rel(self.filename(),
		config.files.nameOfFileDetails(filename))
	    self.write(href(link, '[File Details]', target="main")+'<br>')
	comments = config.comments

	self.write('<b>Declarations:</b><br>')
	# Sort items (by name)
	items = map(lambda decl: (decl.name(), decl), file.declarations())
	items.sort()
	scope, last = [], []
	for name, decl in items:
	    # TODO make this nicer :)
	    entry = config.toc[name]
	    if not entry: continue
	    summary = string.strip("(%s) %s"%(decl.type(),
		anglebrackets(comments.format_summary(self, decl))))
	    # Print link to declaration's page
	    link = rel(self.filename(), entry.link)
	    if isinstance(decl, AST.Function): print_name = decl.realname()
	    else: print_name = name
	    # Increase scope
	    i = 0
	    while i < len(print_name)-1 and i < len(scope) and print_name[i] == scope[i]:
		i = i + 1
	    # Remove unneeded indentation
	    j = i
	    while j < len(scope):
		self.write("</div>")
		j = j + 1
	    # Add new indentation
	    scope[i:j] = []
	    while i < len(print_name)-1:
		scope.append(print_name[i])
		if len(last) >= len(scope) and last[:len(scope)] == scope: div_bit = ""
		else: div_bit = print_name[i]+"<br>"
		self.write('%s<div class="filepage-scope">'%div_bit)
		i = i + 1

	    # Now print the actual item
	    label = anglebrackets(Util.ccolonName(print_name, scope))
	    label = replace_spaces(label)
	    self.write(div('href',href(link, label, target='main', title=summary)))
	    # Store this name incase, f.ex, its a class and the next item is
	    # in that class scope
	    last = list(name)
	# Close open DIVs
	for i in scope:
	    self.write("</div>")
	self.end_file()
	
htmlPageClass = FileIndexer
