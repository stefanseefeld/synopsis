# $Id: Formatter.py,v 1.1 2003/11/11 04:53:41 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Processor import Processor, Parameter
from Synopsis.Core import AST
from FramesIndex import *
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
   file_layout = 'Synopsis.Formatter.HTML.FileLayout.FileLayout'
   pages = ['FramesIndex',
            'ScopePages',
            'ModuleListing',
            'ModuleIndexer',
            'FileListing',
            'FileIndexer',
            'FileDetails',
            'InheritanceTree',
            'InheritanceGraph',
            'NameIndex',
            'FramesIndex']
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
   pages = Parameter(['FramesIndex', #these will become types, but that requires more refactoring
                      'ScopePages',
                      'ModuleListing',
                      'ModuleIndexer',
                      'FileListing',
                      'FileIndexer',
                      'FileDetails',
                      'InheritanceTree',
                      'InheritanceGraph',
                      'FileSource',
                      'NameIndex',
                      'XRefPages'],
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
      format(['-o', self.output], self.ast, config_obj)

      return self.ast
