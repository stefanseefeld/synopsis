#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a DocBook formatter (producing Docbook 4.5 XML output"""

from Synopsis.Processor import *
from Synopsis import ASG, DeclarationSorter
from Syntax import *
from Markup.Javadoc import Javadoc
try:
    from Markup.RST import RST
except ImportError:
    from Markup import Formatter as RST

import sys, getopt, os, os.path, re

def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),]:
        text = text.replace(*p)
    return text


class _BaseClasses(ASG.Visitor):

    def __init__(self):
        self.classes = [] # accumulated set of classes
        self.classes_once = [] # classes not to be included again

    def visit_declared_type(self, declared):
        declared.declaration.accept(self)

    def visit_class(self, class_):
        self.classes.append(class_)
        for p in class_.parents:
            p.accept(self)

    def visit_inheritance(self, inheritance):

        if 'private' in inheritance.attributes:
            return # Ignore private base classes, they are not visible anyway.
        elif inheritance.parent in self.classes_once:
            return # Do not include virtual base classes more than once.
        if 'virtual' in inheritance.attributes:
            self.classes_once.append(inheritance.parent)
        inheritance.parent.accept(self)


_summary_syntax = {
    'IDL': Syntax,
    'C++': CxxSummarySyntax,
    'C': CxxSummarySyntax,
    'Python': Syntax
    }

_detail_syntax = {
    'IDL': Syntax,
    'C++': CxxDetailSyntax,
    'C': CxxDetailSyntax,
    'Python': Syntax
    }

class FormatterBase:

    def __init__(self, processor, output):
        self.processor = processor
        self.output = output
        self.__scope = ()
        self.__scopestack = []
        self.__indent = 0
        self.__elements = []

    def scope(self): return self.__scope
    def push_scope(self, newscope):

        self.__scopestack.append(self.__scope)
        self.__scope = newscope

    def pop_scope(self):

        self.__scope = self.__scopestack[-1]
        del self.__scopestack[-1]

    def write(self, text):
        """Write some text to the output stream, replacing \n's with \n's and
        indents."""

        indent = ' ' * self.__indent
        self.output.write(text.replace('\n', '\n'+indent))

    def start_element(self, type, **params):
        """Write the start of an element, ending with a newline"""

        self.__elements.append(type)
        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        self.write('<' + type + param_text + '>')
        self.__indent = self.__indent + 2
        self.write('\n')

    def end_element(self):
        """Write the end of an element, starting with a newline"""

        type = self.__elements.pop()
        self.__indent = self.__indent - 2
        self.write('\n</' + type + '>')

    def write_element(self, element, body, **params):
        """Write a single element on one line (though body may contain
        newlines)"""

        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        self.write('<' + element + param_text + '>')
        self.__indent = self.__indent + 2
        self.write(body)
        self.__indent = self.__indent - 2
        self.write('</' + element + '>')

    def element(self, element, body, **params):
        """Return but do not write the text for an element on one line"""

        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        return '<%s%s>%s</%s>'%(element, param_text, body, element)

    def reference(self, ref, label):
        """reference takes two strings, a reference (used to look up the symbol and generated the reference),
        and the label (used to actually write it)"""

        location = self.__toc.lookup(ref)
        if location != '': return href('#' + location, label)
        else: return span('type', str(label))


    def label(self, ref):

        location = self.__toc.lookup(ref)
        ref = str(self.scope().prune(ref))
        if location != '': return name('"' + location + '"', ref)
        else: return ref



class SummaryFormatter(FormatterBase, ASG.Visitor):
    """A SummaryFormatter."""

    def process_doc(self, decl):

        if decl.annotations.get('doc', ''):
            summary = self.processor.documentation.summary(decl)
            if summary:
                # FIXME: Unfortunately, as of xsl-docbook 1.73, a section role
                #        doesn't translate into a div class attribute, so there
                #        is no way to style summaries and descriptions per se.
                #self.write('<section role="summary">\n%s<section>\n'%summary)
                self.write('%s\n'%summary)

    #################### ASG Visitor ###########################################

    def visit_declaration(self, declaration):

        language = declaration.file.annotations['language']
        syntax = _summary_syntax[language](self.output)
        declaration.accept(syntax)
        syntax.finish()
        self.process_doc(declaration)
        

    visit_module = visit_declaration
    visit_class = visit_declaration
    def visit_meta_module(self, meta):
        self.visit_module(meta.module_declarations[0])

    visit_function = visit_declaration

    def visit_enumerator(self, enumerator):
        print "sorry, <enumerator> not implemented"

    def visit_enum(self, enum):
        print "sorry, <enum> not implemented"


