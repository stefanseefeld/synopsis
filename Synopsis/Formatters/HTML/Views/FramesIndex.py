import Page
from core import config
from Tags import *

class FramesIndex (Page.Page):
    """A class that creates an index with frames"""
    # TODO figure out how to set the default frame contents
    def __init__(self, manager):
	Page.Page.__init__(self, manager)

    def process(self, start):
	"""Creates a frames index file"""
	me = config.files.nameOfIndex()
	# TODO use project name..
	self.startFile(me, "Synopsis - Generated Documentation", body='')
	fcontents = rel(me, config.page_contents)
	findex = rel(me, config.page_index)
	fglobal = rel(me, config.files.nameOfScope(start.name()))
	frame1 = solotag('frame', name='contents', src=fcontents)
	frame2 = solotag('frame', name='index', src=findex)
	frame3 = solotag('frame', name='main', src=fglobal)
	frameset1 = entity('frameset', frame1+frame2, rows="30%,*")
	frameset2 = entity('frameset', frameset1+frame3, cols="200,*")
	self.write(frameset2)
	self.endFile()


htmlPageClass = FramesIndex
