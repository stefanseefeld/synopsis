from Synopsis.process import process
from Synopsis.Processor import Composite
from Synopsis.Parsers import Python
from Synopsis.Formatters import Dump

process(parse = Composite(Python.Parser(), Dump.Formatter()))

