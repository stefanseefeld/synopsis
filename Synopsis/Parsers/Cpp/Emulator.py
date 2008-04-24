#
# Copyright (C) 2005 Stefan Seefeld
# All rights reserved.
# Licensed to the public under the terms of the GNU LGPL (>= 2),
# see the file COPYING for details.
#

__docformat__ = 'reStructuredText'

import sys, os, os.path, re, string, stat, tempfile
from Synopsis.config import version


class TempFile:
    # Use tempfile.NamedTemporaryFile once we can rely on Python 2.4
    def __init__(self, suffix):

        self.name = tempfile.mktemp(suffix)
        self.file = open(self.name, 'w')
        self.file.close()


    def __del__(self):

        os.unlink(self.name)


def find_ms_compiler_info():
    """
    Try to find a (C++) MSVC compiler.
    Return tuple of include path list and macro dictionary."""

    vc6 = ('SOFTWARE\\Microsoft\\DevStudio\\6.0\\Products\\Microsoft Visual C++', 'ProductDir')
    vc7 = ('SOFTWARE\\Microsoft\\VisualStudio\\7.0', 'InstallDir')
    vc71 = ('SOFTWARE\\Microsoft\\VisualStudio\\7.1', 'InstallDir')
    vc8 = ('SOFTWARE\\Microsoft\\VisualStudio\\8.0', 'InstallDir')
    vc9e = ('SOFTWARE\\Microsoft\\VCExpress\\9.0\\Setup\\VC', 'ProductDir')

    vc6_macros =  [('__uuidof(x)', 'IID()'),
                   ('__int64', 'long long'),
                   ('_MSC_VER', '1200'),
                   ('_MSC_EXTENSIONS', ''),
                   ('_WIN32', ''),
                   ('_M_IX86', ''),
                   ('_WCHAR_T_DEFINED', ''),
                   ('_INTEGRAL_MAX_BITS', '64'),
                   ('PASCAL', ''),
                   ('RPC_ENTRY', ''),
                   ('SHSTDAPI', 'HRESULT'),
                   ('SHSTDAPI_(x)', 'x')]
    vc6_paths = ['Include']

    vc7_macros = [('__forceinline', '__inline'),
                  ('__uuidof(x)', 'IID()'),
                  ('__w64', ''),
                  ('__int64', 'long long'),
                  ('_MSC_VER', '1300'),
                  ('_MSC_EXTENSIONS', ''),
                  ('_WIN32', ''),
                  ('_M_IX86', ''),
                  ('_WCHAR_T_DEFINED', ''),
                  ('_INTEGRAL_MAX_BITS', '64'),
                  ('PASCAL', ''),
                  ('RPC_ENTRY', ''),
                  ('SHSTDAPI', 'HRESULT'),
                  ('SHSTDAPI_(x)', 'x')]
    vc7_paths = ['..\\..\\Vc7\\Include',
                 '..\\..\\Vc7\\PlatformSDK\\Include']

    vc71_macros = [('__forceinline', '__inline'),
                   ('__uuidof(x)', 'IID()'),
                   ('__w64', ''),
                   ('__int64', 'long long'),
                   ('_MSC_VER', '1310'),
                   ('_MSC_EXTENSIONS', ''),
                   ('_WIN32', ''),
                   ('_M_IX86', ''),
                   ('_WCHAR_T_DEFINED', ''),
                   ('_INTEGRAL_MAX_BITS', '64'),
                   ('PASCAL', ''),
                   ('RPC_ENTRY', ''),
                   ('SHSTDAPI', 'HRESULT'),
                   ('SHSTDAPI_(x)', 'x')]
    vc71_paths = ['..\\..\\Vc7\\Include',
                  '..\\..\\Vc7\\PlatformSDK\\Include']

    vc8_macros = [('__cplusplus', '1'),
                  ('__forceinline', '__inline'),
                  ('__uuidof(x)', 'IID()'),
                  ('__w64', ''),
                  ('__int8', 'char'),
                  ('__int16', 'short'),
                  ('__int32', 'int'),
                  ('__int64', 'long long'),
                  ('__ptr64', ''),
                  ('_MSC_VER', '1400'),
                  ('_MSC_EXTENSIONS', ''),
                  ('_WIN32', ''),
                  ('_M_IX86', ''),
                  ('_WCHAR_T_DEFINED', ''),
                  ('_INTEGRAL_MAX_BITS', '64'),
                  ('PASCAL', ''),
                  ('RPC_ENTRY', ''),
                  ('SHSTDAPI', 'HRESULT'),
                  ('SHSTDAPI_(x)', 'x')]
    vc8_paths = ['..\\..\\Vc\\Include',
                 '..\\..\\Vc\\PlatformSDK\\Include']

    vc9e_macros = [('__cplusplus', '1'),
                  ('__forceinline', '__inline'),
                  ('__uuidof(x)', 'IID()'),
                  ('__w64', ''),
                  ('__int8', 'char'),
                  ('__int16', 'short'),
                  ('__int32', 'int'),
                  ('__int64', 'long long'),
                  ('__ptr64', ''),
                  ('_MSC_VER', '1400'),
                  ('_MSC_EXTENSIONS', ''),
                  ('_WIN32', ''),
                  ('_M_IX86', ''),
                  ('_WCHAR_T_DEFINED', ''),
                  ('_INTEGRAL_MAX_BITS', '64'),
                  ('PASCAL', ''),
                  ('RPC_ENTRY', ''),
                  ('SHSTDAPI', 'HRESULT'),
                  ('SHSTDAPI_(x)', 'x')]
    vc9e_paths = ['..\\..\\Vc\\Include',
                 '..\\..\\Vc\\PlatformSDK\\Include']

    compilers = [(vc9e, vc9e_macros, vc9e_paths),
                 (vc8, vc8_macros, vc8_paths),
                 (vc71, vc71_macros, vc71_paths),
                 (vc7, vc7_macros, vc7_paths),
                 (vc6, vc6_macros, vc6_paths)]

    found, paths, macros = False, [], []

    import _winreg
    for c in compilers:
        try:
            key = _winreg.OpenKey(_winreg.HKEY_LOCAL_MACHINE, c[0][0])
            path, type = _winreg.QueryValueEx(key, c[0][1])
	    found = True
            paths.extend([os.path.join(str(path), p) for p in c[2]])
            macros.extend(c[1])
            break
        except:
            continue

    return found, paths, macros

