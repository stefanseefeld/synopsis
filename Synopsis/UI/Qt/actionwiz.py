# $Id: actionwiz.py,v 1.7 2002/11/11 14:28:41 chalky Exp $
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
# Revision 1.7  2002/11/11 14:28:41  chalky
# New Source Edit dialog, with Insert Rule wizard and new s/path/rules with
# extra features.
#
# Revision 1.6  2002/07/11 09:30:16  chalky
# Added page for configuring the cacher action
#
# Revision 1.5  2002/07/04 06:44:41  chalky
# Can now edit define list for C++ Parser.
#
# Revision 1.4  2002/06/22 07:03:27  chalky
# Updates to GUI - better editing of Actions, and can now execute some.
#
# Revision 1.3  2002/04/26 01:21:14  chalky
# Bugs and cleanups
#
# Revision 1.2  2001/11/09 08:06:59  chalky
# More GUI fixes and stuff. Double click on Source Actions for a dialog.
#
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#


import sys, pickle, Synopsis, cStringIO, types, copy
from qt import *
from Synopsis import Config
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree

from sourceoptionsdialog import SourceOptionsDialog
from sourceinsertwizard import SourceInsertWizard
from sourceeditexclude import SourceEditExclude
from sourceeditsimple import SourceEditSimple
from sourceeditglob import SourceEditGlob

class Struct:
    "Dummy class. Initialise with keyword args."
    def __init__(self, **keys):
        for name, value in keys.items(): setattr(self, name, value)

def make_relative(base, file):
    """Useful function to make a filename relative to the given base
    filename"""
    if file[:len(base)] == base:
	# Simple case: base is a prefix of file
	return file[len(base):]
    elif file and file[0] == '/' and base[0] == '/':
	# More complex case: must insert ../../'s
	fs = string.split(file, os.sep)
	bs = string.split(base[:-1], os.sep)
	while len(fs) and len(bs) and fs[0] == bs[0]:
	    del fs[0]
	    del bs[0]
	fs = ['..']*len(bs) + fs
	return string.join(fs, os.sep)
    # If either is not absolute, can't assume anything
    return file

