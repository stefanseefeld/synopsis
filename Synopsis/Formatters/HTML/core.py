# $Id: core.py,v 1.35 2002/10/28 06:12:30 chalky Exp $
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
# Revision 1.35  2002/10/28 06:12:30  chalky
# Add structs_as_classes option
#
# Revision 1.34  2002/10/27 12:05:44  chalky
# Add 2 verbose levels, make default (1) less verbose
#
# Revision 1.33  2002/10/11 06:02:33  chalky
# Allow GUi to pass config object
#
# Revision 1.32  2002/08/23 04:37:26  chalky
# Huge refactoring of Linker to make it modular, and use a config system similar
# to the HTML package
#
# Revision 1.31  2002/07/04 06:43:18  chalky
# Improved support for absolute references - pages known their full path.
#
# Revision 1.30  2002/07/04 05:16:19  chalky
# Impl output_dir option for config to replace -o argument to HTML formatter.
#
# Revision 1.29  2002/03/14 00:19:47  chalky
# Added demo of template specializations, and fixed HTML formatter to deal with
# angle brackets in class names :)
#
# Revision 1.28  2002/01/13 09:44:51  chalky
# Allow formatted source in GUI
#
# Revision 1.27  2002/01/09 11:43:41  chalky
# Inheritance pics
#
# Revision 1.26  2002/01/09 10:16:35  chalky
# Centralized navigation, clicking links in (html) docs works.
#
# Revision 1.25  2001/11/09 15:35:04  chalky
# GUI shows HTML pages. just. Source window also scrolls to correct line.
#
# Revision 1.24  2001/07/19 05:10:39  chalky
# Use filenames stored in AST object
#
# Revision 1.23  2001/07/19 04:03:05  chalky
# New .syn file format.
#
# Revision 1.22  2001/07/10 14:41:22  chalky
# Make treeformatter config nicer
#
# Revision 1.21  2001/07/10 02:55:51  chalky
# Better comments in some files, and more links work with the nested layout
#
# Revision 1.20  2001/07/05 02:08:35  uid20151
# Changed the registration of pages to be part of a two-phase construction
#
# Revision 1.19  2001/07/04 08:17:48  uid20151
# Comments
#
# Revision 1.18  2001/06/28 07:22:18  stefan
# more refactoring/cleanup in the HTML formatter
#
# Revision 1.17  2001/06/26 04:32:16  stefan
# A whole slew of changes mostly to fix the HTML formatter's output generation,
# i.e. to make the output more robust towards changes in the layout of files.
#
# the rpm script now works, i.e. it generates source and binary packages.
#
# Revision 1.16  2001/06/16 01:29:42  stefan
# change the HTML formatter to not use chdir, as this triggers a but in python's import implementation
#
# Revision 1.15  2001/06/05 05:28:34  chalky
# Some old tree abstraction
#
# Revision 1.14  2001/05/25 13:45:49  stefan
# fix problem with getopt error reporting
#
# Revision 1.13  2001/04/06 02:39:26  chalky
# Get toc in/out from config
#
# Revision 1.12  2001/04/06 01:41:09  chalky
# More verbose verbosity
#
# Revision 1.11  2001/03/29 14:05:33  chalky
# Can give namespace to manager._calculateStart. Timing of pages if verbose
#
# Revision 1.10  2001/02/13 02:54:15  chalky
# Added Name Index page
#
# Revision 1.9  2001/02/12 04:08:09  chalky
# Added config options to HTML and Linker. Config demo has doxy and synopsis styles.
#
# Revision 1.8  2001/02/07 14:13:51  chalky
# Small fixes.
#
# Revision 1.7  2001/02/06 05:13:05  chalky
# Fixes
#
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
Core module for the HTML Formatter.

