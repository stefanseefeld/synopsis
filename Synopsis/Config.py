#  $Id: Config.py,v 1.9 2002/01/13 09:47:59 chalky Exp $
#
#  This file is a part of Synopsis.
#  Copyright (C) 2000, 2001 Stefan Seefeld
#
#  Synopsis is free software; you can redistribute it and/or modify it
#  under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
#  02111-1307, USA.
"""Configuration script module.
There are a large number of command line options to control Synopsis, but
there are many more options than it is feasable to implement in this fashion.
Rather, Synopsis opts for the config file approach if you want full control
over the process.

<heading>The problem</heading>
Synopsis is very modular, and it is desirable to separate the options from
different modules - something achieved by the -W flag. The modules form a
hierarchical structure however, with up to three levels of depth. Some modules
are used rarely, and the user may want different sets of settings depending on
what they are doing - eg: generating HTML or Docbook, parsing different
sections of the code, etc. We tossed about various ideas of representing
options in an related way, and came up with the idea of a Python script that
builds an object hierarchy that mirrors that of the modules.

<heading>The Config Script</heading>
A config script is specified by passing the -c option to 'synopsis'. Options may
be passed to the script via the -Wc option, as key=value pairs. For example:

<example>synopsis -c config.py -Wc,formatter=C++</example>

The script is interpreted as a Python source file, and the only requirement is
that once loaded it have a global object called Config. This Config object is
usually a class which is constructed with the options dictionary to retrieve an
object with nested configuration objects. Every config object has at least one
attribute 'name', which identifies the module the object configures.

If no config script is specified on the command line, the default class Base
is used, defined in this Config module. It is recommended that the Config
class in your config script derive from this Base class to extend it as you
wish.

<heading>Modules</heading>
In many places modules or lists of modules can be specified to perform tasks.
These may be selected in one of four ways: 
    
1. From a list of built-in defaults for that task as a simple string (depends
on attribute), 

2. As a member of a module or package as a simple string (depends on
attribute),

3. From anywhere via a tuple of two strings: (module, class-name); for
example, to use the provided DOxygen Scope Sorter, you specify
('Synopsis.Formatter.HTML.doxygen', 'DOScopeSorter') or to use your own
something like ('mymodules.py', 'ScopeSorter') - Note that ending the first
string in '.py' forces it to be loaded from a file, something which cannot be
achieved using the next method:

4. From anywhere via an absolute dotted reference, eg:
    'Synopsis.Formatter.HTML.doxygen.DOScopeSorter'.

Of these methods, 1 or 2 are preferable for brevity and 4 for absolute
references, unless a filename is needed in which case you need 3.

@see Synopsis.Config.Base
"""

import sys

prefix = '@prefix@'

