# $Id: core.py,v 1.6 2001/02/05 07:58:39 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stephen Davies
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
# $Log: core.py,v $
# Revision 1.6  2001/02/05 07:58:39  chalky
# Cleaned up image copying for *JS. Added synopsis logo to ScopePages.
#
# Revision 1.5  2001/02/05 05:26:24  chalky
# Graphs are separated. Misc changes
#
# Revision 1.4  2001/02/01 20:08:35  chalky
# Added types to config
#
# Revision 1.3  2001/02/01 18:36:55  chalky
# Moved TOC out to Formatter/TOC.py
#
# Revision 1.2  2001/02/01 15:23:24  chalky
# Copywritten brown paper bag edition.
#
# Revision 1.1  2001/02/01 14:58:48  chalky
# Modularized HTML formatter a bit :)
#
# These are from the old HTML.py:
# Revision 1.55  2001/01/24 18:05:16  stefan
# extended the HTML submodule loading to allow for external formatters
#
# Revision 1.54  2001/01/24 13:06:48  chalky
# Fixed bug related to /**/ causing following //. to be on same line
#
# Revision 1.53  2001/01/24 12:50:23  chalky
# Fixes to get summary/detail linking again since AST.Visitor changed ...
#
# Revision 1.52  2001/01/24 01:38:36  chalky
# Added docstrings to all modules
#
# Revision 1.51  2001/01/23 00:20:27  chalky
# Added employee.cc demo. Fixed small bug wrt linking to details of variables.
#
# Revision 1.50  2001/01/22 20:14:36  stefan
# forgot two flags in the usage() function
#
# Revision 1.49  2001/01/22 19:54:41  stefan
# better support for help message
#
# Revision 1.48  2001/01/22 17:06:15  stefan
# added copyright notice, and switched on logging
#

# HTML2 Formatter by Stephen Davies
# Based on HTML Formatter by Stefan Seefeld
#
# Generates a page for each Module/Class and an inheritance tree
# Also 3-frame format inspired by Javadoc

"""
HTML formatter. Output specifies a directory. See manual for usage.
"""

# System modules
import sys, getopt, os, os.path, string, types, errno, stat, re

# Synopsis modules
from Synopsis.Core import AST, Type, Util
from Synopsis.Formatter import TOC, ClassTree

verbose=0

# Create the Config class before importing any HTML modules
class Config:
    """Central configuration repository for HTML formatter.
    This class holds references to the current formatters, tocs, etc."""
    def __init__(self):
	"""Constructor - initialise objects to None."""
	self.commentFormatter = None
	self.commentFormatterList = []
	self.types = None # Filled in first thing in format()
	self.toc = None
	self.basename = None
	self.stylesheet = ""
	self.stylesheet_file = None
	self.sorter = None
	self.namespace = ""
	self.files = None
	self.pageset = None
	self.sorter = None
	self.classTree = None
	self.page_contents = "" # page contents frame (top-left)
	self.page_index = "" # page for index frame (left)

    def fillDefaults(self):
	"""Fill in empty options with defaults"""
	if not self.sorter:
	    import ScopeSorter
	    self.sorter = ScopeSorter.ScopeSorter()
    
    def set_contents_page(self, page):
	"""Call this method to set the contents page. First come first served
	-- whatever module the user puts first in the list that sets this is
	it. This is the frame in the top-left if you use the default frameset."""
	if not self.page_contents: self.page_contents = page
    
    def set_index_page(self, page):
	"""Call this method to set the index page. First come first served
	-- whatever module the user puts first in the list that sets this is
	it. This is the frame on the left if you use the default frameset."""
	if not self.page_index: self.page_index = page

# Create a globally accessible Config. After this point the HTML modules may
# import it into their namespace for ease of use
config = Config()

# HTML modules
import CommentFormatter, FileLayout, ScopeSorter, Page
from Tags import *

def sort(list):
    "Utility func to sort and return the given list"
    list.sort()
    return list

def reference(name, scope, label=None, **keys):
    """Utility method to insert a reference to a name.
    @see ASTFormatter.BaseFormatter.reference()
    """
    if not label: label = Util.ccolonName(name, scope)
    entry = config.toc[name]
    if entry: return apply(href, (entry.link, label), keys)
    return label or ''

class Struct:
    "Dummy class. Initialise with keyword args."
    def __init__(self, **keys):
	for name, value in keys.items(): setattr(self, name, value)

class CommentDictionary:
    """This class just maintains a mapping from declaration to comment, since
    any particular comment is required at least twice. Upon initiation, an
    instance of this class installs itself in the config object as
    "comments"."""
    def __init__(self):
	self.__dict = {}
	self._parser = CommentFormatter.CommentParser()
	config.comments = self
    def commentForName(self, name):
	if self.__dict.has_key(name): return self.__dict[name]
	return None
    def commentFor(self, decl):
	"Returns a comment struct (@see CommentParser) for given decl"
	key = decl.name()
	if self.__dict.has_key(key): return self.__dict[key]
	self.__dict[key] = comment = self._parser.parse(decl)
	return comment
    __getitem__ = commentFor