This module is the first to be loaded, and it creates the global 'core.config'
object before creating any pages. It also handles the command line parsing for
this module, and coordinates the actual output generation.
"""

# System modules
import sys, getopt, os, os.path, string, types, errno, stat, re, time

# Synopsis modules
from Synopsis.Config import Base
from Synopsis.Core import AST, Type, Util
from Synopsis.Formatter import TOC, ClassTree
from Synopsis.Formatter.HTML import TreeFormatter

from Synopsis.Core.Util import import_object

verbose=0

# Create the Config class before importing any HTML modules
class Config:
    """Central configuration repository for HTML formatter.
    This class holds references to the current formatters, tocs, etc."""
    def __init__(self):
	"""Constructor - initialise objects to None."""
	self.ast = None
	self.commentFormatter = None
	self.commentFormatterList = []
	self.types = None # Filled in first thing in format()
	self.toc = None
	self.toc_out = ""
	self.toc_in = []
	self.basename = None
	self.datadir = None
	self.stylesheet = ""
	self.stylesheet_file = None
	self.sorter = None
	self.namespace = ""
	self.files = None
	self.files_class = None
	self.pageset = None
	self.sorter = None
	self.classTree = None
	self.structs_as_classes = 0
	self.treeFormatterClass = TreeFormatter.TreeFormatter
	self.page_contents = "" # page contents frame (top-left)
	self.page_index = "" # page for index frame (left)
	self.pages = [
	    'ScopePages', 'ModuleListing', 'ModuleIndexer', 'FileTree',
	    'InheritanceTree', 'InheritanceGraph', 'NameIndex', 'FramesIndex'
	]
	self.verbose = 0


    def fillDefaults(self):
	"""Fill in empty options with defaults"""
	if not self.sorter:
	    import ScopeSorter
	    self.sorter = ScopeSorter.ScopeSorter()
    
    def use_config(self, obj):
	"""Extracts useful attributes from 'obj' and stores them. The object
	itself is also stored as config.obj"""
	# obj.pages is a list of module names
	self.obj = obj
	if hasattr(obj, 'verbose'): self.verbose = obj.verbose
	options = ('pages', 'sorter', 'datadir', 'stylesheet', 'stylesheet_file',
	    'comment_formatters', 'toc_out', 'toc_in', 'tree_formatter',
	    'file_layout', 'output_dir', 'structs_as_classes')
	for option in options:
	    if hasattr(obj, option):
		getattr(self, '_config_'+option)(getattr(obj, option))
	    elif self.verbose > 1: print "Option",option,"not found in config."

    def _config_pages(self, pages):
	"Configures from the given list of pages"
	if type(pages) != types.ListType:
	    raise TypeError, "HTML.pages must be a list."
	if self.verbose > 1: print "Using pages:",pages
	self.pages = pages

    def _config_sorter(self, sorter):
	if self.verbose > 1: print "Using sorter:",sorter
	self.sorter = import_object(sorter)()

    def _config_datadir(self, datadir):
	if self.verbose > 1: print "Using datadir:", datadir
	self.datadir = datadir

    def _config_output_dir(self, output_dir):
	if self.verbose > 1: print "Using output_dir:", output_dir
	self.basename = output_dir

    def _config_stylesheet(self, stylesheet):
	if self.verbose > 1: print "Using stylesheet:", stylesheet
	self.stylesheet = stylesheet

    def _config_stylesheet_file(self, stylesheet_file):
	if self.verbose > 1: print "Using stylesheet file:", stylesheet_file
	self.stylesheet_file = stylesheet_file

    def _config_comment_formatters(self, comment_formatters):
	if self.verbose > 1: print "Using comment formatters:", comment_formatters
	basePackage = 'Synopsis.Formatter.HTML.CommentFormatter.'
	for formatter in comment_formatters:
	    if type(formatter) == types.StringType:
		if CommentFormatter.commentFormatters.has_key(formatter):
		    self.commentFormatterList.append(
			CommentFormatter.commentFormatters[formatter]()
		    )
		else:
		    raise ImportError, "No builtin comment formatter '%s'"%formatter
	    else:
		clas = import_object(formatter, basePackage)
		self.commentFormatterList.append(clas())
    
    def _config_toc_in(self, toc_in):
	if self.verbose > 1: print "Will read toc(s) from",toc_in
	self.toc_in = toc_in

    def _config_toc_out(self, toc_out):
	if self.verbose > 1: print "Will save toc to",toc_out
	self.toc_out = toc_out

    def _config_tree_formatter(self, tree_class):
	if self.verbose > 1: print "Using tree class",tree_class
	clas = import_object(tree_class, basePackage='Synopsis.Formatter.HTML.')
	self.treeFormatterClass = clas
    
    def _config_file_layout(self, layout):
	if self.verbose > 1: print "Using file layout",layout
	self.files_class = import_object(layout)

    def _config_structs_as_classes(self, yesno):
	if self.verbose > 1: print "Using structs as classes:",yesno
	self.structs_as_classes = yesno
    
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

def old_reference(name, scope, label=None, **keys):
    """Utility method to insert a reference to a name.
    @see ASTFormatter.BaseFormatter.reference()
    """
    if not label: label = anglebrackets(Util.ccolonName(name, scope))
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
	for file in config.ast.filenames():
	    self.__files[file] = {}
    
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
	if not file: return #print "Decl",decl,"has no file."
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
	self.__files = {} # map from filename to (page,scope)
	self._loadPages()

    def globalScope(self):
	"Return the global scope"
	return self.__global

    def _calculateStart(self, root, namespace=None):
	"Calculates the start scope using the 'namespace' config var"
	scope_names = string.split(namespace or config.namespace, "::")
	start = root # The running result
	config.sorter.set_scope(root)
	scope = [] # The running name of the start
	for scope_name in scope_names:
	    if not scope_name: break
	    scope.append(scope_name)
	    try:
		child = config.sorter.child(tuple(scope))
		if isinstance(child, AST.Scope):
		    start = child
		    config.sorter.set_scope(start)
		else:
		    raise TypeError, 'calculateStart: Not a Scope'
	    except:
		# Don't continue if scope traversal failed!
		import traceback
		traceback.print_exc()
		print "Fatal: Couldn't find child scope",scope
		print "Children:",map(lambda x:x.name(), config.sorter.children())
		sys.exit(3)
	return start

    def addPage(self, pageClass):
	"""Add a page of the given class. An instance is created and stored,
	and its root() method is called and the name,link tuple stored if None
	isn't returned."""
	page = pageClass(self)
	self.__pages.append(page)
	page.register()
    
    def addRootPage(self, file, label, target, visibility):
	"""Adds a named link to the list of root pages. Called from the
	constructors of Page objects. The root pages are displayed at the top
	of every page, depending on their visibility (higher = more visible).
	@param file	    the filename, to be used when generating the link
	@param label	    the label of the page
        @param target       target frame
	@param visibility   should be a number such as 1 or 2. Visibility 2 is
			    shown on all pages, 1 only on pages with lots of
			    room. For example, pages for the top-left frame
			    only show visibility 2 pages."""
	self.__roots.append(Struct(file=file, label=label, target=target, visibility=visibility))

    def formatHeader(self, origin, visibility=1):
	"""Formats the list of root pages to HTML. The origin specifies the
	generated page itself (which shouldn't be linked), such that the relative
        links can be generated. Only root pages of 'visibility' or
	above are included."""
        #filter out roots that are visible
	roots = filter(lambda x,v=visibility: x.visibility >= v, self.__roots)
        #a function generating a link
	other = lambda x, o=origin, span=span: span('root-other', href(rel(o, x.file), x.label, target=x.target))
        #a function simply printing label
	current = lambda x, span=span: span('root-current', x.label)
        # generate the header
	roots = map(lambda x, o=origin, other=other, current=current: x.file==o and current(x) or other(x), roots)
	return string.join(roots, ' | \n')+'\n<hr>\n'

    def process(self, root):
	"""Create all pages from the start Scope, derived from the root Scope"""
	self.__global = root
	start = self._calculateStart(root)
	for page in self.__pages:
	    if config.verbose: start_time = time.time()
	    page.process(start)
	    if config.verbose: print "Time for %s: %f"%(page.__class__.__name__, time.time() - start_time)

    def _loadPages(self):
	"""Loads the page objects from the config.pages list. Each element is
	either a string or a tuple of two strings. One string means load the named
	module and look for a 'htmlPageClass' attribute in it. A tuple of two
	strings means load the module from the first string, and look for an
	attribute using the second string."""
	defaultAttr = 'htmlPageClass'
	basePackage = 'Synopsis.Formatter.HTML.'
	for page in config.pages:
	    self.addPage(import_object(page, defaultAttr, basePackage))
    
    def register_filename(self, filename, page, scope):
	"""Registers a file for later production"""
	self.__files[str(filename)] = (page, scope)

    def filename_info(self, filename):
	"""Returns information about a registered file, as a (page,scope)
	pair. May throw a KeyError if the filename isn't registered."""
	return self.__files[str(filename)]

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

