from Synopsis.process import process
from Synopsis.Processor import Composite
from Synopsis.Parsers import Cxx
from Synopsis.Formatters import Dump

process(parse = Composite(Cxx.Parser(), Dump.Formatter(show_ids = False,
                                                       stylesheet = None)))

