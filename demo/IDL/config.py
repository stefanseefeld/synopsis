# Config file for IDL demos
# The IDLs all have comments in //. style

from Synopsis.Config import Base

class Config (Base):
    class Parser:
	class IDL (Base.Parser.IDL):
	    include_path = ['.']
	modules = {
	    'IDL':IDL,
	}
	    
    class Linker:
	class Linker (Base.Linker.Linker):
	    comment_processors = ['ssd']
	modules = {
	    'Linker':Linker,
	}

    class Formatter:
	class HTML (Base.Formatter.HTML):
	    stylesheet_file = '../html.css'
	    def __init__(self, argv):
		"force style to be synopsis"
		argv['style'] = 'synopsis'
		Base.Formatter.HTML.__init__(self, argv)
	modules = Base.Formatter.modules
	modules['HTML'] = HTML

