# $Id: Parser.py,v 1.1 2003/11/18 21:33:46 stefan Exp $
#
# Copyright (C) 2003 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

"""Parser for IDL using omniidl for low-level parsing."""

from Synopsis.Processor import Processor, Parameter
from Synopsis import AST
import omni

class Parser(Processor):

   include_paths = Parameter([], 'list of include paths')
   main_file_only = Parameter(True, 'should only main file be processed')
   comments_before = Parameter(True, 'Comments are associated with following declaration')
   base_path = Parameter('', 'path prefix to strip off of the file names')
   
   def process(self, ast, **kwds):

      input = kwds.get('input')
      self.set_parameters(kwds)
      self.ast = ast
      for file in input:
         self.ast = omni.parse(self.ast, file,
                               self.verbose,
                               self.main_file_only,
                               self.base_path,
                               self.include_paths,
                               self.comments_before)

      return self.output_and_return_ast()

