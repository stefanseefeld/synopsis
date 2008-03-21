#
# Copyright (C) 2006 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

from Synopsis.Formatters.HTML.Tags import *
import sys, time

class Frame:
    """A Frame is a mediator for views that get displayed in it (as well
    as other frames. It supports the creation of links across views."""

    def __init__(self, processor, views, noframes = False):

        self.processor = processor
        self.views = views
        self.noframes = noframes
        if self.noframes:
            self.views[0].main = True
        for v in self.views:
            v.register(self)

    def process(self):

        for v in self.views:
            v.register_filenames()

        for v in self.views:
            if self.processor.profile:
                print 'Time for %s:'%v.__class__.__name__,
                sys.stdout.flush()
                start_time = time.time()
            v.process()
            if self.processor.profile:
                print '%f seconds'%(time.time() - start_time)


    def navigation_bar(self, view):
        """Generates a navigation bar for the given view."""

        # Only include views that set a root title.
        views = [v for v in self.views if v.root()[1]]

        def item(v):
            """Generate a navigation bar item."""

            url, label = v.root()
            if url == view.filename():
                return span('selected', label) + '\n'
            else:
                return span('normal', href(rel(view.filename(), url), label)) + '\n'

        items = [item(v) for v in views]
        return items and div('navigation', '\n' + ''.join(items)) or ''


        
