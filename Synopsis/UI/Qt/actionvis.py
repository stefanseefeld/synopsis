# $Id: actionvis.py,v 1.1 2001/11/05 06:52:11 chalky Exp $
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
# $Log: actionvis.py,v $
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#


import sys, pickle, Synopsis, cStringIO
from qt import *
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Core.Project import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree

from actionwiz import AddWizard

class CanvasStrategy:
    "An interface for a strategy to handle mouse events"
    def __init__(self, canvas, view):
	self.canvas = canvas
	self.view = view
    def reset(self): pass
    def set(self): pass
    def press(self, event): pass
    def release(self, event): pass
    def move(self, event): pass
    def doubleClick(self, event): pass
    def key(self, event):
	if event.key() == Qt.Key_Escape:
	    self.view.window.setMode('select')
	else:
	    event.ignore()

class SelectionStrategy (CanvasStrategy):
    def __init__(self, canvas, view):
	CanvasStrategy.__init__(self, canvas, view)
	self.__drag_action = None
	self.__icon = None
	self.__last = None
	self.__normal_cursor = QCursor(ArrowCursor)
	self.__moving_cursor = QCursor(BlankCursor)
	self.__hint_cursor = QCursor(SizeAllCursor)

    def reset(self):
	self.__drag_item = None

    def press(self, event):
	action = self.canvas.get_action_at(event.x(), event.y())
	if action:
	    self.__drag_action = action
	    self.__last = QPoint(event.pos())
	    self.view.setCursor(self.__moving_cursor)
	    
    def release(self, event):
	if self.__drag_action:
	    self.__drag_action = None
	    self.view.setCursor(self.__hint_cursor)
	    
    def move(self, event):
	if self.__drag_action:
	    # Move the dragging action
	    dx = event.x() - self.__last.x()
	    dy = event.y() - self.__last.y()
	    self.view.actions.move_action_by(self.__drag_action, dx, dy)
	    self.__last = QPoint(event.pos())
	else:
	    if self.canvas.get_action_at(event.x(), event.y()):
		self.view.setCursor(self.__hint_cursor)
	    else:
		self.view.setCursor(self.__normal_cursor)
    
    def doubleClick(self, event):
	action = self.canvas.get_action_at(event.x(), event.y())
	if action:
	    print action.name()
    
    def key(self, event):
	"Override default set-mode-to-select behaviour"
	event.ignore()

class ConnectStrategy (CanvasStrategy):
    def __init__(self, canvas, view):
	CanvasStrategy.__init__(self, canvas, view)
	self.__normal_cursor = QCursor(ArrowCursor)
	self.__hint_cursor = QCursor(UpArrowCursor)
	self.__find_cursor = QCursor(SizeHorCursor)
	self.templine = QCanvasLine(self.canvas)
	self.templine.setPen(QPen(Qt.blue, 1, Qt.DotLine))

    def set(self):
	self.source = None
	self.templine.hide()
	self.canvas.update()
    def reset(self):
	self.templine.hide()
	self.canvas.update()
    def move(self, event):
	if self.source:
	    self.templine.setPoints(self.source.x()+16, self.source.y()+16, event.x(), event.y())
	    self.canvas.update()
	action = self.canvas.get_action_at(event.x(), event.y())
	if action and action is not self.source:
	    self.view.setCursor(self.__hint_cursor)
	elif self.source:
	    self.view.setCursor(self.__find_cursor)
	else:
	    self.view.setCursor(self.__normal_cursor)
    def press(self, event):
	action = self.canvas.get_action_at(event.x(), event.y())
	if not action: return
	if not self.source: self.setSource(action, event)
	elif action is not self.source: self.setDest(action)
    def release(self, event):
	action = self.canvas.get_action_at(event.x(), event.y())
	if action and self.source and action is not self.source: self.setDest(action)
    def setSource(self, action, event):
	self.source = action
	self.templine.setPoints(action.x()+16, action.y()+16, event.x(), event.y())
	self.templine.show()
	self.canvas.update()
    def setDest(self, action):
	self.templine.hide()
	self.view.actions.add_channel(self.source, action)
	self.source = None
	self.view.window.setMode('select')