class SourceActionEditor:
    """Presents a modal GUI for editing a Source Action"""
    def __init__(self, parent, project):
	self.parent = parent
	self.project = project
	self.action = None
	self.dialog = None 
	self.insert_wizard = None
	self.rules = None
	self.rule_list_items = []
    def edit(self, action):
	"""Edits the given action"""
	self.action = action
	self.init_dialog()
	result = self.dialog.exec_loop()
	if result:
	    self.keep_changes()
	print "result is",result
    def keep_changes(self):
	dialog, action = self.dialog, self.action
	# If the name is changed, must use Project to rename
	name = str(dialog.NameEdit.text())
	if action.name() != name:
	    print "Setting name to ",name
	    self.project.actions().rename_action(action, name)
	    self.project.actions().action_changed(action)
	# Copy the list of rules back into the action
	self.action.rules()[:] = self.rules

    def init_dialog(self):
	dialog, action = self.dialog, self.action
	if not self.dialog:
	    dialog = self.dialog = SourceOptionsDialog(self.parent, "source", 1)
	    self.dialog.action = action
	    dialog.connect(dialog.RuleList, SIGNAL("selectionChanged()"), self.on_selection)
	    dialog.connect(dialog.InsertButton, SIGNAL("clicked()"), self.on_insert)
	    dialog.connect(dialog.DeleteButton, SIGNAL("clicked()"), self.on_delete)
	    dialog.connect(dialog.EditButton, SIGNAL("clicked()"), self.on_edit)
	    dialog.connect(dialog.DownButton, SIGNAL("clicked()"), self.on_down)
	    dialog.connect(dialog.UpButton, SIGNAL("clicked()"), self.on_up)
	    dialog.connect(dialog.TestButton, SIGNAL("clicked()"), self.on_test)
	# Deep copy list of source rules
	self.rules = map(lambda x: x.clone(), action.rules())
	# Set fields from the Action
	dialog.NameEdit.setText(action.name())
	self.update_list()

    def on_insert(self):
	# Run the wizard
	self.insert_wiz = wiz = SourceInsertWizard(self.dialog)
	# Add connections
	wiz.connect(wiz, SIGNAL("selected(const QString&)"), self.on_insert_finished)
	wiz.CurrentDirectoryLabel.setText('Current Directory: '+os.getcwd())
	result = wiz.exec_loop()
	print "Wizard result:",result
	# Deal with result
	if not result: return
	base = None
	if wiz.relative_option == 1:
	    base = os.getcwd()
	elif wiz.relative_option == 2:
	    base = str(wiz.RelativeThisEdit.text())
	if base and base[-1] != '/': base = base + '/'
	print "Base is:",base
	if wiz.rule_type == "simple":
	    files = []
	    item = wiz.FileList.firstChild()
	    while item:
		file = str(item.text(0))
		file = make_relative(base, file)
		files.append(file)
		item = item.nextSibling()
	    self.rules.append(SimpleSourceRule(files))
	elif wiz.rule_type == "exclude":
	    glob = str(wiz.ExcludeCombo.currentText())
	    self.rules.append(ExcludeSourceRule(glob))
	else:
	    dirs = []
	    item = wiz.DirList.firstChild()
	    while item:
		dir = str(item.text(0))
		dir = make_relative(base, dir)
		dirs.append(dir)
		item = item.nextSibling()
	    glob = str(wiz.FileGlobEdit.text())
	    if wiz.rule_type == 'glob':
		self.rules.append(GlobSourceRule(dirs, glob, 0))
	    else:
		self.rules.append(GlobSourceRule(dirs, glob, 1))

	# Update the list
	self.update_list()

    def on_insert_finished(self, title):
	if str(title) != "Finished":
	    return
	# Figure out text to put in the "rule info" box
	wiz = self.insert_wiz
	base = None
	if wiz.relative_option == 1:
	    base = os.getcwd()
	elif wiz.relative_option == 2:
	    base = str(wiz.RelativeThisEdit.text())
	if base and base[-1] != '/': base = base + '/'
	print "Base is:",base
	if wiz.rule_type == "simple":
	    files = []
	    item = wiz.FileList.firstChild()
	    while item:
		file = str(item.text(0))
		file = make_relative(base, file)
		files.append(file)
		item = item.nextSibling()
	    wiz.RuleInfo.setText("Type: Simple\n"+
		"Files:\n "+string.join(files, "\n "))
	elif wiz.rule_type == "exclude":
	    wiz.RuleInfo.setText("Type: Exclude\n"+
		"Glob expression:\n "+str(wiz.ExcludeCombo.currentText()))
	else:
	    # Glob or recursive glob
	    dirs = []
	    item = wiz.DirList.firstChild()
	    while item:
		dir = str(item.text(0))
		dir = make_relative(base, dir)
		dirs.append(dir)
		item = item.nextSibling()
	    wiz.RuleInfo.setText("Type: %s\n"%wiz.rule_type +
		"Directories:\n "+string.join(dirs, "\n ")+
		"\nFiles:\n "+str(wiz.FileGlobEdit.text()))

    def update_list(self):
	dialog, action = self.dialog, self.action
	dialog.RuleList.clear()
	items = []
	self.__rule_list_items = items
	# NB: we must reverse the list before adding (and then unreverse the
	# items list later). I don't know why QListView adds things at the top
	# rather than the bottom, and I sure hope it doesn't change later on!
	rules = list(self.rules)
	rules.reverse()
	for rule in rules:
	    if rule.type == 'Simple':
		if len(rule.files) <= 1:
		    dir, name = os.path.split(rule.files[0])
		    items.append(QListViewItem(dialog.RuleList, 
			'Simple', dir, name, ''))
		else:
		    # Make into a tree
		    items.append(QListViewItem(dialog.RuleList,
			'Simple', '', '(%d files)'%len(rule.files), ''))
		    for file in rule.files:
			dir, name = os.path.split(file)
			QListViewItem(items[-1], '', dir, name, '')
	    elif rule.type == 'Glob':
		recursive = ''
		if rule.recursive: recursive = 'recursive'
		if len(rule.dirs) <= 1:
		    items.append(QListViewItem(dialog.RuleList, 
			'Glob', string.join(rule.dirs, '\n'), 
			rule.glob, recursive))
		else:
		    items.append(QListViewItem(dialog.RuleList, 
			'Glob', '(%d directories)'%len(rule.dirs), 
			rule.glob, recursive))
		    for dir in rule.dirs:
			QListViewItem(items[-1], '', dir, '', '')
	    elif rule.type == 'Exclude':
		items.append(QListViewItem(dialog.RuleList, 
		    'Exclude', rule.glob, '', ''))
	items.reverse()

    def get_selected_index(self):
	"""Returns the index of the selected item, or None"""
	sel_item = self.dialog.RuleList.selectedItem()
	if not sel_item: return None
	index = self.__rule_list_items.index(sel_item)
	if index == -1:
	    print "Warning: QListViewItem not found in list"
	    return None
	return index

    def on_selection(self):
	dialog = self.dialog
	index = self.get_selected_index()
	if index != None:
	    dialog.DeleteButton.setEnabled(1)
	    dialog.EditButton.setEnabled(1)
	    if index > 0:
		dialog.UpButton.setEnabled(1)
	    else:
		dialog.UpButton.setEnabled(0)
	    if index < len(self.rules) - 1:
		dialog.DownButton.setEnabled(1)
	    else:
		dialog.DownButton.setEnabled(0)
	else:
	    dialog.DeleteButton.setEnabled(0)
	    dialog.EditButton.setEnabled(0)
	    dialog.UpButton.setEnabled(0)
	    dialog.DownButton.setEnabled(0)
    
    def on_delete(self):
	index = self.get_selected_index()
	if index is None: return
	button = QMessageBox.warning(self.dialog, "Confirm delete", 
	    "Really delete this source rule?",
	    QMessageBox.Yes, QMessageBox.No)
	if button == QMessageBox.Yes:
	    del self.rules[index]
	    self.update_list()
	    self.on_selection()
    
    def on_up(self):
	index = self.get_selected_index()
	if index is None or index == 0: return
	# Swap with rule above
	this_rule = self.rules[index]
	self.rules[index] = self.rules[index-1]
	self.rules[index-1] = this_rule
	self.update_list()
	self.dialog.RuleList.setSelected(self.__rule_list_items[index-1], 1)
	
    def on_down(self):
	index = self.get_selected_index()
	if index is None or index == len(self.rules)-1: return
	# Swap with rule below
	this_rule = self.rules[index]
	self.rules[index] = self.rules[index+1]
	self.rules[index+1] = this_rule
	self.update_list()
	self.dialog.RuleList.setSelected(self.__rule_list_items[index+1], 1)

    def on_edit(self):
	index = self.get_selected_index()
	if index is None: return
	rule = self.rules[index]
	if rule.type == 'Simple':
	    # Bring up the dialog for editing simple rules
	    dlg = SourceEditSimple(self.dialog)
	    dlg.make_relative = make_relative
	    # Copy files from rule into ListView
	    for file in rule.files:
		QListViewItem(dlg.FileList, file)
	    result = dlg.exec_loop()	
	    if not result: return
	    # Copy ListView back into rule
	    rule.files = []
	    item = dlg.FileList.firstChild()
	    while item:
		rule.files.append(str(item.text(0)))
		item = item.nextSibling()
	elif rule.type == 'Glob':
	    # Bring up the dialog for editing glob rules
	    dlg = SourceEditGlob(self.dialog)
	    dlg.make_relative = make_relative
	    # Copy dirs from rule into ListView
	    for dir in rule.dirs:
		QListViewItem(dlg.DirList, dir)
	    dlg.FileGlobEdit.setText(rule.glob)
	    dlg.RecurseCheckBox.setChecked(rule.recursive)
	    result = dlg.exec_loop()
	    if not result: return
	    # Copy ListView, edit and checkbox back into rule
	    rule.dirs = []
	    item = dlg.DirList.firstChild()
	    while item:
		rule.dirs.append(str(item.text(0)))
		item = item.nextSibling()
	    rule.glob = str(dlg.FileGlobEdit.text())
	    rule.recursive = dlg.RecurseCheckBox.isChecked()
	elif rule.type == 'Exclude':
	    # Bring up the dialog for editing exclude rules
	    dlg = SourceEditExclude(self.dialog)
	    # Copy glob into dlg
	    dlg.GlobEdit.setText(rule.glob)
	    result = dlg.exec_loop()
	    if not result: return
	    # Copy glob from dialog back into rule
	    rule.glob = str(dlg.GlobEdit.text())
	# Update the display with new info
	self.update_list()

    def on_test(self):
	from Synopsis.Core.Executor import ExecutorCreator
	project = self.project
	source_exec = ExecutorCreator(project).create(self.action)
	names = source_exec.get_output_names()
	names = map(lambda x: x[0], names)
	dialog = QDialog(self.dialog, 'test results', 1)
	dialog.setCaption('Files matched by source action %s'%self.action.name())
	dialog.setMinimumSize(600,400)
	vbox = QVBoxLayout(dialog)
	listbox = QListBox(dialog)
	listbox.insertStrList(names)
	button = QPushButton('&Close', dialog)
	dialog.connect(button, SIGNAL('clicked()'), dialog.accept)
	vbox.addWidget(listbox)
	vbox.addWidget(button)
	dialog.exec_loop()

