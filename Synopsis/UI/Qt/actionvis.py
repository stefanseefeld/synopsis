# $Id: actionvis.py,v 1.12 2002/11/11 14:28:41 chalky Exp $
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
# Revision 1.12  2002/11/11 14:28:41  chalky
# New Source Edit dialog, with Insert Rule wizard and new s/path/rules with
# extra features.
#
# Revision 1.11  2002/11/02 06:36:19  chalky
# Upgrade to Qt3
#
# Revision 1.10  2002/10/11 06:03:23  chalky
# Use config from project
#
# Revision 1.9  2002/09/28 05:53:31  chalky
# Refactored display into separate project and browser windows. Execute projects
# in the background
#
# Revision 1.8  2002/08/23 04:36:35  chalky
# Use icons! :)
#
# Revision 1.7  2002/06/22 07:03:27  chalky
# Updates to GUI - better editing of Actions, and can now execute some.
#
# Revision 1.6  2002/06/12 12:58:33  chalky
# Updates to display - hilite items and right click menus
#
# Revision 1.5  2002/04/26 01:21:14  chalky
# Bugs and cleanups
#
# Revision 1.4  2001/11/09 08:06:59  chalky
# More GUI fixes and stuff. Double click on Source Actions for a dialog.
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


import sys, pickle, Synopsis, cStringIO, math
from qt import *

# In later versions of python-qt aka PyQt this is in a separate module
if not globals().has_key('QCanvasView'):
    from qtcanvas import *

from Synopsis.Config import prefix
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Core.Project import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree

