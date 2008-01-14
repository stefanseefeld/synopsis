#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import config
from Synopsis.Processor import Processor, Parameter
from Synopsis import IR, ASG
from Synopsis.DocString import DocString
from Synopsis.FileTree import FileTree as SourceFileTree
from Synopsis.Formatters.TOC import TOC
from Synopsis.Formatters.ClassTree import ClassTree
from Synopsis.Formatters.XRef import CrossReferencer
from DirectoryLayout import *
from Views import *
from Frame import Frame
from FrameSet import FrameSet
from Synopsis.Formatters.HTML.Markup.Javadoc import Javadoc
try:
    from Synopsis.Formatters.HTML.Markup.RST import RST
except ImportError:
    from Synopsis.Formatters.HTML.Markup import Formatter as RST
import Markup
import Tags

import time

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


    def _process(self, decl, view):
        """Return the documentation for the given declaration."""

        key = id(decl)
        if key not in self._doc_cache:
            doc = decl.annotations.get('doc')
            if doc:
                formatter = self._markup_formatters.get(doc.markup,
                                                        self._markup_formatters[''])
                doc = formatter.format(decl, view)
            else:
                doc = Markup.Struct()
            # FIXME: Unfortunately we can't easily cache these, as they may
            #        contain relative URLs that aren't valid across views.
            # self._doc_cache[key] = doc
            return doc
        else:
            return self._doc_cache[key]


    def summary(self, decl, view):
        """"""

        doc = self._process(decl, view)
        return doc.summary


    def details(self, decl, view):
        """"""

        doc = self._process(decl, view)
        return doc.details


class Formatter(Processor):

    title = Parameter('Synopsis - Generated Documentation', 'title to put into html header')
    stylesheet = Parameter(os.path.join(config.datadir, 'html.css'), 'stylesheet to be used')
    directory_layout = Parameter(NestedDirectoryLayout(), 'how to lay out the output files')
    toc_in = Parameter([], 'list of table of content files to use for symbol lookup')
    toc_out = Parameter('', 'name of file into which to store the TOC')

    index = Parameter([ModuleTree(), FileTree()], '')
    detail = Parameter([ModuleIndex(), FileIndex()], '')
    content = Parameter([Scope(),
                         FileDetails(),
                         InheritanceTree(),
                         InheritanceGraph(),
                         NameIndex()],
                        '')
   
    markup_formatters = Parameter({'javadoc':Javadoc(), 'rst':RST()},
                                  'Markup-specific formatters.')

    def process(self, ir, **kwds):

        self.set_parameters(kwds)

        # FIXME: wrap the declarations into a single ASG.Module
        #        during processing, but put the original ir back at the end.
        ir = self.merge_input(ir)
        root = ASG.Module(None, -1, 'Global', ())
        root.declarations = ir.declarations
        self.ir = IR.IR(ir.files, [root], ir.types)

        self.directory_layout.init(self)
        self.documentation = DocCache(self, self.markup_formatters)

        # Create the class tree (shared by inheritance graph / tree views).
        self.class_tree = ClassTree()
        for d in root.declarations:
            d.accept(self.class_tree)

        # Create the file tree (shared by file listing / tree views).
        self.file_tree = SourceFileTree()
        self.file_tree.set_ir(self.ir)

        # Create the cross reference table (shared by XRef / Scope views)
        self.xref = CrossReferencer()

        from Synopsis.DeclarationSorter import DeclarationSorter
        self.sorter = DeclarationSorter()

        # Make all views queryable through Formatter.has_view()
        self.views = self.content + self.index + self.detail

        frames = []
        # If only content contains views don't use frames.
        if self.index or self.detail:
            Tags.using_frames = True
            
            frames.append(Frame(self, self.index))
            frames.append(Frame(self, self.detail))
            frames.append(Frame(self, self.content))
        else:
            Tags.using_frames = False

            frames.append(Frame(self, self.content, noframes = True))

        self.__files = {} # map from filename to (view,scope)

        # The table of content is by definition the TOC of the first
        # view on the content frame.
        self.toc = self.content[0].toc()
        if self.verbose: print "TOC size:", self.toc.size()
        if self.toc_out: self.toc.store(self.toc_out)
        # load external references from toc files, if any
        for t in self.toc_in: self.toc.load(t)
   
        if self.verbose: print "HTML Formatter: Generating views..."

        # Process the views.
        if len(frames) > 1:
            frameset = FrameSet()
            frameset.process(self.output, self.directory_layout.index(),
                             self.title,
                             self.index[0].root()[0] or self.index[0].filename(),
                             self.detail[0].root()[0] or self.detail[0].filename(),
                             self.content[0].root()[0] or self.content[0].filename())

        for frame in frames: frame.process()
        # Return original IR, not the synthetic ASG.Module created above.
        return ir

    def has_view(self, name):
        """test whether the given view is part of the views list."""

        return name in [x.__class__.__name__ for x in self.views]

    def register_filename(self, filename, view, scope):
        """Registers a file for later production. The first view to register
        the filename gets to keep it."""

        filename = str(filename)
        if not self.__files.has_key(filename):
            self.__files[filename] = (view, scope)

    def filename_info(self, filename):
        """Returns information about a registered file, as a (view,scope)
        pair. Will return None if the filename isn't registered."""
        
        return self.__files.get(filename)

