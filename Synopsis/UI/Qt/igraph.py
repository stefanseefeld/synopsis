# $Id: igraph.py,v 1.1 2001/11/05 06:52:11 chalky Exp $
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
# $Log: igraph.py,v $
# Revision 1.1  2001/11/05 06:52:11  chalky
# Major backside ui changes
#


import sys, pickle, Synopsis, cStringIO
from qt import *
from Synopsis.Core import AST, Util
from Synopsis.Core.Action import *
from Synopsis.Formatter.ASCII import ASCIIFormatter
from Synopsis.Formatter.ClassTree import ClassTree


class IGraphWindow (QCanvasView):
    class Icon:
	def __init__(self, canvas, classname, x, y):
	    self.classname = classname
	    name = Util.ccolonName(classname[-1:])
	    self.text = QCanvasText(name, canvas)
	    self.text.setZ(2)
	    self.text.show()
	    r = self.text.boundingRect()
	    self.border = QCanvasRectangle(x , y, r.width()+4,r.height()+4, canvas)
	    self.border.setBrush(QBrush(QColor(255,255,200)))
	    self.border.setZ(1)
	    self.border.show()
	    self.subs = []
	    self.supers = []
	    self.sub_lines = []
	    self.super_lines = []
	    self.move_to(x, y)
	def move_to(self, x, y):
	    self.x = x
	    self.y = y
	    self.border.move(x, y)
	    self.text.move(x+2,y+2)
	    r = self.border.boundingRect()
	    self.mid_x = (r.left() + r.right())/2
	    self.max_x = r.right()
	    self.top_y = r.top()
	    self.mid_y = (r.top() + r.bottom())/2
	    self.max_y = r.bottom()
	    for sub in self.sub_lines: self.move_sub(sub)
	    for super in self.super_lines: self.move_super(super)
	def width(self):
	    "returns the width of this icon"
	    return self.border.boundingRect().width()
	def mid(self):
	    "Returns the x, y coords of the middle of this icon"
	    return self.mid_x, self.mid_y
	def add_sub(self, sub, line):
	    self.subs.append(sub)
	    self.sub_lines.append(line)
	    self.move_sub(line)
	def move_sub(self, line):
	    other = line.endPoint()
	    line.setPoints(self.mid_x, self.mid_y, other.x(), other.y())
	def add_super(self, super, line):
	    self.supers.append(super)
	    self.super_lines.append(line)
	    self.move_super(line)
	def move_super(self, line):
	    other = line.startPoint()
	    line.setPoints(other.x(), other.y(), self.mid_x, self.top_y)
	def clear(self):
	    "Removes all references to other icons etc so they can be garbage collected"
	    del self.subs
	    del self.supers
	    del self.sub_lines
	    del self.super_lines
	    self.text.hide()
	    self.border.hide()


    def __init__(self, parent, main_window, classTree):
	self.__canvas = QCanvas()
	self.main_window = main_window
	self.classTree = classTree
	QCanvasView.__init__(self, self.__canvas, parent)
	self.canvas().resize(parent.width(), parent.height())

	# Empty icon list
	self.icons = {}

    def set_class(self, classname):
	self.mainclass = classname
	graphs = self.classTree.graphs()
	classes = None
	for graph in graphs:
	    if classname in graph:
		classes = graph
		break
	if not classes: return

	# Reset icons
	for icon in self.icons.values():
	    icon.clear()
	self.__canvas.setChanged(QRect(QPoint(0,0),self.__canvas.size()))
	self.__canvas.update()

	# Create new icons
	self.icons = {}
	for clas in classes:
	    self.icons[clas]= self.Icon(self.__canvas, clas, 0,0)
	    if clas == classname:
		self.icons[clas].text.setColor(Qt.red)

	# Now connect their supers and subs lists
	for name, icon in self.icons.items():
	    for child in self.classTree.subclasses(name):
		line = QCanvasLine(self.__canvas)
		sub = self.icons[child]
		icon.add_sub(sub, line)
		sub.add_super(icon, line)
		if name == classname or sub.classname == classname:
		    line.setPen(QPen(Qt.red))
		line.show()
	
	self.organize()

	self.__canvas.update()

	self.show()

    def organize(self):
	# find a root
	roots = filter(lambda x: not x.supers, self.icons.values())
	if not roots: return # this is an error
	root = roots[0]
	self._organized = {}
	self._ranks = {}
	self.rank_down(root)
	self.space_ranks()
	self.sort_ranks()

	self.un_offset()

    def un_offset(self):
	"""Moves the graph to the top-left corner of the display"""
	# Find top-left of graph
	min_x, min_y = 0, 0
	for node in self.icons.values():
	    if node.x < min_x: min_x = node.x
	    if node.y < min_y: min_y = node.y
	# Move to top-left of display
	min_x, min_y = min_x - 10, min_y - 10
	max_x, max_y = 0, 0
	for node in self.icons.values():
	    node.move_to(node.x - min_x, node.y - min_y)
	    if node.max_x > max_x: max_x = node.max_x
	    if node.max_y > max_y: max_y = node.max_y
	self.__canvas.resize(max_x+10, max_y+10)

    def rank_down(self, node):
	self._organized[node] = None
	if not self._ranks.has_key(node.y): self._ranks[node.y] = []
	self._ranks[node.y].append(node)
	min_y = node.y + 30
	for sub in node.subs:
	    if self._organized.has_key(sub) and min_y < sub.y: continue
	    sub.move_to(sub.x, node.y + 30)
	    self.rank_down(sub)

	for super in node.supers:
	    if self._organized.has_key(super): continue
	    super.move_to(super.x, node.y - 30)
	    self.rank_down(super)
    
    def space_ranks(self):
	"Spaces out the ranks vertically"
	ranks = self._ranks.keys()
	ranks.sort()
	space = 0
	for rank in ranks:
	    nodes = self._ranks[rank]
	    space = space + len(nodes)
	    new_rank = rank + space

	    del self._ranks[rank]
	    self._ranks[new_rank] = nodes

	    for node in nodes:
		node.move_to(node.x, new_rank)
	    
	
    
    def sort_ranks(self):
	ranks = self._ranks.keys()
	ranks.sort()
	for rank in ranks:
	    nodes = self._ranks[rank]
	    done = {}
	    order = []
	    rank_x = -1e6
	    for node in nodes:
		for super in node.supers:
		    # Create a set of nodes that are childen of super at this rank
		    set = filter(lambda n, y=rank: n.y==y, super.subs)
		    # Eliminate already ordered nodes
		    set = filter(lambda n, d=done: not d.has_key(n), set)
		    # Sort the set relative to where their parents are
		    summer = lambda sum, num: (sum[0]+num, sum[1]+1)
		    def average(sum): return sum[1] and float(sum[0])/sum[1] or 0
		    super_order = lambda s, ds=node.supers: ds.index(s)
		    node_order = lambda n, summer=summer, super_order=super_order, average=average: \
		    	average(reduce(summer, map(super_order, n.supers), (0,0)))
		    set.sort(lambda x,y,no=node_order: cmp(no(x), no(y)))
		    order.extend(set)
		    # Make the nodes as done
		    for s in set: done[s] = 1
		    # Calculate the width of the set
		    set_width = 0
		    for s in set: set_width = set_width + s.width() + 2
		    # Move nodes to their new order
		    x = super.mid()[0] - set_width/2
		    if x < rank_x: x = rank_x
		    for s in set:
			s.move_to(x, s.y)
			x = x + s.width() + 2
		    rank_x = x + 2
		if not node.supers:
		    # Backup: no siblings to rank with so just use rank_x
		    if rank_x < -1e5: rank_x = 0
		    node.move_to(rank_x, node.y)
		    rank_x = rank_x + node.width() + 2 + 5*len(node.subs)
		    order.append(node)
		    done[node] = 1
	    #print rank, map(lambda n:'%s:%d'%(n.classname[-1],n.x), order)


