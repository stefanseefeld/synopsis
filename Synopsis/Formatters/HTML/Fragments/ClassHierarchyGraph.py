#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import IR
from Synopsis.Processor import InvalidCommand
from ClassHierarchySimple import ClassHierarchySimple
import os

class ClassHierarchyGraph(ClassHierarchySimple):
    """Prints a graphical hierarchy for classes, using the Dot formatter.
   
    @see Formatters.Dot
    """
    def format_class(self, class_):

        from Synopsis.Formatters import Dot
        super = self.processor.class_tree.superclasses(class_.name)
        sub = self.processor.class_tree.subclasses(class_.name)
        if len(super) == 0 and len(sub) == 0:
            # Skip classes with a boring graph
            return ''
        #label = self.processor.files.scoped_special('inheritance', clas.name)
        label = self.formatter.filename()[:-5] + '-inheritance.html'
        tmp = os.path.join(self.processor.output, label)
        ir = IR.IR({}, [class_], self.processor.ir.types)
        dot = Dot.Formatter()
        dot.toc = self.processor.toc
        try:
            dot.process(ir,
                        output=tmp,
                        format='html',
                        base_url=self.formatter.filename(),
                        type='single',
                        title=label)
            text = ''
            input = open(tmp, "r+")
            line = input.readline()
            while line:
                text = text + line
                line = input.readline()
            input.close()
            os.unlink(tmp)
            return text
        except InvalidCommand, e:
            print 'Warning : %s'%str(e)
            return ''

