class Command:

    def __init__(self, *args, **kwds):

        print args
        print kwds

    def execute(self):
        
        print 'hello world'
