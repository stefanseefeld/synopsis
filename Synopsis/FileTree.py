#
# Copyright (C) 2000 Stefan Seefeld
# Copyright (C) 2000 Stephen Davies
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

import os.path

class Node:
    """Base class for directories and files in the tree.
    @attr path the path of this node as a string.
    @attr filename the name of the node, i.e. the last element of the
    path string
    """
    def __init__(self, path, filename):
        self.path = path
        self.filename = filename
            
class Directory(Node):
    """FileTree node for directories.
    @attr path the path of this node as a string.
    @attr filename the name of the directory, i.e. the last element of the
    path string
    @attr children the children of this directory, each Directory or File
    objects.
    """
    def __init__(self, path, filename):
        Node.__init__(self, path, filename)
        self.children = []
    
class File(Node):
    """FileTree node for files.
    @attr path the path of this node as a string.
    @attr filename the name of the file, i.e. the last element of the
    path string
    @attr decls the list of declarations in this file.
    """
    def __init__(self, path, filename, declarations):
        Node.__init__(self, path, filename)
        self.declarations = declarations

def make_file_tree(files):

    tmp_dirs = {}
    root = Directory('', '')

    def insert_dir(path):
        """Recursively add a directory to the tree"""
        parent_dir, filename = os.path.split(path)
        if parent_dir == path:
            # The root directory is added below the root node
            # This is in case absolute filenames are mixed with relative ones
            parent = root
            filename = '/'
        else:
            parent_dir, filename = os.path.split(path)
            if parent_dir:
                if tmp_dirs.has_key(parent_dir):
                    parent = tmp_dirs[parent_dir]
                else:
                    parent = insert_dir(parent_dir)
            else:
             # No parent means a relative name like 'home/foo/bar'
             parent = root
        new_dir = Directory(path, filename)
        tmp_dirs[path] = new_dir
        parent.children.append(new_dir)
        return new_dir

    def insert_file(file, declarations):
        parent_dir, filename = os.path.split(file)
        if parent_dir:
            if tmp_dirs.has_key(parent_dir):
                parent = tmp_dirs[parent_dir]
            else:
                parent = insert_dir(parent_dir)
        else:
            # No parent means an relative name like 'home/foo/bar'
            parent = root
        new_file = File(file, filename, declarations)
        parent.children.append(new_file)
    

    for f in files:
        if f.annotations['primary']:
            insert_file(f.name, f.declarations)

    return root
