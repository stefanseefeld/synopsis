#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cpp
from Synopsis.Parsers import Cxx
from Synopsis.Formatters import Dot

cpp = Cpp.Parser(base_path='../src/', flags=['-I../src'], primary_file_only = False)
cxx = Cxx.Parser(base_path='../src/', cppflags=['-I../src'], primary_file_only = False)
dot = Dot.Formatter(format='png', bgcolor='#ffcc99')

process(files = Composite(cpp, Dot.Formatter(type = 'file', format = 'png', bgcolor=(.1, 0.2, 0.8))),
        classes = Composite(cxx, dot),
        cxx = cxx,
        dot = dot)
