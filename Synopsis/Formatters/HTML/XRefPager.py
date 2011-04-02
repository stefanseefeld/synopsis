#
# Copyright (C) 2008 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

class XRefPager:
    """Generates pages of cross-references."""

    def __init__(self, ir):

        self.page_map = {}
        self.page_info = []

        # Split the data into multiple files based on size
        page = 0
        count = 0
        names = ir.sxr.keys()
        if names:
            self.page_info.append([])
        names.sort()
        for name in names:
            if count > 200:
                count = 0
                page += 1
                self.page_info.append([])
            self.page_info[page].append(name)
            self.page_map[name] = page
            entry = ir.sxr[name]
            d, c, r = len(entry.definitions), len(entry.calls), len(entry.references)
            count += 1 + d + c + r
            if d: count += 1
            if c: count += 1
            if r: count += 1

    def get(self, name):
        """Returns the number of the page that the xref info for the given
        name is on, or None if not found."""
        
        return self.page_map.get(name)

    def pages(self):
        """Returns a list of pages, each consisting of a list of names on that
        page. This method is intended to be used by whatever generates the
        files..."""

        return self.page_info
