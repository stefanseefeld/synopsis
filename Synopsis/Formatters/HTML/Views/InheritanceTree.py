import Page
from core import config
from Tags import *

class InheritanceTree(Page.Page):
    def __init__(self, manager):
	Page.Page.__init__(self, manager)
	self.__filename = config.files.nameOfSpecial('tree')
	link = href(self.__filename, 'Inheritance Tree', target='main')
	manager.addRootPage('Inheritance Tree', link, 1)
 
    def process(self, start):
	"""Creates a file with the inheritance tree"""
	classes = config.classTree.classes()
	def root(name): return not config.classTree.superclasses(name)
	roots = filter(root, classes)

	self.startFile(self.__filename, "Synopsis - Class Hierarchy")
	self.write(entity('h1', "Inheritance Tree"))
	self.write('<ul>')
	map(self.processClassInheritance, map(lambda a,b=start.name():(a,b), roots))
	self.write('</ul>')
	self.endFile()   

    def processClassInheritance(self, args):
	name, rel_name = args
	self.write('<li>')
	self.write(config.toc.referenceName(name, rel_name))
	parents = config.classTree.superclasses(name)
	if parents:
	    self.write(' <i>(%s)</i>'%string.join(map(Util.ccolonName, parents), ", "))
	subs = config.classTree.subclasses(name)
	if subs:
	    self.write('<ul>')
	    map(self.processClassInheritance, map(lambda a,b=name:(a,b), subs))
	    self.write('</ul>\n')
	self.write('</li>')

htmlPageClass = InheritanceTree
