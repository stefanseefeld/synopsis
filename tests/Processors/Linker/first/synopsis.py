from Synopsis.process import process
from Synopsis.Processor import Composite
from Synopsis.Parsers import Cxx
from Synopsis.Processors import Linker
from Synopsis.Formatters import Dump

process(parse = Composite(Cxx.Parser(),
                          Linker(remove_empty_modules=False),
                          Dump.Formatter(show_ids = False,
                                         stylesheet = None)))
