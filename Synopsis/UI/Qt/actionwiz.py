# $Id: actionwiz.py,v 1.3 2002/04/26 01:21:14 chalky Exp $
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
# $Log: actionwiz.py,v $
# Revision 1.3  2002/04/26 01:21:14  chalky
# Bugs and cleanups
#
# Revision 1.2  2001/11/09 08:06:59  chalky
# More GUI fixes and stuff. Double click on Source Actions for a dialog.
#
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#


import sys, pickle, Synopsis, cStringIO
from qt import *
from Synopsis import Config
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree

class SourcePage (QVBox):
    """The Page (portion of dialog) that displays config options for Source
    Actions"""
    def __init__(self, parent, action_container):
	QVBox.__init__(self, parent)
	self.__ac = action_container
	self._make_layout()
    def action(self):
	return self.__ac.action
    def set_action(self, action):
	self.__ac.action = action
    def _make_layout(self):
	self.setSpacing(4)
	label = QLabel("<p>Source actions are responsible for knowing what files to load "+
	       "from disk for parsing. They watch specified directories for new "+
	       "files that match the extensions they are looking for.", self)

	# Make action name field
	hbox = QHBox(self)
	label = QLabel("Action &name:", hbox)
	self.source_name = QLineEdit(hbox)
	QToolTip.add(self.source_name, "A unique name for this Action")
	label.setBuddy(self.source_name)
	self.connect(self.source_name, SIGNAL('textChanged(const QString&)'), self.onActionName)
	hbox.setSpacing(4)

	# Make source path area
	label = QLabel("&Path list:", self)
	self.source_paths = QListBox(self)
	label.setBuddy(self.source_paths)
	# Add buttons
	bhbox = QHBox(self)
	bhbox.setSpacing(4)
	self.source_add_path = QPushButton('&Add', bhbox)
	self.source_remove_path = QPushButton('&Remove', bhbox)
	self.source_test_path = QPushButton('&Test', bhbox)
	QToolTip.add(self.source_add_path, "Add a new path setting")
	QToolTip.add(self.source_remove_path, "Remove the selected path setting")
	QToolTip.add(self.source_test_path, "Displays which files your settings will select")
	self.connect(self.source_add_path, SIGNAL('clicked()'), self.onSourceAddPath)
	self.connect(self.source_test_path, SIGNAL('clicked()'), self.onSourceTest)

    def pre_show(self):
	self.source_name.setText(self.action().name())
	self.__update_list()

    def showEvent(self, ev):
	QVBox.showEvent(self, ev)
	# Focus name field
	self.source_name.setFocus()
	self.source_name.selectAll()

    def onActionName(self, name):
	self.action().set_name(str(name))

    def onSourceAddPath(self):
	strlist = QStringList()
	types = ['Simple file name', 'Directory and wildcard', 'Base directory and wildcard']
	for typestr in types: strlist.append(typestr)
	typestr, ok = QInputDialog.getItem("Select type of Path",
	    """<p>Select the type of path to add. <p>A path can be a simple
	    file name (selecting multiple files will create multiple path
	    entries), a directory and glob-expression (eg: *.py) combination, or a base
	    directory and glob-expression combination. The base directory allows
	    searching in subdirectories for matching files, the simple
	    directory does not.""",
	    strlist, 0, 0, self)
	if not ok: return
	typenum = types.index(str(typestr))
	if typenum == 0:
	    # Simple file name
	    paths = QFileDialog.getOpenFileNames(None, None, self)
	    print paths,map(str, paths)
	    for path in map(str, paths):
		self.action().paths().append(
		    SourcePath('Simple', path))
	elif typenum == 1:
	    # Dir + glob
	    dir = QFileDialog.getExistingDirectory(None, self, 'getdir', 
		'Select the directory the files are in', 1)
	    dir = str(dir)
	    if len(dir) == 0: return
	    glob, ok = QInputDialog.getText("Enter glob expression",
		"""<p>Enter the glob expression that will be used to match
		files against. You may use spaces to enter multiple
		expressions. Example: "*.py q*.cpp ??base.h" """)
	    if not ok: return
	    globs = string.split(str(glob))
	    print globs
	    for glob in globs:
		self.action().paths().append(
		    SourcePath('Dir', dir, glob))
	elif typenum == 2:
	    # Base dir + glob
	    dir = QFileDialog.getExistingDirectory(None, self, 'getdir', 
		'Select the base directory to start searching from', 1)
	    dir = str(dir)
	    glob, ok = QInputDialog.getText("Enter glob expression",
		"""<p>Enter the glob expression that will be used to match
		files against. You may use spaces to enter multiple
		expressions. Example: "*.py q*.cpp ??base.h" """)
	    if not ok: return
	    globs = string.split(str(glob))
	    print globs
	    for glob in globs:
		self.action().paths().append(
		    SourcePath('Base', dir, glob))
	# Update the list
	self.__update_list()

    def __update_list(self):
	strings = []
	for path in self.action().paths():
	    if path.type == 'Simple':
		strings.append('File: '+path.dir)
	    elif path.type == 'Dir':
		strings.append('Dir: %s Glob: %s'%(path.dir, path.glob))
	    else:
		strings.append('Base: %s Glob: %s'%(path.dir, path.glob))
	self.source_paths.clear()
	self.source_paths.insertStrList(strings)    

    def onSourceTest(self):
	from Synopsis.Core.Executor import ExecutorCreator
	project = self.__ac.project
	source_exec = ExecutorCreator(project).create(self.action())
	names = source_exec.get_output_names()
	print type(names), names
	names = map(lambda x: x[0], names)
	dialog = QDialog(self, 'test results', 1)
	dialog.setMinimumSize(600,400)
	vbox = QVBoxLayout(dialog)
	listbox = QListBox(dialog)
	listbox.insertStrList(names)
	button = QPushButton('&Close', dialog)
	self.connect(button, SIGNAL('clicked()'), dialog.accept)
	vbox.addWidget(listbox)
	vbox.addWidget(button)
	dialog.exec_loop()
	
