#! /usr/bin/env python

from Synopsis.process import process
from Synopsis.Parsers import Python
from Synopsis.Formatters import HTML

process(parse = Python.Parser(),
        format = HTML.Formatter(stylesheet_file = '../html.css'))