class FileTree(AST.Visitor):
    """Maintains a tree of directories and files"""
    def __init__(self):
	"Installs self in config object as 'fileTree'"
	config.fileTree = self
	self.__files = {}
    
    def buildTree(self):
	"Takes the visited info and makes a tree of directories and files"
	# Split filenames into lists of directories
	files = []
	for file in self.__files.keys():
	    files.append(string.split(file, os.sep))
	self.__root = Struct(path=(), children={})
	process_list = [self.__root]
	while len(process_list):
	    # Get node
	    node = process_list[0]
	    del process_list[0]
	    for file in files:
		if len(file) <= len(node.path) or tuple(file[:len(node.path)]) != node.path:
		    continue
		child_path = tuple(file[:len(node.path)+1])
		if node.children.has_key(child_path): continue
		child = Struct(path=child_path, children={})
		node.children[child_path] = child
		if len(child_path) < len(file):
		    process_list.append(child)
		else:
		    fname = string.join(file, os.sep)
		    child.decls = self.__files[fname]
    
    def root(self):
	"""Returns the root node in the file tree.
	Each node is a Struct with the following members:
	 path - tuple of dir or filename (eg: 'Prague','Sys','Thread.hh')
	 children - dict of children by their path tuple
	Additionally, leaf nodes have the attribute:
	 decls - dict of declarations in this file by scoped name
	"""
	return self.__root
    
    ### AST Visitor
    
    def visitAST(self, ast):
	for decl in ast.declarations():
	    decl.accept(self)
    def visitDeclaration(self, decl):
	file = decl.file()
	if not file: print "Decl",decl,"has no file."
	if not self.__files.has_key(file):
	    self.__files[file] = {}
	self.__files[file][decl.name()] = decl
    def visitForward(self, decl):
	# Don't include (excluded) forward decls in file listing
	pass
    def visitClass(self, scope):
	self.visitDeclaration(scope)
	# Only nested classes may be in different files
	for decl in scope.declarations():
	    if isinstance(decl, AST.Class):
		self.visitClass(decl)
    def visitMetaModule(self, scope):
	for decl in scope.declarations():
	    decl.accept(self)
	



class PageManager:
    """This class manages and coordinates the various pages. The user adds
    pages by passing their class object to the addPage method. Pages should be
    derived from Page.Page, and their constructors may want to call the
    addRootPage method of the PageManager object to register a name and link
    that is listed along with other root or top-level pages.
    @see Page.Page
    """
    def __init__(self):
	self.__pages = [] #all pages
	self.__roots = [] #pages with roots, list of Structs
	self.__global = None # The global scope

    def globalScope(self):
	"Return the global scope"
	return self.__global

    def _calculateStart(self, root):
	"Calculates the start scope using the 'namespace' config var"
	scope_names = string.split(config.namespace, "::")
	start = root # The running result
	config.sorter.set_scope(root)
	scope = [] # The running name of the start
	for scope_name in scope_names:
	    if not scope_name: break
	    scope.append(scope_name)
	    try:
		child = self.sorter.child(tuple(scope))
		if isinstance(child, AST.Scope):
		    start = child
		    self.sorter.set_scope(start)
		else:
		    raise TypeError, 'calculateStart: Not a Scope'
	    except:
		# Don't continue if scope traversal failed!
		import traceback
		traceback.print_exc()
		print "Fatal: Couldn't find child scope",scope
		sys.exit(3)
	return start

    def addPage(self, pageClass):
	"""Add a page of the given class. An instance is created and stored,
	and its root() method is called and the name,link tuple stored if None
	isn't returned."""
	page = pageClass(self)
	self.__pages.append(page)
    
    def addRootPage(self, name, link, visibility):
	"""Adds a named link to the list of root pages. Called from the
	constructors of Page objects. The root pages are displayed at the top
	of every page, depending on their visibility (higher = more visible).
	@param name	    a string you can use to avoid linking to yourself
	@param link	    a string with the full <a></a> tag (you may want to
			    put a target= in it, for example)
	@param visibility   should be a number such as 1 or 2. Visibility 2 is
			    shown on all pages, 1 only on pages with lots of
			    room. For example, pages for the top-left frame
			    only show visibility 2 pages."""
	self.__roots.append(Struct(name=name,link=link,visibility=visibility))

    def formatRoots(self, from_name, visibility=1):
	"""Formats the list of root pages to HTML. The from_name specifies the
	page that shouldn't be linked. Only root pages of 'visibility' or
	above are included."""
	roots = filter(lambda x,v=visibility: x.visibility >= v, self.__roots)
	roots = map(lambda x,f=from_name: x.name==f and f or x.link, roots)
	return string.join(roots, ' | ')

    def process(self, root):
	"""Create all pages from the start Scope, derived from the root Scope"""
	self.__global = root
	start = self._calculateStart(root)
	for page in self.__pages:
	    page.process(start)

