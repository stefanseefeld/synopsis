#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG, Util

class NameMapper(Processor, ASG.Visitor):
    """Abstract base class for name mapping."""

    def visit_group(self, node):
        """Recursively visits declarations under this group/scope/etc"""

        self.visit_declaration(node)
        for d in node.declarations:
            d.accept(self)   

class NamePrefixer(NameMapper):
    """This class adds a prefix to all declaration and type names."""

    prefix = Parameter([], 'the prefix which to prepend to all declared types')
    type = Parameter('Language', 'type to use for the new toplevel modules')

    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)

        if not self.prefix:
            return self.output_and_return_ir()

        for decl in self.ir.declarations:
            decl.accept(self)

        # Now we need to put the declarations in actual nested MetaModules
        for index in range(len(self.prefix), 0, -1):
            module = ASG.MetaModule(self.type, self.prefix[:index])
            module.declarations.extend(self.ir.declarations)
            self.ir.types[module.name] = ASG.Declared('',
                                                      module.name,
                                                      module)
            self.ir.declarations[:] = [module]

        return self.output_and_return_ir()

    def visit_declaration(self, decl):
        """Changes the name of this declaration and its associated type"""

        # Change the name of the decl
        name = decl.name
        new_name = tuple(self.prefix + list(name))
        decl.name = new_name
        # Change the name of the associated type
        try:
            type = self.ir.types[name]
            del self.ir.types[name]
            type.name = new_name
            self.ir.types[new_name] = type
        except KeyError, msg:
            if self.verbose: print "Warning: Unable to map name of type:",msg

