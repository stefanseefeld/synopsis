#
# $Id: emul.py,v 1.5 2002/10/25 02:46:46 chalky Exp $
#
# This file is a part of Synopsis.
# Copyright (C) 2002 Stephen Davies
#
# Synopsis is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
# 02111-1307, USA.

# This module contains functions for getting the info needed to emulate
# different compilers

# $Log: emul.py,v $
# Revision 1.5  2002/10/25 02:46:46  chalky
# Try to fallback to gcc etc if compiler info fails, and display warning
#
# Revision 1.4  2002/10/24 23:41:09  chalky
# Cache information for the lifetime of the module about which compilers are
# available and which are not
#
# Revision 1.3  2002/10/11 11:09:39  chalky
# Add cygwin support to compiler emulations
#
# Revision 1.2  2002/10/02 03:51:01  chalky
# Allow more flexible gcc version (eg: 2.95.3-5 for cygwin)
#
# Revision 1.1  2002/09/28 05:51:06  chalky
# Moved compiler emulation stuff to this module
#

import sys, os, os.path, re, string, stat
from Synopsis.Core import Util

# The filename where infos are stored
user_emulations_file = '~/.synopsis/cpp_emulations'

# The list of default compilers. Note that the C syntax is not quite
# supported, so including a bazillion different C compilers is futile.
default_compilers = [
    'cc', 'c++', 'gcc', 'g++', 'g++-2.95', 'g++-3.0', 'g++-3.1', 'g++-3.2'
]

# A cache of the compiler info, loaded from the emulations file or calculated
# by inspecting available compilers
compiler_infos = {}

# A map of failed compilers. This map is kept for the lifetype of Synopsis, so
# you may have to restart it if a compiler is 'fixed'
failed = {}

class CompilerInfo:
    """Info about one compiler.

    @attr compiler The name of the compiler, typically the executable name,
    which must either be in the path or given as an absolute pathname
    @attr is_custom True if this is a custom compiler - in which case it will
    never be updated automatically.
    @attr timestamp The timestamp of the compiler binary
    @attr include_paths A list of strings indicating the include paths
    @attr macros A list of string 2-tuples indicating key=value pairs for
    macros. A value of '' (empty string) indicates an empty definition. A
    value of None (not a string) indicates that the macro should be undefined.
    """
    def __init__(self, compiler, is_custom, timestamp, include_paths, macros):
	self.compiler = compiler
	self.is_custom = is_custom
	self.timestamp = timestamp
	self.include_paths = include_paths
	self.macros = macros

def main():
    """The main function - parses the arguments and controls the program"""
    if len(sys.argv) < 2:
	print usage
	return

    filename = sys.argv[1]
    print "Filename is ", filename
    if len(sys.argv) > 2:
	compilers = sys.argv[2:]
    else:
	compilers = default_compilers
    print "compilers are:", compilers

    infos = get_compiler_infos(compilers)
    if not infos:
	print "No compilers found. Not writing file!"
	return

    file = open(filename, 'wt')
    write_compiler_infos(infos, file)
    file.close()

    print "Found info about the following compilers:"
    print string.join(map(lambda x: x[0], infos), ', ')

def get_fallback(preferred, is_first_time):
    """Tries to return info from a fallback compiler, and prints a warning
    message to the user, unless their preferred compiler was 'none'"""
    if is_first_time and preferred != 'none':
	print "Warning: The specified compiler (%s) could not be found."%(preferred,)
	print "You may want to retry with the full pathname of the compiler"
	print "or with it in your path. If you don't have this compiler, you"
	print "will need to modify the C++ Parser part of your config file."
    for compiler in ('g++', 'gcc', 'c++', 'cc'):
	if compiler_infos.has_key(compiler):
	    print "Warning: Falling back to compiler '%s'"%(compiler,)
	    return compiler_infos[compiler]
    if preferred != 'none':
	print "Warning: Unable to fallback to a default compiler emulation."
	print "Unless you have set appropriate paths, expect errors."
    return None

def get_compiler_info(compiler):
    """Returns the compiler info for the given compiler. The info is returned
    as a CompilerInfo object, or None if the compiler isn't found. 
    """
    global failed, compiler_infos
    # Check if this compiler has already been found to not exist
    if failed.has_key(compiler):
	return get_fallback(compiler, 0)
    # See if already found it
    if len(compiler_infos) == 0:
	# Try to load the emulations file
	compiler_infos = load_compiler_infos()
    # See if wanted compiler was in file
    if compiler_infos.has_key(compiler):
	info = compiler_infos[compiler]
	if info.is_custom: return info
	file_stamp = get_compiler_timestamp(compiler)
	# If compiler hasn't changed since then, return cached info
	if file_stamp and info.timestamp == file_stamp:
	    return info
    else:
	# Add compiler to map, but with a dummy value to indicate nothing is
	# known about it
	compiler_infos[compiler] = None
    
    # Regenerate infos
    refresh_compiler_infos(compiler_infos)

    # Cache results to disk
    write_compiler_infos(compiler_infos)

    # Return discovered info, if compiler was found
    if compiler_infos.has_key(compiler):
	return compiler_infos[compiler]
    else:
	return get_fallback(compiler, 1)

def load_compiler_infos():
    """Loads the compiler infos from a file"""
    glob = {'__builtins__':{}, '__name__':'__main__', 'setattr':setattr}
    filename = os.path.expanduser(user_emulations_file)
    try: execfile(filename, glob, glob)
    except IOError: return {}
    if glob.has_key('infos'):
	return glob['infos']
    return {} 

