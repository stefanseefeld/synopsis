# $Id: browse.py,v 1.10 2002/09/28 05:53:31 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2000, 2001 Stefan Seefeld
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
# $Log: browse.py,v $
# Revision 1.10  2002/09/28 05:53:31  chalky
# Refactored display into separate project and browser windows. Execute projects
# in the background
#
# Revision 1.9  2002/08/23 04:35:56  chalky
# Use png images w/ Dot. Only show classes in Classes list
#
# Revision 1.8  2002/04/26 01:21:14  chalky
# Bugs and cleanups
#
# Revision 1.7  2002/01/13 09:45:52  chalky
# Show formatted source code (only works with refmanual..)
#
# Revision 1.6  2002/01/09 11:43:41  chalky
# Inheritance pics
#
# Revision 1.5  2002/01/09 10:16:35  chalky
# Centralized navigation, clicking links in (html) docs works.
#
# Revision 1.4  2001/11/09 15:35:04  chalky
# GUI shows HTML pages. just. Source window also scrolls to correct line.
#
# Revision 1.3  2001/11/09 08:06:59  chalky
# More GUI fixes and stuff. Double click on Source Actions for a dialog.
#
# Revision 1.2  2001/11/07 05:58:21  chalky
# Reorganised UI, opening a .syn file now builds a simple project to view it
#
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#


import sys, pickle, Synopsis, cStringIO, string, re
from qt import *
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.HTML import core, Page, ScopePages, FilePages
from Synopsis.Formatter import Dot
from Synopsis.Formatter import ClassTree
from Synopsis.Formatter.HTML.core import FileTree
from igraph import IGraphWindow

