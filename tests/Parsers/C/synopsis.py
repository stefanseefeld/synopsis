from Synopsis.process import process
from Synopsis.Processor import Composite
from Synopsis.Parsers import Cpp
from Synopsis.Parsers import C
from Synopsis.Formatters import Dump

process(parse = Composite(C.Parser(), Dump.Formatter(show_ids = False,
                                                     stylesheet = None)))

