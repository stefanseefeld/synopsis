# $Id: Project.py,v 1.6 2002/06/22 07:03:27 chalky Exp $
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
# $Log: Project.py,v $
# Revision 1.6  2002/06/22 07:03:27  chalky
# Updates to GUI - better editing of Actions, and can now execute some.
#
# Revision 1.5  2002/06/12 12:58:09  chalky
# Added remove channel facility.
#
# Revision 1.4  2002/04/26 01:21:13  chalky
# Bugs and cleanups
#
# Revision 1.3  2001/11/07 05:58:21  chalky
# Reorganised UI, opening a .syn file now builds a simple project to view it
#
# Revision 1.2  2001/11/06 08:47:11  chalky
# Silly bug, arrows, channels are saved
#
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#

import sys
from Synopsis.Core import Util
from Action import *

class Project:
    """Encapsulates a single project. A project is a set of actions connected
    by channels - each action may have one or more inputs and outputs which
    are other actions. A project also has project-wide configuration, such as
    the data directory."""
    def __init__(self):
	self.__data_dir = './'
	self.__filename = None
	self.__actions = ProjectActions(self)
	self.__name = 'New Project'
	self.__default_formatter = None
    def filename(self):
	"Returns the filename of this project, or None if not set yet"
	return self.__filename
    def set_filename(self, filename):
	"Sets the filename of this file"
	self.__filename = filename
    def data_dir(self): return self.__data_dir
    def set_data_dir(self, dir): self.__data_dir = dir

    def name(self): return self.__name
    def set_name(self, name): self.__name = name

    def default_formatter(self): return self.__default_formatter
    def set_default_formatter(self, action):
	if isinstance(action, FormatAction) or action is None:
	    self.__default_formatter = action

    def actions(self):
	"Returns a ProjectActions object"
	return self.__actions

    def save(self):
	if self.__filename is None:
	    raise Exception, 'Project has no filename'
	file = open(self.__filename, 'w')
	writer = ProjectWriter(file)
	
	writer.write(
	    '"""This file is auto-generated. If you must make changes by hand, '+
	    'please make a backup just in case."""\n')
	# write project stuff like name, base dir, etc here
	writer.write_item(self)

	writer.flush()
	file.close()

    def load(self, filename):
	ProjectReader(self).read(filename)

class ProjectActions:
    """Manages the actions in a project.

    Clients can register for events related to the actions. The events
    supported by the listener interface are:

    def action_added(self, action):
    def action_moved(self, action):
    def action_removed(self, action):
    def channel_added(self, source, dest):
    def channel_removed(self, source, dest):
    """

    def __init__(self, project):
	"""Constructor"""
	self.project = project
	self.__actions = []
	self.__action_names = {}
	self.__listeners = []

    def actions(self):
	"""Returns the list of actions in this project Actions. The list
	returned should be considered read-only"""
	return self.__actions

    def get_action(self, name):
	"""Returns the Action object by name. This method uses a dictionary
	lookup so should be preferred to iteration. Returns None if the name
	is not found."""
	if not self.__action_names.has_key(name): return None
	return self.__action_names[name]

    def add_listener(self, l):
	"""Adds a listener to this Projec Actions' events. The listener may
	implement any of the supported methods and will receive those events.
	@see ProjectActions for more info."""
	self.__listeners.append(l)

    def __fire(self, signal, *args):
	"""Fires the given event to all listeners"""
	for l in self.__listeners:
	    if hasattr(l, signal):
		apply(getattr(l, signal), args)

    def add_action(self, action):
	"""Adds the given action to this project"""
	self.check_name(action)
	self.__actions.append(action)
	self.__action_names[action.name()] = action
	self.__fire('action_added', action)

    def move_action(self, action, x, y):
	"""Moves the given action to the given screen coordinates"""
	action.move_to(x, y)
	self.__fire('action_moved', action)

    def move_action_by(self, action, dx, dy):
	"""Moves the given action by the given screen delta-coordinates"""
	action.move_by(dx, dy)
	self.__fire('action_moved', action)

    def remove_action(self, action):
	"""Removes the given action, and destroys all channels to/from it"""
	if action not in self.__actions: return
	# Remove all channels
	for src in tuple(action.inputs()):
	    self.remove_channel(src, action)
	for dst in tuple(action.outputs()):
	    self.remove_channel(action, dst)
	# Remove action
	self.__actions.remove(action)
	del self.__action_names[action.name()]
	# Check default_formatter
	if action is self.project.default_formatter():
	    self.project.set_default_formatter(None)
	self.__fire('action_removed', action)

    def rename_action(self, action, newname):
	"""Renames the given action to the given name"""
	del self.__action_names[action.name()]
	action.set_name(newname)
	self.__action_names[newname] = action

    def add_channel(self, source, dest):
	"""Adds a "channel" between two actions. Causes the output of the
	first to be connected to the input of the second. The event
	'channel_added' is fired."""
	if dest in source.outputs() and source in dest.inputs(): return
	source.outputs().append(dest)
	dest.inputs().append(source)
	self.__fire('channel_added', source, dest)

    def remove_channel(self, source, dest):
	"""Removes a "channel" between two actions. If the channel doesn't
	exist, it is silently ignored. The event 'channel_removed' is
	fired."""
	if dest not in source.outputs() and source not in dest.inputs():
	    return
	source.outputs().remove(dest)
	dest.inputs().remove(source)
	self.__fire('channel_removed', source, dest)

    def action_changed(self, action):
	"""Indicates that the given action has changed, so that listeners can
	update themselves"""
	self.__fire('action_changed', action)

    def check_name(self, action):
	"Checks the name, and renames if necessary"
	if not self.__action_names.has_key(action.name()): return
	copy = 1
	name = action.name()
	newname = '%s (%d)'%(name, copy)
	while self.__action_names.has_key(newname):
	    copy = copy + 1
	    newname = '%s (%d)'%(name, copy)
	action.set_name(newname)

    def is_valid_channel(self, source, dest):
	"""Returns true if the given source-dest pair would form a valid
	channel.
	Invalid pairs (eg: formatter->formatter) return false.
	Cyclic channels are also disallowed."""
	valid_pairs = {
	    SourceAction : (ParserAction,),
	    ParserAction : (CacherAction,LinkerAction,FormatAction),
	    LinkerAction : (CacherAction,LinkerAction,FormatAction),
	    CacherAction : (CacherAction,LinkerAction,FormatAction),
	    FormatAction : ()
	}
	if not isinstance(source, Action) or not isinstance(dest, Action):
	    return None
	# First, check valid pair
	if not valid_pairs.has_key(source.__class__):
	    return None
	if dest.__class__ not in valid_pairs[source.__class__]:
	    return None
	# Second, check that dest isn't already output of source
	if dest in source.outputs() or dest is source:
	    return None
	# Third, check cycles: descend input tree checking for dest
	check = list(source.inputs())
	while len(check):
	    action = check.pop(0)
	    if action is dest:
		return None
	    check.extend(action.inputs())
	# If got this far, combination is valid!
	return 1