def find_gcc_compiler_info(language, compiler, flags):
    """
    Try to find a GCC-based C or C++ compiler.
    Return tuple of include path list and macro dictionary."""

    found, paths, macros = False, [], []
    flags = ' '.join(flags)
    temp = TempFile(language == 'C++' and '.cc' or '.c')
    # The output below assumes the en_US locale, so make sure we use that.
    command = 'LANG=en_US %s %s -E -v -dD %s'%(compiler, flags, temp.name)
    cin, out, err = os.popen3(command)
    lines = err.readlines()
    cin.close()
    err.close()

    state = 0
    for line in lines:
        line = line.rstrip()
        if state == 0:
            if line[:11] == 'gcc version': state = 1
        elif state == 1:
            state = 2
        elif state == 2:
            if line == '#include <...> search starts here:':
                state = 3
        elif state == 3:
            if line == 'End of search list.':
                state = 4
            else:
                paths.append(line.strip())
    # now read built-in macros
    state = 0
    for line in out.readlines():
        line = line.rstrip()
        if state == 0:
            if line == '# 1 "<built-in>"' or line == '# 1 "<command line>"':
                state = 1
        elif state == 1:
            if line.startswith('#define '):
                tokens = line[8:].split(' ', 1)
                if len(tokens) == 1: tokens.append('')
                macros.append(tuple(tokens))
            elif line == '# 1 "%s"'%temp:
                state = 0

    out.close()
    
    # Per-compiler adjustments
    for name, value in tuple(macros):
        if name == '__GNUC__' and value == '2':
            # gcc 2.x needs this or it uses nonstandard syntax in the headers
            macros.append(('__STRICT_ANSI__', ''))

    return True, paths, macros


def find_compiler_info(language, compiler, flags):

    found, paths, macros = False, [], []

    if compiler == 'cl' and os.name == 'nt':
        if flags:
            sys.stderr.write('Warning: ignoring unknown flags for MSVC compiler\n')
        found, paths, macros = find_ms_compiler_info()

    else:
        found, paths, macros = find_gcc_compiler_info(language, compiler, flags)
        
    return found, paths, macros


def get_compiler_timestamp(compiler):
    """Returns the timestamp for the given compiler, or 0 if not found"""

    path = os.getenv('PATH', os.defpath)
    path = string.split(path, os.pathsep)
    for directory in path:
        # Try to stat the compiler in this directory, if it exists
        filename = os.path.join(directory, compiler)
        if os.name == 'nt': filename += '.exe'
        try: stats = os.stat(filename)
        except OSError: continue
        return stats[stat.ST_CTIME]
    # Not found
    return 0


