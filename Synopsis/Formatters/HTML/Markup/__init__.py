#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#
"""Markup formatters."""

from Synopsis.Processor import Parametrized, Parameter

class Struct:

    def __init__(self, summary = '', details = ''):
        self.summary = summary
        self.details = details
        

class Formatter(Parametrized):
    """Interface class that takes an annotation and generatesformats its summary and/or
    detail strings."""

    def init(self, processor):
      
        self.processor = processor

    def format(self, decl, view):
        """Format the declaration's documentation.
        @param view the View to use for references and determining the correct
        relative filename.
        @param decl the declaration
        @returns Struct containing summary / details pair.
        """

        doc = decl.annotations.get('doc')
        text = doc and doc.text or ''
        return Struct('', text)
