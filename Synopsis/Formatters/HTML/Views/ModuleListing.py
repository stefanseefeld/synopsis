# Synopsis modules
from Synopsis.Core import AST, Util

# HTML modules
import Page
import core
from core import config
from Tags import *

class ModuleListing(Page.Page): 
    """Create an index of all modules."""
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__filename = config.files.nameOfSpecial('module_listing')
	link = href(self.__filename, 'Modules', target="contents")
	manager.addRootPage('Modules', link, 2)
	config.set_contents_page(self.__filename)

    def process(self, start):
	"""Create a page with an index of all modules"""
	self.startFile(self.__filename, "Module Index")
	self.write(self.manager.formatRoots('Modules', 2))
	self.write('<hr>')
	self.indexModule(start, start.name())
	self.endFile()

    def indexModule(self, ns, rel_scope):
	"Write a link for this module and recursively visit child modules."
	my_scope = ns.name()
	# Print link to this module
	name = Util.ccolonName(my_scope, rel_scope) or "Global Namespace"
	link = config.files.nameOfModuleIndex(ns.name())
	self.write(href(link, name, target='index') + '<br>')
	# Add children
	config.sorter.set_scope(ns)
	config.sorter.sort_sections()
	for child in config.sorter.children():
	    if isinstance(child, AST.Module):
		self.write('<div class="module-section">')
		self.indexModule(child, my_scope)
		self.write('</div>')

htmlPageClass = ModuleListing