def __parseArgs(args, config_obj):
    global verbose, commentParser, toc_out, toc_in
    commentFormatters = CommentFormatter.commentFormatters
    # Defaults

    # Convert the arguments to a list with custom getopt
    try:
        opts,remainder = Util.getopt_spec(args, "hvo:s:n:c:C:S:t:r:d:")
    except Util.getopt.error, e:
        sys.stderr.write("Error in arguments: " + str(e) + "\n")
        sys.exit(1)

    # Check for verbose first so config loading can use it
    for o, a in opts: 
	if o == '-v': 
	    config.verbose = verbose = 1

    # Use config object if present
    if config_obj: config.use_config(config_obj)
    elif config.verbose: print "No config object given"

    # Parse the list of arguments
    for opt in opts:
        o,a = opt
        if o == "-o":
            config.basename = a #open(a, "w")
        elif o == "-d":
            config.datadir = a
        elif o == "-s":
            config.stylesheet = a
	elif o == "-S":
	    config.stylesheet_file = a
	elif o == "-n":
	    config.namespace = a
	elif o == "-c": config.commentFormatterList.append(Util._import(a))
	elif o == "-C":
	    if commentFormatters.has_key(a):
		config.commentFormatterList.append(commentFormatters[a]())
	    else:
		print "Error: Unknown formatter. Available comment formatters are:",string.join(commentFormatters.keys(), ', ')
		sys.exit(1)
        elif o == "-t":
            config.toc_out = a
        elif o == "-r":
            config.toc_in.append(a)
	elif o == "-h":
	    usage()
	    sys.exit(1)
	elif o == "-v":
	    verbose=1
	    config.verbose = 1

    # Fill in any unspecified defaults
    config.fillDefaults()

