import os, sys, string

from distutils.command import build
from distutils.dir_util import mkpath
from distutils.spawn import spawn, find_executable
from shutil import *

class build_doc(build.build):

    description = "build documentation"

    def run(self):

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