class CompilerInfo:
    """Info about one compiler."""

    compiler = ''
    """
    The name of the compiler, typically the executable name,
    which must either be in the path or given as an absolute,
    pathname."""
    flags = []
    "Compiler flags that impact its characteristics."
    language = ''
    "The programming language the compiler is used for."
    kind = ''
    """
    A string indicating the type of this info:
    one of 'system', 'custom', ''.
    'custom' compilers will never be automatically updated,
    and an empty string indicates a failure to look up the 
    given compiler."""
    timestamp = ''
    "The timestamp of the compiler binary."
    include_paths = []
    "A list of strings indicating the include paths."
    macros = []
    """
    A list of (name,value) pairs. Values may be empty, or None.
    The latter ase indicates that the macro is to be undefined."""

    def _write(self, os):
        item = id(self) >= 0 and id(self) or -id(self)
        os.write('class Item%u:\n'%item)
        for name, value in CompilerInfo.__dict__.iteritems():
            if name[0] != '_':
                os.write('    %s=%r\n'%(name, getattr(self, name)))
        os.write('\n')


class CompilerList(object):

    user_emulations_file = '~/.synopsis/parsers/cpp/emulator'
    "The cache file."

    def __init__(self, filename = ''):

        self.compilers = []
        self.no_cache = os.environ.has_key('SYNOPSIS_NO_CACHE')
        self.load(filename)

    def list(self):

        return [c.compiler for c in self.compilers]


    def _query(self, language, compiler, flags):
        """Construct and return a CompilerInfo object for the given compiler."""

        ci = CompilerInfo()
        ci.compiler = compiler
        ci.flags = flags
        ci.language = language
        try:
            found, paths, macros = find_compiler_info(language, compiler, flags)
            if found:
                ci.kind = 'system'
                ci.timestamp = get_compiler_timestamp(compiler)
                ci.include_paths = paths
                ci.macros = macros
        except:
            ci.kind = '' # failure
            ci.timestamp = 0
            ci.include_paths = []
            ci.macros = []
        return ci
    
    def add_default_compilers(self):

        self.compilers.append(self._query('C++', 'c++', []))
        self.compilers.append(self._query('C++', 'g++', []))
        self.compilers.append(self._query('C', 'cc', []))
        self.compilers.append(self._query('C', 'gcc', []))

        
    def load(self, filename = ''):
        """Loads the compiler infos from a file."""

        if self.no_cache:
            self.add_default_compilers()
            return

        compilers = []

        glob = {}
        glob['version'] = version
        class Type(type):
            """Factory for CompilerInfo objects.
            This is used to read in an emulation file."""

            def __init__(cls, name, bases, dict):

                if glob['version'] == version:
                    compiler = CompilerInfo()
                    for name, value in CompilerInfo.__dict__.items():
                        if name[0] != '_':
                            setattr(compiler, name, dict.get(name, value))
                    compilers.append(compiler)


        if not filename:
            filename = CompilerList.user_emulations_file
        filename = os.path.expanduser(filename)
        glob['__builtins__'] = __builtins__
        glob['__name__'] = '__main__'
        glob['__metaclass__'] = Type
        try:
            execfile(filename, glob, glob)
        except IOError:

            self.add_default_compilers()
            self.save()
        else:
            self.compilers = compilers

    def save(self, filename = ''):
        
        if self.no_cache:
            return

        if not filename:
            filename = CompilerList.user_emulations_file
        filename = os.path.expanduser(filename)
        dirname = os.path.dirname(filename)
        if not os.path.exists(dirname):
            os.makedirs(dirname)
        emu = open(filename, 'wt')
        emu.write("""# This file was generated by Synopsis.Parsers.Cpp.Emulator.
# When making any manual modifications to any of the classes
# be sure to set the 'kind' field to 'custom' so it doesn't get
# accidentally overwritten !\n""")
        emu.write('\n')
        emu.write('version=%r\n'%version)
        emu.write('\n')
        for c in self.compilers:
            c._write(emu)
        

    def refresh(self):
        """Refreshes the compiler list.
        Regenerate all non-custom compilers without destroying custom
        compilers."""

        compilers = []
        for ci in self.compilers:
            if ci.is_custom:
                compilers.append(ci)
            ci = _query(ci.language, ci.compiler, ci.flags)
            if ci:
                compilers.append(ci)
                
        self.compilers = compilers
        self.save()

    def find(self, language, compiler, flags):

        if not flags:
            flags = []
        for ci in self.compilers:
            if (not compiler and language == ci.language
                or (compiler == ci.compiler and flags == ci.flags)):
                return ci
        ci = self._query(language, compiler, flags)
        self.compilers.append(ci)
        self.save()
        return ci


# Cache that makes multiple calls to 'get_compiler_info' more efficient.
compiler_list = None


def get_compiler_info(language, compiler = '', flags = None):
    """
    Returns the compiler info for the given compiler. If none is
    specified (''), return the first available one for the given language.
    The info is returned as a CompilerInfo object, or None if the compiler
    isn't found. 
    """
    global compiler_list

    if not compiler_list:
        compiler_list = CompilerList()

    ci = compiler_list.find(language, compiler, flags)
    return ci
