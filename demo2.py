#!/usr/bin/env python
#
# this demo shows how to use the Synopsis API to insert existing type definitions
# into a tree and how to format this tree
#
import idl
import Synopsis
from Synopsis import *
from Formatter import IDLFormatter

formatter = IDLFormatter.IDLFormatter()

print idl.foo(5, 4)

figures = Module("input.idl", "IDL", "module", "Figures", [])
circle = Class("input.idl", "IDL", "interface", "Circle", [])
operation = Operation("input.idl", "IDL", "operation", "draw", [])
type = Class("input.idl", "IDL", "interface", "DrawTraversal", [])
argument = Argument("in", type, "", "traversal")
operation.arguments().append(argument)
circle.operations().append(operation)
figures.add(circle)

Tree.add(figures)
Tree.output(formatter)