class DetailFormatter(FormatterBase, ASG.Visitor):


    #################### Type Visitor ##########################################

    def visit_base_type(self, type):

        self.__type_ref = str(type.name)
        self.__type_label = str(type.name)

    def visit_unknown_type(self, type):

        self.__type_ref = str(type.name)
        self.__type_label = str(self.scope().prune(type.name))
        
    def visit_declared_type(self, type):

        self.__type_label = str(self.scope().prune(type.name))
        self.__type_ref = str(type.name)

    def visit_modifier_type(self, type):

        type.alias.accept(self)
        self.__type_ref = ''.join(type.premod) + ' ' + self.__type_ref + ' ' + ''.join(type.postmod)
        self.__type_label = escape(''.join(type.premod) + ' ' + self.__type_label + ' ' + ''.join(type.postmod))

    def visit_parametrized(self, type):

        type.template.accept(self)
        type_label = self.__type_label + '&lt;'
        parameters_label = []
        for p in type.parameters:
            p.accept(self)
            parameters_label.append(self.__type_label)
        self.__type_label = type_label + ', '.join(parameters_label) + '&gt;'

    def visit_function_type(self, type):

        # TODO: this needs to be implemented
        self.__type_ref = 'function_type'
        self.__type_label = 'function_type'

    def process_doc(self, decl):

        if decl.annotations.get('doc', ''):
            detail = self.processor.documentation.details(decl)
            if detail:
                # FIXME: Unfortunately, as of xsl-docbook 1.73, a section role
                #        doesn't translate into a div class attribute, so there
                #        is no way to style summaries and descriptions per se.
                #self.write('<section role="description">\n%s<section>\n'%detail)
                self.write('%s\n'%detail)

    #################### ASG Visitor ###########################################

    def visit_declaration(self, declaration):

        if self.processor.hide_undocumented and not declaration.annotations.get('doc'):
            return
        self.start_element('section')
        self.write_element('title', escape(declaration.name[-1]))
        if str(declaration.type) in ('function', 'function template', 'member function', 'member function template'):
            # The primary index term is the unqualified name, the secondary the qualified name.
            # This will result in index terms being grouped by name, with each
            # qualified name being listed within that group.
            indexterm = self.element('primary', escape(declaration.real_name[-1]))
            indexterm += self.element('secondary', escape(str(declaration.real_name)))
            self.write_element('indexterm', indexterm, type='functions')

        language = declaration.file.annotations['language']
        syntax = _detail_syntax[language](self.output)
        declaration.accept(syntax)
        syntax.finish()
        self.process_doc(declaration)
        self.end_element()

    def format_module_or_group(self, module, title):
        self.start_element('section')
        self.write_element('title', title)
        self.write('\n')

        sorter = DeclarationSorter.DeclarationSorter(module.declarations)
        if self.processor.generate_summary:
            self.start_element('section')
            self.write_element('title', 'Summary')
            self.write('\n')
            summary = SummaryFormatter(self.processor, self.output)
            for s in sorter:
                if s[-1] == 's': title = s + 'es Summary'
                else: title = s + 's Summary'
                self.start_element('section')
                self.write_element('title', escape(title))
                self.write('\n')
                for d in sorter[s]:
                    if not self.processor.hide_undocumented or d.annotations.get('doc'):
                        d.accept(summary)
                self.end_element()
            self.end_element()
            self.write('\n')
            self.start_element('section')
            self.write_element('title', 'Details')
            self.write('\n')
        self.process_doc(module)
        self.push_scope(module.name)
        suffix = self.processor.generate_summary and ' Details' or ''
        for section in sorter:
            title = section + suffix
            self.start_element('section')
            self.write_element('title', escape(title))
            self.write('\n')
            for d in sorter[section]:
                d.accept(self)
            self.end_element()
        self.pop_scope()
        self.end_element()
        self.write('\n')
        if self.processor.generate_summary:
            self.end_element()
            self.write('\n')

    def visit_module(self, module):
        if module.name:
            title = '%s %s'%(module.type, module.name[-1])
        else:
            title = 'Global %s'%module.type.capitalize()
            
        self.format_module_or_group(module, title)
        
    def visit_group(self, group):
        self.format_module_or_group(group, group.name[-1].capitalize())

    def visit_class(self, class_):

        if self.processor.hide_undocumented and not class_.annotations.get('doc'):
            return
        self.start_element('section')
        title = '%s %s'%(class_.type, class_.name[-1])
        self.write_element('title', escape(title))
        self.write('\n')
        indexterm = self.element('primary', escape(class_.name[-1]))
        indexterm += self.element('secondary', escape(str(class_.name)))
        self.write_element('indexterm', indexterm, type='types')
        self.write('\n')

        declarations = class_.declarations
        # If so desired, flatten inheritance tree
        if self.processor.inline_inherited_members:
            declarations = class_.declarations[:]
            bases = _BaseClasses()
            for i in class_.parents:
                bases.visit_inheritance(i)
            for base in bases.classes:
                for d in base.declarations:
                    if type(d) == ASG.Operation:
                        if d.real_name[-1] == base.name[-1]:
                            # Constructor
                            continue
                        elif d.real_name[-1] == '~' + base.name[-1]:
                            # Destructor
                            continue
                        elif d.real_name[-1] == 'operator=':
                            # Assignment
                            continue
                    declarations.append(d)

        sorter = DeclarationSorter.DeclarationSorter(declarations)

        if self.processor.generate_summary:
            self.start_element('section')
            self.write_element('title', 'Summary')
            self.write('\n')
            summary = SummaryFormatter(self.processor, self.output)
            summary.process_doc(class_)
            for section in sorter:
                title = section + ' Summary'
                self.start_element('section')
                self.write_element('title', escape(title))
                self.write('\n')
                for d in sorter[section]:
                    if not self.processor.hide_undocumented or d.annotations.get('doc'):
                        d.accept(summary)
                self.end_element()
            self.end_element()
            self.write('\n')
            self.start_element('section')
            self.write_element('title', 'Details')
            self.write('\n')
        self.process_doc(class_)
        self.push_scope(class_.name)
        suffix = self.processor.generate_summary and ' Details' or ''
        for section in sorter:
            title = section + suffix
            self.start_element('section')
            self.write_element('title', escape(title))
            self.write('\n')
            for d in sorter[section]:
                d.accept(self)
            self.end_element()
        self.pop_scope()
        self.end_element()
        self.write('\n')
        if self.processor.generate_summary:
            self.end_element()
            self.write('\n')


    def visit_inheritance(self, inheritance):

        for a in inheritance.attributes: self.element("modifier", a)
        self.element("classname", str(self.scope().prune((inheritance.parent.name))))

    def visit_parameter(self, parameter):

        parameter.type.accept(self)

    visit_function = visit_declaration

    def visit_enumerator(self, enumerator):
        print "sorry, <enumerator> not implemented"

    def visit_enum(self, enum):
        print "sorry, <enum> not implemented"

   
