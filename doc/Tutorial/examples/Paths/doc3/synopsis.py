from Synopsis.process import process
from Synopsis.Processor import Processor, Parameter, Composite
from Synopsis.Parsers import Cxx
from Synopsis.Processors import *
from Synopsis.Processors import Comments
from Synopsis.Formatters import HTML
from Synopsis.Formatters.HTML.FileLayout import *
from Synopsis.Formatters.HTML.TreeFormatterJS import *
from Synopsis.Formatters.HTML.Views import *
from Synopsis.Formatters import Dot

cxx = Cxx.Parser(base_path='../src/',
                 syntax_prefix='links',
                 xref_prefix='xref')

cxx_ssd = Composite(cxx, Comments.SSDFilter())

html = HTML.Formatter(file_layout = NestedFileLayout(),
                      tree_formatter = TreeFormatterJS(),
                      views = [FramesIndex(),
                               Scope(),
                               Source(prefix = 'links'),
                               XRef(xref_file = 'Paths.xref'),
                               ModuleListing(),
                               ModuleIndexer(),
                               FileListing(),
                               FileIndexer(),
                               FileDetails(),
                               InheritanceTree(),
                               InheritanceGraph(),
                               NameIndex()])

cxx_ss = Composite(cxx,
                   Comments.SSFilter(),
                   Comments.Grouper(),
                   Comments.Translator())
cxx_ssd_prev = Composite(cxx,
                         Comments.SSDFilter(),
                         Comments.Grouper(),
                         Comments.Previous(),
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
        xref = XRefCompiler(prefix='xref'),
        html = html)
