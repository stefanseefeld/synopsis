#! /usr/bin/env python

import os, os.path, sys
sys.path.insert(0, os.getcwd())

import PTree
import Processor

class Walker(PTree.Visitor):

    def visit_atom(self, a):
        print a
    def visit_list(self, l):
        if l.car():
            l.car().accept(self)
        if l.cdr():
            l.cdr().accept(self)

test = os.path.join(os.path.dirname(__file__), 'test.cc')
buffer = Processor.Buffer(test)
ptree = Processor.parse(buffer)
walker = Walker()
ptree.accept(walker)