class DocCache:
    """"""

    def __init__(self, processor, markup_formatters):

        self._processor = processor
        self._markup_formatters = markup_formatters
        # Make sure we have a default markup formatter.
        if '' not in self._markup_formatters:
            self._markup_formatters[''] = Markup.Formatter()
        for f in self._markup_formatters.values():
            f.init(self._processor)
        self._doc_cache = {}


    def _process(self, decl):
        """Return the documentation for the given declaration."""

        key = id(decl)
        if key not in self._doc_cache:
            doc = decl.annotations.get('doc')
            if doc:
                formatter = self._markup_formatters.get(doc.markup,
                                                        self._markup_formatters[''])
                doc = formatter.format(decl)
            else:
                doc = Markup.Struct()
            self._doc_cache[key] = doc
            return doc
        else:
            return self._doc_cache[key]


    def summary(self, decl):
        """"""

        doc = self._process(decl)
        return doc.summary


    def details(self, decl):
        """"""

        doc = self._process(decl)
        return doc.details



class Formatter(Processor):
    """Generate a DocBook reference."""

    markup_formatters = Parameter({'rst':RST(), 'javadoc':Javadoc()},
                                  'Markup-specific formatters.')
    title = Parameter(None, 'title to be used in top-level section')
    generate_summary = Parameter(False, 'generate scope summaries')
    hide_undocumented = Parameter(False, 'hide declarations without a doc-string')
    inline_inherited_members = Parameter(False, 'show inherited members')
   
    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        if not self.output: raise MissingArgument('output')
        self.ir = self.merge_input(ir)

        self.documentation = DocCache(self, self.markup_formatters)

        output = open(self.output, 'w')
        output.write('<section>\n')
        if self.title:
            output.write('<title>%s</title>\n'%self.title)
        detail_formatter = DetailFormatter(self, output)
        for d in self.ir.declarations:
            d.accept(detail_formatter)

        output.write('</section>\n')
        output.close()
      
        return self.ir
