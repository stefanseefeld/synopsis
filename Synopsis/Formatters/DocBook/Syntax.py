#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import ASG, Type

def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),]:
        text = text.replace(*p)
    return text

class Syntax(ASG.Visitor, Type.Visitor):
    """Even though DocBook provides markup for some programming artifacts,
    it is incomplete, and the XSL stylesheets are buggy, resulting in
    incorrect syntax. Thus, we use the 'synopsis' element, and attempt to
    reproduce the original syntax with language-specific subclasses."""

    def __init__(self, output):

        self.output = output

    def finish(self): pass


class CxxSyntax(Syntax):

    def visit_function(self, node):
        text = '%s(%s)\n'%(escape(node.real_name[-1]),
                           ', '.join([self.visit_parameter(p)
                                      for p in node.parameters]))
        self.output.write(text)

    def visit_parameter(self, parameter):
        text = escape(str(parameter.type))
        if parameter.name:
            text += ' %s'%parameter.name
        if parameter.value:
            text += ' = %s'%parameter.value
        return text

    def visit_variable(self, variable):
        variable.vtype.accept(self)
        self.output.write('%s %s\n'%(self._typename, variable.name[-1]))

    def visit_const(self, const):
        const.ctype.accept(self)
        self.output.write('%s %s\n'%(self._typename, const.name[-1]))

    def visit_base_type(self, type):
        self._typename = type.name[-1]
    def visit_unknown(self, type): self._typename = '&lt;unknown&gt;'
    def visit_declared(self, type): self._typename = type.name[-1]
    def visit_modifier(self, type):
        type.alias.accept(self)
        self._typename = '%s %s %s'%(escape(' '.join(type.premod)),
                                   self._typename,
                                   escape(' '.join(type.postmod)))
    def visit_array(self, type): self._typename = 'array'
    def visit_template(self, type): self._typename = 'A'
    def visit_parametrized(self, type): self._typename = escape(str(type))
    def visit_function_type(self, type): self._typename = 'C'
    def visit_dependent(self, type): self._typename = 'D'


class CxxSummarySyntax(CxxSyntax):
    """Generate DocBook Synopsis for C++ declarations."""
    
    def __init__(self, output):
        super(CxxSummarySyntax, self).__init__(output)
        self.output.write('<synopsis>')

    def finish(self):
        self.output.write('</synopsis>\n')

    def visit_macro(self, macro):
        self.output.write(macro.name[-1])
        if macro.parameters:
            self.output.write('(%)'%(', '.join([p for p in macro.parameters])))
        self.output.write('\n')

    def visit_forward(self, node): pass
    def visit_group(self, node):
        for d in node.declarations:
            d.accept(self)
    def visit_module(self, module):
        self.output.write('%s %s\n'%(module.type, module.name[-1]))
        
    def visit_class(self, class_):
        self.output.write('%s %s\n'%(class_.type, escape(class_.name[-1])))
        
    def visit_class_template(self, class_): self.visit_class(class_)
    def visit_typedef(self, node): pass
    def visit_enumerator(self, node): pass
    def visit_enum(self, node):
        self.visit_declaration(node)
        for e in node.enumerators:
            e.accept(self)

    def visit_inheritance(self, node): pass


class CxxDetailSyntax(CxxSyntax):
    """Generate DocBook Synopsis for C++ declarations."""
    
    def __init__(self, output):
        super(CxxDetailSyntax, self).__init__(output)
        self.output.write('<synopsis>')

    def finish(self):
        self.output.write('</synopsis>\n')

    def visit_macro(self, macro):
        self.output.write(macro.name[-1])
        if macro.parameters:
            self.output.write('(%s)'%(', '.join([p for p in macro.parameters])))
        self.output.write('\n')

    def visit_forward(self, node): pass
    def visit_group(self, node):
        for d in node.declarations:
            d.accept(self)
    def visit_module(self, module):
        self.output.write('%s %s\n'%(module.type, module.name[-1]))
        
    def visit_class(self, class_):
        self.output.write('%s %s\n'%(class_.type, escape(class_.name[-1])))
        
    def visit_class_template(self, node): pass
    def visit_typedef(self, node): pass
    def visit_enumerator(self, node): pass
    def visit_enum(self, node):
        self.visit_declaration(node)
        for e in node.enumerators:
            e.accept(self)

    def visit_inheritance(self, node): pass

