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
from Synopsis.Formatters.HTML.View import View
from Synopsis.Formatters.HTML.Tags import *
import time

class NameIndex(View):
    """Creates an index of all names on one view in alphabetical order."""

    columns = Parameter(2, 'the number of columns for the listing')

    def filename(self):

        if self.main:
            return self.directory_layout.index()
        else:
            return self.directory_layout.special('NameIndex')
            
    def title(self):

        return 'Name Index'

    def root(self):

        return self.filename(), self.title()

    def process(self):

        self.start_file()
        self.write_navigation_bar()
        self.write(element('h1', 'Name Index'))
        self.write('<i>Hold the mouse over a link to see the scope of each name</i>\n')

        dict = self.make_dictionary()
        keys = dict.keys()
        keys.sort()
        linker = lambda key: '<a href="#key%d">%s</a>'%(ord(key),key)
        self.write(div('nameindex-index', ''.join([linker(k) for k in keys])) + '\n')
        for key in keys:
            self.write('<a id="key%d">'%ord(key)+'</a>')
            self.write(element('h2', key) + '\n')
            self.write('<table summary="table of names">\n')
            self.write('<col width="*"/>'*self.columns + '\n')
            self.write('<tr>\n')
            items = dict[key]
            numitems = len(items)
            start = 0
            for column in range(self.columns):
                end = numitems * (column + 1) / self.columns
                self.write('<td valign="top">\n')
                for item in items[start:end]:
                    self._process_item(item)
                self.write('</td>\n')
                start = end
            self.write('</tr>\n</table>\n')
	
        self.end_file()

    def make_dictionary(self):
        """Returns a dictionary of items. The keys of the dictionary are the
        headings - the first letter of the name. The values are each a sorted
        list of items with that first letter."""

        dict = {}
        def hasher(type):
            name = type.name
            try: key = name[-1][0]
            except:
                print 'name:',name, 'type:',repr(type), id(type)
                raise
            if key >= 'a' and key <= 'z': key = chr(ord(key) - 32)
            if dict.has_key(key): dict[key].append(type)
            else: dict[key] = [type]
        # Fill the dict
        [hasher(t) for t in self.processor.ir.asg.types.values()
         if isinstance(t, ASG.DeclaredTypeId) and
         not isinstance(t.declaration, ASG.Builtin)]

        # Now sort the dict
        def name_cmp(a,b):
            a, b = a.name, b.name
            res = cmp(a[-1],b[-1])
            if res == 0: res = cmp(a,b)
            return res
        for items in dict.values():
            items.sort(name_cmp)

        return dict

    def _process_item(self, type):
        """Process the given name for output"""

        name = type.name
        decl = type.declaration # non-declared types are filtered out
        if isinstance(decl, ASG.Function):
            realname = escape(decl.real_name[-1]) + '()'
        else:
            realname = escape(name[-1])
        self.write('\n')
        title = escape(str(name))
        type = decl.type
        name = self.reference(name, (), realname, title=title)+' '+type
        self.write(div('nameindex-item', name))

    def end_file(self):
        """Overrides end_file to provide synopsis logo"""

        self.write('\n')
        now = time.strftime(r'%c', time.localtime(time.time()))
        logo = img(src=rel(self.filename(), 'synopsis.png'), alt='logo')
        logo = href('http://synopsis.fresco.org', logo + ' synopsis', target='_blank')
        logo += ' (version %s)'%config.version
        self.write(div('logo', 'Generated on ' + now + ' by \n<br/>\n' + logo))
        View.end_file(self)