class AddActionStrategy (CanvasStrategy):
    def __init__(self, canvas, view):
	CanvasStrategy.__init__(self, canvas, view)
	self.__drag_action = None
	self.__normal_cursor = QCursor(ArrowCursor)
	self.__moving_cursor = QCursor(BlankCursor)

    def set(self):
	self.action = Action(self.view.last_pos.x()-16,
			     self.view.last_pos.y()-16, "New action")
	try:
	    # Run the wizard to set the type of the action
	    wizard = AddWizard(self.view, self.action)
	    if wizard.exec_loop() == QDialog.Rejected:
		# Abort adding
		self.action = None
		self.view.window.setMode('select')
	    else:
		# Wizard will create a new more derived action object
		self.view.actions.add_action(wizard.action)
		self.action = wizard.action
		self.view.setCursor(self.__moving_cursor)
	except Exception, msg:
	    print "An error occured in add wizard:\n", msg
	    self.action = None
	    self.view.window.setMode('select')
	    import traceback
	    traceback.print_exc()

    def reset(self):
	if self.action:
	    self.view.actions.remove_action(self.action)
	self.view.setCursor(self.__normal_cursor)

    def move(self, event):
	self.view.actions.move_action(self.action, event.x()-16, event.y()-16)

    def release(self, event):
	self.action = None
	self.view.window.setMode('select')

class ActionPropertiesDialog (QDialog):
    def __init__(self, parent, action):
	QDialog.__init__(self, parent)
	self.action = action

class ActionColorizer (ActionVisitor):
    def __init__(self, action=None):
	self.color = None
	if action: action.accept(self)
    def visitAction(self, action): self.color=Qt.black
    def visitSource(self, action): self.color=Qt.magenta
    def visitParser(self, action): self.color=Qt.red
    def visitLinker(self, action): self.color=Qt.yellow
    def visitCacher(self, action): self.color=Qt.green
    def visitFormat(self, action): self.color=Qt.cyan

class Icon:
    "Encapsulates the canvas display of an Action"

    def __init__(self, canvas, action):
	self.action = action
	self.canvas = canvas
	self.img = QCanvasRectangle(action.x(), action.y(), 32, 32, canvas)
	self.img.setBrush(QBrush(ActionColorizer(action).color))
	self.img.setZ(1)
	self.text = QCanvasText(action.name(), canvas)
	self.text.setZ(1)
	self.img.show()
	self.text.show()
	self.update_pos()
    def update_pos(self):
	self.img.move(self.action.x(), self.action.y())
	rect = self.text.boundingRect()
	irect = self.img.boundingRect()
	y = irect.bottom()
	x = irect.center().x() - rect.width()/2
	self.text.move(x, y)

class Line:
    "Encapsulates the canvas display of a channel between two Actions"
    def __init__(self, canvas, source, dest):
	self.canvas = canvas
	self.source = source
	self.dest = dest
	self.line = QCanvasLine(canvas)
	self.line.setPen(QPen(Qt.blue))
	self.update_pos()
	self.line.show()
    def update_pos(self):
	source, dest = self.source, self.dest
	self.line.setPoints(
	    source.x()+16, source.y()+16,
	    dest.x()+16, dest.y()+16)


class ActionCanvas (QCanvas):
    """Extends QCanvas to automatically fill and update the canvas when
    notified of events by an ActionManager"""
    def __init__(self, actions, parent):
	QCanvas.__init__(self, parent)
	self.actions = actions
	self._item_to_action_map = {}
	self._action_to_icon_map = {}
	self._action_lines = {}

	self.actions.add_listener(self)

    def get_action_at(self, x, y):
	"Returns the Action (if any) at the given coordinates"
	items = self.collisions(QPoint(x, y))
	if items and self._item_to_action_map.has_key(items[0]):
	    return self._item_to_action_map[items[0]]

    def action_added(self, action):
	"Callback from ActionManager. Adds an Icon for the new Action"
	icon = Icon(self, action)
	self._item_to_action_map[icon.img] = action
	self._action_to_icon_map[action] = icon
	self.update()

    def action_removed(self, action):
	"Callback from ActionManager. Removes the Icon for the Action"
	icon = self._action_to_icon_map[action]
	del self._action_to_icon_map[action]
	del self._item_to_action_map[icon.img]

    def action_moved(self, action):
	"Callback from ActionManager. Moves the Icon to follow the Action"
	icon = self._action_to_icon_map[action]
	icon.update_pos()
	if self._action_lines.has_key(action):
	    for line in self._action_lines[action]:
		line.update_pos()
	self.update()

    def channel_added(self, source, dest):
	"Callback from ActionManager. Adds a channel between the given actions"
	line = Line(self, source, dest)
	if not self._action_lines.has_key(source):
	    self._action_lines[source] = []
	if not self._action_lines.has_key(dest):
	    self._action_lines[dest] = []
	self._action_lines[source].append(line)
	self._action_lines[dest].append(line)
	self.update()