import actionwiz

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
    """The normal CanvasStrategy to handle mouse actions."""

    class MenuHandler:
	"""Wraps menu callbacks with an action or line object"""
	def __init__(self, sel, obj):
	    self.sel = sel
	    self.obj = obj 
	def on_properties(self):
	    self.sel.on_properties(self.obj)
	def on_delete_action(self):
	    self.sel.on_delete_action(self.obj)
	def on_delete_line(self):
	    self.sel.on_delete_line(self.obj)
	def on_default_formatter(self):
	    self.sel.on_default_formatter(self.obj)

    def __init__(self, canvas, view):
	CanvasStrategy.__init__(self, canvas, view)
	self.__drag_action = None
	self.__icon = None
	self.__last = None
	self.__hilite = None # the action or line to hilight
	self.__normal_cursor = QCursor(Qt.ArrowCursor)
	self.__moving_cursor = QCursor(Qt.BlankCursor)
	self.__hint_cursor = QCursor(Qt.SizeAllCursor)

    def reset(self):
	self.__drag_item = None

    def press(self, event):
	action = self.canvas.get_action_at(event.x(), event.y())
	if action and event.button() == Qt.LeftButton:
	    # Drag the action if left button pressed
	    self.__drag_action = action
	    self.__last = QPoint(event.pos())
	    self.view.setCursor(self.__moving_cursor)
	    return
	if event.button() == Qt.RightButton:
	    if action and event.button() == Qt.RightButton:
		# Show a menu for the right button
		handler = self.MenuHandler(self, action)
		menu = QPopupMenu(self.view)
		menu.insertItem("Delete Action", handler.on_delete_action)
		if isinstance(action, FormatAction):
		    id = menu.insertItem("Default Formatter", handler.on_default_formatter)
		    check = self.canvas.project.default_formatter() is action
		    menu.setItemChecked(id, check)
		menu.insertItem("Properties...", handler.on_properties)
		menu.exec_loop(QCursor.pos())
		return
	    line = self.canvas.get_line_at(event.x(), event.y())
	    if line:
		# Show a menu for the right button
		handler = self.MenuHandler(self, line)
		menu = QPopupMenu(self.view)
		menu.insertItem("Delete Channel", handler.on_delete_line)
		menu.exec_loop(QCursor.pos())
		return
	    
    def release(self, event):
	if self.__drag_action:
	    self.__drag_action = None
	    self.view.setCursor(self.__hint_cursor)
	    
    def move(self, event):
	# Drag an action if we're dragging
	if self.__drag_action:
	    # Move the dragging action
	    dx = event.x() - self.__last.x()
	    dy = event.y() - self.__last.y()
	    self.view.actions.move_action_by(self.__drag_action, dx, dy)
	    self.__last = QPoint(event.pos())
	    return
	# Hilite an action if mouse is over one
	action = self.canvas.get_action_at(event.x(), event.y())
	if action:
	    self.view.setCursor(self.__hint_cursor)
	    self.hilite(self.canvas.get_icon_for(action))
	    return
	# Hilite a line if mouse is over one
	line = self.canvas.get_line_at(event.x(), event.y())
	if line:
	    self.hilite(line)
	elif self.__hilite:
	    # Not doing anything..
	    self.hilite(None)
	    self.view.setCursor(self.__normal_cursor)
    
    def hilite(self, obj):
	"""Sets the currently hilited object. The object may be None, an Icon
	or a Line"""
	if obj is self.__hilite:
	    return
	if self.__hilite:
	    self.__hilite.set_hilite(0)
	self.__hilite = obj
	if obj:
	    obj.set_hilite(1)
	self.canvas.update()
   
    def doubleClick(self, event):
	action = self.canvas.get_action_at(event.x(), event.y())
	if action: self.on_properties(action)
    
    def key(self, event):
	"Override default set-mode-to-select behaviour"
	event.ignore()

    def on_delete_line(self, line):
	print "Deleting line",line.source.name(),"->",line.dest.name()
	button = QMessageBox.warning(self.view, "Confirm delete",\
	    'Delete line from "%s" to "%s"?'%(line.source.name(),
	    line.dest.name()), QMessageBox.Yes, QMessageBox.No)
	if button is QMessageBox.No: return
	self.canvas.actions.remove_channel(line.source, line.dest)

    def on_delete_action(self, action):
	print "Deleting action",action.name()
	button = QMessageBox.warning(self.view, "Confirm delete",\
	    'Delete action "%s"?'%(action.name(),),
	    QMessageBox.Yes, QMessageBox.No)
	if button is QMessageBox.No: return
	self.canvas.actions.remove_action(action)

    def on_default_formatter(self, action):
	print "Changing default formatter:",action.name()
	if self.canvas.project.default_formatter() is action:
	    self.canvas.project.set_default_formatter(None)
	else:
	    self.canvas.project.set_default_formatter(action)

    def on_properties(self, action):
	print action.name()
	if isinstance(action, SourceAction):
	    editor = actionwiz.SourceActionEditor(self.view, self.canvas.project)
	    result = editor.edit(action)
	    print result
	    return
	actionwiz.ActionDialog(self.view, action, self.canvas.project)

class ConnectStrategy (CanvasStrategy):
    def __init__(self, canvas, view):
	CanvasStrategy.__init__(self, canvas, view)
	self.__normal_cursor = QCursor(Qt.ArrowCursor)
	self.__hint_cursor = QCursor(Qt.UpArrowCursor)
	self.__find_cursor = QCursor(Qt.SizeHorCursor)
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
	# Find action under cursor and change cursor type
	if self.source:
	    self.templine.setPoints(self.source.x()+16, self.source.y()+16, event.x(), event.y())
	    self.canvas.update()
	action = self.canvas.get_action_at(event.x(), event.y())
	if action and action is not self.source:
	    if self.view.actions.is_valid_channel(self.source, action):
		self.view.setCursor(self.__hint_cursor)
		return
	if self.source:
	    self.view.setCursor(self.__find_cursor)
	else:
	    self.view.setCursor(self.__normal_cursor)
    def press(self, event):
	action = self.canvas.get_action_at(event.x(), event.y())
	if not action: return
	if not self.source: self.setSource(action, event)
	elif action is not self.source:
	    if self.view.actions.is_valid_channel(self.source, action):
		self.setDest(action)
    def release(self, event):
	action = self.canvas.get_action_at(event.x(), event.y())
	if action and self.source and action is not self.source:
	    if self.view.actions.is_valid_channel(self.source, action):
		self.setDest(action)
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
    def __init__(self, canvas, view, action_type):
	CanvasStrategy.__init__(self, canvas, view)
	self.__drag_action = None
	self.__normal_cursor = QCursor(Qt.ArrowCursor)
	self.__moving_cursor = QCursor(Qt.BlankCursor)
	self.__action_type = action_type

    def set(self):
	action_class = getattr(Synopsis.Core.Action, '%sAction'%self.__action_type)
	self.action = action_class(self.view.last_pos.x()-16,
			     self.view.last_pos.y()-16, 
			     "New %s action"%self.__action_type)
	try:
	    # Run the wizard to set the type of the action
	    wizard = actionwiz.AddWizard(self.view, self.action, self.canvas.project)
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

