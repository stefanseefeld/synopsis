# $Id: actionwiz.py,v 1.4 2002/06/22 07:03:27 chalky Exp $
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

class Struct:
    "Dummy class. Initialise with keyword args."
    def __init__(self, **keys):
        for name, value in keys.items(): setattr(self, name, value)

class SourcePage (QVBox):
    """The Page (portion of dialog) that displays config options for Source
    Actions"""
    def __init__(self, parent, action_container):
	QVBox.__init__(self, parent)
	self.__ac = action_container
	self._make_layout()
    def title(self):
	return "Source Config"
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
	self.connect(self.source_remove_path, SIGNAL('clicked()'), self.onSourceRemovePath)
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

    def onSourceRemovePath(self):
	"""Remove the currently selected Path object from the list"""
	id = self.source_paths.currentItem()
	if id < 0: return
	del self.action().paths()[id]
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
	self.path_list = QListBox(self)
	label.setBuddy(self.path_list)
	self._update_list()
	self.connect(self.path_list, SIGNAL('clicked(QListBoxItem*)'), self.onPathSelected)

	# Make buttons
	bhbox = QHBox(self)
	bhbox.setSpacing(4)
	self.badd = QPushButton('&Add', bhbox)
	self.bremove = QPushButton('&Remove', bhbox)
	self.bgcc = QPushButton('Add &GCC paths', bhbox)
	QToolTip.add(self.badd, "Add a new path setting")
	QToolTip.add(self.bremove, "Remove the selected path setting")
	self.connect(self.badd, SIGNAL('clicked()'), self.onAddPath)
	self.connect(self.bremove, SIGNAL('clicked()'), self.onRemovePath)
	self.connect(self.bgcc, SIGNAL('clicked()'), self.onAddGCC)

    def _update_list(self):
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

    def pre_show(self):
	pass

    def showEvent(self, ev):
	QVBox.showEvent(self, ev)
	# Focus name field
	#self.source_name.setFocus()
	#self.source_name.selectAll()

    def onPathSelected(self, item):
	path = item.text()

    def onAddPath(self):
	pass

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
	    elif o == 'cacher': pass
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
	