class CacherPage (QVBox):
    """The Page (portion of dialog) that displays options for a Cacher
    Action"""
    def __init__(self, parent, action_container):
	QVBox.__init__(self, parent)
	self.__ac = action_container
	self._make_layout()
    def title(self):
	return "Cache"
    def action(self):
	return self.__ac.action
    def set_action(self, action):
	self.__ac.action = action
    def _make_layout(self):
	self.setSpacing(4)
	label = QLabel("<p>Cacher actions can load a stored AST from disk, "+
	"or cache generated AST's between other actions (eg: parsing and "+
	"linking, or linking and formatting).", self)

	# Make action name field
	hbox = QHBox(self)
	label = QLabel("Action &name:", hbox)
	self.action_name = QLineEdit(hbox)
	QToolTip.add(self.action_name, "A unique name for this Action")
	label.setBuddy(self.action_name)
	self.connect(self.action_name, SIGNAL('textChanged(const QString&)'), self.onActionName)
	hbox.setSpacing(4)

	# Make options
	bgroup = QVButtonGroup("Type of Cacher:", self)
	self.bfile = QRadioButton("Load a single AST file", bgroup)
	self.bdir = QRadioButton("Cache inputs to many files below a directory", bgroup)

	label = QLabel("Select the file to load:", self)
	hbox = QHBox(self)
	self.line_file = QLineEdit('', hbox)
	self.brfile = QPushButton('Browse', hbox)
	label = QLabel("Select the directory to cache files in:", self)
	hbox = QHBox(self)
	self.line_dir = QLineEdit('', hbox)
	self.brdir = QPushButton('Browse', hbox)

	self.connect(self.bfile, SIGNAL('clicked()'), self.onButtonFile)
	self.connect(self.bdir, SIGNAL('clicked()'), self.onButtonDir)
	self.connect(self.line_file, SIGNAL('textChanged(const QString&)'), self.onFileChanged)
	self.connect(self.line_dir, SIGNAL('textChanged(const QString&)'), self.onDirChanged)
	self.connect(self.brfile, SIGNAL('clicked()'), self.onBrowseFile)
	self.connect(self.brdir, SIGNAL('clicked()'), self.onBrowseDir)

    def pre_show(self):
	action = self.action()
	self.action_name.setText(action.name())
	self.line_file.setText(str(action.file or ''))
	self.line_dir.setText(str(action.dir or ''))

	if not action.file:
	    self.line_file.setEnabled(0)
	    self.brfile.setEnabled(0)
	elif not action.dir:
	    self.line_dir.setEnabled(0)
	    self.brdir.setEnabled(0)
	if action.file:
	    self.bfile.setOn(1)
	elif action.dir:
	    self.bdir.setOn(1)

    def showEvent(self, ev):
	QVBox.showEvent(self, ev)
	# Focus name field
	self.action_name.setFocus()
	self.action_name.selectAll()

    def onActionName(self, name):
	self.action().set_name(str(name))

    def onButtonFile(self):
	self.action().dir = ''
	self.line_file.setEnabled(1)
	self.brfile.setEnabled(1)
	self.line_dir.setEnabled(0)
	self.brdir.setEnabled(0)

    def onButtonDir(self):
	self.action().file = ''
	self.line_dir.setEnabled(1)
	self.brdir.setEnabled(1)
	self.line_file.setEnabled(0)
	self.brfile.setEnabled(0)

    def onFileChanged(self, text):
	text = str(text)
	print "file =",text
	self.action().file = text

    def onDirChanged(self, text):
	text = str(text)
	print "dir =",text
	self.action().dir = text

    def onBrowseFile(self):
	result = QFileDialog.getOpenFileName(str(self.action().file or ''), '*.syn',
	    self, 'open', 'Select an AST file to load from')
	result = str(result)
	if result:
	    self.line_file.setText(result)

    def onBrowseDir(self):
	result = QFileDialog.getExistingDirectory(str(self.action().dir or ''),
	    self, 'open', 'Select an AST file to load from')
	result = str(result)
	if result:
	    self.line_dir.setText(result)

