#!/usr/bin/env python
#
# $Id: setup.py,v 1.3 2003/09/18 02:54:14 stefan Exp $
#
# Setup script for synopsis
#
# Usage: python setup.py install
#

from distutils.core import setup, Extension

import os, sys, re

from Synopsis.dist.command.config import config
from Synopsis.dist.command.build_doc import build_doc
from Synopsis.dist.command.build import build
from Synopsis.dist.command.build_ext import build_ext

def prefix(list, pref): return map(lambda x, p=pref: p + x, list)

py_packages = ["Synopsis.Core",
               "Synopsis.Parser.IDL", "Synopsis.Parser.Python",
               "Synopsis.Linker",
               "Synopsis.Formatter"] 

data_files = ["synopsis.jpg", "synopsis200.jpg", "syn-down.png", "syn-right.png", "syn-dot.png"]
from Synopsis import __version__

setup(cmdclass={'config':config,
                'build_doc':build_doc,
                'build_ext':build_ext,
                'build':build},
      name="synopsis",
      version=__version__,
      author="Stefan Seefeld & Stephen Davies",
      author_email="synopsis-devel@lists.sf.net",
      description="source code inspection tool",
      url="http://synopsis.fresco.org",
      packages=py_packages,
      data_files=[('share/Synopsis', prefix(data_files, "share/"))])
