from Synopsis.Config import Base

class Config(Base):
    class Linker(Base.Linker):
        class SS(Base.Linker.Linker):
            comment_processors = ['ss']
        class Group(Base.Linker.Linker):
            comment_processors = ['group']
        class Dummy(Base.Linker.Linker):
            comment_processors = ['dummy']
        class Prev(Base.Linker.Linker):
            comment_processors = ['prev']
        modules = {'SS': SS,
                   'Group': Group,
                   'Dummy': Dummy,
                   'Prev': Prev}
