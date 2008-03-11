#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Markup import *
from docutils.nodes import *
from docutils.core import *
from docutils.readers import standalone
from docutils.transforms import Transform
import re, StringIO


class SummaryExtractor(NodeVisitor):
    """A SummaryExtractor creates a document containing the first sentence of
    a source document."""

    def __init__(self, document):

        NodeVisitor.__init__(self, document)
        self.summary = None
        

    def visit_paragraph(self, node):
        """Copy the paragraph but only keep the first sentence."""

        if self.summary is not None:
            return

        summary_pieces = []

        # Extract the first sentence.
        for child in node:
            if isinstance(child, Text):
                m = re.match(r'(\s*[\w\W]*?\.)(\s|$)', child.data)
                if m:
                    summary_pieces.append(Text(m.group(1)))
                    break
                else:
                    summary_pieces.append(Text(child))
            else:
                summary_pieces.append(child)
            
        self.summary = self.document.copy()
        para = node.copy()
        para[:] = summary_pieces
        self.summary[:] = [para]


    def unknown_visit(self, node):
        'Ignore all unknown nodes'

        pass


class RST(Formatter):
    """Format summary and detail documentation according to restructured text markup.
    """

    def format(self, decl, view):

        formatter = self
        
        class Linker(Transform):
            """Resolve references that don't have explicit targets
            in the same doctree."""

            default_priority = 1
    
            def apply(self):
                for ref in self.document.traverse(reference):
                    if ref.resolved or not ref.hasattr("refname"):
                        return
                    name = ref['name']
                    uri = formatter.lookup_symbol(name, decl.name[:-1])
                    if uri:
                        ref.resolved = 1
                        ref['refuri'] = rel(view.filename(), uri)
                    else:
                        ref.replace_self(Text(name))

        class Reader(standalone.Reader):

            def get_transforms(self):
                return [Linker] + standalone.Reader.get_transforms(self)


        errstream = StringIO.StringIO()
        settings = {}
        settings['halt_level'] = 2
        settings['warning_stream'] = errstream
        settings['traceback'] = True

        doc = decl.annotations.get('doc')
        if doc:
            try:
                doctree = publish_doctree(doc.text, reader=Reader(),
                                          settings_overrides=settings)
                # Extract the summary.
                extractor = SummaryExtractor(doctree)
                doctree.walk(extractor)

                reader = docutils.readers.doctree.Reader(parser_name='null')

                # Publish the summary.
                if extractor.summary:
                    pub = Publisher(reader, None, None,
                                    source=io.DocTreeInput(extractor.summary),
                                    destination_class=io.StringOutput)
                    pub.set_writer('html')
                    pub.process_programmatic_settings(None, None, None)
                    dummy = pub.publish(enable_exit_status=None)
                    summary = pub.writer.parts['html_body']
                    # Hack to strip off some redundant blocks to make the output
                    # more compact.
                    if (summary.startswith('<div class="document">\n') and
                        summary.endswith('</div>\n')):
                        summary=summary[23:-7]
                else:
                    summary = ''
                
                # Publish the details.
                pub = Publisher(reader, None, None,
                                source=io.DocTreeInput(doctree),
                                destination_class=io.StringOutput)
                pub.set_writer('html')
                pub.process_programmatic_settings(None, None, None)
                dummy = pub.publish(enable_exit_status=None)
                details = pub.writer.parts['html_body']
                # Hack to strip off some redundant blocks to make the output
                # more compact.
                if (details.startswith('<div class="document">\n') and
                    details.endswith('</div>\n')):
                    details=details[23:-7]
                
                return Struct(summary, details)
            
            except docutils.utils.SystemMessage, error:
                xx, line, message = str(error).split(':', 2)
                print 'In DocString attached to declaration at %s:%d:'%(decl.file.name,
                                                                        decl.line)
                print '  line %s:%s'%(line, message)

        return Struct('', '')