class BrowserWindow (QSplitter):
    """The browser window displays an AST in the familiar (from JavaDoc)
    three-pane view. In addition to JavaDoc, the right pane can display
    documentation, source or a class hierarchy graph. The bottom-left pane can
    also display classes or files."""

    class SelectionListener:
	"""Defines the interface for an object that listens to the browser
	selection"""
	def current_decl_changed(self, decl):
	    "Called when the current decl changes"
	    pass
	
	def current_package_changed(self, package):
	    "Called when the current package changes"
	    pass

	def current_ast_changed(self, ast):
	    """Called when the current AST changes. Browser's glob will be
	    updated first"""
	    pass
    
    def __init__(self, main_window, filename, project_window):
	QSplitter.__init__(self, main_window.workspace)
	self.main_window = main_window
	self.project_window = project_window
	self.filename = filename
	self.setCaption('Output Window')

	self.classTree = ClassTree.ClassTree()
	self.fileTree = None #FileTree()
	#self.tabs = QTabWidget(self)
	#self.listView = QListView(self)
	self._make_left()
	self._make_right()

	#CanvasWindow(parent, main_window, self.project)
	
	self.setSizes([30,70])
	#self.showMaximized()
	#self.show()

	self.__ast = None     # The current AST
	self.__package = None # The current package
	self.__decl = None    # The current decl
	self.__listeners = [] # Listeners for changes in current selection

	self.glob = AST.Scope('', -1, '', 'Global', ('global',))

	# Connect things up
	self.connect(self.right_tab, SIGNAL('currentChanged(QWidget*)'), self.tabChanged)

	# Make the menu, to be inserted in the app menu upon window activation
	self._file_menu = QPopupMenu(self.main_window.menuBar())
	self._graph_id = self._file_menu.insertItem("&Graph class inheritance", self.openGraph, Qt.CTRL+Qt.Key_G)

	self.__activated = 0
	self.connect(self.parent(), SIGNAL('windowActivated(QWidget*)'), self.windowActivated)

	#self.glob.accept(self.classTree)

	self.browsers = (
	    PackageBrowser(self),
	    ClassBrowser(self),
	    DocoBrowser(self),
	    SourceBrowser(self),
	    GraphBrowser(self)
	)

	if filename:
	    self.load_file()
	    self.setCaption(filename+' : Output Window')
	
	main_window.add_window(self)


    def _make_left(self):
	self.left_split = QSplitter(Qt.Vertical, self)
	self.package_list = QListView(self.left_split)
	self.package_list.addColumn('Package')
	self.left_tab = QTabWidget(self.left_split)
	self.class_list = QListView(self.left_tab)
	self.class_list.addColumn('Class')
	self.file_list = QListView(self.left_tab)
	self.file_list.addColumn('File')
	self.left_tab.addTab(self.class_list, "Classes")
	self.left_tab.addTab(self.file_list, "Files")
	self.left_split.setSizes([30,70])

    def _make_right(self):
	self.right_tab = QTabWidget(self)
	self.doco_display = QTextBrowser(self.right_tab)
	self.doco_display.setTextFormat(Qt.RichText)
	self.doco_display.setText("<i>Select a package/namespace to view from the left.")
	self.source_display = QTextBrowser(self.right_tab)
	self.graph_display = IGraphWindow(self.right_tab, self.main_window, self.classTree)
	self.right_tab.addTab(self.doco_display, "Documentation")
	self.right_tab.addTab(self.source_display, "Source")
	self.right_tab.addTab(self.graph_display, "Graph")

    def add_listener(self, listener):
	"""Adds a listener for changes. The listener must implement the
	SelectionListener interface"""
	if not isinstance(listener, BrowserWindow.SelectionListener):
	    raise TypeError, 'Not an implementation of SelectionListener'
	self.__listeners.append(listener)

    def current_decl(self):
	"""Returns the current declaration being viewed by the project"""
	return self.__decl

    def set_current_decl(self, decl):
	"""Sets the current declaration being viewed by the project. This will
	also notify all displays"""
	self.__decl = decl

	for listener in self.__listeners:
	    try: listener.current_decl_changed(decl)
	    except:
		import traceback
		print "Exception occurred dispatching to",listener
		traceback.print_exc()
    
    def set_current_package(self, package):
	"""Sets the current package (a Scope declaration) being viewed by the
	project. This will also notify all displays"""
	self.__package = package

	for listener in self.__listeners:
	    listener.current_package_changed(package)

    def set_current_ast(self, ast):
	self.__ast = ast
	self.glob.declarations()[:] = ast.declarations()
	self.glob.accept(self.classTree)

	for listener in self.__listeners:
	    listener.current_ast_changed(ast)
    
    def current_ast(self):
	return self.__ast

    def windowActivated(self, widget):
	if self.__activated:
	    if widget is not self: self.deactivate()
	elif widget is self: self.activate()
    
    def activate(self):
	self.__activated = 1
	self._menu_id = self.main_window.menuBar().insertItem('AST', self._file_menu)

    def deactivate(self):
	self.__activated = 0
	self.main_window.menuBar().removeItem(self._menu_id)


    def openGraph(self):
	IGraphWindow(self.main_window.workspace, self.main_window,
		     self.classTree).set_class(self.decl.name())

    def setGraphEnabled(self, enable):
	self._file_menu.setItemEnabled(self._graph_id, enable)
	if enable:
	    self.graph_display.set_class(self.decl.name())
	#else:
	#    self.window.graph_display.hide()

    def tabChanged(self, widget):
	self.set_current_decl(self.current_decl())

    def load_file(self):
	"""Loads the AST from disk."""
	try:
	    self.set_current_ast(AST.load(self.filename))
	except Exception, e:
	    print e