class CanvasView (QCanvasView):
    def __init__(self, canvas, parent):
	QCanvasView.__init__(self, canvas, parent)
	self.actions = parent.actions
	self.window = parent
	self.last_pos = QPoint(-50,-50)
	self.viewport().setFocusPolicy(QWidget.ClickFocus)
	self.__canvas = canvas
	self.__strategies = {
	    'select' : SelectionStrategy(canvas, self),
	    'connect': ConnectStrategy(canvas, self),
	    'action' : AddActionStrategy(canvas, self)
	}
	self.viewport().setMouseTracking(1)
	
	self.__strategy = self.__strategies['select']
	self.connect(parent, PYSIGNAL('modeChanged(string)'), self.modeChanged)
    def modeChanged(self, tool):
	if self.__strategies.has_key(tool):
	    self.__strategy.reset()
	    self.__strategy = self.__strategies[tool]
	    self.__strategy.set()
    def contentsMousePressEvent(self, event):
	self.__strategy.press(event)
    def contentsMouseReleaseEvent(self, event):
	self.__strategy.release(event)
    def contentsMouseMoveEvent(self, event):
	self.last_pos = QPoint(event.pos())
	self.__strategy.move(event)
    def contentsMouseDoubleClickEvent(self, event):
	self.__strategy.doubleClick(event)
    def keyPressEvent(self, event):
	self.__strategy.key(event)
	

class CanvasWindow (QVBox):
    def __init__(self, parent, main_window, filename = None):
	QVBox.__init__(self, parent)
	self.setCaption("Canvas")
	self.main_window = main_window
	self.__activated = 0

	# Make the toolbar
	self.buttons = QButtonGroup()
	self.buttons.setExclusive(1)
	self.tool = QToolBar("Canvas", self.main_window, self)
	self.tool.setHorizontalStretchable(0)
	pixmap = QPixmap(16,16); pixmap.fill()
	self.tool_sel = QToolButton(pixmap, "Select", "Select actions in the display", self.setSelect, self.tool)
	self.tool_sel.setUsesTextLabel(1)
	self.tool_sel.setToggleButton(1)
	self.buttons.insert(self.tool_sel)

	pixmap = QPixmap(16,16); pixmap.fill(Qt.blue)
	self.tool_con = QToolButton(pixmap, "Connect", "Connect two actions", self.setConnect, self.tool)
	self.tool_con.setUsesTextLabel(1)
	self.tool_con.setToggleButton(1)
	self.buttons.insert(self.tool_con)

	pixmap = QPixmap(16,16); pixmap.fill(Qt.red)
	self.tool_add = QToolButton(pixmap, "Add Action", "Add a new action to the project", self.newAction, self.tool)
	self.tool_add.setUsesTextLabel(1)
	self.tool_add.setToggleButton(1)
	self.buttons.insert(self.tool_add)

	# Make the menu, to be inserted in the app menu upon window activation
	self._file_menu = QPopupMenu(main_window.menuBar())
	self._file_menu.insertItem("New &Action", self.newAction, Qt.CTRL+Qt.Key_A)
	self._file_menu.insertItem("&Save Project", self.saveProject, Qt.CTRL+Qt.Key_S)
	self._file_menu.insertItem("Save Project &As...", self.saveProjectAs)


	self.__tools = {
	    'select' : self.tool_sel,
	    'connect': self.tool_con,
	    'action' : self.tool_add
	}
	# Make the canvas
	self.project = Project()
	self.actions = self.project.actions()
	self.canvas = ActionCanvas(self.actions, self)
	self.canvas.resize(parent.width(), parent.height())
	CanvasView(self.canvas, self)
	# Load the project
	if filename:
	    self.project.load(filename)

	
	self.show()

	self.connect(parent, SIGNAL('windowActivated(QWidget*)'), self.windowActivated)

	self.setMode('select')
	self.activate()
	
    def setMode(self, mode):
	self.mode = mode
	self.__tools[mode].setOn(1)

	self.emit(PYSIGNAL('modeChanged(string)'), (self.mode,))

    def windowActivated(self, widget):
	if self.__activated:
	    if widget is not self: self.deactivate()
	elif widget is self: self.activate()
    
    def activate(self):
	self.__activated = 1
	self._menu_id = self.main_window.menuBar().insertItem('Canvas', self._file_menu)

    def deactivate(self):
	self.__activated = 0
	self.main_window.menuBar().removeItem(self._menu_id)

    def setSelect(self):
	self.setMode('select')

    def setConnect(self):
	self.setMode('connect')

    def newAction(self):
	self.setMode('action')

    def saveProject(self):
	if self.project.filename() is None:
	    filename = QFileDialog.getSaveFileName('project.synopsis', '*.synopsis',
		self, 'ProjectSave', "Save Project...")
	    filename = str(filename)
	    if not filename:
		# User clicked cancel
		return
	    print "filename =",filename
	    self.project.set_filename(filename)
	self.project.save()
	
    def saveProjectAs(self):
	filename = QFileDialog.getSaveFileName('project.synopsis', '*.synopsis',
	    self, 'ProjectSave', "Save Project As...")
	filename = str(filename)
	if not filename:
	    # User clicked cancel
	    return
	print "filename =",filename
	self.project.set_filename(filename)
	self.project.save()
	

