#
# Copyright (C) 2007 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""a DocBook formatter (producing Docbook 4.5 XML output"""

from Synopsis.Processor import *
from Synopsis import ASG, DeclarationSorter
from Synopsis.Formatters import quote_name
from Syntax import *
from Markup.Javadoc import Javadoc
try:
    from Markup.RST import RST
except ImportError:
    from Markup import Formatter as RST

import os

def escape(text):

    for p in [('&', '&amp;'), ('"', '&quot;'), ('<', '&lt;'), ('>', '&gt;'),]:
        text = text.replace(*p)
    return text

def reference(name):
    """Generate an id suitable as a 'linkend' / 'id' attribute, i.e. for linking."""

    return quote_name(str(name))


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
    'IDL': CxxSummarySyntax,
    'C++': CxxSummarySyntax,
    'C': CxxSummarySyntax,
    'Python': PythonSummarySyntax
    }

_detail_syntax = {
    'IDL': CxxDetailSyntax,
    'C++': CxxDetailSyntax,
    'C': CxxDetailSyntax,
    'Python': PythonDetailSyntax
    }

class ModuleLister(ASG.Visitor):
    """Maps a module-tree to a (flat) list of modules."""

    def __init__(self):

        self.modules = []

    def visit_module(self, module):

        self.modules.append(module)
        for d in module.declarations:
            d.accept(self)


class InheritanceFormatter:

    def __init__(self, base_dir):

        self.base_dir = base_dir

    def format_class(self, class_, format):

        if not class_.parents:
            return ''
        
        from Synopsis.Formatters import Dot
        filename = os.path.join(self.base_dir, escape(str(class_.name)) + '.%s'%format)
        dot = Dot.Formatter()
        try:
            dot.process(IR.IR(declarations = [class_]),
                        output=filename,
                        format=format,
                        type='single')
            return filename
        except InvalidCommand, e:
            print 'Warning : %s'%str(e)
            return ''


