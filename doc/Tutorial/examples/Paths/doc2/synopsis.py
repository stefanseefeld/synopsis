from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Processors.Comments import SSComments
from Synopsis.Processors.Comments import SSDComments
from Synopsis.Processors.Comments import JavaComments
from Synopsis.Processors.Comments import Previous
from Synopsis.Processors.Comments import JavaTags
from Synopsis.Processors.Comments import Grouper1
from Synopsis.Formatters import SXR

cxx = Cxx.Parser(base_path='../src/',
                 syntax_prefix='links',
                 xref_prefix='xref')

cxx_ssd = Composite(cxx, SSDComments())

process(cxx_ssd = cxx_ssd,
        cxx_ss = Composite(cxx, SSComments()),
        cxx_ssd_prev = Composite(cxx, SSDComments(), Previous()),
        cxx_javadoc = Composite(cxx, JavaComments(), JavaTags()),
        link = Linker(Grouper1()),
        sxr = SXR.Formatter(src_dir = '../src/',
                            xref_prefix='xref',
                            syntax_prefix='links'))