class ListFiller( AST.Visitor ):
    """A visitor that fills in a QListView from an AST"""
    def __init__(self, main, listview, types = None, anti_types = None):
	self.map = {}
	self.main = main
	self.listview = listview
	self.stack = [self.listview]
	self.types = types
	self.anti_types = anti_types
	self.auto_open = 1

    def clear(self):
	self.listview.clear()
	self.map = {}
	self.stack = [self.listview]

    def fillFrom(self, decl):
	self.addGroup(decl)
	self.listview.setContentsPos(0,0)

    def visitDeclaration(self, decl):
	if self.types and decl.type() not in self.types: return
	if self.anti_types and decl.type() in self.anti_types: return
	self.addDeclaration(decl)

    def addDeclaration(self, decl):
	item = QListViewItem(self.stack[-1], decl.name()[-1], decl.type())
	self.map[item] = decl
	self.__last = item

    def visitGroup(self, group):
	if self.types and group.type() not in self.types: return
	if self.anti_types and group.type() in self.anti_types: return
	self.addGroup(group)

    def addGroup(self, group):
	self.addDeclaration(group)
	item = self.__last
	self.stack.append(item)
	for decl in group.declarations(): decl.accept(self)
	self.stack.pop()
	if len(self.stack) <= self.auto_open: self.listview.setOpen(item, 1)

    def visitForward(self, fwd): pass

    def visitEnum(self, enum):
	if self.types and enum.type() not in self.types: return
	if self.anti_types and enum.type() in self.anti_types: return
	self.addDeclaration(enum)
	item = self.__last
	self.stack.append(item)
	for decl in enum.enumerators(): decl.accept(self)
	self.stack.pop()
	if len(self.stack) <= self.auto_open: self.listview.setOpen(item, 1)

class PackageBrowser (BrowserWindow.SelectionListener):
    """Browser that manages the package view"""
    def __init__(self, browser):
	self.__browser = browser
	browser.add_listener(self)

	# Create the filler. It only displays a few types
	self.filler = ListFiller(self, browser.package_list, (
	    'package', 'module', 'namespace', 'global'))
	self.filler.auto_open = 3

	browser.connect(browser.package_list, SIGNAL('selectionChanged(QListViewItem*)'), self.select_package_item)

    def select_package_item(self, item):
	"""Show a given package (by item)"""
	decl = self.filler.map[item]
	self.__browser.set_current_package(decl)
	self.__browser.set_current_decl(decl)

    def DISABLED_current_package_changed(self, decl):
	self.setGraphEnabled(0)
	# Grab the comments and put them in the text view
	os = cStringIO.StringIO()
	for comment in decl.comments():
	    os.write(comment.text())
	    os.write('<hr>')
	self.__browser.doco_display.setText(os.getvalue())

    def current_ast_changed(self, ast):
	self.filler.fillFrom(self.__browser.glob)

class ClassBrowser (BrowserWindow.SelectionListener):
    """Browser display that manages the class view"""
    def __init__(self, browser):
	self.__browser = browser
	self.__glob = AST.Scope('', -1, '', 'Global Classes', ('global',))
	browser.add_listener(self)

	# Create the filler
	self.filler = ListFiller(self, browser.class_list, None, (
	    'Package', 'Module', 'Namespace', 'Global'))

	browser.connect(browser.class_list, SIGNAL('selectionChanged(QListViewItem*)'), self.select_decl_item)
	browser.connect(browser.class_list, SIGNAL('expanded(QListViewItem*)'), self.selfish_expansion)

    def select_decl_item(self, item):
	"""Show a given declaration (by item)"""
	decl = self.filler.map[item]
	self.__browser.set_current_decl(decl)

    def current_package_changed(self, package):
	"Refill the tree with the new package as root"
	self.filler.clear()
	self.filler.fillFrom(package)

    def selfish_expansion(self, item):
	"""Selfishly makes item the only expanded node"""
	if not item.parent(): return
	iter = item.parent().firstChild()
	while iter:
	    if iter != item: self.__browser.class_list.setOpen(iter, 0)
	    iter = iter.nextSibling()

    def current_ast_changed(self, ast):
	self.filler.clear()
	glob_all = self.__browser.glob.declarations()
	classes = lambda decl: decl.type() == 'class'
	glob_cls = filter(classes, glob_all)
	self.__glob.declarations()[:] = glob_cls
	self.filler.fillFrom(self.__glob)