class FormatterBase:

    def __init__(self, processor, output, base_dir,
                 nested_modules, secondary_index, inheritance_graphs):
        self.processor = processor
        self.output = output
        self.base_dir = base_dir
        self.nested_modules = nested_modules
        self.secondary_index = secondary_index
        self.inheritance_graphs = inheritance_graphs
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

    def write_element(self, element, body, end = '\n', **params):
        """Write a single element on one line (though body may contain
        newlines)"""

        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        self.write('<' + element + param_text + '>')
        self.__indent = self.__indent + 2
        self.write(body)
        self.__indent = self.__indent - 2
        self.write('</' + element + '>' + end)

    def element(self, element, body, **params):
        """Return but do not write the text for an element on one line"""

        param_text = ''
        if params: param_text = ' ' + ' '.join(['%s="%s"'%(p[0].lower(), p[1]) for p in params.items()])
        return '<%s%s>%s</%s>'%(element, param_text, body, element)


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
        if isinstance(declaration, ASG.Function):
            # The primary index term is the unqualified name,
            # the secondary the qualified name.
            # This will result in index terms being grouped by name, with each
            # qualified name being listed within that group.
            indexterm = self.element('primary', escape(declaration.real_name[-1]))
            if self.secondary_index:
                indexterm += self.element('secondary', escape(str(declaration.real_name)))
            self.write_element('indexterm', indexterm, type='functions')

        language = declaration.file.annotations['language']
        syntax = _detail_syntax[language](self.output)
        declaration.accept(syntax)
        syntax.finish()
        self.process_doc(declaration)
        self.end_element()

    def generate_module_list(self, modules):

        if modules:
            modules.sort(cmp=lambda a,b:cmp(a.name, b.name))
            self.start_element('section')
            self.write_element('title', 'Modules')
            self.start_element('itemizedlist')
            for m in modules:
                link = self.element('link', escape(str(m.name)), linkend=reference(m.name))
                self.write_element('listitem', self.element('para', link))
            self.end_element()
            self.end_element()
            

    def format_module_or_group(self, module, title, sort):
        self.start_element('section', id=reference(module.name))
        self.write_element('title', title)

        declarations = module.declarations
        if not self.nested_modules:
            modules = [d for d in declarations if isinstance(d, ASG.Module)]
            declarations = [d for d in declarations if not isinstance(d, ASG.Module)]
            self.generate_module_list(modules)

        sorter = DeclarationSorter.DeclarationSorter(declarations)
        if self.processor.generate_summary:
            self.start_element('section')
            self.write_element('title', 'Summary')
            summary = SummaryFormatter(self.processor, self.output)
            if sort:
                for s in sorter:
                    if s[-1] == 's': title = s + 'es Summary'
                    else: title = s + 's Summary'
                    self.start_element('section')
                    self.write_element('title', escape(title))
                    for d in sorter[s]:
                        if not self.processor.hide_undocumented or d.annotations.get('doc'):
                            d.accept(summary)
                    self.end_element()
            else:
                for d in declarations:
                    if not self.processor.hide_undocumented or d.annotations.get('doc'):
                        d.accept(summary)
            self.end_element()
            self.write('\n')
            self.start_element('section')
            self.write_element('title', 'Details')
        self.process_doc(module)
        self.push_scope(module.name)
        suffix = self.processor.generate_summary and ' Details' or ''
        if sort:
            for section in sorter:
                title = section + suffix
                self.start_element('section')
                self.write_element('title', escape(title))
                for d in sorter[section]:
                    d.accept(self)
                self.end_element()
        else:
            for d in declarations:
                d.accept(self)            
        self.pop_scope()
        self.end_element()
        self.write('\n')
        if self.processor.generate_summary:
            self.end_element()
            self.write('\n')

    def visit_module(self, module):
        if module.name:
            # Only print qualified names when modules are flattened.
            name = self.nested_modules and module.name[-1] or str(module.name)
            title = '%s %s'%(module.type.capitalize(), name)
        else:
            title = 'Global %s'%module.type.capitalize()
            
        self.format_module_or_group(module, title, True)
        
    def visit_group(self, group):
        self.format_module_or_group(group, group.name[-1].capitalize(), False)

    def visit_class(self, class_):

        if self.processor.hide_undocumented and not class_.annotations.get('doc'):
            return
        self.start_element('section')
        title = '%s %s'%(class_.type, class_.name[-1])
        self.write_element('title', escape(title))
        indexterm = self.element('primary', escape(class_.name[-1]))
        if self.secondary_index:
            indexterm += self.element('secondary', escape(str(class_.name)))
        self.write_element('indexterm', indexterm, type='types')
        
        if self.inheritance_graphs:
            formatter = InheritanceFormatter(os.path.join(self.base_dir, 'images'))
            png = formatter.format_class(class_, 'png')
            svg = formatter.format_class(class_, 'svg')
            if png or svg:
                self.start_element('mediaobject')
                if png:
                    imagedata = self.element('imagedata', '', fileref=png, format='PNG')
                    self.write_element('imageobject', imagedata)
                if svg:
                    imagedata = self.element('imagedata', '', fileref=svg, format='SVG')
                    self.write_element('imageobject', imagedata)
                self.end_element()

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
            summary = SummaryFormatter(self.processor, self.output)
            summary.process_doc(class_)
            for section in sorter:
                title = section + ' Summary'
                self.start_element('section')
                self.write_element('title', escape(title))
                for d in sorter[section]:
                    if not self.processor.hide_undocumented or d.annotations.get('doc'):
                        d.accept(summary)
                self.end_element()
            self.end_element()
            self.write('\n')
            self.start_element('section')
            self.write_element('title', 'Details')
        self.process_doc(class_)
        self.push_scope(class_.name)
        suffix = self.processor.generate_summary and ' Details' or ''
        for section in sorter:
            title = section + suffix
            self.start_element('section')
            self.write_element('title', escape(title))
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

    def visit_enum(self, enum):

        if self.processor.hide_undocumented and not declaration.annotations.get('doc'):
            return

        self.start_element('section')
        self.write_element('title', escape(enum.name[-1]))
        indexterm = self.element('primary', escape(enum.name[-1]))
        if self.secondary_index:
            indexterm += self.element('secondary', escape(str(enum.name)))
        self.write_element('indexterm', indexterm, type='types')

        language = enum.file.annotations['language']
        syntax = _detail_syntax[language](self.output)
        enum.accept(syntax)
        syntax.finish()
        self.process_doc(enum)
        self.end_element()

   
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
    nested_modules = Parameter(False, """Map the module tree to a tree of docbook sections.""")
    generate_summary = Parameter(False, 'generate scope summaries')
    hide_undocumented = Parameter(False, 'hide declarations without a doc-string')
    inline_inherited_members = Parameter(False, 'show inherited members')
    secondary_index_terms = Parameter(True, 'add fully-qualified names to index')
    with_inheritance_graphs = Parameter(True, 'whether inheritance graphs should be generated')
   
    def process(self, ir, **kwds):

        self.set_parameters(kwds)
        if not self.output: raise MissingArgument('output')
        self.ir = self.merge_input(ir)

        self.documentation = DocCache(self, self.markup_formatters)

        output = open(self.output, 'w')
        output.write('<section>\n')
        if self.title:
            output.write('<title>%s</title>\n'%self.title)
        detail_formatter = DetailFormatter(self, output,
                                           os.path.dirname(self.output),
                                           self.nested_modules,
                                           self.secondary_index_terms,
                                           self.with_inheritance_graphs)

        declarations = self.ir.declarations

        if not self.nested_modules:

            modules = [d for d in declarations if isinstance(d, ASG.Module)]
            detail_formatter.generate_module_list(modules)

            module_lister = ModuleLister()
            for d in self.ir.declarations:
                d.accept(module_lister)
            modules = module_lister.modules
            modules.sort(cmp=lambda a,b:cmp(a.name, b.name))
            declarations = [d for d in self.ir.declarations
                            if not isinstance(d, ASG.Module)]
            declarations.sort(cmp=lambda a,b:cmp(a.name, b.name))
            declarations = modules + declarations

        for d in declarations:
            d.accept(detail_formatter)
            
        output.write('</section>\n')
        output.close()
      
        return self.ir
