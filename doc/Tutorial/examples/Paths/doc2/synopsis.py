from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Processors import Comments
from Synopsis.Formatters import SXR

cxx = Cxx.Parser(base_path='../src/',
                 syntax_prefix='links',
                 xref_prefix='xref')

cxx_ssd = Composite(cxx,
                    Comments.SSDFilter(),
                    Comments.Grouper(),
                    Comments.Translator())
cxx_ss = Composite(cxx,
                   Comments.SSFilter(),
                   Comments.Grouper(),
                   Comments.Translator())
cxx_ssd_prev = Composite(cxx,
                         Comments.SSDFilter(),
                         Comments.Previous(),
                         Comments.Grouper(),
                         Comments.Translator())
cxx_javadoc = Composite(cxx,
                        Comments.JavaFilter(),
                        Comments.Grouper(),
                        Comments.Translator(markup='javadoc'))

process(cxx_ssd = cxx_ssd,
        cxx_ss = cxx_ss,
        cxx_ssd_prev = cxx_ssd_prev,
        cxx_javadoc = cxx_javadoc,
        link = Linker(),
        sxr = SXR.Formatter(src_dir = '../src/',
                            xref_prefix='xref',
                            syntax_prefix='links'))
