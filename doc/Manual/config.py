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
		'FramesIndex'
	    ]

	    class FilePages:
		"Override defaults"
		file_path = '../../../%s'
		links_path = 'syn/%s-links'
		toc_files = ['links.toc']
	    class FileTree:
		link_to_pages = 1
	modules = {'HTML':HTML}
