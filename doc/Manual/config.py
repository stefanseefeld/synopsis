from Synopsis.Config import Base
class Config (Base):
    class Parser (Base.Parser):
	class CXX (Base.Parser.CXX):
	    main_file = 1
	    basename = '../../'
	    preprocessor='gcc'
	class Python (Base.Parser.Python):
	    basename = '../../'
	modules = {
	    'C++':CXX,
	    'Py':Python
	}

    class Linker (Base.Linker):
	class Cxx (Base.Linker.Linker):
	    comment_processors = ['ssd']
	class CxxMap (Base.Linker.Linker):
	    map_declaration_names = 'Synopsis::Parser::C++', 'Package'
	class Py (Base.Linker.Linker):
	    pass
	class All (Base.Linker.Linker):
	    # For linking all the .syn files together
	    languagize = 0
	modules = {
	    'C++':Cxx,
	    'C++Final':CxxMap,
	    'Py':Py,
	    'All':All
	}

    class Formatter (Base.Formatter):
	class HTML (Base.Formatter.HTML):
	    toc_out = 'links.toc'
	    stylesheet_file = '../../demo/html.css'
	    pages = [
		'FilePages',
		'ModuleListing',
		'ScopePages',
		'ModuleIndexer',
		'FileTree',
		'InheritanceTree',
		'InheritanceGraph',
		'NameIndex',
		('modules.py', 'ConfScope'),
		'FramesIndex'
	    ]
	    tree_formatter = 'Synopsis.Formatter.HTML.TreeFormatterJS.TreeFormatterJS'

	    # Add custom comment formatter
	    comment_formatters = [
		'summary', 'javadoc', 'section',
		('modules.py', 'RefCommentFormatter')
	    ]

	    class FilePages:
		"Override defaults"
		file_path = '../../../%s'
		links_path = 'syn/%s-links'
		toc_files = ['links.toc']
		scope = 'Synopsis::Parser::C++::'
	    class FileTree:
		link_to_pages = 1
	    class ScopePages (Base.Formatter.HTML.ScopePages):
		summary_formatters = [
		    ('Synopsis.Formatter.HTML.ASTFormatter','SummaryASTFormatter'),
		    ('Synopsis.Formatter.HTML.ASTFormatter','SummaryASTCommenter'),
		    ('Synopsis.Formatter.HTML.ASTFormatter','FilePageLinker'),
		]
	    class ModuleListing (Base.Formatter.HTML.ModuleListing):
		child_types = ['Package']
	class ConfigHTML (HTML):
	    pages = [
		('modules.py', 'ConfScope'),
		('modules.py', 'ConfScopePage')
	    ]
	    synopsis_pages = pages
	class HTML_Doxygen (Base.Formatter.HTML_Doxygen):
	    stylesheet_file = '../../demo/html-doxy.css'
	    class ScopePages (Base.Formatter.HTML_Doxygen.ScopePages):
		summary_formatters = Base.Formatter.HTML_Doxygen.ScopePages.summary_formatters
		summary_formatters.insert(3,'FilePageLinker')
	    def __init__(self, argv):
		Base.Formatter.HTML_Doxygen.__init__(self, argv)
		# Import the config from HTML
		for attr in ['toc_output','pages','comment_formatters','FilePages','FileTree']:
		    setattr(self, attr, getattr(Config.Formatter.HTML, attr))
	modules = Base.Formatter.modules
	modules.update({
	    'HTML':HTML,
	    'Doxygen':HTML_Doxygen,
	    'ConfigHTML':ConfigHTML
	})
