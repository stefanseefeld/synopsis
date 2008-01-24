#
# Copyright (C) 2000 Stephen Davies
# Copyright (C) 2000 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis import config
from Synopsis.Processor import Parameter
from Synopsis import ASG
from Synopsis.Formatters.TOC import TOC
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
from Synopsis.Formatters.HTML.Parts import *
import time

class Scope(View):
    """A module for creating a view for each Scope with summaries and
    details. This module is highly modular, using the classes from
    ASGFormatter to do the actual formatting. The classes to use may be
    controlled via the config script, resulting in a very configurable output.
    @see ASGFormatter The ASGFormatter module
    @see Config.Formatters.HTML.ScopeViews Config for ScopeViews
    """

    parts = Parameter([Heading(),
                       Summary(),
                       Inheritance(),
                       Detail()],
                      '')
   
    def register(self, frame):

        super(Scope, self).register(frame)
        for part in self.parts: part.register(self)

        self.__scopes = []
        self.__toc = TOC(self.directory_layout)
        for d in self.processor.ir.declarations:
            d.accept(self.__toc)
      
    def toc(self):
      
        return self.__toc

    def filename(self):

        return self.__filename

    def title(self):

        return self.__title

    def root(self):

        if self.main:
            url = self.directory_layout.index()
        else:
            url = self.directory_layout.scope()
        return url, 'Global Module'

    def scope(self):
        """return the current scope processed by this object"""

        return self.__scope

    def process(self):
        """Creates a view for every Scope."""

        # FIXME: see HTML.Formatter
        module = self.processor.ir.declarations[0]
        self.__scopes = [module]
        while self.__scopes:
            scope = self.__scopes.pop(0)
            self.process_scope(scope)
            scopes = [c for c in scope.declarations if isinstance(c, ASG.Scope)]
            self.__scopes.extend(scopes)

    def register_filenames(self):
        """Registers a view for every Scope."""

        # FIXME: see HTML.Formatter
        self.__scopes = [self.processor.ir.declarations[0]]
        while self.__scopes:
            scope = self.__scopes.pop(0)
            if scope.name:
                filename = self.directory_layout.scope(scope.name)
            else:
                filename = self.root()[0]
            self.processor.register_filename(filename, self, scope)

            scopes = [c for c in scope.declarations if isinstance(c, ASG.Module)]
            self.__scopes.extend(scopes)
     
    def process_scope(self, scope):
        """Creates a view for the given scope"""

        # Open file and setup scopes
        self.__scope = scope.name
        if self.__scope:
            self.__filename = self.directory_layout.scope(self.__scope)
        else:
            self.__filename = self.root()[0]
        self.__title = escape(' '.join(self.__scope))
        self.start_file()
        self.write_navigation_bar()
        # Loop throught all the view Parts
        for part in self.parts:
            part.process(scope)
        self.end_file()
    
    def end_file(self):
        """Overrides end_file to provide synopsis logo"""

        self.write('\n')
        now = time.strftime(r'%c', time.localtime(time.time()))
        logo = href('http://synopsis.fresco.org', 'synopsis') + ' (version %s)'%config.version
        self.write(div('logo', 'Generated on ' + now + ' by \n<br/>\n' + logo))
        View.end_file(self)
