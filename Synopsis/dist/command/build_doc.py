import os, sys, string, re, stat
from stat import *
import os.path
from shutil import *
import glob

from distutils.command import build
from distutils.spawn import spawn, find_executable
from distutils.dep_util import newer, newer_group
from distutils.dir_util import copy_tree, remove_tree
from distutils.file_util import copy_file
from distutils import sysconfig

class Target:
    def __init__(self, cmd, rule, dependencies, **kw):

        self.cmd = cmd
        self.rule = rule
        self.dependencies = dependencies
        self.dict = kw
        self.mtime = 0
        # assume output is a single file
        if kw.has_key('output') and os.path.exists(kw['output']):
            self.mtime = os.stat(kw['output'])[ST_MTIME]

    def announce(self, msg):
        self.cmd.announce(msg)

    def needs_update(self):
        """Return true if any of the dependencies was modified after self"""
        latest = 0
        for d in self.dependencies:
            if isinstance(d, Target):
                if d.mtime > latest: latest = d.mtime
            else: # assume a filename
                mtime = os.stat(d)[ST_MTIME]
                if mtime > latest: latest = mtime
        # always consider input as an implicit dependency
        if self.dict.has_key('input'):
            for i in self.dict['input']:
                mtime = os.stat(i)[ST_MTIME]
                if mtime > latest: latest = mtime
        return latest > self.mtime
    
    def process(self):
        for d in self.dependencies:
            d.process()
        if self.rule and self.needs_update():
            dict = self.dict.copy()
            # if input is given, it is a list, so convert it to a string here
            if self.dict.has_key('input'):
                dict['input'] = string.join(self.dict['input'])
            command = self.rule%dict
            self.announce(command)
            spawn(string.split(command))