class ParserPage (QVBox):
    """The Page (portion of dialog) that displays options for a Parser
    Action"""
    def __init__(self, parent, action_container):
	QVBox.__init__(self, parent)
	self.__ac = action_container
	self._make_layout()
    def action(self):
	return self.__ac.action
    def set_action(self, action):
	self.__ac.action = action
    def _make_layout(self):
	self.setSpacing(4)
	label = QLabel("<p>Parser actions take a list of source files from "+
	       "connected Source actions, and pass them through a parser "+
	       "by one.", self)

	# Make action name field
	hbox = QHBox(self)
	label = QLabel("Action &name:", hbox)
	self.source_name = QLineEdit(hbox)
	QToolTip.add(self.source_name, "A unique name for this Action")
	label.setBuddy(self.source_name)
	self.connect(self.source_name, SIGNAL('textChanged(const QString&)'), self.onActionName)
	hbox.setSpacing(4)

	# Make module type area
	label = QLabel("Select a &Parser module to use:", self)
	self.module_list = QListBox(self)
	label.setBuddy(self.module_list)
	self.module_items = {}
	for name in ['C++', 'Python', 'IDL']:
	    # Listbox item is automatically added to the list
	    item = QListBoxText(self.module_list, name)
	    self.module_items[name] = item
	self.connect(self.module_list, SIGNAL('highlighted(const QString&)'), self.onModuleSelected)

    def pre_show(self):
	self.source_name.setText(self.action().name())
	config = self.action().config()
	if config and self.module_items.has_key(config.name):
	    self.module_list.setCurrentItem(self.module_items[config.name])

    def showEvent(self, ev):
	QVBox.showEvent(self, ev)
	# Focus name field
	self.source_name.setFocus()
	self.source_name.selectAll()

    def onActionName(self, name):
	self.action().set_name(str(name))

    def onModuleSelected(self, module):
	module = str(module) #un-QString-ify it
	print module,"selected"
	action = self.action()
	config = action.config()
	if config is None:
	    # New action, copy example config for this module
	    action.set_config(Config.Base.Parser.modules[module])
	else:
	    # Changing module for existing action - copy attributes from
	    # example config if they're not already set
	    config.name = module
	    copy = Config.Base.Parser.modules[module]
	    for attr, value in copy.__dict__.items():
		if not hasattr(config, attr):
		    setattr(config, attr, value)


class ActionDialog (QDialog):
    def __init__(self, page_class, parent, action, project):
	QDialog.__init__(self, parent, 'Action Properties', 1, 
	    Qt.WStyle_Customize | Qt.WStyle_NormalBorder | Qt.WStyle_Title)
	self.setCaption('Action Properties')
	self.project = project
	self.action = action
	self.setMinimumSize(500,400)
	self.vbox = QVBoxLayout(self)
	self.vbox.setMargin(4)
	self.vbox.setSpacing(4)
	self.inner_page = page_class(self, self)
	self.inner_page.pre_show()
	frame = QFrame(self)
	frame.setFrameStyle(QFrame.HLine | QFrame.Sunken)
	self.vbox.addWidget(self.inner_page)
	self.vbox.addWidget(frame)
	#self.vbox.addWidget(self.hbox)
	self.hbox = QHBoxLayout(self.vbox)
	self.ok = QPushButton("Okay", self)
	self.cancel = QPushButton("Cancel", self)
	self.hbox.addStretch()
	self.hbox.addWidget(self.ok)
	self.hbox.addWidget(self.cancel)
	self.connect(self.ok, SIGNAL('clicked()'), self.accept)
	self.connect(self.cancel, SIGNAL('clicked()'), self.reject)
	self.exec_loop()
	# TODO: handle okay/cancel
	self.project.actions().action_changed(self.action)