class DocoBrowser (BrowserWindow.SelectionListener):
    """Browser that manages the documentation view"""
    class BufferScopePages (ScopePages.ScopePages, Page.BufferPage):
	def __init__(self, manager):
	    ScopePages.ScopePages.__init__(self, manager)
	    Page.BufferPage._take_control(self)

    def __init__(self, browser):
	self.__browser = browser
	browser.add_listener(self)

	self.mime_factory = SourceMimeFactory()
	self.mime_factory.set_browser(self)
	self.__browser.doco_display.setMimeSourceFactory(self.mime_factory)

	self.__generator = None

	self.__getting_mime = 0
	
    def generator(self):
	if not self.__generator:
	    self.__generator = self.BufferScopePages(core.manager)
	return self.__generator

    def current_decl_changed(self, decl):
	if not self.__browser.doco_display.isVisible():
	    # Not visible, so ignore
	    return
	if isinstance(decl, AST.Scope):
	    # These we can use the HTML scope formatter on
	    pages = self.generator()
	    pages.process_scope(decl)
	    self.__text = pages.get_buffer()
	    if self.__getting_mime: return
	    context = pages.filename()
	    self.__browser.doco_display.setText(self.__text, context)
	elif decl:
	    # Need more work to use HTML on this.. use ASCIIFormatter for now
	    os = cStringIO.StringIO()
	    os.write('<pre>')
	    formatter = ASCIIFormatter(os)
	    formatter.set_scope(decl.name())
	    decl.accept(formatter)
	    self.__browser.doco_display.setText(os.getvalue())
    
    def current_ast_changed(self, ast):
	core.configure_for_gui(ast)

	scope = AST.Scope('',-1,'','','')
	scope.declarations()[:] = ast.declarations()
	self.generator().register_filenames(scope)

    def get_mime_data(self, name):
	if name[-16:] == '-inheritance.png':
	    # inheritance graph.
	    # Horrible Hack Time
	    try:
		# Convert to .html name
		html_name = name[:-16] + '.html'
		page, scope = core.manager.filename_info(html_name) # may throw KeyError

		super = core.config.classTree.superclasses(scope.name())
		sub = core.config.classTree.subclasses(scope.name())
		if len(super) == 0 and len(sub) == 0:
		    # Skip classes with a boring graph
		    return None
		tmp = '/tmp/synopsis-inheritance.png'
		dot_args = ['-o', tmp, '-f', 'png', '-s']
		Dot.toc = core.config.toc
		Dot.nodes = {}
		ast = AST.AST([''], [scope], core.config.types)
		Dot.format(dot_args, ast, None)
		data = QImageDrag(QImage(tmp))
		os.unlink(tmp)
		return data
	    except KeyError:
		print "inheritance doesnt have a page!"
		pass
	# Try for html page
	try:
	    page, scope = core.manager.filename_info(name)
	    if page is self.generator():
		self.__getting_mime = 1
		self.__browser.set_current_decl(scope)
		self.__getting_mime = 0
		return QTextDrag(self.__text, self.__browser.doco_display, '')
	except KeyError:
	    pass
	return None
	    

class SourceMimeFactory (QMimeSourceFactory):
    def set_browser(self, browser): self.__browser = browser
    def data(self, name):
	d = self.__browser.get_mime_data(str(name))
	return d    


re_tag = re.compile('<(?P<close>/?)(?P<tag>[a-z]+)( class="(?P<class>[^"]*?)")?(?P<href> href="[^"]*?")?( name="[^"]*?")?[^>]*?>')
tags = {
    'file-default' : ('', ''),
    'file-indent' : ('<tt>', '</tt>'),
    'file-linenum' : ('<font color=red><tt>', '</tt></font>'),
    'file-comment' : ('<font color=purple>', '</font>'),
    'file-keyword' : ('<b>', '</b>'),
    'file-string' : ('<font color=#008000>', '</font>'),
    'file-number' : ('<font color=#000080>', '</font>'),
}
def format_source(text):
    """The source relies on stylesheets, and Qt doesn't have powerful enough
    stylesheets. This function manually converts the html..."""
    #print '===\n%s\n==='%text
    mo = re_tag.search(text)
    stack = [] # stack of close tags
    result = [] # list of strings for result text
    pos = 0
    while mo:
	start, end, tag = mo.start(), mo.end(), mo.group('tag')
	result.append(text[pos:start])
	#if mo.group('close') != '/':
	    #print "OPEN::",tag,mo.group()
	if mo.group('close') == '/':
	    # close tag
	    result.append(stack.pop())
	    #print "CLOSE:",tag,result[-1]
	    #print len(stack), stack
	elif tag == 'span':
	    # open tag
	    span_class = mo.group('class')
	    if tags.has_key(span_class):
		open, close = tags[span_class]
		result.append(open)
		stack.append(close)
	    else:
		# unknown class
		result.append(mo.group())
		#print "UNKNOWN:",span_class
		stack.append('</span>')
	elif tag == 'a':
	    result.append(mo.group())
	    if mo.group('href'):
		result.append('<font color=#602000>')
		stack.append('</font></a>')
	    else:
		stack.append('</a>')
	elif tag in ('br', 'hr'):
	    result.append(mo.group())
	else:
	    result.append(mo.group())
	    stack.append('</%s>'%tag)
	mo = re_tag.search(text, end)
	pos = end
    result.append(text[pos:])
    text = string.join(result, '')
    #print '===\n%s\n==='%text
    return text


