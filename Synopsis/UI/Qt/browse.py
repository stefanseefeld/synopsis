# $Id: browse.py,v 1.5 2002/01/09 10:16:35 chalky Exp $
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


import sys, pickle, Synopsis, cStringIO, string
from qt import *
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.HTML import Page, ScopePages, core

class Browser:
    """Controls the browse widgets of the project window"""

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
    
    def __init__(self, window):
	"""Initialises the browser widgets"""
	#QSplitter.__init__(self, parent)
	self.window = window
	self.main_window = window.main_window
	self.__ast = None
	self.__package = None
	self.__decl = None
	self.__listeners = []

	self.glob = AST.Scope('', -1, '', 'Global', ('global',))

	window.doco_display.setTextFormat(Qt.RichText)

	# Connect things up
	window.connect(window.right_tab, SIGNAL('currentChanged(QWidget*)'), self.tabChanged)

	window.doco_display.setText("<i>Select a package/namespace to view from the left.")

	# Make the menu, to be inserted in the app menu upon window activation
	self._file_menu = QPopupMenu(window.main_window.menuBar())
	self._graph_id = self._file_menu.insertItem("&Graph class inheritance", self.openGraph, Qt.CTRL+Qt.Key_G)

	self.__activated = 0
	window.connect(window.parent(), SIGNAL('windowActivated(QWidget*)'), self.windowActivated)

	self.classTree = window.classTree
	#self.glob.accept(self.classTree)

	self.browsers = (
	    PackageBrowser(window, self),
	    ClassBrowser(window, self),
	    DocoBrowser(window, self),
	    SourceBrowser(window, self),
	    GraphBrowser(window, self)
	)

	self.decl = None

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

    def add_listener(self, listener):
	"""Adds a listener for changes. The listener must implement the
	SelectionListener interface"""
	if not isinstance(listener, Browser.SelectionListener):
	    raise TypeError, 'Not an implementation of SelectionListener'
	self.__listeners.append(listener)

    def set_current_ast(self, ast):
	self.__ast = ast
	self.glob.declarations()[:] = ast.declarations()
	self.glob.accept(self.classTree)

	for listener in self.__listeners:
	    listener.current_ast_changed(ast)

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
	    self.window.graph_display.set_class(self.decl.name())
	#else:
	#    self.window.graph_display.hide()

    def showDecl(self, decl):
	self.decl = decl
	# todo: split these up.. mvc, events etc
	if self.window.graph_display.isVisible():
	    if isinstance(decl, AST.Class):
		#self.setGraphEnabled(isinstance(decl, AST.Class))
		self.window.graph_display.set_class(self.decl.name())
	    else:
		self.window.graph_display.set_class(None)
   
    def tabChanged(self, widget):
	self.set_current_decl(self.current_decl())

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

class PackageBrowser (Browser.SelectionListener):
    """Browser that manages the package view"""
    def __init__(self, window, browser):
	self.__window = window
	self.__browser = browser
	browser.add_listener(self)

	# Create the filler. It only displays a few types
	self.filler = ListFiller(self, window.package_list, (
	    'Package', 'Module', 'Namespace', 'Global'))
	self.filler.auto_open = 3

	window.connect(window.package_list, SIGNAL('selectionChanged(QListViewItem*)'), self.select_package_item)

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
	self.window.doco_display.setText(os.getvalue())

    def current_ast_changed(self, ast):
	self.filler.fillFrom(self.__browser.glob)

class ClassBrowser (Browser.SelectionListener):
    """Browser that manages the class view"""
    def __init__(self, window, browser):
	self.__window = window
	self.__browser = browser
	browser.add_listener(self)

	# Create the filler
	self.filler = ListFiller(self, window.class_list, None, (
	    'Package', 'Module', 'Namespace', 'Global'))

	window.connect(window.class_list, SIGNAL('selectionChanged(QListViewItem*)'), self.select_decl_item)
	window.connect(window.class_list, SIGNAL('expanded(QListViewItem*)'), self.selfish_expansion)

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
	    if iter != item: self.__window.class_list.setOpen(iter, 0)
	    iter = iter.nextSibling()

    def current_ast_changed(self, ast):
	self.filler.clear()
	self.filler.fillFrom(self.__browser.glob)