def format(args, ast, config_obj):
    global toc_out, toc_in
    __parseArgs(args, config_obj)
    config.ast = ast
    config.types = ast.types()
    declarations = ast.declarations()

    # Instantiate the files object
    config.files = config.files_class()

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
    if len(config.toc_out): config.toc.store(config.toc_out)
    
    # load external references from toc files, if any
    for t in config.toc_in: config.toc.load(t)

    for d in declarations:
	d.accept(config.classTree)
	d.accept(config.fileTree)

    config.fileTree.buildTree()
    
    if verbose: print "HTML Formatter: Writing Pages..."
    # Create the pages
    # Create a dummy Module for the global namespace to simplify things
    root = AST.Module('',-1,"C++","Global",())
    root.declarations()[:] = declarations
    manager = PageManager()
    manager.process(root)

def configure_for_gui(ast, config_obj):
    global manager

    if config.ast is ast: return

    __parseArgs(["-o","/tmp"], config_obj)
    config.ast = ast
    config.types = ast.types()
    declarations = ast.declarations()
    config.files = config.files_class()
    CommentDictionary()
    config.classTree = ClassTree.ClassTree()
    FileTree()

    # Create table of contents index
    config.toc = TOC.TableOfContents(config.files)

    # Add all declarations to the namespace tree
    for d in declarations:
	d.accept(config.toc)
    #if len(config.toc_out): config.toc.store(config.toc_out)
    
    # load external references from toc files, if any
    for t in config.toc_in: config.toc.load(t)

    for d in declarations:
	d.accept(config.classTree)
	d.accept(config.fileTree)

    config.fileTree.buildTree()
    
    # Create the pages
    # Create a dummy Module for the global namespace to simplify things
    #root = AST.Module('',-1,"C++","Global",())
    #root.declarations()[:] = declarations
    manager = PageManager()
    #manager.process(root)

 
