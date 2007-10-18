#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Parameter
from Synopsis import Util
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *

class InheritanceTree(View):

    def filename(self):

        if self.main:
            return self.directory_layout.index()
        else:
            return self.directory_layout.special('InheritanceTree')

    def title(self):

        return 'Inheritance Tree'

    def root(self):

        return self.filename(), self.title()

    def process(self):

        self.start_file()
        self.write_navigation_bar()
        self.write(entity('h1', "Inheritance Tree"))
        self.write('<ul>')
        # FIXME: see HTML.Formatter
        module = self.processor.ir.declarations[0]
        for r in self.processor.class_tree.roots():
            self.process_inheritance(r, module.name())
        self.write('</ul>')
        self.end_file()   

    def process_inheritance(self, name, rel_name):
        self.write('<li>')
        self.write(self.reference(name, rel_name))
        parents = self.processor.class_tree.superclasses(name)
        if parents:
            self.write(' <i>(%s)</i>'%', '.join([Util.ccolonName(p)
                                                 for p in parents]))
        subs = self.processor.class_tree.subclasses(name)
        if subs:
            self.write('<ul>')
            for s in subs:
                self.process_inheritance(s, name)
            self.write('</ul>\n')
        self.write('</li>')