class SourceBrowser (BrowserWindow.SelectionListener):
    """Browser that manages the source view"""
    class BufferFilePages (FilePages.FilePages, Page.BufferPage):
	def __init__(self, manager):
	    FilePages.FilePages.__init__(self, manager)
	    Page.BufferPage._take_control(self)

    def __init__(self, browser):
	self.__browser = browser
	browser.add_listener(self)

	self.mime_factory = SourceMimeFactory()
	self.mime_factory.set_browser(self)
	self.__browser.source_display.setMimeSourceFactory(self.mime_factory)

	self.__generator = None

	self.__current_file = None

	self.__getting_mime = 0

	self.__browser.connect(self.__browser.source_display,
	    SIGNAL('highlighted(const QString&)'), self.highlighted)

    def highlighted(self, text):
	print text

    def generator(self):
	if not self.__generator:
	    fileconfig = core.config.obj.FilePages
	    fileconfig.links_path = 'docs/RefManual/syn/%s-links'
	    self.__generator = self.BufferFilePages(core.manager)
	    self.__generator.register_filenames(None)
	return self.__generator

    def current_decl_changed(self, decl):
	if not self.__browser.source_display.isVisible():
	    # Not visible, so ignore
	    return
	if decl is None:
	    self.__browser.source_display.setText('')
	    self.__current_file = ''
	    return
	file, line = decl.file(), decl.line()
	if self.__current_file != file:
	    # Check for empty file
	    if not file:
		self.__browser.source_display.setText('')
		return
	    print "looking for", file
	    
	    filepath = string.split(file, os.sep)
	    filename = core.config.files.nameOfScopedSpecial('page', filepath)
	    page, scope = core.manager.filename_info(filename)
	    pages = self.generator()
	    if page is pages:
		pages.process_scope(scope)
		self.__text = pages.get_buffer()
		self.__text = format_source(self.__text)
		if self.__getting_mime: return
		context = pages.filename()
		self.__browser.source_display.setText(self.__text, context)
	    else:
		# Open the new file and give it line numbers
		text = open(file).read()
		# number the lines
		lines = string.split(text, '\n')
		for line in range(len(lines)): lines[line] = str(line+1)+'| '+lines[line]
		text = string.join(lines, '\n')
		# set text
		self.__browser.source_display.setText(text)
	    self.__current_file = file
	# scroll to line
	if type(line) != type(''): # some lines are '' for some reason..
	    y = (self.__browser.source_display.fontMetrics().height()+1) * line
	    self.__browser.source_display.setContentsPos(0, y)
	#print line, y

    def current_ast_changed(self, ast):
	core.configure_for_gui(ast)

	scope = AST.Scope('',-1,'','','')
	scope.declarations()[:] = ast.declarations()
	self.generator().register_filenames(scope)



class GraphBrowser (BrowserWindow.SelectionListener):
    """Browser that manages the graph view"""
    def __init__(self, browser):
	self.__browser = browser
	browser.add_listener(self)
    def current_decl_changed(self, decl):
	if not self.__browser.graph_display.isVisible():
	    # Not visible, so ignore
	    return
	if decl is None: return
	self.__browser.graph_display.set_class(decl.name())

