# $Id: Project.py,v 1.1 2001/11/05 06:52:11 chalky Exp $
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
	self.__actions = ProjectActions()
    def filename(self):
	"Returns the filename of this project, or None if not set yet"
	return self.__filename
    def set_filename(self, filename):
	"Sets the filename of this file"
	self.__filename = filename
    def data_dir(self): return self.__data_dir
    def set_data_dir(self, dir): self.__data_dir = dir

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
	self.set_filename(filename)
	exec_dict = {}
	execfile(filename, exec_dict)
	if not exec_dict.has_key('Project'):
	    raise Exception, 'Project load failed: Project attribute not found'
	project = exec_dict['Project']
	# extract project stuff like name, base dir, etc from project here
	try:
	    self.__actions.read(project)
	except:
	    raise Exception, 'Project load failed: '+sys.exc_info()[0]+sys.exc_info()[1]

class ProjectActions:
    """Manages the actions in a project"""
    class Listener:
	#def action_added(self, action):
	#def action_moved(self, action):
	#def action_removed(self, action):
	#def channel_added(self, source, dest):
	pass
    def __init__(self):
	self.__actions = []
	self.__action_names = {}
	self.__listeners = []
    def actions(self):
	return self.__actions
    def add_listener(self, l):
	self.__listeners.append(l)
    def fire(self, signal, *args):
	for l in self.__listeners:
	    if hasattr(l, signal):
		apply(getattr(l, signal), args)
    def add_action(self, action):
	self.check_name(action)
	self.__actions.append(action)
	self.__action_names[action.name()] = action
	self.fire('action_added', action)
    def move_action(self, action, x, y):
	action.move_to(x, y)
	self.fire('action_moved', action)
    def move_action_by(self, action, dx, dy):
	action.move_by(dx, dy)
	self.fire('action_moved', action)
    def remove_action(self, action):
	self.__actions.remove(action)
	del self.__action_names[action.name()]
	self.fire('action_removed', action)
    def rename_action(self, action, newname):
	del self.__action_names[action.name()]
	action.set_name(newname)
	self.__action_names[newname] = action
    def add_channel(self, source, dest):
	source.outputs().append(dest)
	dest.inputs().append(source)
	self.fire('channel_added', source, dest)

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

class ProjectWriter (Util.PyWriter):
    def write_Project(self, project):
	self.write("class Project:\n")
	self.indent()
	# write project stuff like name, base dir, etc here
	# ...
	self.write_item(project.actions())
	self.outdent()

    def write_ProjectActions(self, actions):
	self.write_attr('actions', self.long(actions.actions()))

    def write_SourcePath(self, path):
	self.write_list((path.type, path.dir, path.glob))

    def write_SourceAction(self, action):
	self.write_list(('SourceAction', 
	    action.x(), action.y(), action.name(),
	    self.long(action.paths()) ))

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
    def read_Project(self, project_obj):
	# Read project stuff like name
	# ...
	self.read_ProjectActions(project_obj.actions)
    def read_ProjectActions(self, action_obj):
	# action_obj should be a list
	for action in action_obj:
	    t = action[0]
	    if t == 'SourceAction': self.read_SourceAction(action)
	    else:
		raise Exception, 'Unknown action type: %s'%action[0]

    def read_SourceAction(self, action):
	type, x, y, name, paths = action
	self.action = SourceAction(x, y, name) 
	self.project.actions().add_action(self.action)
	for path in paths: self.read_SourcePath(path)

    def read_SourcePath(self, path):
	type, dir, glob = path
	self.action.paths().append(SourcePath(type, dir, glob))
		

