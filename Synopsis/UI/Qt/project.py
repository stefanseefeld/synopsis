# $Id: project.py,v 1.6 2002/10/11 06:03:23 chalky Exp $
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
# Revision 1.6  2002/10/11 06:03:23  chalky
# Use config from project
#
# Revision 1.5  2002/09/28 05:53:31  chalky
# Refactored display into separate project and browser windows. Execute projects
# in the background
#
# Revision 1.4  2002/06/22 07:03:27  chalky
# Updates to GUI - better editing of Actions, and can now execute some.
#
# Revision 1.3  2002/01/09 10:16:35  chalky
# Centralized navigation, clicking links in (html) docs works.
#
# Revision 1.2  2001/11/09 08:06:59  chalky
# More GUI fixes and stuff. Double click on Source Actions for a dialog.
#
# Revision 1.1  2001/11/07 05:58:21  chalky
# Reorganised UI, opening a .syn file now builds a simple project to view it
#

"""The project window. Displays the output of the project."""

import os, sys, tempfile, traceback
from qt import *
from Synopsis.Core import Action, Executor, AST
from Synopsis.Core.Project import Project

from actionvis import CanvasWindow
from browse import BrowserWindow

class ProjectWindow (QSplitter):
    def __init__(self, main_window, filename):
	QSplitter.__init__(self, main_window.workspace)
	self.setOrientation(Qt.Vertical)
	self.setCaption('Project Window')

	self.main_window = main_window
	self.project = Project()
	self.browser = None
	self.__activated = 0

	self.actions_display = CanvasWindow(self, self.main_window, self.project)
	self.exec_output = QMultiLineEdit(self)
	self.exec_output.setReadOnly(1)
	self.exec_output.hide()

	if filename:
	    self.project.load(filename)
	    self.setCaption(self.project.name()+' : Project Window')

	self.connect(self.parent(), SIGNAL('windowActivated(QWidget*)'), self.windowActivated)
	
	#CanvasWindow(parent, main_window, self.project)
	
	self.setSizes([30,70])
	#self.showMaximized()
	#self.show()
	main_window.add_window(self)

    def windowActivated(self, widget):
	if self.__activated:
	    if widget is not self: self.deactivate()
	elif widget is self: self.activate()
    
    def activate(self):
	self.__activated = 1
	self._menu_id = self.main_window.menuBar().insertItem('&Project', self.main_window.project_menu)

    def deactivate(self):
	self.__activated = 0
	self.main_window.menuBar().removeItem(self._menu_id)


    def execute_project(self):
	"""Tries to show some output in the browser parts of the project
	window. To do this is searches for an appropriate FormatAction (ie,
	one that has HTML options), checks that everything is up to date, and
	loads the AST into the browser windows."""
	# Find output
	formatter = None
	if formatter is None:
	    # Try to get default from Project
	    formatter = self.project.default_formatter()
	if formatter is None:
	    # Try to find a HTML formatter
	    for action in self.project.actions().actions():
		if isinstance(action, Action.FormatAction):
		    # Check html..
		    formatter = action
	if not formatter:
	    # Maybe tell user..
	    return
	self.format_action = formatter

	# Find input to formatter
	inputs = formatter.inputs()
	if len(inputs) != 1:
	    # Maybe tell user..
	    return
	self.cache_action = cache = inputs[0]
	if not isinstance(cache, Action.CacherAction) or len(cache.inputs()) != 1:
	    print "Error: Selected formatter must have a cache with a single input"
	    return

	# Now that we have verified the action to use, fork a process to do
	# the work in the background
	self.tempfile = file = tempfile.TemporaryFile('w+b', 64, 'synopsis-output')
	self.temp_pos = 0

	#TODO: use popen for windows compat.
	self.child_pid = os.fork()
	if self.child_pid == 0:
	    try:
		# Redirect output to file
		os.dup2(self.tempfile.fileno(), 1)
		os.dup2(self.tempfile.fileno(), 2)
		print "Executing project "+self.project.name()
		# Create executor for the input
		input_exec = Executor.ExecutorCreator(self.project, 1).create(cache)

		# Get AST from the input (forces project creation)
		names = input_exec.get_output_names()
		ast = input_exec.get_output(names[0][0])
		# Ignore the ast, since parent process will read it
		print "Done."
	    except:
		print "An exception occurred"
		traceback.print_exc()
	    self.tempfile.flush()
	    sys.exit(0)
	
	# Create a timer
	self.timer = QTimer()
	self.timer.changeInterval(100)
	self.connect(self.timer, SIGNAL('timeout()'), self.checkThreadOutput)

	self.exec_output.show()
	self.setSizes([80,20])

    def checkThreadOutput(self):
	"""This method is called periodically to check output on the
	tempfile"""

	# Read doesn't work, so we have to pull these seek tricks
	self.tempfile.seek(0, 2)
	end = self.tempfile.tell()
	if end != self.temp_pos:
	    self.tempfile.seek(self.temp_pos)
	    input = self.tempfile.read(end - self.temp_pos)
	    self.temp_pos = end
	    self.exec_output.append(input)
	    self.exec_output.setCursorPosition(self.exec_output.numLines(), 0)
	
	# Check if child finished
	pid, status = os.waitpid(self.child_pid, os.WNOHANG)
	if pid != 0:
	    # Thread ended
	    # Give AST to browser
	    if not self.browser:
		config = self.format_action.config()
		self.browser = BrowserWindow(self.main_window, None, self, config)
		self.browser.setCaption(self.project.name()+' : Browser Window')

	    # Now load ast from cache
	    try:
		input_exec = Executor.ExecutorCreator(self.project, 0).create(self.cache_action)
		file = input_exec.get_cache_filename(input_exec.get_output_names()[0][0])
		ast = AST.load(file)
		if ast:
		    self.browser.set_current_ast(ast)
	    except:
		print "An exception occurred"
		traceback.print_exc()

	    # Destroy timer
	    self.timer.stop()
	    del self.timer

	    # Restore file descriptors
	    self.tempfile.close()
	    del self.tempfile
	


