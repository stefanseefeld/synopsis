# $Id: browse.py,v 1.1 2001/11/05 06:52:11 chalky Exp $
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
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#


import sys, pickle, Synopsis, cStringIO
from qt import *
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree

from igraph import IGraphWindow

class BrowseWindow (QSplitter):
    """A window that browses a given .syn file"""
    def __init__(self, parent, main_window, filename, global_decl):
	QSplitter.__init__(self, parent)
	self.filename = filename
	self.global_decl = global_decl
	self.main_window = main_window

	self.setCaption("'%s' - Browse"%filename)

	# Split up the GUI
	splitV = QSplitter(Qt.Vertical, self)

	# Create a top-left listview for the packages
	self.packages = QListView(splitV)
	self.packages.setMinimumSize(150,100)
	self.packages.addColumn("Name")

	# Create a bottom-left listview for the other stuff
	self.listview = QListView(splitV)
	self.listview.setMinimumSize(150,300)
	self.listview.addColumn("Name")
	self.listview.addColumn("Type")

	# Create a textbrowser to show info about selected stuff
	# This could be just QTextEdit or something I guess... :)
	splitter = QSplitter(Qt.Vertical, self)
	self.textview = QTextBrowser(splitter)
	self.textview.setMinimumSize(500,150)
	self.textview.setTextFormat(Qt.RichText)


	# Create the fillers. The first only displays a few types
	self.packfiller = ListFiller(self, self.packages, (
	    'Package', 'Module', 'Namespace', 'Global'))
	self.packfiller.auto_open = 3
	self.listfiller = ListFiller(self, self.listview)

	# Connect things up
	self.connect(self.packages, SIGNAL('selectionChanged(QListViewItem*)'), self.showPackage)
	self.connect(self.listview, SIGNAL('selectionChanged(QListViewItem*)'), self.showDecl)
	self.connect(self.listview, SIGNAL('expanded(QListViewItem*)'), self.selfishExpansion)

	self.textview.setText("<i>Select a package/namespace to view from the left.")
	self.packfiller.fillFrom(global_decl)

	# Make the menu, to be inserted in the app menu upon window activation
	self._file_menu = QPopupMenu(self.main_window.menuBar())
	self._graph_id = self._file_menu.insertItem("&Graph class inheritance", self.openGraph, Qt.CTRL+Qt.Key_G)

	self.__activated = 0
	self.connect(parent, SIGNAL('windowActivated(QWidget*)'), self.windowActivated)

	self.classTree = ClassTree()
	self.global_decl.accept(self.classTree)

	self.decl = None
	self.graph_window = IGraphWindow(splitter, self.main_window, self.classTree)

	self.show()

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
	    self.graph_window.set_class(self.decl.name())
	else:
	    self.graph_window.hide()

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
	self.textview.setText(os.getvalue())

    def showDecl(self, item):
	"""Show a given declaration (by item)"""
	decl = self.listfiller.map[item]
	self.decl = decl
	self.setGraphEnabled(isinstance(decl, AST.Class))
	# Here we use ASCIIFormatter to quickly get us *something* to display
	# :)
	os = cStringIO.StringIO()
	os.write('<pre>')
	formatter = ASCIIFormatter(os)
	formatter.set_scope(decl.name())
	decl.accept(formatter)
	self.textview.setText(os.getvalue())

    def selfishExpansion(self, item):
	"""Selfishly makes item the only expanded node"""
	if not item.parent(): return
	iter = item.parent().firstChild()
	while iter:
	    if iter != item: self.listview.setOpen(iter, 0)
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



