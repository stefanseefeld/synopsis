from Synopsis.Config import Base
class Config (Base):
    class Formatter (Base.Formatter):
	class HTML (Base.Formatter.HTML):
	    toc_output = 'links.toc'
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
	    synopsis_pages = pages

	    # Add custom comment formatter
	    comment_formatters = [
		'summary', 'javadoc', 'section',
		('modules.py', 'RefCommentFormatter')
	    ]
	    # Also use it when either style is specified:
	    synopsis_comment_formatters = comment_formatters
	    doxygen_comment_formatters = comment_formatters

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
	modules = {
	    'HTML':HTML,
	    'ConfigHTML':ConfigHTML
	}
