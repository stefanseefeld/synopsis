from Synopsis.process import process
from Synopsis.Processor import Composite
from Synopsis.Parsers import IDL
from Synopsis.Formatters import Dump

process(parse = Composite(IDL.Parser(), Dump.Formatter()))

