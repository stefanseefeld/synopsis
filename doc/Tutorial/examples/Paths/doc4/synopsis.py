from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cpp
from Synopsis.Parsers import Cxx
from Synopsis.Formatters import Dot

cpp = Cpp.Parser(base_path='../src/', flags=['-I../src'], main_file_only = False)
cxx = Cxx.Parser(base_path='../src/', cppflags=['-I../src'], main_file_only = False)
dot = Dot.Formatter(format='png', bgcolor='#ffcc99')

process(files = Composite(cpp, Dot.Formatter(type = 'file', format = 'png', bgcolor=(.1, 0.2, 0.8))),
        classes = Composite(cxx, dot),
        cxx = cxx,
        dot = dot)