def stdPage(name):
    module = Util._import('Synopsis.Formatter.HTML.'+name)
    if not hasattr(module, 'htmlPageClass'):
	print module.__dict__
    return module.htmlPageClass

def defaultPageset(manager):
    """The default pageset function. This is the default, which creates a
    reasonable set of pages. If you provide your own, you need to call
    manager.addPage() for all the pages you want."""
    manager.addPage(stdPage('ScopePages'))
    manager.addPage(stdPage('ModuleListingJS'))
    #manager.addPage(stdPage('ModuleListing'))
    manager.addPage(stdPage('ModuleIndexer'))
    manager.addPage(stdPage('FileTree'))
    manager.addPage(stdPage('InheritanceTree'))
    manager.addPage(stdPage('InheritanceGraph'))
    # This goes last so others can set default pages
    manager.addPage(stdPage('FramesIndex'))

def usage():
    """Print usage to stdout"""
    print \
"""
  -o <dir>                             Output directory, created if it doesn't exist.
  -s <filename>                        Filename of stylesheet in output directory
  -S <filename>                        Filename of stylesheet to copy
                                       If this is newer than the one in the output directory then it
		                       is copied over it.
  -n <namespace>                       Namespace to output
  -c <formatter>                       add external Comment formatter
  -C <formatter>                       add std Comment formatter (part of the HTML module) to use
                                       - default Nothing
		                       - ssd     Filters for and strips //. comments
		                       - javadoc @tag style comments
                                       - section test section breaks.
		                       You may use multiple -c options
  -t <filename>                        Generate a table of content and write it to <filename>
  -r <filename> ['|'<url>|<directory>] merge in table of content from <filename>, possibly prefixing entries with the
                                       given url or directory, to resolve external symbols"""

def __parseArgs(args):
    global verbose, commentParser, toc_out, toc_in
    global commentFormatterList
    commentFormatters = CommentFormatter.commentFormatters
    # Defaults
    toc_out = ""
    toc_in = []
    config.pageset = defaultPageset
    # Convert the arguments to a list with custom getopt
    try:
        opts,remainder = Util.getopt_spec(args, "hvo:s:n:c:C:S:t:r:")
    except Util.getopt.error, e:
        sys.stderr.write("Error in arguments: " + e + "\n")
        sys.exit(1)

    # Parse the list of arguments
    for opt in opts:
        o,a = opt
        if o == "-o":
            config.basename = a #open(a, "w")
        elif o == "-s":
            config.stylesheet = a
	elif o == "-S":
	    config.stylesheet_file = a
	elif o == "-n":
	    config.namespace = a
	elif o == "-c": commentFormatterList.append(Util._import(a))
	elif o == "-C":
	    if commentFormatters.has_key(a):
		config.commentFormatterList.append(commentFormatters[a]())
	    else:
		print "Error: Unknown formatter. Available comment formatters are:",string.join(commentFormatters.keys(), ', ')
		sys.exit(1)
        elif o == "-t":
            toc_out = a
        elif o == "-r":
            toc_in.append(a)
	elif o == "-h":
	    usage()
	    sys.exit(1)
	elif o == "-v":
	    verbose=1

    # Fill in any unspecified defaults
    config.fillDefaults()

def format(types, declarations, args):
    global toc_out, toc_in
    __parseArgs(args)
    config.types = types

    # Create the file namer
    config.files = FileLayout.FileLayout()

    # Create the Comments Dictionary
    CommentDictionary()

    # Create the Class Tree (TODO: only if needed...)
    config.classTree = ClassTree.ClassTree()

    # Create the File Tree (TODO: only if needed...)
    FileTree()

    # Create table of contents index
    config.toc = TOC.TableOfContents(config.files)
    if verbose: print "HTML Formatter: Initialising TOC"

    # Add all declarations to the namespace tree
    for d in declarations:
	d.accept(config.toc)
    if verbose: print "TOC size:",config.toc.size()
    if len(toc_out): config.toc.store(toc_out)
    
    # load external references from toc files, if any
    for t in toc_in: config.toc.load(t)

    for d in declarations:
	d.accept(config.classTree)
	d.accept(config.fileTree)

    config.fileTree.buildTree()
    
    if verbose: print "HTML Formatter: Writing Pages..."
    # Create the pages
    config.files.chdirBase()
    # TODO: have synopsis pass an AST with a "root" node to formatters.
    root = AST.Module('',-1,"C++","Global",())
    root.declarations()[:] = declarations
    manager = PageManager()
    config.pageset(manager)
    manager.process(root)

    config.files.chdirRestore()