class ParserPage (QVBox):
    """The Page (portion of dialog) that displays options for a Parser
    Action"""
    def __init__(self, parent, action_container):
	QVBox.__init__(self, parent)
	self.__ac = action_container
	self._make_layout()
    def title(self):
	return "Module"
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
	self.connect(self.module_list, SIGNAL('clicked(QListBoxItem*)'), self.onModuleSelected)

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

    def onModuleSelected(self, item):
	module = str(item.text()) #un-QString-ify it
	print module,"selected"
	action = self.action()
	config = action.config()
	if config is None:
	    # New action, create empty struct
	    config = Struct()
	    action.set_config(config)
	else:
	    # Confirm with user
	    button = QMessageBox.information(self, "Confirm module change",
		"Warning: Changing the module type may cause configuration\n"+\
		"settings to be lost. Are you sure?", QMessageBox.Yes,
		QMessageBox.No)
	    print button
	    if button == QMessageBox.No:
		print "User aborted"
		return
	# Changing module for existing action - copy attributes from
	# example config if they're not already set
	config.name = module
	copy = Config.Base.Parser.modules[module]
	for attr, value in copy.__dict__.items():
	    if not hasattr(config, attr):
		setattr(config, attr, value)

class CppParserPage (QVBox):
    """The Page (portion of dialog) that displays options for a C++ Parser Action"""
    def __init__(self, parent, action_container):
	QVBox.__init__(self, parent)
	self.__ac = action_container
	self._make_layout()
    def title(self):
	return "C++ Options"
    def action(self):
	return self.__ac.action
    def set_action(self, action):
	self.__ac.action = action
    def _make_layout(self):
	self.setSpacing(4)
	config = self.action().config()

	# Make options area
	if not hasattr(config, 'main_file'): config.main_file = 0
	self.main_only = QCheckBox("Include declarations from &main file only", self)
	self.main_only.setChecked(config.main_file)
	self.connect(self.main_only, SIGNAL('toggled(bool)'), self.onMainFile)
	# Make include path area
	label = QLabel("&Include paths:", self)
	hbox = QHBox(self)
	self.path_list = QListBox(hbox)
	label.setBuddy(self.path_list)
	self._update_path_list()
	self.connect(self.path_list, SIGNAL('clicked(QListBoxItem*)'), self.onPathSelected)

	# Make buttons
	bbox = QVBox(hbox)
	bbox.setSpacing(4)
	self.badd = QPushButton('&Add', bbox)
	self.bremove = QPushButton('&Remove', bbox)
	self.bgcc = QPushButton('Add &GCC paths', bbox)
	QToolTip.add(self.badd, "Add a new path setting")
	QToolTip.add(self.bremove, "Remove the selected path setting")
	self.connect(self.badd, SIGNAL('clicked()'), self.onAddPath)
	self.connect(self.bremove, SIGNAL('clicked()'), self.onRemovePath)
	self.connect(self.bgcc, SIGNAL('clicked()'), self.onAddGCC)

	# Make Define area
	label = QLabel("Preprocessor &defines:", self)
	hbox = QHBox(self)
	self.def_list = QListBox(hbox)
	label.setBuddy(self.def_list)
	self._update_def_list()
	self.connect(self.def_list, SIGNAL('clicked(QListBoxItem*)'), self.onDefSelected)

	# Make buttons
	bbox = QVBox(hbox)
	bbox.setSpacing(4)
	self.bdadd = QPushButton('&Add', bbox)
	self.bdremove = QPushButton('&Remove', bbox)
	QToolTip.add(self.bdadd, "Add a new define")
	QToolTip.add(self.bdremove, "Remove the selected define")
	self.connect(self.bdadd, SIGNAL('clicked()'), self.onAddDefine)
	self.connect(self.bdremove, SIGNAL('clicked()'), self.onRemoveDefine)


    def _update_path_list(self):
	config = self.action().config()
	self.path_list.clear()
	self.path_items = {}
	if not hasattr(config, 'include_path') or \
		type(config.include_path) != type([]):
	    config.include_path = []
	for path in config.include_path:
	    # Listbox item is automatically added to the list
	    print "path:",path
	    item = QListBoxText(self.path_list, path)
	    self.path_items[path] = item

    def _update_def_list(self):
	config = self.action().config()
	self.def_list.clear()
	self.def_items = {}
	if not hasattr(config, 'defines') or \
		type(config.defines) != type([]):
	    config.defines = []
	for define in config.defines:
	    # Listbox item is automatically added to the list
	    print "Define:", define
	    item = QListBoxText(self.def_list, define)
	    self.def_items[define] = item

    def pre_show(self):
	pass

    def showEvent(self, ev):
	QVBox.showEvent(self, ev)
	# Focus name field
	#self.source_name.setFocus()
	#self.source_name.selectAll()

    def onPathSelected(self, item):
	path = item.text()

    def onDefSelected(self, item):
	pass

    def onAddPath(self):
	dir = QFileDialog.getExistingDirectory()
	dir = str(dir)
	if dir:
	    self.action().config().include_path.append(dir)
	    self._update_path_list()

    def onRemovePath(self):
	pass

    def onAddGCC(self):
	config = self.action().config()
	config.include_path.extend([
	    '/usr/include/g++-3/',
	    '/usr/lib/gcc-lib/i386-linux/2.95.4/include'
	])
	print "config:",config.include_path
	self._update_list()

    def onMainFile(self, on):
	self.action().config().main_file = not not on

    def onAddDefine(self):
	define, okay = QInputDialog.getText("Add define",
	    "Enter the new define. Eg: 'DEBUG' or 'MODE=1' without quotes.")
	define = str(define)
	if okay and define:
	    self.action().config().defines.append(define)
	    self._update_def_list()
    
    def onRemoveDefine(self):
	curr = self.def_list.currentItem()
	if curr < 0: return
	del self.action().config().defines[curr]

