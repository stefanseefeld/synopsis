from Synopsis.Config import Base
class Config (Base):
    class Formatter (Base.Formatter):
	class HTML (Base.Formatter.HTML):
	    toc_output = 'links.toc'
	    stylesheet_file = '../../demo/html.css'
	    pages = [
		'ScopePages',
		'ModuleListingJS',
		'ModuleIndexer',
		'FileTreeJS',
		'InheritanceTree',
		'InheritanceGraph',
		'NameIndex',
		'FilePages',
		('modules.py', 'ConfScopeJS'),
		'FramesIndex'
	    ]

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
	    class FileTree:
		link_to_pages = 1
	    class ScopePages (Base.Formatter.HTML.ScopePages):
		summary_formatters = [
		    ('Synopsis.Formatter.HTML.ASTFormatter','SummaryASTFormatter'),
		    ('Synopsis.Formatter.HTML.ASTFormatter','SummaryASTCommenter'),
		    ('Synopsis.Formatter.HTML.ASTFormatter','FilePageLinker'),
		]
	class ConfigHTML (HTML):
	    pages = [
		('modules.py', 'ConfScopeJS'),
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
	modules = {
	    'HTML':HTML,
	    'Doxygen':HTML_Doxygen,
	    'ConfigHTML':ConfigHTML
	}