class ActionIcon (ActionVisitor):
    def __init__(self, action=None):
	self.icon = None
	if action: action.accept(self)
    def visitAction(self, action): pass #self.icon='syn-icon-action.png'
    def visitSource(self, action): self.icon='syn-icon-c++.png'
    def visitParser(self, action): self.icon='syn-icon-parse.png'
    def visitLinker(self, action): self.icon='syn-icon-link.png'
    def visitCacher(self, action): self.icon='syn-icon-cache.png'
    def visitFormat(self, action): self.icon='syn-icon-html.png'

class Icon:
    "Encapsulates the canvas display of an Action"

    def __init__(self, canvas, action):
	self.action = action
	self.canvas = canvas
	self.icon = ActionIcon(action).icon
	if self.icon:
	    icon = prefix+'/share/synopsis/'+self.icon
	    self.pixmap = QPixmap(icon)
	    self.array = QCanvasPixmapArray([self.pixmap])
	    self.img = QCanvasSprite(self.array, canvas)
	else:
	    self.img = QCanvasRectangle(action.x(), action.y(), 32, 32, canvas)
	    self.brush = QBrush(ActionColorizer(action).color)
	    self.img.setBrush(self.brush)
	self.width = self.img.width()
	self.height = self.img.height()
	self.img.setZ(1)
	self.text = QCanvasText(action.name(), canvas)
	self.text.setZ(1)
	self.img.show()
	self.text.show()
	self.hilite = 0
	self.update_pos()
    def hide(self):
	"""Hides icon on canvas"""
	# Note, items must be deleted, which will occur when GC happens.
	# Until them, make them invisible
	self.img.hide()
	self.text.hide()
    def set_hilite(self, yesno):
	self.hilite = yesno
	if not hasattr(self, 'brush'): return
	if yesno:
	    colour = self.brush.color()
	    brush = QBrush(QColor(colour.red()*3/4, colour.green()*3/4,
		colour.blue()*3/4))
	else:
	    brush = self.brush
	self.img.setBrush(brush)
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
	self.hilite = 0
	self.line = QCanvasLine(canvas)
	self.line.setPen(QPen(Qt.blue))
	self.arrow = QCanvasPolygon(canvas)
	self.arrow.setBrush(QBrush(Qt.blue))
	self.update_pos()
	self.line.show()
	self.arrow.show()
    def hide(self):
	"""Hide line on canvas"""
	self.line.hide()
	self.arrow.hide()
    def set_hilite(self, yesno):
	self.hilite = yesno
	if yesno:
	    pen = QPen(Qt.blue, 2)
	else:
	    pen = QPen(Qt.blue)
	self.line.setPen(pen)
	self.arrow.setPen(pen)
	self.update_pos()
    def update_pos(self):
	source, dest = self.source, self.dest
	src_action, dst_action = source.action, dest.action
	# Find centers of the two icons
	src_xrad, src_yrad = source.width/2, source.height/2
	dst_xrad, dst_yrad = dest.width/2, dest.height/2
	if src_xrad == 0: src_xrad, src_yrad = 10, 10
	if dst_xrad == 0: dst_xrad, dst_yrad = 10, 10
	x1, y1 = src_action.x()+src_xrad, src_action.y()+src_yrad
	x2, y2 = dst_action.x()+dst_xrad, dst_action.y()+dst_yrad
	# Calculate the distance
	dx, dy = x2 - x1, y2 - y1
	d = math.sqrt(dx*dx + dy*dy)
	if d < 32:
	    # Don't draw lines that are too short (they will be behind the
	    # icons)
	    self.line.setPoints(x1, y1, x2, y2)
	    self.arrow.setPoints(QPointArray([]))
	    return
	# Normalize direction vector
	dx, dy = dx / d, dy / d
	# Find the end-point for the arrow
	if math.fabs(dx) * dst_yrad < math.fabs(dy) * dst_xrad:
	    if dy < 0:
		# Bottom
		x2 = x2 + dst_yrad * dx / dy
		y2 = y2 + dst_yrad
	    else:
		# Top
		x2 = x2 - dst_yrad * dx / dy
		y2 = y2 - dst_yrad
	else:
	    if dx < 0:
		# Left
		y2 = y2 + dst_xrad * dy / dx
		x2 = x2 + dst_xrad
	    else:
		# Right
		y2 = y2 - dst_xrad * dy / dx
		x2 = x2 - dst_xrad
	self.line.setPoints(x1, y1, x2, y2)
	alen = 8 + self.hilite
	awid = 5 + self.hilite
	self.arrow.setPoints(QPointArray([
	    x2 + self.hilite*dx, y2 + self.hilite*dy,
	    x2 - alen*dx - awid*dy, y2 - alen*dy + awid*dx,
	    x2 - alen*dx + awid*dy, y2 - alen*dy - awid*dx]))

