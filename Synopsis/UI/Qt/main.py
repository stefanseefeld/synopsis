# $Id: main.py,v 1.1 2001/11/05 06:52:11 chalky Exp $
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
# $Log: main.py,v $
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#


import sys, pickle, Synopsis, cStringIO
from qt import *
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree

from actionvis import CanvasWindow
from igraph import IGraphWindow
from browse import BrowseWindow

class MainWindow (QMainWindow):
    """The main window of the applet. It controls the whole GUI so far, and
    has two ListFillers, one for each QListView."""
    def __init__(self):
	QMainWindow.__init__(self, None, "Synopsis")

	# Make the menu
	menu = self.menuBar()
	file = QPopupMenu(self)
	file.insertItem("&Open File...", self.open, Qt.CTRL+Qt.Key_O)
	file.insertItem("Open &Project...", self.openProject, Qt.CTRL+Qt.Key_P)
	file.insertItem("&Canvas", self.openCanvas, Qt.CTRL+Qt.Key_C)
	file.insertItem("&Quit", qApp, SLOT( "quit()" ), Qt.CTRL+Qt.Key_Q )
	menu.insertItem("&File", file)

	self.workspace = QWorkspace(self)
	self.setCentralWidget(self.workspace)

    def open(self):
	"""Displays the file open dialog, and loads a file if selected"""
	file = QFileDialog.getOpenFileName(".", "Synopsis files (*.*syn)", self, "file", "Open a Synopsis data file")
	if file: self.openFile(str(file))

    def openProject(self):
	"""Opens a project"""
	filename = str(QFileDialog.getOpenFileName(".", 
	    "Synopsis Project files (*.synopsis)", self, 
	    "projectopen", "Open Project..."))
	if filename:
	    CanvasWindow(self.workspace, self, filename).show()


    def openFile(self, filename):
	"""Loads a given file"""
	try:
	    unpickler = pickle.Unpickler(open(filename, "r"))
	    # Load the file
	    version = unpickler.load()
	    ast = unpickler.load()
	    # Fill the GUI
	    glob = AST.Scope('', -1, '', 'Global', ('global',))
	    glob.declarations().extend(ast.declarations())
	    win = BrowseWindow(self.workspace, self, filename, glob)
	    win.show()
	except IOError, e:
	    # Oops..
	    QMessageBox.critical(self, "Synopsis", 
		"An error occurred opening:\n%s\n\n%s"%(filename, str(e)))
    
    def openCanvas(self):
	CanvasWindow(self.workspace, self).show()

