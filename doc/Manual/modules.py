# Custom modules for the Reference Manual

import re, os, os.path

import Synopsis.Formatter.HTML.CommentFormatter
import Synopsis.Formatter.HTML.ScopePages
import Synopsis.Formatter.HTML.ModuleListing
from Synopsis.Core import AST, Type
from Synopsis.Formatter import HTML
from Synopsis.Formatter.HTML.Tags import *
config = HTML.core.config

class RefCommentFormatter (HTML.CommentFormatter.CommentFormatterStrategy):
    "A formatter that formats <heading> and <example> tags"
    __re_heading = r'<heading>(.*)</heading>'
    __re_example = r'<example>(.*)</example>'
    __heading = '<h2>%s</h2>'
    __example = '<blockquote><tt>%s</tt></blockquote>'
    def __init__(self):
	self.re_heading = re.compile(self.__re_heading)
	self.re_example = re.compile(self.__re_example)
    def format(self, page, decl, text):
	"Format heading and example tags in the given comment's detail section"
	if not text: return
	text = self.parse_tag(text, self.re_heading, self.__heading)
	text = self.parse_tag(text, self.re_example, self.__example)
	return text
    def parse_tag(self, str, regexp, replace):
	"Use regexp to do search&replace in str with replace"
	ret = ''
	mo = regexp.search(str)
	while mo:
	    text = replace % mo.group(1)
	    ret = ret + str[:mo.start()] + text
	    str = str[mo.end():]
	    mo = regexp.search(str)
	return ret + str
	    
class ConfScopePage (HTML.ScopePages.ScopePages):
    def process(self, start):
	"""Creates one page for all Scopes"""
	self.startFileScope(start.name())
	self.__namespaces = [start]
	while self.__namespaces:
	    ns = self.__namespaces.pop(0)
	    self.processScope(ns)
	self.endFile()

    def processScope(self, ns):
	"""Creates a page for the given scope"""
	details = {} # A hash of lists of detailed children by type
	sections = [] # a list of detailed sections
	
	# Open file and setup scopes
	#self.startFileScope(ns.name())
	config.sorter.set_scope(ns)
	config.sorter.sort_section_names()
	
	self.summarizer.set_scope(ns.name())
	self.detailer.set_scope(ns.name())

	# Write heading
	self.write(self.manager.formatRoots('')+'<hr>')
	if ns is self.manager.globalScope(): 
	    self.write(entity('h1', "Global Namespace"))
	else:
	    # Use the detailer to print an appropriate heading
	    ns.accept(self.detailer)
	
	# Loop throught all the types of children
	self.printScopeSummaries(ns, details, sections)
	self.printScopeDetails(details, sections)
	#self.endFile()
	
	# Queue child namespaces
	for child in config.sorter.children():
	    if isinstance(child, AST.Scope):
		self.__namespaces.append(child)
 
class ConfScope (HTML.ModuleListing.ModuleListing):
    """Creates a page with a Tree of Config classes. This code is based on the
    ModuleListing page, via a few template methods. It is to aide the
    process of finding config information by only showing the tree that exists
    under Config.py."""

    def filename(self): return config.files.nameOfSpecial('ConfScopes')
    def title(self): return 'Config'

    def register(self):
	"""Initialise with the special Config name"""
	self.manager.addRootPage(self.filename(), 'Config', 'index', 1)
	self._link_target = 'main'
	#config.set_index_page(self.__filename)
    def _link_href(self, ns):
	"""Template method to return the href of a link"""
	return rel(self.filename(), config.files.nameOfScope(ns.name()))
    def process(self, start):
	"""Decorate the process() method to set the start"""
	config.sorter.set_scope(start)
	#start = config.sorter.child(('Python Namespace',))
	start = self.manager.calculateStart(start, "Synopsis::Config")
	HTML.ModuleListing.ModuleListing.process(self, start)
    def _child_filter(self, child):
	"""Override template method to display all scopes (not just modules)"""
	return isinstance(child, AST.Scope)