class ActionCanvas (QCanvas):
    """Extends QCanvas to automatically fill and update the canvas when
    notified of events by an ActionManager"""
    def __init__(self, actions, parent, project):
	QCanvas.__init__(self, parent)
	self.actions = actions
	self.project = project
	self._item_to_action_map = {}
	self._action_to_icon_map = {}
	self._action_lines = {}
	self._item_to_line_map = {}

	self.actions.add_listener(self)

    def get_action_at(self, x, y):
	"Returns the Action (if any) at the given coordinates"
	items = self.collisions(QPoint(x, y))
	if items and self._item_to_action_map.has_key(items[0]):
	    return self._item_to_action_map[items[0]]
    
    def get_line_at(self, x, y):
	"""Returns the Actions forming a line which crosses x,y, as a
	line object"""
	items = self.collisions(QPoint(x, y))
	if items and self._item_to_line_map.has_key(items[0]):
	    return self._item_to_line_map[items[0]]
    
    def get_icon_for(self, action):
	return self._action_to_icon_map[action]

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
	icon.hide()
	self.update()

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
	src_icon = self._action_to_icon_map[source]
	dst_icon = self._action_to_icon_map[dest]
	line = Line(self, src_icon, dst_icon)
	self._action_lines.setdefault(source, []).append(line)
	self._action_lines.setdefault(dest, []).append(line)
	self._item_to_line_map[line.line] = line
	self.update()

    def channel_removed(self, source, dest):
	"Callback from ActionManager. Adds a channel between the given actions"
	# NB: check source and dest separately to handle inconsistancies
	# better, if they should arise
	if self._action_lines[source]:
	    for line in self._action_lines[source]:
		if line.dest is dest:
		    self._action_lines[source].remove(line)
		    if self._item_to_line_map.has_key(line.line):
			self._item_to_line_map[line.line].hide()
			del self._item_to_line_map[line.line]
	if self._action_lines[dest]:
	    for line in self._action_lines[dest]:
		if line.source is source:
		    self._action_lines[dest].remove(line)
		    if self._item_to_line_map.has_key(line.line):
			self._item_to_line_map[line.line].hide()
			del self._item_to_line_map[line.line]
	self.update()

    def action_changed(self, action):
	"Callback from ProjectActions. Indicates changes, including rename"
	icon = self._action_to_icon_map[action]
	icon.text.setText(action.name())
	icon.update_pos()
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
	    'new_source' : AddActionStrategy(canvas, self, 'Source'),
	    'new_parse' : AddActionStrategy(canvas, self, 'Parser'),
	    'new_link' : AddActionStrategy(canvas, self, 'Linker'),
	    'new_cache' : AddActionStrategy(canvas, self, 'Cacher'),
	    'new_format' : AddActionStrategy(canvas, self, 'Format')
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
    def __init__(self, parent, main_window, project):
	QVBox.__init__(self, parent)
	self.setCaption("Canvas")
	self.main_window = main_window
	self.__activated = 0

	# Make the toolbar
	self.buttons = QButtonGroup()
	self.buttons.setExclusive(1)
	self.tool = QToolBar("Canvas", self.main_window, self)
	#self.tool.setHorizontalStretchable(0)
	pixmap = QPixmap(16,16); pixmap.fill()
	self.tool_sel = QToolButton(QIconSet(pixmap), "Select", "Select actions in the display", self.setSelect, self.tool)
	self.tool_sel.setUsesTextLabel(1)
	self.tool_sel.setToggleButton(1)
	self.buttons.insert(self.tool_sel)

	pixmap = QPixmap(16,16); pixmap.fill(Qt.blue)
	self.tool_con = QToolButton(QIconSet(pixmap), "Connect", "Connect two actions", self.setConnect, self.tool)
	self.tool_con.setUsesTextLabel(1)
	self.tool_con.setToggleButton(1)
	self.buttons.insert(self.tool_con)

	#pixmap = QPixmap(16,16); pixmap.fill(Qt.red)
	#self.tool_add = QToolButton(QIconSet(pixmap), "Add Action", "Add a new action to the project", self.newAction, self.tool)
	#self.tool_add.setUsesTextLabel(1)
	#self.tool_add.setToggleButton(1)
	#self.buttons.insert(self.tool_add)
	tools = (('Source','source','c++'), ('Parser','parse','parse'),
		 ('Linker','link','link'), ('Cache','cache', 'cache'),
		 ('Formatter','format', 'html'))
	for name, short_name, icon in tools:
	    self._makeNewTool(name, short_name, icon)

	# Make the menu, to be inserted in the app menu upon window activation
	self._file_menu = QPopupMenu(main_window.menuBar())
	#self._file_menu.insertItem("New &Action", self.newAction, Qt.CTRL+Qt.Key_A)
	self._file_menu.insertItem("&Save Project", self.saveProject, Qt.CTRL+Qt.Key_S)
	self._file_menu.insertItem("Save Project &As...", self.saveProjectAs)


	self.__tools = {
	    'select' : self.tool_sel,
	    'connect': self.tool_con,
	    'new_source' : self.tool_source,
	    'new_parse' : self.tool_parse,
	    'new_link' : self.tool_link,
	    'new_cache' : self.tool_cache,
	    'new_format' : self.tool_format
	}
	# Make the canvas
	self.project = project
	self.actions = self.project.actions()
	self.canvas = ActionCanvas(self.actions, self, project)
	self.canvas.resize(parent.width(), parent.height())
	self.canvas_view = CanvasView(self.canvas, self)

	self.show()

	self.connect(parent.parentWidget(), SIGNAL('windowActivated(QWidget*)'), self.windowActivated)

	self.setMode('select')
	self.activate()

    def _makeNewTool(self, name, short_name, icon_id):
	icon = prefix+'/share/synopsis/syn-icon-%s.png'%icon_id
	image = QImage(icon)
	# Resize icon to correct size
	image.smoothScale(16, 16, QImage.ScaleMin)
	pixmap = QPixmap(image)
	tool = QToolButton(QIconSet(pixmap), "New %s"%name,
	    "Add a new %s action to the project"%name, 
	    getattr(self, 'new%sAction'%name), self.tool)
	tool.setUsesTextLabel(1)
	tool.setToggleButton(1)
	self.buttons.insert(tool)
	# Store with appropriate name in self
	setattr(self, 'tool_'+short_name, tool)

    def resizeEvent(self, ev):
	QVBox.resizeEvent(self, ev)
	self.canvas.resize(self.canvas_view.width(), self.canvas_view.height())

	
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

    def newSourceAction(self):
	self.setMode('new_source')

    def newParserAction(self):
	self.setMode('new_parse')

    def newLinkerAction(self):
	self.setMode('new_link')

    def newCacheAction(self):
	self.setMode('new_cache')

    def newFormatterAction(self):
	self.setMode('new_format')

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
	