class build_doc(build.build):
    """Defines the specific procedure to build synopsis' documentation."""

    description = "build documentation"

    def run(self):
        """Run this command, i.e. do the actual document generation."""

        self.manual()
    
    def manual(self):

        synopsis = "synopsis -c config.py"
        py_sources = []
        cxx_sources = []
        py = re.compile(r'.*\.py$')
        # only parse C++ code for now
        #cxx = re.compile(r'((.*)\.(hh|h|cc)$)')
        cxx = re.compile(r'((.*)\.(hh|cc)$)')
        def add_py(arg, dirname, names):
            # exclude the dist stuff for now,
            # or else a synopsis bug will be triggered
            if dirname == 'Synopsis/dist/command':
                return
            arg.extend(map(lambda f, d=dirname: os.path.join(d, f),
                           filter(lambda f, re=py: re.match(f), names)))
        def add_cxx(arg, dirname, names):
            # only parse the C++ parser for now
            if dirname[:19] != 'Synopsis/Parser/C++':
                return
            arg.extend(map(lambda f, d=dirname: os.path.join(d, f),
                           filter(lambda f, re=cxx: re.match(f), names)))
        def add_hh(arg, dirname, names):
            arg.extend(map(lambda f, d=dirname: os.path.join(d, f),
                           filter(lambda f, re=hh: re.match(f), names)))
        def add_cc(arg, dirname, names):
            arg.extend(map(lambda f, d=dirname: os.path.join(d, f),
                           filter(lambda f, re=cc: re.match(f), names)))

        os.path.walk('Synopsis', add_py, py_sources)
        os.path.walk('Synopsis', add_cxx, cxx_sources)
        
        cwd = os.getcwd()
        os.chdir(os.path.normpath('docs/RefManual'))
        command = synopsis + " -Wc,parser=Py,linker=Py -o %(output)s %(input)s"
        py_syns = map(lambda f,s=self:Target(s, command, [],
                                             output=re.sub('\.py$', '.syn', f),
                                             input=[os.path.join('..','..',f)]),
                      py_sources)
        py_syn = Target(self, synopsis + " -o %(output)s %(input)s", py_syns,
                        output='py.syn', input=map(lambda f:re.sub('\.py$', '.syn', f),
                                                   py_sources))
        command = synopsis + " -I ../../Synopsis/Parser/C++ -I ../../Synopsis/Parser/C++/gc/include -I " + sysconfig.get_python_inc()
        command += " -Wc,parser=C++,linker=C++ -Wp,-s,syn/%s-links,%s"
        cxx_syns = map(lambda f,s=self:Target(s, command%(f[0], f[1]) + " -o %(output)s %(input)s", [],
                                              output=f[0] + ".syn",
                                              input=[os.path.join('..','..',f[0])]),
                       # this maps <file> to (<file>, <stem>, <ext>)
                       map(lambda f:cxx.match(f).groups(), cxx_sources))
        cxx_syn = Target(self, synopsis + " -Wc,linker=C++Final -o %(output)s %(input)s", cxx_syns,
                         output='c++.syn',
                         input=map(lambda f:f + ".syn", cxx_sources))

        core_ast_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Core::AST' -o %(output)s %(input)%",
                              [py_syn], output="core-ast.syn", input=['py.syn'])
        core_type_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Core::Type' -o %(output)s %(input)%",
                               [py_syn], output="core-type.syn", input=['py.syn'])
        core_util_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Core::Util' -o %(output)s %(input)%",
                               [py_syn], output="core-util.syn", input=['py.syn'])
        parser_cxx_py_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Parser::C++' -o %(output)s %(input)%",
                                   [py_syn], output="parser-c++-py.syn", input=['py.syn'])
        parser_cxx_cpp_syn = Target(self, synopsis + " -Wc,linker=All -o %(output)s %(input)%",
                                    [py_syn], output="parser-c++-cpp.syn", input=['py.syn'])
        parser_cxx_syn = Target(self, synopsis + " -Wc,linker=All -o %(output)s %(input)%",
                                [parser_cxx_py_syn, parser_cxx_cpp_syn], output="parser-c++.syn", input=['py.syn'])
        parser_idl_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Parser::IDL' -o %(output)s %(input)%",
                                [py_syn], output="parser-idl.syn", input=['py.syn'])
        parser_py_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Parser::Python' -o %(output)s %(input)%",
                               [py_syn], output="parser-py.syn", input=['py.syn'])
        linker_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Linker' -o %(output)s %(input)%",
                            [py_syn], output="linker.syn", input=['py.syn'])
        formatter_ascii_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Formatter::ASCII' -o %(output)s %(input)%",
                                     [py_syn], output="formatter-ascii.syn", input=['py.syn'])
        formatter_html_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Formatter::HTML' -o %(output)s %(input)%",
                                    [py_syn], output="formatter-html.syn", input=['py.syn'])
        formatter_dump_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Formatter::DUMP' -o %(output)s %(input)%",
                                    [py_syn], output="formatter-dump.syn", input=['py.syn'])
        formatter_dia_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Formatter::Dia' -o %(output)s %(input)%",
                                   [py_syn], output="formatter-dia.syn", input=['py.syn'])
        formatter_docbook_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Formatter::Docbook' -o %(output)s %(input)%",
                                       [py_syn], output="formatter-docbook.syn", input=['py.syn'])
        formatter_dot_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Formatter::Dot' -o %(output)s %(input)%",
                                       [py_syn], output="formatter-dot.syn", input=['py.syn'])
        formatter_html_simple_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Formatter::HTML_Simple' -o %(output)s %(input)%",
                                           [py_syn], output="formatter-html-simple.syn", input=['py.syn'])
        formatter_texi_syn = Target(self, synopsis + " -Wc,linker=All -Wl,-s,'Synopsis::Formatter::TexInfo' -o %(output)s %(input)%",
                                    [py_syn], output="formatter-texi.syn", input=['py.syn'])

        all_syn = Target(self, synopsis + " -Wc,linker=All -o %(output)s %(input)s",
                                    [py_syn, cxx_syn], output="all.syn", input=['py.syn', 'c++.syn'])

        html = Target(self, synopsis + " -Wc,formatter=HTML %(input)s",
                                    [all_syn], output="html", input=['all.syn'])

        html.process()
        #for t in py_syn:
        #    t.process()
        os.chdir(cwd)


    def tutorial(self):
        xmlto = find_executable('xmlto')
        if not xmlto:
            self.announce("cannot build html docs without 'xmlto'")
            return
        self.announce("building html manual")
        srcdir = os.path.abspath('docs/Manual/')
        tempdir = os.path.abspath(os.path.join(self.build_temp, 'share/doc/synopsis'))
        builddir = os.path.abspath(os.path.join(self.build_lib, 'share/doc/synopsis'))
        cwd = os.getcwd()
        mkpath(tempdir, 0777, self.verbose, self.dry_run)
        os.chdir(tempdir)
        spawn([xmlto, '--skip-validation', '-o', 'html',
               '-m', os.path.join(srcdir, 'synopsis-html.xsl'),
               'html', os.path.join(srcdir, 'synopsis.xml')])

        mkpath(builddir, 0777, self.verbose, self.dry_run)
        if os.path.isdir(os.path.join(builddir, 'html')):
            rmtree(os.path.join(builddir, 'html'), 1)
        copytree(os.path.join(tempdir, 'html'), os.path.join(builddir, 'html'))

        docbook2pdf = find_executable('docbook2pdf')
        if not docbook2pdf:
            self.announce("cannot build pdf docs without 'docbook2pdf'")
            return
        self.announce("building pdf manual")
        spawn([docbook2pdf, os.path.join(srcdir, 'synopsis.xml')])
        copy2('synopsis.pdf', builddir)
        os.chdir(cwd)
