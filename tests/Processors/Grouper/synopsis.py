from Synopsis.process import process
from Synopsis.Processor import Composite
from Synopsis.Parsers import Cxx
from Synopsis.Processors.Comments import SSComments, Grouper1
from Synopsis.Formatters import Dump

process(parse = Composite(Cxx.Parser(),
                          SSComments(),
                          Grouper1(),
                          Dump.Formatter(show_ids = False)))