def get_compiler_timestamp(compiler):
    """Returns the timestamp for the given compiler, or 0 if not found"""
    path = os.getenv('PATH', os.defpath)
    path = string.split(path, os.pathsep)
    for directory in path:
	# Try to stat the compiler in this directory, if it exists
	filename = os.path.join(directory, compiler)
	try: stats = os.stat(filename)
	except OSError: continue
	return stats[stat.ST_CTIME]
    # Not found
    return 0

def refresh_compiler_infos(infos):
    """Refreshes the list of infos, by rediscovering all non-custom compilers
    in the map. The map is modified in-place."""
    global failed
    # Refresh each non-custom compiler in the map 
    for compiler, info in infos.items():
	if info and info.is_custom:
	    # Skip custom compilers
	    continue
	if failed.has_key(compiler):
	    # Skip compilers that have already failed
	    del infos[compiler]
	    continue
	info = find_compiler_info(compiler)
	if info: infos[compiler] = info
	else:
	    del infos[compiler]
	    failed[compiler] = None
    # Now try to add defaults
    for compiler in default_compilers:
	# Don't retry already-failed compilers
	if failed.has_key(compiler):
	    continue
	info = find_compiler_info(compiler)
	if info: infos[compiler] = info
	

#def get_compiler_infos(compilers):
#    infos = filter(None, map(find_compiler_info, compilers))
#    return infos

def write_compiler_infos(infos):
    filename = os.path.expanduser(user_emulations_file)
    dirname = os.path.dirname(filename)
    if not os.path.exists(dirname):
	os.mkdir(dirname)
    file = open(filename, 'wt')
    writer = Util.PyWriter(file)
    writer.write_top('"""This file contains info about compilers for use by '+
    'Synopsis.\nSee the Synopsis RefManual for info on customizing this list."""\n')
    writer.ensure_struct()
    writer.write('infos = {\n')
    writer.indent()
    ccomma = 0
    for compiler, info in infos.items():
	if ccomma: writer.write(",\n")
	else: ccomma = 1
	writer.write("'%s' : struct(\n"%compiler)
	writer.write('is_custom = %d,\n' % info.is_custom)
	writer.write('timestamp = %d,\n' % info.timestamp)
	writer.write("include_paths = ")
	writer.write_long_list(info.include_paths)
	writer.write(",\nmacros = ")
	writer.write_long_list(info.macros)
	writer.write(")")
    writer.outdent()
    writer.write("\n}\n")
    writer.flush()
    file.close()
    
    

re_specs = re.compile('^Reading specs from (.*/)lib/gcc-lib/(.*)/([0-9]+\.[0-9]+\.[0-9]+.*)/specs')
re_version = re.compile('([0-9]+)\.([0-9]+)\.([0-9]+)')

def find_compiler_info(compiler):
    print "Finding info for '%s'"%compiler

    # Run the compiler with -v and get first line to stderr (assumes gcc!!!)
    cin, out,err = os.popen3(compiler + " -v")
    version = err.readline().rstrip()
    cin.close()
    out.close()
    err.close()

    # Parse the first line for the 'specs' path - all the info we need is in
    # the path
    mo = re_specs.match(version)
    if not mo:
	print "Warning: unrecognised version string '%s'"%version
	return None
    prefix, hosttype, version = mo.groups()

    # Construct a list of include paths
    paths = []
    # The version-specific dir
    paths.append('%slib/gcc-lib/%s/%s/include'%(prefix, hosttype, version))
    # For some reason (Debian?) has 2.x as g++-3 and 3.x as g++-v3
    if version[0] == '3':
	paths.append(prefix + 'include/g++-v3')
	paths.append(prefix + 'include/g++-v3/' + hosttype)
    else:
	paths.append(prefix + 'include/g++-3')
	paths.append(prefix + 'include/g++-3/' + hosttype)
    # The prefix include
    paths.append(prefix + 'include')
    # The /usr/include if different
    if prefix != '/usr/':
	paths.append('/usr/include')

    # As for macros, these are somewhat contrived as I don't have many
    # compilers/architectures to test with ;) However, I did derive them from
    # the gcc sources (CVS mid sep '02).
    # Defines to the empty string are just defines
    # Defines to None are undefines (removes previous definition)
    macros = []

    v1, v2, v3 = re_version.match(version).groups()
    macros.append( ('__GNUC__', str(v1)) )
    macros.append( ('__GNUC_MINOR__', str(v2)) )
    macros.append( ('__GNUC_PATCHLEVEL__', str(v3)) )
    # UCPP already defines these next two, and not the same as GCC does (ucpp
    # sets version to C99 value)
    # macros.append( ('__STDC__', '1') )
    # macros.append( ('__STDC_VERSION__', '199409L') )
    macros.append( ('__GNUG__', str(v1)) )
    macros.append( ('__cplusplus__', '1') ) # STD says '199711L', but gcc isn't fully compliant...
    macros.append( ('__VERSION__', version) )
    macros.append( ('system', 'posix') )
    if hosttype[-6:] == 'cygwin':
	# These last three are architecture dependant:
	macros.append( ('__CYGWIN__', '') )
	macros.append( ('__cygwin__', '') )
	macros.append( ('__i386__', '') )
	macros.append( ('__cdecl', '') ) # Removes parse errors due to this keyword
	macros.append( ('__declspec(x)', '') ) # Removes parse errors due to this keyword
    else: 
	# These last three are architecture dependant:
	macros.append( ('__ELF__', '') )
	macros.append( ('linux', '') )
	macros.append( ('unix', '') )

    timestamp = get_compiler_timestamp(compiler)
    return CompilerInfo(compiler, 0, timestamp, paths, macros)
    
