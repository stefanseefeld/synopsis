#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import ASG

def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),]:
        text = text.replace(*p)
    return text

class Syntax(ASG.Visitor):
    """Even though DocBook provides markup for some programming artifacts,
    it is incomplete, and the XSL stylesheets are buggy, resulting in
    incorrect syntax. Thus, we use the 'synopsis' element, and attempt to
    reproduce the original syntax with language-specific subclasses."""

    def __init__(self, output):

        self.output = output

    def finish(self): pass

    def typeid(self, type):
        
        self._typeid = ''
        type.accept(self)
        return self._typeid


class PythonSyntax(Syntax):

    def visit_function(self, node):
        text = escape(str(node.return_type))
        text += '%s(%s)\n'%(escape(node.real_name[-1]),
                            ', '.join([self.visit_parameter(p)
                                       for p in node.parameters]))
        self.output.write(text)

    def visit_parameter(self, parameter):
        text = str(parameter.name)
        if parameter.value:
            text += ' = %s'%escape(parameter.value)
        return text

    def visit_variable(self, variable):
        variable.vtype.accept(self)
        self.output.write(variable.name[-1])

    def visit_const(self, const):
        const.ctype.accept(self)
        self.output.write(const.name[-1])


class PythonSummarySyntax(PythonSyntax):
    """Generate DocBook Synopsis for Python declarations."""
    
    def __init__(self, output):
        super(PythonSummarySyntax, self).__init__(output)
        self.output.write('<synopsis>')

    def finish(self):
        self.output.write('</synopsis>\n')

    def visit_group(self, node):
        for d in node.declarations:
            d.accept(self)
    def visit_module(self, module):
        self.output.write('%s %s\n'%(module.type, module.name[-1]))
        
    def visit_class(self, class_):
        self.output.write('class %s\n'%escape(class_.name[-1]))
        
    def visit_inheritance(self, node): pass


class PythonDetailSyntax(PythonSyntax):
    """Generate DocBook Synopsis for Python declarations."""
    
    def __init__(self, output):
        super(PythonDetailSyntax, self).__init__(output)
        self.output.write('<synopsis>')

    def finish(self):
        self.output.write('</synopsis>\n')

    def visit_group(self, node):
        for d in node.declarations:
            d.accept(self)
    def visit_module(self, module):
        self.output.write('%s %s\n'%(module.type, module.name[-1]))
        
    def visit_class(self, class_):
        self.output.write('class %s\n'%escape(class_.name[-1]))
        
    def visit_inheritance(self, node): pass


class CxxSyntax(Syntax):

    def visit_function(self, node):

        text = node.return_type and self.typeid(node.return_type) or ''
        text += ' %s(%s);\n'%(escape(node.real_name[-1]),
                              ', '.join([self.visit_parameter(p)
                                         for p in node.parameters]))
        self.output.write(text)

    def visit_parameter(self, parameter):

        text = escape(str(parameter.type))
        if parameter.name:
            text += ' %s'%parameter.name
        if parameter.value:
            text += ' = %s'%escape(parameter.value)
        return text

    def visit_typedef(self, node):

        self.output.write('typedef %s %s;\n'%(self.typeid(node.alias), node.name[-1]))

    def visit_variable(self, variable):
        self.output.write('%s %s;\n'%(self.typeid(variable.vtype), variable.name[-1]))

    def visit_const(self, const):
        self.output.write('%s %s;\n'%(self.typeid(const.ctype), const.name[-1]))

    def visit_base_type(self, type): self._typeid = type.name[-1]
    def visit_unknown_type(self, type): self._typeid = escape(str(type.name))
    def visit_declared_type(self, type): self._typeid = escape(str(type.name))
    def visit_modifier_type(self, type):

        self._typeid = '%s %s %s'%(escape(' '.join(type.premod)),
                                   self.typeid(type.alias),
                                   escape(' '.join(type.postmod)))
    def visit_array(self, type):

        self._typeid = '%s%s'%(escape(str(type.name)), ''.join(['[%s]'%s for s in type.sizes]))

    def visit_template(self, type): self._typeid = escape(str(type.name))

    def visit_parametrized(self, type):

        self._typeid = '%s&lt;%s&gt;'%(self.typeid(type.template),
                                       ', '.join([self.typeid(p) for p in type.parameters]))

    def visit_function_type(self, type):

        self._typeid = '%s(%s)'%(self.typeid(type.return_type),
                                 ', '.join([self.typeid(p) for p in type.parameters]))
                                   
    def visit_dependent(self, type): self._typeid = type.name[-1]


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
        self.output.write('%s %s;\n'%(module.type, module.name[-1]))
        
    def visit_class(self, class_):
        self.output.write('%s %s;\n'%(class_.type, escape(class_.name[-1])))
        
    def visit_class_template(self, class_): self.visit_class(class_)

    def visit_enumerator(self, node):
        if node.value:
            return '%s=%s'%(node.name[-1], escape(node.value))
        else:
            return node.name[-1]
        
    def visit_enum(self, node):
        self.output.write('%s %s { '%(node.type, node.name[-1])) 
        self.output.write(', '.join([self.visit_enumerator(e)
                                     for e in node.enumerators
                                     if isinstance(e, ASG.Enumerator)]))
        self.output.write('};\n')

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
        self.output.write('%s %s;\n'%(module.type, module.name[-1]))
        
    def visit_class(self, class_):
        self.output.write('%s %s;\n'%(class_.type, escape(class_.name[-1])))
        
    def visit_class_template(self, node): pass

    def visit_enumerator(self, node):
        if node.value:
            return '%s=%s'%(node.name[-1], escape(node.value))
        else:
            return node.name[-1]
        
    def visit_enum(self, node):
        self.output.write('%s %s { '%(node.type, node.name[-1])) 
        self.output.write(', '.join([self.visit_enumerator(e)
                                     for e in node.enumerators
                                     if isinstance(e, ASG.Enumerator)]))
        self.output.write('};\n')

    def visit_inheritance(self, node): pass