class ProjectWriter (Util.PyWriter):
    def write_Project(self, project):
	self.write("class Project:\n")
	self.indent()
	# write project stuff like name, base dir, etc here
	self.write_attr('name', project.name())
	self.write_attr('data_dir', project.data_dir())
	self.write_item(project.actions())
	dfm = project.default_formatter()
	if dfm: self.write_attr('default_formatter', dfm.name())
	self.outdent()

    def write_ProjectActions(self, actions):
	self.write_attr('actions', self.long(actions.actions()))
	channels = []
	for source in actions.actions():
	    for dest in source.outputs():
		channels.append( (source.name(), dest.name()) )
	self.write_attr('channels', self.long(channels))

    def write_SourcePath(self, path):
	self.write_list((path.type, path.dir, path.glob))

    def write_SourceAction(self, action):
	self.write_list(('SourceAction', 
	    action.x(), action.y(), action.name(),
	    self.long(action.paths()) ))
    
    def write_ParserAction(self, action):
	self.write_list(('ParserAction',
	    action.x(), action.y(), action.name(),
	    self.flatten_struct(action.config()) ))
    
    def write_LinkerAction(self, action):
	self.write_list(('LinkerAction',
	    action.x(), action.y(), action.name(),
	    self.flatten_struct(action.config()) ))
    
    def write_FormatAction(self, action):
	self.write_list(('FormatAction',
	    action.x(), action.y(), action.name(),
	    self.flatten_struct(action.config()) ))
    
    def write_CacherAction(self, action):
	self.write_list(('CacherAction',
	    action.x(), action.y(), action.name(),
	    action.dir, action.file))

class ProjectReader:
    """A class that reads projects"""
    def __init__(self, project):
	self.project = project
    def read(self, filename):
	# Execute the file
	exec_dict = {}
	execfile(filename, exec_dict)
	# Get 'Project' attribute
	if not exec_dict.has_key('Project'):
	    raise Exception, 'Project load failed: Project attribute not found'
	project = exec_dict['Project']
	# Read attributes
	self.project.set_filename(filename)
	self.read_Project(project)
    def read_Project(self, proj_obj):
	project = self.project
	# Read project stuff like name
	if hasattr(proj_obj, 'name'): project.set_name(proj_obj.name)
	if hasattr(proj_obj, 'data_dir'): project.set_name(proj_obj.data_dir)
	self.read_ProjectActions(proj_obj)
	if hasattr(proj_obj, 'default_formatter'):
	    action = project.actions().get_action(proj_obj.default_formatter)
	    if action: project.set_default_formatter(action)
    def read_ProjectActions(self, project_obj):
	for action in project_obj.actions:
	    t = action[0]
	    if t in ('SourceAction', 'CacherAction', 'FormatAction',
		    'ParserAction', 'LinkerAction'):
		getattr(self, 'read_'+t)(action)
	    else:
		raise Exception, 'Unknown action type: %s'%action[0]
	if hasattr(project_obj, 'channels'):
	    actions = self.project.actions()
	    for source, dest in project_obj.channels:
		src_obj = actions.get_action(source)
		dst_obj = actions.get_action(dest)
		actions.add_channel(src_obj, dst_obj)

    def read_SourceAction(self, action):
	type, x, y, name, paths = action
	self.action = SourceAction(x, y, name) 
	self.project.actions().add_action(self.action)
	for path in paths: self.read_SourcePath(path)

    def read_ParserAction(self, action):
	type, x, y, name, config = action
	self.action = ParserAction(x, y, name) 
	self.project.actions().add_action(self.action)
	self.action.set_config(config)

    def read_LinkerAction(self, action):
	type, x, y, name, config = action
	self.action = LinkerAction(x, y, name) 
	self.project.actions().add_action(self.action)
	self.action.set_config(config)

    def read_FormatAction(self, action):
	type, x, y, name, config = action
	self.action = FormatAction(x, y, name) 
	self.project.actions().add_action(self.action)
	self.action.set_config(config)

    def read_SourcePath(self, path):
	type, dir, glob = path
	self.action.paths().append(SourcePath(type, dir, glob))

    def read_CacherAction(self, action):
	type, x, y, name, dir, file = action
	self.action = CacherAction(x, y, name)
	self.project.actions().add_action(self.action)
	self.action.dir = dir
	self.action.file = file
		

