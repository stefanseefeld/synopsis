# $Id: browse.py,v 1.4 2001/11/09 15:35:04 chalky Exp $
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
    def __init__(self, window):
	#QSplitter.__init__(self, parent)
	self.ast = None
	self.window = window
	self.main_window = window.main_window

	self.glob = AST.Scope('', -1, '', 'Global', ('global',))
	#self.glob.declarations().extend(ast.declarations())
	#self.setCaption("'%s' - Browse"%filename)

	# Split up the GUI
	#splitV = QSplitter(Qt.Vertical, self)

	# Create a top-left listview for the packages
	#window.package_list.setMinimumSize(150,100)
	#self.packages.addColumn("Name")

	# Create a bottom-left listview for the other stuff
	#self.listview = QListView(splitV)
	#self.listview.setMinimumSize(150,300)
	#self.listview.addColumn("Name")
	#self.listview.addColumn("Type")

	# Create a textbrowser to show info about selected stuff
	# This could be just QTextEdit or something I guess... :)
	#splitter = QSplitter(Qt.Vertical, self)
	#self.textview = QTextBrowser(splitter)
	#self.textview.setMinimumSize(500,150)
	window.doco_display.setTextFormat(Qt.RichText)


	# Create the fillers. The first only displays a few types
	self.packfiller = ListFiller(self, window.package_list, (
	    'Package', 'Module', 'Namespace', 'Global'))
	self.packfiller.auto_open = 3
	self.listfiller = ListFiller(self, window.class_list)

	# Connect things up
	window.connect(window.package_list, SIGNAL('selectionChanged(QListViewItem*)'), self.showPackage)
	window.connect(window.class_list, SIGNAL('selectionChanged(QListViewItem*)'), self.showDecl)
	window.connect(window.class_list, SIGNAL('expanded(QListViewItem*)'), self.selfishExpansion)

	window.doco_display.setText("<i>Select a package/namespace to view from the left.")
	#self.packfiller.fillFrom(self.glob)

	# Make the menu, to be inserted in the app menu upon window activation
	self._file_menu = QPopupMenu(window.main_window.menuBar())
	self._graph_id = self._file_menu.insertItem("&Graph class inheritance", self.openGraph, Qt.CTRL+Qt.Key_G)

	self.__activated = 0
	window.connect(window.parent(), SIGNAL('windowActivated(QWidget*)'), self.windowActivated)

	self.classTree = window.classTree
	#self.glob.accept(self.classTree)

	self.decl = None

    def show_ast(self, ast):
	self.ast = ast
	self.glob.declarations()[:] = ast.declarations()
	self.packfiller.fillFrom(self.glob)
	self.glob.accept(self.classTree)
	self.listfiller.clear()
	self.listfiller.fillFrom(self.glob)
	core.configure_for_gui(ast)

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

    def showPackage(self, item):
	"""Show a given package (by item)"""
	decl = self.packfiller.map[item]
	self.setGraphEnabled(0)
	# Fill the main list
	self.listfiller.clear()
	self.listfiller.fillFrom(decl)
	# Grab the comments and put them in the text view
	os = cStringIO.StringIO()
	for comment in decl.comments():
	    os.write(comment.text())
	    os.write('<hr>')
	self.window.doco_display.setText(os.getvalue())

    def showDecl(self, item):
	"""Show a given declaration (by item)"""
	decl = self.listfiller.map[item]
	self.decl = decl
	# todo: split these up.. mvc, events etc
	if self.window.graph_display.isVisible():
	    if isinstance(decl, AST.Class):
		#self.setGraphEnabled(isinstance(decl, AST.Class))
		self.window.graph_display.set_class(self.decl.name())
	    else:
		self.window.graph_display.set_class(None)
	if self.window.doco_display.isVisible():
	    # Here we use ASCIIFormatter to quickly get us *something* to display
	    # :)
	    if isinstance(decl, AST.Scope):
		class BufferScopePages (Page.BufferPage, ScopePages.ScopePages):
		    def __init__(self, manager):
			ScopePages.ScopePages.__init__(self, manager)
		pages = BufferScopePages(core.manager)
		pages.process_scope(decl)
		text = pages.get_buffer()
		self.window.doco_display.setText(text)
	    else:
		os = cStringIO.StringIO()
		os.write('<pre>')
		formatter = ASCIIFormatter(os)
		formatter.set_scope(decl.name())
		decl.accept(formatter)
		self.window.doco_display.setText(os.getvalue())
	if self.window.source_display.isVisible():
	    file, line = decl.file(), decl.line()
	    print file
	    if not hasattr(self, 'curfile') or self.curfile != file:
		# read from file
		text = open(file).read()
		# number the lines
		lines = string.split(text, '\n')
		for line in range(len(lines)): lines[line] = str(line+1)+'| '+lines[line]
		text = string.join(lines, '\n')
		# set text
		self.window.source_display.setText(text)
		self.curfile = file
	    # scroll to line
	    y = (self.window.source_display.fontMetrics().height()+1) * line
	    self.window.source_display.setContentsPos(0, y)
	    print line, y
		

    def selfishExpansion(self, item):
	"""Selfishly makes item the only expanded node"""
	if not item.parent(): return
	iter = item.parent().firstChild()
	while iter:
	    if iter != item: self.window.class_list.setOpen(iter, 0)
	    iter = iter.nextSibling()

class ListFiller( AST.Visitor ):
    """A visitor that fills in a QListView from an AST"""
    def __init__(self, main, listview, types = None):
	self.map = {}
	self.main = main
	self.listview = listview
	self.stack = [self.listview]
	self.types = types
	self.auto_open = 1

    def clear(self):
	self.listview.clear()
	self.map = {}
	self.stack = [self.listview]

    def fillFrom(self, decl):
	self.visitGroup(decl)
	self.listview.setContentsPos(0,0)

    def visitDeclaration(self, decl):
	if self.types and decl.type() not in self.types: return
	item = QListViewItem(self.stack[-1], decl.name()[-1], decl.type())
	self.map[item] = decl
	self.__last = item

    def visitGroup(self, group):
	if self.types and group.type() not in self.types: return
	self.visitDeclaration(group)
	item = self.__last
	self.stack.append(item)
	for decl in group.declarations(): decl.accept(self)
	self.stack.pop()
	if len(self.stack) <= self.auto_open: self.listview.setOpen(item, 1)

    def visitForward(self, fwd): pass

    def visitEnum(self, enum):
	if self.types and enum.type() not in self.types: return
	self.visitDeclaration(enum)
	item = self.__last
	self.stack.append(item)
	for decl in enum.enumerators(): decl.accept(self)
	self.stack.pop()
	if len(self.stack) <= self.auto_open: self.listview.setOpen(item, 1)