class Base:
    """The base class for configuration objects. 
    If no config script is specified on the command line, then this class is
    instantiated directly.
    @attr parser The parser config object to use, or None if no parsing should
    be done. This attribute is set by __init__()
    @attr linker The linker config object to use, or None if no linking should
    be done. This attribute is set by __init__()
    @attr formatter The formatter config object to use, or None if no
    formatting should be done. This attribute is set by __init__()
    @see Base.__init__()
    """
    name = 'synopsis'
    parser = None
    linker = None
    formatter = None
    class Parser:
	"""Contains nested classes for the different Parser modules.
	@attr modules a dictionary mapping names to config objects. This
	dictionary may be used to select an object from a command line option.
	"""
        class IDL:
	    """Config object for the IDL parser.
	    @attr name Name of this config object: 'IDL'
	    @attr verbose Verbosity flag. This attribute is set by __init__(),
	    but only if 'verbose' was passed as a config option.
	    @attr main_file Flag that selects if should only store the AST
	    generated from the file(s) being processed, and not included
	    files. This attribute is true by default.
	    @attr basename A file path to strip from the start of all
	    filenames before storing. Setting this option will for example
	    remove any redundant parent directories in the HTML FileTree page.
	    @attr include_path A list of strings, each specifying a path to
	    add to the include path. For example: ['/usr/local/corba/']
	    @attr keep_comments If set to true (ie: 1) then comments are kept
	    in the documentation. This is true by default.
	    @see Synopsis.Parser.IDL
	    """
            name = 'IDL'
	    keep_comments = 1
	    main_file = 1
            def __init__(self, argv):
                if argv.has_key('verbose'): self.verbose = 1
        class CXX:
	    """Config object for the C++ parser.
	    @attr name Name of this config object: 'C++'
	    @attr verbose Verbosity flag. This attribute is set by __init__(),
	    but only if 'verbose' was passed as a config option.
	    @attr main_file Flag that selects if should only store the AST
	    generated from the file(s) being processed, and not included
	    files. This attribute is set by __init__ always
	    @attr basename A file path to strip from the start of all
	    filenames before storing. Setting this option for example will
	    remove any redundant parent directories in the HTML FileTree page.
	    @attr include_path A list of strings, each specifying a path to
	    add to the include path. For example: ['/usr/local/corba/']
	    @attr defines A list of strings, each specifying a define to pass
	    to the preprocessor. For example: ['FOO', 'BAR=true']
	    @attr preprocessor Which preprocessor to use. Not setting this
	    causes the builtin ucpp to be used, which can track macro
	    expansions when doing SXR stuff. Setting it to 'gcc' will cause
	    gcc (well, really g++) to be used instead.
	    @attr extract_tails If set to true, then the parser will look for
	    trailing comments before close braces. If it finds them, it will
	    create dummy declarations to hold the comments. If you set this,
	    you should probably also use the 'dummy' or 'prev' comment
	    processors in the Linker options.
	    @attr storage If set, this must be a string which defines the file
	    to store links into. Setting this also causes the parser to look
	    more carefully at the input file, so that it can generate the
	    links from inside blocks of code (otherwise it skips over them).
	    Note that you usually set this from the command-line with your
	    Makefile via "-Wp,-s,$@.links" or something.
	    @attr fake_std If set, this causes the parser to construct a fake
	    using directive from the std namespace to the global namespace. In
	    effect, this fixes problems seen with using the stdc++ headers for
	    gcc 2.95.x where most things dont get placed in the std namespace.
	    @see Synopsis.Parser.C++.__init__
	    """
            name = 'C++'
	    include_path = []
            def __init__(self, argv):
                if argv.has_key('verbose'): self.verbose = 1
                if not hasattr(self, 'main_file'):
		    self.main_file = 1
        class Python:
	    """Config object for the Python parser.
	    @attr name Name of this config object: 'Python'
	    @attr verbose Verbosity flag. This attribute is set by __init__(),
	    but only if 'verbose' was passed as a config option.
	    @attr main_file Flag that selects if should only store the AST
	    generated from the file(s) being processed, and not included
	    files. This attribute is set by __init__ always
	    @see Synopsis.Parser.Python
	    """
            name = 'Python'
            def __init__(self, argv):
                if argv.has_key('verbose'): self.verbose = 1
                self.main_file = 1

        modules = {'C++': CXX,
                   'IDL': IDL,
                   'Python': Python}
    
    class Linker:
	"""Contains nested classes for the Linker modules. Currently there is
	just Linker.
	@attr modules Dictionary mapping names to nested classes
	@see Synopsis.Linker
	@see Synopsis.Linker.Linker The main linker module
	@see Synopsis.Linker.Comments The comment linker module
	"""
        class Linker:
	    """Config for the main linker module.
	    @attr map_declaration_names Prepends some namespace onto each name
	    in the AST. This is used for example by the Synopsis RefManual to
	    place all C++ code in the Synopsis.Parser.C++ package. The value
	    should be a tuple of two strings - the namespace separated by ::'s
	    and the type of module to use (eg: Module or Package)
	    """
            name = 'Linker'
            def __init__(self, argv):
                if argv.has_key('verbose'): self.verbose = 1

        modules = {'Linker': Linker}

    class Formatter:
	"""Contains nested classes for the different Formatter modules.
	@attr modules a dictionary mapping names to config objects. This
	dictionary may be used to select an object from a command line option.
	"""
        class HTML:
	    """Config object for HTML Formatter.
	    This is the most complicated config object, because the HTML
	    formatter is itself very modular and flexible, allowing the user
	    to customise output at many levels. All this flexibility comes at
	    a price however - but if you read this (and send feedback to the
	    Synopsis developers) then it should get easier.
	    @attr name The name of the module, ie: 'HTML'
	    @attr stylesheet The filename of the stylesheet file linked to by all
	    generated pages.
	    @attr stylesheet_file If this attribute is set, then it specifies
	    a file which is read and written over the stylesheet file in the
	    output directory. It is only copied if it has a newer timestamp
	    than the existing stylesheet file.
	    @attr pages Lists the names of page modules to load and process in
	    order. Each module generates a different type of output, and some
	    register names to be shown as a link at the top of every page. For
	    example, ScopePages generates the pages for Classes, Modules, etc.
	    ModuleIndexer creates the index pages for all the modules that
	    usually go in the bottom-left frame. FramesIndex creates the
	    index.html with the frames. The default is ['ScopePages',
	    'ModuleListingJS', 'ModuleIndexer', 'FileTreeJS',
	    'InheritanceTree', 'InheritanceGraph', 'NameIndex', 'FramesIndex']
	    @attr comment_formatters Lists the formatters to be applied to
	    all comments. The default is ['summary', 'javadoc', 'section'].
	    The summary formatter splits comments into 'summary' and 'detail'
	    sections by looking for the first sentence. Javadoc formats
	    javadoc-style @tags. Section splits blank lines into paragraph
	    breaks.
	    @attr sorter Specifies the Sorter to be used to sort declarations
	    on Scope Pages. You may set this to override the default sorting,
	    for example using ('Synopsis.Formatter.HTML.doxygen',
	    'DOScopeSorter')
	    @attr tree_formatter Specifies which tree formatter to use. There
	    is a default package of 'Synopsis.Formatter.HTML' and the default
	    value is 'TreeFormatter.TreeFormatter'
	    @attr file_layout Specifies the file layout to use. This must be a
	    class that implements the FileLayout interface defined in the
	    HTML.FileLayout module.
	    @see Synopsis.Formatter.HTML The HTML Formatter
	    @see Synopsis.Formatter.HTML.core The main HTML module
	    """
            name = 'HTML'
	    # Defaults
            datadir = prefix + '/share/synopsis'
	    stylesheet = 'style.css'
	    file_layout = 'Synopsis.Formatter.HTML.FileLayout.FileLayout'
            pages = ['ScopePages',
                              'ModuleListing',
                              'ModuleIndexer',
                              'FileTree',
                              'InheritanceTree',
                              'InheritanceGraph',
			      'NameIndex',
                              'FramesIndex']
            comment_formatters = ['summary', 'javadoc', 'section']
	    tree_formatter = 'TreeFormatter.TreeFormatter'
	    class FilePages:
		"""This is the config object for the FilePages module.
		FilePages creates html pages that contain the actual source
		code for the program, which depending on the language may be
		highlighted and hyperlinked. Currently only the C++ parser
		provides this - other languages are displayed without
		formatting.
		
		The formatting information is stored in a '.links'
		file for each input file, and since the location is specific
		to your project you must set this here, and FilePages is not
		in the default list of Page modules to use.

		@attr file_path A string with one %s, which when replaced with the
		'filename()' attribute (ie: filename with the basepath
		stripped from it by the parser) will give the input filename.
		The default is './%s'.
		@attr links_path is the same as file_path, but must give the
		'links' file. The default is './%s-links'
		@attr toc_files is a list of '.toc' files to load and use to
		link references from the file.
		@attr scope is the base scope to prepend to all names for
		looking up in the TOC. Eg: the RefManual for Synopsis maps all
		C++ things to Synopsis::Parser::C++::, so that string must be
		set here and prepended to all names to look up in the TOC.
		@see Synopsis.Formatter.HTML.FilePages
		"""
		file_path = './%s'
		links_path = './%s-links'
		toc_files = []
		scope = ''
	    class FileTree:
		"""Config object for the FileTree module.
		FileTree creates a page with a tree of filenames, for use in
		the top-left frame.
		@attr link_to_pages If true, then links are generated to the
		hyperlinked source files.
		@see Synopsis.Formatter.HTML.FileTree
		"""
		link_to_pages = 0
	    class ScopePages:
		"""Config for ScopePages module. ScopePages is the module that
		creates the bulk of the documentation - the pages for modules
		and classes, with summary and detail sections for each type of
		ast node. ScopePages is very modular, and all the modules it
		uses are in the ASTFormatter and FormatStrategy modules, which
		is where it looks if you use the 'simple' module spec.
		(FIXME)

		@param parts modules to use to generate parts of each page.
		The default package is Synopsis.Formatter.HTML.ASTFormatter
		and the default list is 'Heading','Summary', and 'Detail'

		@param heading_formatters list of modules for Heading page
		Part. The default is 'Heading', 'ClassHierarchyGraph', and
		'DetailCommenter'.

		@param summary_formatters list of modules for SummaryFormatter
		to use. The default is ['SummaryAST', 'SummaryCommenter']

		@param detail_formatters list of modules for DetailFormatter
		to use. The default is ['DetailAST',
		'ClassHierarchyGraph', 'DetailCommenter']. Note that it is
		these modules that are used to display the top of the class
		and module pages (hence the ClassHierarchyGraph module, which
		shows class hierarchies at the top of class pages).

		@see Synopsis.Formatter.HTML.ScopePages
		@see Synopsis.Formatter.HTML.ASTFormatter
		"""
		parts = [
		    'Heading',
		    'Summary',
		    'Inheritance',
		    'Detail'
		]
		heading_formatters = [
		    'Heading',
		    'ClassHierarchyGraph',
		    'DetailCommenter'
		]
		summary_formatters = [
		    'SummaryAST',
		    'SummaryCommenter'
		]
		detail_formatters = [
		    'DetailAST',
		    'DetailCommenter'
		]
	    class InheritanceGraph:
		"""Config for InheritanceGraph module.
		@attr min_size Minimum grouping size of graphs. The graph is
		split into subgraphs because graphviz can makes graphs tens of
		thousands of pixels wide otherwise. The time to process the
		graphs is proportional to the number of times graphviz is run,
		so setting this attribute will attempt to group smaller graphs
		into larger graphs with a minimum number of nodes. The default
		is 5.
		@see Synopsis.Formatter.HTML.InheritanceGraph.InheritanceGraph
		"""
		min_size = 5

	    class ModuleListing:
		"""Config for ModuleListing module.
		@attr child_types The types of children to include in the
		module listing. Children are found by looking for Module AST
		nodes, but their (string) type may depend on other factors
		(eg: Python creates Package and Module types). The default is
		to just print all Module AST nodes.
		"""
		pass

            def __init__(self, argv):
		"""Initialise HTML config object.
		If there is a verbose argument, then the verbose attribute is
		set to true, causing various verbose information to be printed
		out.
		@param argv a dictionary of key:value arguments
		"""
                if argv.has_key('verbose'): self.verbose = 1

	class HTML_Doxygen (HTML):
	    """Doxygen-style HTML. This Config class actually derives from
	    the HTML class but overrides a few options to provide an output
	    that looks more like Doxygen's output. You may use this via
	    something like:
		<example>synopsis -Wcformatter=HTML_Doxygen</example>
	    @see HTML The parent HTML config
	    """
	    # Doxygen-style options
	    sorter = ('Synopsis.Formatter.HTML.doxygen', 'DOScopeSorter')
	    class ScopePages:
		"""Overrides the default ScopePages with doxygen modules."""
		summarizer = ('Synopsis.Formatter.HTML.doxygen', 'DOSummary')
		detailer = ('Synopsis.Formatter.HTML.doxygen', 'DODetail')
		summary_formatters = [
		    ('Synopsis.Formatter.HTML.doxygen', 'DOSummaryAST'),
		    ('Synopsis.Formatter.HTML.doxygen', 'PreSummaryDiv'),
		    ('Synopsis.Formatter.HTML.doxygen', 'DOSummaryCommenter'),
		    ('Synopsis.Formatter.HTML.doxygen', 'PostSummaryDiv'),
		]
		detail_formatters = [
		    ('Synopsis.Formatter.HTML.doxygen', 'PreDivFormatter'),
		    ('Synopsis.Formatter.HTML.doxygen', 'DODetailAST'),
		    ('Synopsis.Formatter.HTML.doxygen', 'PostDivFormatter'),
		    ('Synopsis.Formatter.HTML.FormatStrategy', 'ClassHierarchyGraph'),
		    ('Synopsis.Formatter.HTML.FormatStrategy', 'DetailCommenter'),
		]

	class DocBook:
	    name = 'DocBook'
	    def __init__(self, argv): pass

	class TexInfo:
	    name = 'TexInfo'
	    def __init__(self, argv): pass

	class Dot:
	    name = 'Dot'
	    def __init__(self, argv): pass

	class HTML_Simple:
	    name = 'HTML_Simple'
	    def __init__(self, argv): pass

	class ASCII:
	    name = 'ASCII'
	    def __init__(self, argv): pass

	class DUMP:
	    name = 'DUMP'
	    def __init__(self, argv): pass

	class Dia:
	    name = 'Dia'
	    def __init__(self, argv): pass

        modules = {
            'HTML': HTML, 'DocBook':DocBook, 'TexInfo':TexInfo, 'Dot':Dot,
	    'HTML_Simple':HTML_Simple, 'ASCII':ASCII,
	    'DUMP':DUMP, 'Dia':Dia,
	    'HTML_Doxygen':HTML_Doxygen
	}

    def __init__(self, argv):
	"""Initialise the Config object.
	The argv dictionary is used to fill in the attributes 'parser',
	'linker' and 'formatter'. For example, if the dictionary contains a
	parser argument, then its value is used to select the parser to use
	by setting self.parser to the config object for that parser. The
	modules are selected from the 'modules' dictionary attribute of the
	Parser, Linker and Formatter nested classes.
	@param argv A dictionary of keyword arguments passed to synopsis via
	-Wc
	"""
        self.parser = None
        if 'parser' in argv.keys():
            parser = argv['parser']
            try:
		parsers = self.Parser.modules
                self.parser = parser and \
                              parsers[parser](argv) or \
                              parsers.values()[0](argv)
            except KeyError:
                sys.stderr.write("Synopsis.Config: parser '" + parser + "' unknown\n")
                sys.exit(-1)
        self.linker = None
        if 'linker' in argv.keys():
            linker = argv['linker']
            try:
		linkers = self.Linker.modules
                self.linker = linker and \
                              linkers[linker](argv) or \
                              linkers.values()[0](argv)
            except KeyError:
                sys.stderr.write("Synopsis.Config: linker '" + linker + "' unknown\n")
                sys.exit(-1)
        self.formatter = None
        if 'formatter' in argv.keys():
            formatter = argv['formatter']
            try:
		formatters = self.Formatter.modules
                self.formatter = formatter and \
                                 formatters[formatter](argv) or \
                                 formatters.values()[0](argv)
            except KeyError:
                sys.stderr.write("Synopsis.Config: formatter '" + formatter + "' unknown\n")
                sys.exit(-1)
