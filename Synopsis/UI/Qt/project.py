# $Id: project.py,v 1.2 2001/11/09 08:06:59 chalky Exp $
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
# $Log: project.py,v $
# Revision 1.2  2001/11/09 08:06:59  chalky
# More GUI fixes and stuff. Double click on Source Actions for a dialog.
#
# Revision 1.1  2001/11/07 05:58:21  chalky
# Reorganised UI, opening a .syn file now builds a simple project to view it
#

"""The project window. Displays the output of the project."""

from qt import *
from Synopsis.Core import Action, Executor
from Synopsis.Core.Project import Project
from Synopsis.Formatter import ClassTree
from Synopsis.Formatter.HTML.core import FileTree

from actionvis import CanvasWindow
from igraph import IGraphWindow
from browse import Browser

class ProjectWindow (QSplitter):
    def __init__(self, parent, main_window, filename):
	QSplitter.__init__(self, parent)
	self.main_window = main_window
	self.project = Project()
	self.setCaption('Project Window')

	self.classTree = ClassTree.ClassTree()
	self.fileTree = None #FileTree()
	#self.tabs = QTabWidget(self)
	#self.listView = QListView(self)
	self._make_left()
	self._make_right()
	self.browser = Browser(self)

	if filename:
	    self.project.load(filename)
	
	#CanvasWindow(parent, main_window, self.project)
	
	self.setSizes([30,70])
	self.showMaximized()
	self.show()

    def _make_left(self):
	self.left_split = QSplitter(Qt.Vertical, self)
	self.package_list = QListView(self.left_split)
	self.package_list.addColumn('Package')
	self.left_tab = QTabWidget(self.left_split)
	self.class_list = QListView(self.left_tab)
	self.class_list.addColumn('Class')
	self.file_list = QListView(self.left_tab)
	self.file_list.addColumn('File')
	self.project_list = QListView(self.left_tab)
	self.project_list.addColumn('Action')
	self.left_tab.addTab(self.class_list, "Classes")
	self.left_tab.addTab(self.file_list, "Files")
	self.left_tab.addTab(self.project_list, "Project")
	self.left_split.setSizes([30,70])

    def _make_right(self):
	self.right_tab = QTabWidget(self)
	self.doco_display = QTextBrowser(self.right_tab)
	self.source_display = QTextBrowser(self.right_tab)
	self.actions_display = CanvasWindow(self, self.main_window, self.project)
	self.graph_display = IGraphWindow(self.right_tab, self.main_window, self.classTree)
	self.right_tab.addTab(self.doco_display, "Documentation")
	self.right_tab.addTab(self.source_display, "Source")
	self.right_tab.addTab(self.graph_display, "Graph")
	self.right_tab.addTab(self.actions_display, "Actions")

    def show_output(self):
	"""Tries to show some output in the browser parts of the project
	window. To do this is searches for an appropriate FormatAction (ie,
	one that has HTML options), checks that everything is up to date, and
	loads the AST into the browser windows."""
	# Find output
	formatter = None
	for action in self.project.actions().actions():
	    if isinstance(action, Action.FormatAction):
		# Check html..
		formatter = action
	if not formatter:
	    # Maybe tell user..
	    return

	# Find input to formatter
	inputs = formatter.inputs()
	if len(inputs) != 1:
	    # Maybe tell user..
	    return
	input = inputs[0]

	# Create executor for the input
	input_exec = Executor.ExecutorCreator(self.project).create(input)

	# Get AST from the input
	names = input_exec.get_output_names()
	ast = input_exec.get_output(names[0])

	# Give AST to browser
	self.browser.show_ast(ast)
	


