# $Id: main.py,v 1.2 2001/11/07 05:58:21 chalky Exp $
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
# Revision 1.2  2001/11/07 05:58:21  chalky
# Reorganised UI, opening a .syn file now builds a simple project to view it
#
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#


import sys, pickle, Synopsis, cStringIO, os
from qt import *
from Synopsis.Core import AST, Util, Action
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree

from igraph import IGraphWindow
from project import ProjectWindow

class MainWindow (QMainWindow):
    """The main window of the applet. It controls the whole GUI so far, and
    has two ListFillers, one for each QListView."""
    def __init__(self):
	QMainWindow.__init__(self, None, "Synopsis")

	# Make the menu
	menu = self.menuBar()
	file = QPopupMenu(self)
	file.insertItem("&Open File...", self.open, Qt.CTRL+Qt.Key_O)
	file.insertItem("&New Project...", self.newProject, Qt.CTRL+Qt.Key_N)
	file.insertItem("Open &Project...", self.openProject, Qt.CTRL+Qt.Key_P)
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
	    ProjectWindow(self.workspace, self, filename)


    def openFile(self, filename):
	projwin = ProjectWindow(self.workspace, self, None)
	project = projwin.project
	project.set_name('Project for %s'%os.path.split(filename)[1])
	cacher = Action.CacherAction(10, 10, 'Load File')
	cacher.file = filename
	project.actions().add_action(cacher)
	format = Action.FormatAction(100, 10, 'Format')
	project.actions().add_action(format)
	project.actions().add_channel(cacher, format)

	# Display file in browser
	projwin.show_output()


    def old_openFile(self, filename):
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
    
    def newProject(self):
	ProjectWindow(self.workspace, self, None)

