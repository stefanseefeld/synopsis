#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import ASG

class ScopeStripper(Processor, ASG.Visitor):
    """Strip common prefix from the declaration's name.
    Keep a list of root nodes, such that children whos parent
    scopes are not accepted but which themselfs are correct can
    be maintained as new root nodes."""

    scope = Parameter('', 'strip all but the given scope')

    def __init__(self, **kwds):

        Processor.__init__(self, **kwds)
        self.__root = []
        self.__in = 0


    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        self.ir = self.merge_input(ir)
        if self.scope:

            self.__scope = tuple(self.scope.split('::'))
            # strip prefixes and remove non-matching declarations
            self.strip_declarations(self.ir.declarations)

            # Remove types not in strip
            self.strip_types(self.ir.types)

        return self.output_and_return_ir()

      
    def strip_name(self, name):

        depth = len(self.__scope)
        if name[0:depth] == self.__scope:
            if len(name) == depth: return None
            return name[depth:]
        return None

    
    def strip_declarations(self, declarations):

        for decl in declarations:
            decl.accept(self)
        declarations[:] = self.declarations()

	
    def strip_types(self, types):
        # Remove the empty type (caused by C++ with extract_tails)
        if types.has_key(()): del types[()]
        for name, type in types.items():
            try:
                del types[name]
                name = self.strip_name(name)
                if name:
                    type.name = name
                    types[name] = type
            except:
                print "ERROR Processing:", name, types[name]
                raise


    def declarations(self): return self.__root

   
    def strip(self, declaration):
        """test whether the declaration matches one of the prefixes, strip
        it off, and return success. Success means that the declaration matches
        the prefix set and thus should not be removed from the ASG."""

        passed = 0
        if not self.__scope: return 1
        depth = len(self.__scope)
        name = declaration.name
        if name[0:depth] == self.__scope and len(name) > depth:
            if self.verbose: print "symbol", '::'.join(name),
            declaration.name = name[depth:]
            if self.verbose: print "stripped to", '::'.join(declaration.name)
            passed = 1
        if self.verbose and not passed:
            print "symbol", '::'.join(declaration.name), "removed"
        return passed


    def visit_scope(self, scope):

        root = self.strip(scope) and not self.__in
        if root:
            self.__in = 1
            self.__root.append(scope)
        for declaration in scope.declarations:
            declaration.accept(self)
        if root: self.__in = 0


    def visit_class(self, class_):

        self.visit_scope(class_)


    def visit_class_template(self, class_):

        self.visit_class(class_)
        templ = class_.template
        if templ:
            name = self.strip_name(templ.name)
            if name: templ.name = name


    def visit_declaration(self, decl):

        if self.strip(decl) and not self.__in:
            self.__root.append(decl)


    def visit_enumerator(self, enumerator):

        self.strip(enumerator)


    def visit_enum(self, enum):

        self.visit_declaration(enum)
        for e in enum.enumerators:
            e.accept(self)


    def visit_function(self, function):

        self.visit_declaration(function)
        for parameter in function.parameters:
            parameter.accept(self)


    def visit_function_template(self, function):

        self.visit_function(function)
        if function.template:
            name = self.strip_name(function.template.name)
            if name: function.template.name = name


    def visit_operation(self, operation):

        self.visit_function(operation)


    def visit_operation_template(self, operation):

        self.visit_function_template(operation)


    def visit_meta_module(self, module):

        self.visit_scope(module)
        for d in module.module_declarations:
            name = self.strip_name(d.name)
            if name: d.name = name
