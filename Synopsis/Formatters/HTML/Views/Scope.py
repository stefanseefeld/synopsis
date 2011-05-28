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
from Synopsis.Formatters.HTML import Tags
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
                       Body(),
                       Inheritance()],
                      '')
   
    def register(self, frame):

        super(Scope, self).register(frame)
        for part in self.parts: part.register(self)

        self.scope_queue = []
        self.__toc = TOC(self.directory_layout)
        for d in self.processor.ir.asg.declarations:
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
            url = self.directory_layout.scope(self.processor.root.name)
        title = 'Global %s'%(self.processor.root.type.capitalize())
        return url, title

    def scope(self):
        """return the current scope processed by this object"""

        return self.__scope

    def process(self):
        """Creates a view for every Scope."""

        module = self.processor.root
        self.scopes_queue = [module]
        while self.scopes_queue:
            scope = self.scopes_queue.pop(0)
            self.process_scope(scope, scope == module)
            scopes = [c for c in scope.declarations if isinstance(c, (ASG.Scope, ASG.Group))]
            self.scopes_queue.extend(scopes)
            forwards = [c for c in scope.declarations
                        if isinstance(c, ASG.Forward) and c.specializations]
            # Treat forward-declared class template like a scope if it has
            # specializations, since these are only listed in a Scope view.
            # Process them directly as they don't have child declarations.
            for f in forwards:
                self.process_scope(f)

    def register_filenames(self):
        """Registers a view for every Scope."""

        self.scopes_queue = [self.processor.root]
        while self.scopes_queue:
            scope = self.scopes_queue.pop(0)
            if scope.name:
                filename = self.directory_layout.scope(scope.name)
            else:
                filename = self.root()[0]
            self.processor.register_filename(filename, self, scope)

            scopes = [c for c in scope.declarations if isinstance(c, ASG.Module)]
            self.scopes_queue.extend(scopes)
     
    def process_scope(self, scope, is_root = False):
        """Creates a view for the given scope"""

        # Open file and setup scopes
        self.__scope = scope.name
        if self.__scope:
            # Hack: if this is the toplevel scope in a no-frames context,
            #       generate the index page. This of course is based on a number
            #       of assumptions that may not be valid (such as this module producing
            #       the 'main' view.
            if is_root and not Tags.using_frames:
                self.__filename, xxx = self.root()
            else:
                self.__filename = self.directory_layout.scope(self.__scope)
            self.__title = escape(str(self.__scope))
        else:
            self.__filename, self.__title = self.root()
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
        logo = img(src=rel(self.filename(), 'synopsis.png'), alt='logo')
        logo = href('http://synopsis.fresco.org', logo + ' synopsis', target='_blank')
        logo += ' (version %s)'%config.version
        self.write(div('Generated on ' + now + ' by \n<br/>\n' + logo, class_='logo'))
        View.end_file(self)