class FormatterPage (QVBox):
    """The Page (portion of dialog) that displays options for a Formatter
    Action"""
    def __init__(self, parent, action_container):
	QVBox.__init__(self, parent)
	self.__ac = action_container
	self._make_layout()
    def title(self):
	return "Module"
    def action(self):
	return self.__ac.action
    def set_action(self, action):
	self.__ac.action = action
    def _make_layout(self):
	self.setSpacing(4)

	# Make action name field
	hbox = QHBox(self)
	label = QLabel("Formatter Action &name:", hbox)
	self.formatter_name = QLineEdit(hbox)
	QToolTip.add(self.formatter_name, "A unique name for this Formatter Action")
	label.setBuddy(self.formatter_name)
	self.connect(self.formatter_name, SIGNAL('textChanged(const QString&)'), self.onActionName)
	hbox.setSpacing(4)

    def pre_show(self):
	self.formatter_name.setText(self.action().name())
    
    def showEvent(self, ev):
	QVBox.showEvent(self, ev)
	# Focus name field
	self.formatter_name.setFocus()
	self.formatter_name.selectAll()

    def onActionName(self, name):
	self.action().set_name(str(name))


class DetailsPage (QVBox):
    """The Page (portion of dialog) that displays config details"""
    def __init__(self, parent, action_container):
	QVBox.__init__(self, parent)
	self.__ac = action_container
	self._make_layout()
    def title(self):
	return "Details"
    def action(self):
	return self.__ac.action
    def set_action(self, action):
	self.__ac.action = action
    def _make_layout(self):
	self.setSpacing(4)
	label = QLabel("<p>Configuration options are listed below. Most have"+
	    "comments which you can see by holding your mouse over them.", self)
	# Make properties area
	self.listview = QListView(self)
	self.listview.addColumn('Property', 100)
	self.listview.addColumn('Value')
	self.listview.setRootIsDecorated(1)

    def pre_show(self):
	# Fill properties table
	self.listview.clear()
	config = self.action().config()
	self._fill_list(config, self.listview)

    def _fill_list(self, obj, parent):
	attrs = obj.__dict__.keys()
	attrs.sort()
	for attr in attrs:
	    if attr in ('__name__', '__doc__', '__module__'):
		continue
	    value = getattr(obj, attr)
	    if type(value) in (types.InstanceType, types.ClassType):
		item = QListViewItem(parent, attr)
		self._fill_list(value, item)
	    else:
		item = QListViewItem(parent, attr, str(value))