class AddWizard (QWizard):
    def __init__(self, parent, action, project):
	QWizard.__init__(self, parent, 'AddWizard', 1)
	self.setMinimumSize(600,400)
	self.action = action
	self.project = project

	self.title_font = QFont('Helvetica', 20, QFont.Bold)
	self.title_palette = QPalette(self.palette())
	self.title_palette.setColor(QPalette.Normal, QColorGroup.Foreground, Qt.white)
	self.title_palette.setColor(QPalette.Normal, QColorGroup.Background, Qt.darkBlue)
	self.title_label = QLabel('', self)
	self.title_label.setFrameStyle(QFrame.Panel | QFrame.Sunken)
	self.title_label.setLineWidth(1)
	self.title_label.setMidLineWidth(1)
	self.title_label.setPalette(self.title_palette)
	self.title_label.setFont(self.title_font)

	self._makeStartPage()
	self._makeSourcePage()
	self._makeParserPage()
	self._makeFinishPage()

    def _makeStartPage(self):
	vbox = QVBox(self)
	vbox.layout().setSpacing(4)
	QLabel('What type of action would you like to create?', vbox)
	group = QButtonGroup(1, Qt.Horizontal, "&Action type", vbox)
	radio_source = QRadioButton('&Source - Specifies, monitors and loads source code files', group)
	radio_parser = QRadioButton('&Parser - Reads source files and parses them into ASTs', group)
	radio_linker = QRadioButton('&Linker - Operates on one or more ASTs, typically linking various things together', group)
	radio_cacher = QRadioButton('File &Cache - Stores an AST to disk to speed reprocessing', group)
	radio_format = QRadioButton('&Formatter - Exports an AST to some format, eg: HTML, PDF', group)
	self.__action_type_group = group
	self.__action_types = {
	    radio_source: 'source', radio_parser: 'parser', radio_linker: 'linker',
	    radio_format: 'format', radio_cacher: 'cacher' }
	self.connect(group, SIGNAL('clicked(int)'), self.onActionType)

	self.addPage(vbox, "Select action type")
	self.setNextEnabled(vbox, 0)
	self.setHelpEnabled(vbox, 0)
	self.start = vbox

    def onActionType(self, id):
	t = self.__action_types[self.__action_type_group.find(id)]
	old = self.action
	x, y, name = old.x(), old.y(), old.name()

	if name[0:4] == 'New ' and name[-7:] == ' action':
	    name = 'New %s action' % t

	if t == 'source':
	    self.action = SourceAction(x, y, name)
	    self.setAppropriate(self.source, 1)
	elif t == 'parser':
	    self.action = ParserAction(x, y, name)
	    self.setAppropriate(self.parser, 1)
	elif t == 'linker':
	    self.action = LinkerAction(x, y, name)
	elif t == 'cacher':
	    self.action = CacherAction(x, y, name)
	elif t == 'format':
	    self.action = FormatAction(x, y, name)

	if self.action is not old:
	    # allow user to move to next screen
	    self.nextButton().setEnabled(1)

    def _makeSourcePage(self):
	page = SourcePage(self, self)
	self.addPage(page, "Source action config")
	self.setHelpEnabled(page, 0)
	self.setAppropriate(page, 0)
	self.source = page

    def _makeParserPage(self):
	page = ParserPage(self, self)
	self.addPage(page, "Parser action config")
	self.setHelpEnabled(page, 0)
	self.setAppropriate(page, 0)
	self.parser = page

    def onActionName(self, name):
	self.action.set_name(name)

    def onSourceAddPath(self, name):
	pass

    def _makeFinishPage(self):
	vbox = QVBox(self)
	QLabel('You have finished creating your new action.', vbox)
	self.addPage(vbox, "Finished")
	self.setFinish(vbox, 1)
	self.setFinishEnabled(vbox, 1)
	self.setHelpEnabled(vbox, 0)
	self.finish = vbox

    def showPage(self, page):
	if page is self.source:
	    self.source.pre_show()
	QWizard.showPage(self, page)

	if page is self.finish:
	    self.finishButton().setFocus()

    def layOutTitleRow(self, hbox, title):
	self.title_label.setText(title)
	hbox.add(self.title_label)
	

