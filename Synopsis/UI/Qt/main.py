# $Id: main.py,v 1.6 2002/09/28 05:53:31 chalky Exp $
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
# Revision 1.6  2002/09/28 05:53:31  chalky
# Refactored display into separate project and browser windows. Execute projects
# in the background
#
# Revision 1.5  2002/06/22 07:03:27  chalky
# Updates to GUI - better editing of Actions, and can now execute some.
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


import sys, pickle, Synopsis, cStringIO, os
from qt import *
from Synopsis.Core import AST, Util, Action
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree

from igraph import IGraphWindow
from project import ProjectWindow
from browse import BrowserWindow

class MainWindow (QMainWindow):
    """The main window of the applet. It controls the whole GUI.
    """
    def __init__(self):
	QMainWindow.__init__(self, None, 'Synopsis')
	self.setCaption('Synopsis')

	# Make the menu
	menu = self.menuBar()
	file = QPopupMenu(self)
	file.insertItem("&Open File...", self.open_file, Qt.CTRL+Qt.Key_O)
	file.insertItem("&New Project...", self.new_project, Qt.CTRL+Qt.Key_N)
	file.insertItem("Open &Project...", self.open_project, Qt.CTRL+Qt.Key_P)
	file.insertItem("&Quit", qApp, SLOT( "quit()" ), Qt.CTRL+Qt.Key_Q)
	menu.insertItem("&File", file)
	self.window_menu = window = QPopupMenu(self)
	self.project_menu = QPopupMenu(self)
	self.project_menu.insertItem("&Execute", self.execute_project, Qt.CTRL+Qt.Key_E)

	self.workspace = QWorkspace(self)
	self.setCentralWidget(self.workspace)
	self.windows = []
	self.window_ids = []

    def open_project(self):
	"""Opens a project"""
	filename = str(QFileDialog.getOpenFileName(".", 
	    "Synopsis Project files (*.synopsis)", self, 
	    "projectopen", "Open Project..."))
	if filename:
	    self.do_open_project(filename)
    
    def do_open_project(self, filename):
	"""Opens a project for the given filename"""
	ProjectWindow(self, filename)

    def open_file(self):
	"""A file selection dialog is opened to prompt the user
	for a filename."""
	filename = str(QFileDialog.getOpenFileName(".", 
	    "Synopsis files (*.*syn)", self, 
	    "file", "Open a Synopsis data file"))
	if filename:
	    self.do_open_file(filename)

    def do_open_file(self, filename):
	"""Opens a file in a new project."""
	BrowserWindow(self, filename, None)

    def new_project(self):
	ProjectWindow(self, None)

    def execute_project(self):
	win = self.workspace.activeWindow()
	if isinstance(win, ProjectWindow):
	    win.execute_project()
    
    def add_window(self, window):
	if window in self.windows:
	    return
	# Remove old menu items
	for id in self.window_ids:
	    self.window_menu.removeItem(id)
	# Add new window to list
	self.windows.append(window)
	# Regenerate menu
	for i in range(len(self.windows)):
	    text = "&%d: %s"%(i+1, self.windows[i].caption())
	    id = self.window_menu.insertItem(text)
	    self.window_ids.append(id)
	# If it's the only window, show menu
	if len(self.windows) == 1:
	    self.menuBar().insertItem("&Window", self.window_menu)
	
	# Show window
	if len(self.windows) == 1:
	    window.showMaximized()
	window.show()
    
    def remove_window(self, window):
	pass