class ActionDialog (QTabDialog):
    def __init__(self, parent, action, project):
	QTabDialog.__init__(self, parent, 'Action Properties', 1, 
	    Qt.WStyle_Customize | Qt.WStyle_NormalBorder | Qt.WStyle_Title)
	self.setCaption('Action Properties')
	self.project = project
	self.action = action
	self.setMinimumSize(600,400)
	self.page_classes = self._get_pages()
	if not self.page_classes: return
	self.pages = []
	for page_class in self.page_classes:
	    page = page_class(self, self)
	    self.pages.append(page)
	    self.addTab(page, page.title())
	    page.pre_show()
	self.setOkButton("Close")
	self.exec_loop()
	self.project.actions().action_changed(self.action)

    def _get_pages(self):
	if isinstance(self.action, SourceAction):
	    return [SourcePage]
	elif isinstance(self.action, ParserAction):
	    return [ParserPage, CppParserPage, DetailsPage]
	elif isinstance(self.action, FormatAction):
	    return [FormatterPage, DetailsPage]
	elif isinstance(self.action, CacherAction):
	    return [CacherPage]
	return None



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
	self._makeCacherPage()
	self._makeFormatterPage()
	self._makeFormatPropPage()
	# Finish page goes last
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
	self.old_action_type = None

    def onActionType(self, id):
	t = self.__action_types[self.__action_type_group.find(id)]
	old = self.action
	x, y, name = old.x(), old.y(), old.name()

	if name[0:4] == 'New ' and name[-7:] == ' action':
	    name = 'New %s action' % t
	
	# Make old page not appropriate
	if self.old_action_type and self.old_action_type != t:
	    o = self.old_action_type
	    if o == 'source': self.setAppropriate(self.source, 0)
	    elif o == 'parser': self.setAppropriate(self.parser, 0)
	    elif o == 'linker': pass
	    elif o == 'cacher': self.setAppropriate(self.cacher, 0)
	    elif o == 'format':
		self.setAppropriate(self.formatter, 0)
		self.setAppropriate(self.formatProp, 0)
	self.old_action_type = t

	# Create Action depending on selection and set next page
	if t == 'source':
	    self.action = SourceAction(x, y, name)
	    self.setAppropriate(self.source, 1)
	elif t == 'parser':
	    self.action = ParserAction(x, y, name)
	    self.setAppropriate(self.parser, 1)
	elif t == 'linker':
	    self.action = LinkerAction(x, y, name)
	    # Find the class in the config dictionary
	    config_class = Config.Base.Linker.modules['Linker']
	    config = Struct(name=config_class.name)
	    self.action.set_config( config )
	    # Copy attributes from example config
	    self._copyAttrs(config, config_class, 1)
	elif t == 'cacher':
	    self.action = CacherAction(x, y, name)
	    self.setAppropriate(self.cacher, 1)
	elif t == 'format':
	    self.action = FormatAction(x, y, name)
	    self.setAppropriate(self.formatter, 1)

	if self.action is not old:
	    # allow user to move to next screen if Action changed
	    self.setNextEnabled(self.start, 1)

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

    def _makeCacherPage(self):
	page = CacherPage(self, self)
	self.addPage(page, "Cacher action config")
	self.setHelpEnabled(page, 0)
	self.setAppropriate(page, 0)
	self.cacher = page

    def _makeFormatterPage(self):
	vbox = QVBox(self)
	vbox.layout().setSpacing(4)
	QLabel('Which Formatter module do you want to use?', vbox)
	group = QButtonGroup(1, Qt.Horizontal, "&Formatter module", vbox)
	radio_html = QRadioButton('&HTML - Generates a static website', group)
	radio_docbook = QRadioButton('&DocBook - Generates a DocBook book', group)
	radio_texinfo = QRadioButton('&TexInfo - Generates a TexInfo manual', group)
	radio_dia = QRadioButton('D&ia - Generates diagrams for Dia', group)
	radio_dot = QRadioButton('&GraphViz - Generates diagrams with GraphViz', group)
	radio_dump = QRadioButton('D&ump - Dumps the AST for debugging', group)
	radio_other = QRadioButton('&Other - Choose any module (case matters)', group)
	self.formatter_edit_other = QLineEdit('<module name>', vbox)
	self.formatter_edit_other.setEnabled(0)
	self.__format_module_group = group
	self.__format_modules = {
	    radio_html: 'HTML', radio_docbook: 'DocBook', 
	    radio_texinfo: 'TexInfo', radio_dia: 'Dia', radio_dot: 'Dot',
	    radio_dump: 'DUMP', radio_other: 'other' }
	self.connect(group, SIGNAL('clicked(int)'), self.onFormatModule)

	self.addPage(vbox, "Select Formatter Module")
	self.setNextEnabled(vbox, 0)
	self.setHelpEnabled(vbox, 0)
	self.setAppropriate(vbox, 0)
	self.formatter = vbox

    def onFormatModule(self, id):
	# Find out which option was selected
	t = self.__format_modules[self.__format_module_group.find(id)]

	# Decide which page to go to next..
	if t == 'other':
	    self.formatter_edit_other.setEnabled(1)
	    # Create an empty config with the specified name
	    name = self.formatter_edit_other.text()
	    self.action.set_config(Struct(name=name))
	else:
	    self.formatter_edit_other.setEnabled(0)
	    # Find the class in the config dictionary
	    config_class = Config.Base.Formatter.modules[t]
	    config = Struct(name=config_class.name)
	    self.action.set_config( config )
	    # Copy attributes from example config
	    self._copyAttrs(config, config_class, 1)


	# Enable the Next button
	self.setNextEnabled(self.formatter, 1)
	self.setAppropriate(self.formatProp, 1)

    def _copyAttrs(self, dest, src, overwrite):
	"""Copies attributes from src to dest objects. src may be a class, and
	recursive structs/classes are copied properly. Note that the copy
	module doesn't copy classes, so we do that bit ourselves"""
	for attr, value in src.__dict__.items():
	    if type(value) in (types.InstanceType, types.ClassType):
		if not hasattr(dest, attr): setattr(dest, attr, Struct())
		self._copyAttrs(getattr(dest, attr), value, overwrite)
	    elif type(value) in (types.FunctionType,):
		continue
	    elif attr in ('__doc__', '__module__', '__name__'):
		continue
	    elif overwrite or not hasattr(config, attr):
		setattr(dest, attr, copy.deepcopy(value))

    def _makeFormatPropPage(self):
	page = FormatterPage(self, self)
	self.addPage(page, "Formatter action properties")
	self.setHelpEnabled(page, 0)
	self.setAppropriate(page, 0)
	self.formatProp = page

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
	if page in (self.source, self.formatProp):
	    page.pre_show()
	QWizard.showPage(self, page)

	if page is self.finish:
	    self.finishButton().setFocus()

    def layOutTitleRow(self, hbox, title):
	self.title_label.setText(title)
	hbox.add(self.title_label)
	

