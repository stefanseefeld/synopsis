# $Id: Formatter.py,v 1.3 2003/11/12 16:42:05 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST
from FramesIndex import *
from DirBrowse import *
from ScopePages import *
from ModuleListing import *
from ModuleIndexer import *
from FileListing import *
from FileIndexer import *
from FileDetails import *
from InheritanceTree import *
from InheritanceGraph import *
from FileSource import *
from NameIndex import *
from XRefPages import *
from FileLayout import *
from TreeFormatter import *
from CommentFormatter import *

from core import *

class ConfigHTML:
   """This is a verbatim copy of the HTML class in Config.py.
   It should serve temporarily here so we can incrementally
   refactor the HTML formatter. The goal is to get rid of this
   monster..."""
   
   name = 'HTML'
   # Defaults
   datadir = '/usr/local' + '/share/synopsis'
   stylesheet = 'style.css'
   stylesheet_file = datadir + '/html.css'
   file_layout = 'Synopsis.Formatters.HTML.FileLayout.FileLayout'
   pages = [FramesIndex(), #DirBrowse()
            ScopePages(),
            ModuleListing(),
            ModuleIndexer(),
            FileListing(),
            FileIndexer(),
            FileDetails(),
            InheritanceTree(),
            InheritanceGraph(),
            NameIndex(),
            XRefPages()]
   comment_formatters = ['javadoc', 'section']
   tree_formatter = 'TreeFormatter.TreeFormatter'
   structs_as_classes = 0
   class FileSource:
      file_path = './%s'
      links_path = './%s-links'
      toc_files = []
      scope = ''
   # Old name for FileSource:
   FilePages = FileSource
   class FileTree:
      link_to_pages = 0
   class ScopePages:
      parts = ['Heading',
               'Summary',
               'Inheritance',
               'Detail']
      heading_formatters = ['Heading',
                            'ClassHierarchyGraph',
                            'DetailCommenter']
      summary_formatters = ['SummaryAST',
                            'SummaryCommenter']
      detail_formatters = ['DetailAST',
                           'DetailCommenter']
   class InheritanceGraph:
      min_size = 1
      min_group_size = 5
      direction = 'vertical'

   class ModuleListing:
      pass

   def __init__(self, verbose):
      self.verbose = verbose

class Formatter(Processor):

   stylesheet = Parameter('style.css', '')
   stylesheet_file = Parameter('../html.css', '')
   pages = Parameter([FramesIndex(), #DirBrowse()
                      ScopePages(),
                      ModuleListing(),
                      ModuleIndexer(),
                      FileListing(),
                      FileIndexer(),
                      FileDetails(),
                      InheritanceTree(),
                      InheritanceGraph(),
                      FileSource(),
                      NameIndex(),
                      XRefPages()],
                      '')
   
   comment_formatter = Parameter([QuoteHTML, SectionFormatter],
                                 '')

   # this should go away !
   datadir = Parameter('/usr/local/share/synopsis', '')
   file_layout = Parameter(NestedFileLayout, 'how to lay out the output files')
   tree_formatter = Parameter(TreeFormatter, 'define how to lay out tree views')
   structs_as_classes = Parameter(True, '')

   def process(self, ast, **kwds):

      self.set_parameters(kwds)
      self.ast = self.merge_input(ast)

      config_obj = ConfigHTML(self.verbose)

      config.use_config(config_obj)

      config.basename = self.output

      config.fillDefaults() # <-- end of _parseArgs...

      config.ast = ast
      config.types = ast.types()
      declarations = ast.declarations()

      # Instantiate the files object
      config.files = config.files_class()

      # Create the declaration styler and the comment formatter
      config.decl_style = DeclStyle()
      config.comments = CommentFormatter.CommentFormatter()

      # Create the Class Tree (TODO: only if needed...)
      config.classTree = ClassTree.ClassTree()

      # Create the File Tree (TODO: only if needed...)
      config.fileTree = FileTree()
      config.fileTree.set_ast(ast)

      # Build class tree
      for d in declarations:
         d.accept(config.classTree)

      # Create the page manager, which loads the pages
      core.manager = PageManager()
      root = AST.Module(None,-1,"C++","Global",())
      root.declarations()[:] = declarations

      # Create table of contents index
      start = core.manager.calculateStart(root)
      config.toc = core.manager.get_toc(start)
      if verbose: print "HTML Formatter: Initialising TOC"

      # Add all declarations to the namespace tree
      # for d in declarations:
      #	d.accept(config.toc)
	
      if verbose: print "TOC size:",config.toc.size()
      if len(config.toc_out): config.toc.store(config.toc_out)
    
      # load external references from toc files, if any
      for t in config.toc_in: config.toc.load(t)
   
      if verbose: print "HTML Formatter: Writing Pages..."
      # Create the pages
      core.manager.process(root)

      #format(['-o', self.output], self.ast, config_obj)

      return self.ast
