from Synopsis.process import process
from Synopsis.Processor import Composite
from Synopsis.Parsers import Cpp
from Synopsis.Formatters import Dump

process(parse = Composite(Cpp.Parser(),
                          Dump.Formatter(show_ids = False,
                                         stylesheet = None)))

