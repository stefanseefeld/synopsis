from distutils.command.install import install

class install_syn(install):

    sub_commands = install.sub_commands + [('install_clib', None)]
