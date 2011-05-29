========
Examples
========

.. toctree::
   :hidden:

   Paths/diagrams/index

HTML
====

Default Settings
----------------

`This <Paths/html/index.html>`_ is an API reference generated with the default HTML formatter settings. The sources include documentation in different markup, such as RestructuredText, and Javadoc.

No Frames
---------

`This <Paths/html-noframes/index.html>`_ is the same HTML formatter, but without frames.

Source Cross Reference
----------------------

`This <Paths/html-rich/index.html>`_ is again the HTML formatter, but customized a bit to also include source cross-referencing.

SXR
===

The Synopsis Cross-Reference tool allows to search for (type, variable, function, and file) names by means of a CGI script. Please see the build instructions for the Paths/sxr example for the required steps to start the sxr-server tool. The Synopsis website uses this to navigate its own source code `here <http://synopsis.fresco.org/sxr/>`_

DocBook
=======

Default Settings
----------------

`Paths.xml <Paghs/docbook/Paths.xml>`_

HTML
____

`This <Paths/docbook/index.html>`_ is a C++ API reference generated with the default DocBook formatter settings, using the XHTML 
stylesheets from the `xsl-docbook <http://wiki.docbook.org/topic/DocBookXslStylesheets>`_ package. 
The sources include documentation in different markup, such as `RestructuredText <http://docutils.sourceforge.net/rst.html>`_, or 
`Javadoc <http://java.sun.com/j2se/1.5.0/docs/tooldocs/solaris/javadoc.html>`_.

.. PDF
.. ___

.. `This <Paths/docbook/Paths.pdf>`_ is the same, but formatted as pdf using the FO stylesheets.

Alternative Settings
--------------------

HTML
____

`This <math/index.html>`_ is a C API reference generated with the default DocBook formatter settings, 
using the XHTML stylesheets from the `xsl-docbook <http://wiki.docbook.org/topic/DocBookXslStylesheets>`_ package.

.. PDF
.. ___

.. `This <math/math.pdf>`_ is the same, but formatted as pdf using the FO stylesheets.

Hybrid IDL / C++ code
=====================

Synopsis is able to cross-reference two distinct Reference manuals, using TOCs. 
Moreover, it is possible to map symbols across lanugage-boundaries. 
`This <hybrid/interface/index.html>`_ example shows an IDL interface cross-referenced with a 
`C++ implementation <hybrid/implementation/index.html>`_.
