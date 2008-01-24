#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import cPickle

class CrossReferencer:
   """Handle cross-reference files"""

   def __init__(self):

      self.__data = {}
      self.__index = {}
      self.__file_map = {}
      self.__file_info = []

   def load(self, filename):
      """Loads data from the given filename"""

      f = open(filename, 'rb')
      self.__data, self.__index = cPickle.load(f)
      f.close()
      # Split the data into multiple files based on size
      page = 0
      count = 0
      self.__file_info.append([])
      names = self.__data.keys()
      names.sort()
      for name in names:
         if count > 200:
            count = 0
            page = page + 1
            self.__file_info.append([])
         self.__file_info[page].append(name)
         self.__file_map[name] = page
         target_data = self.__data[name]
         l0, l1, l2 = len(target_data[0]), len(target_data[1]), len(target_data[2])
         count = count + 1 + l0 + l1 + l2
         if l0: count = count + 1
         if l1: count = count + 1
         if l2: count = count + 1

   def get_info(self, name):
      """Retrieves the info for the give name. The info is returned as a
      3-tuple of [list of definitions], [list of calls], [list of
      other references]. The element of each list is another 3-tuple of
      (file, line, scope of reference). The last element is the scope in
      which the reference was made, which is often a function scope, denoted
      as starting with a backtick, eg: Builder::`add_operation(std::string).
      Returns None if there is no xref info for the given name.
      """

      return self.__data.get(tuple(name))

   def get_possible_names(self, name):
      """Returns a list of possible scoped names that end with the given
      name. These scoped names can be passed to get_info(). Returns None if
      there is no scoped name ending with the given name."""

      return self.__index.get(tuple(name))

   def get_page_for(self, name):
      """Returns the number of the page that the xref info for the given
      name is on, or None if not found."""

      return self.__file_map.get(tuple(name))

   def get_page_info(self):
      """Returns a list of pages, each consisting of a list of names on that
      page. This method is intended to be used by whatever generates the
      files..."""

      return self.__file_info

   def get_all_names(self):
      """Returns a list of all names"""

      return self.__data.keys()
    