class DocoBrowser (Browser.SelectionListener):
    """Browser that manages the documentation view"""
    class BufferScopePages (Page.BufferPage, ScopePages.ScopePages):
	def __init__(self, manager):
	    ScopePages.ScopePages.__init__(self, manager)
	
	process_scope = ScopePages.ScopePages.process_scope
	process = ScopePages.ScopePages.process
	register_filenames = ScopePages.ScopePages.register_filenames


    def __init__(self, window, browser):
	self.__window = window
	self.__browser = browser
	browser.add_listener(self)

	self.mime_factory = SourceMimeFactory()
	self.mime_factory.set_browser(self)
	self.__window.doco_display.setMimeSourceFactory(self.mime_factory)

	self.__generator = None

	self.__getting_mime = 0
	
    def generator(self):
	if not self.__generator:
	    self.__generator = self.BufferScopePages(core.manager)
	return self.__generator

    def current_decl_changed(self, decl):
	if not self.__window.doco_display.isVisible():
	    # Not visible, so ignore
	    return
	if isinstance(decl, AST.Scope):
	    # These we can use the HTML scope formatter on
	    pages = self.generator()
	    pages.process_scope(decl)
	    self.__text = pages.get_buffer()
	    if self.__getting_mime: return
	    context = pages.filename()
	    self.__window.doco_display.setText(self.__text, context)
	elif decl:
	    # Need more work to use HTML on this.. use ASCIIFormatter for now
	    os = cStringIO.StringIO()
	    os.write('<pre>')
	    formatter = ASCIIFormatter(os)
	    formatter.set_scope(decl.name())
	    decl.accept(formatter)
	    self.__window.doco_display.setText(os.getvalue())
    
    def current_ast_changed(self, ast):
	core.configure_for_gui(ast)

	scope = AST.Scope('',-1,'','','')
	scope.declarations()[:] = ast.declarations()
	self.generator().register_filenames(scope)

    def get_mime_data(self, name):
	try:
	    page, scope = core.manager.filename_info(name)
	    print page,scope
	    if page is self.generator():
		self.__getting_mime = 1
		self.__browser.set_current_decl(scope)
		self.__getting_mime = 0
		return QTextDrag(self.__text, self.__window.doco_display, '')
	except KeyError:
	    pass
	return None
	    

class SourceMimeFactory (QMimeSourceFactory):
    def set_browser(self, browser): self.__browser = browser
    def data(self, name):
	d = self.__browser.get_mime_data(str(name))
	return d    

class SourceBrowser (Browser.SelectionListener):
    """Browser that manages the source view"""
    def __init__(self, window, browser):
	self.__window = window
	self.__browser = browser
	browser.add_listener(self)

	self.mime_factory = SourceMimeFactory()
	self.__window.source_display.setMimeSourceFactory(self.mime_factory)

	self.__current_file = None

    def current_decl_changed(self, decl):
	if not self.__window.source_display.isVisible():
	    # Not visible, so ignore
	    return
	file, line = decl.file(), decl.line()
	if self.__current_file != file:
	    # Check for empty file
	    if not file:
		self.__window.source_display.setText('')
		return
	    # Open the new file and give it line numbers
	    text = open(file).read()
	    # number the lines
	    lines = string.split(text, '\n')
	    for line in range(len(lines)): lines[line] = str(line+1)+'| '+lines[line]
	    text = string.join(lines, '\n')
	    # set text
	    self.__window.source_display.setText(text)
	    self.__current_file = file
	# scroll to line
	y = (self.__window.source_display.fontMetrics().height()+1) * line
	self.__window.source_display.setContentsPos(0, y)
	#print line, y

class GraphBrowser (Browser.SelectionListener):
    """Browser that manages the graph view"""
    def __init__(self, window, browser):
	self.__window = window
	self.__browser = browser
	browser.add_listener(self)
    def current_decl_changed(self, decl):
	if not self.__window.graph_display.isVisible():
	    # Not visible, so ignore
	    return
	self.__window.graph_display.set_class(decl.name())

