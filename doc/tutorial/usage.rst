Using the synopsis tool
=======================

In this section we are going to explore the possibilities to generate documentation from source code. We will demonstrate how to use synopsis standalone as well as in conjunction with existing build systems. Further, we will see how to adapt synopsis to your coding and commenting style, as well as how to generate the output in a format and style that fulfills your needs.

Option handling
---------------

The synopsis tool combines three optional types of processors: parsers (specified with the -p option), linker processors (specified with the -l option, and formatters (specified with the -f option). If a parser is selected, any input is interpreted as source files of the respective language. Otherwise it will be read in as a stored IR. Similarly, if a formatter is selected, output is generated according to the formatter. Otherwise it will contain a stored IR.

For all of the three main processors, arguments can be passed down using the -W. For example, to find out what parameters are available with the Cxx parser, use the --help option::

  $ synopsis -p Cxx -h
  Parameters for processor 'Synopsis.Parsers.Cxx.Parser':
     profile               output profile data
     cppflags              list of preprocessor flags such as -I or -D
     preprocess            whether or not to preprocess the input
     ...
      

Then, to pass a preprocess option, either of:

  synopsis -p Cxx -Wp,--preprocess ...

  synopsis -p Cxx -Wp,preprocess=True ...

The first form expects an optional string argument, while the second form expects a python expression, thus allowing to pass python objects such as lists. (But be careful to properly escape characters to get the expression through the shell !)

But passing options via the command line has its limits, both, in terms of usability, as well as for the robustness of the interface (all data have to be passed as strings !). Therefor, for any tasks demanding more flexibility a scripting interface is provided, which will be discussed in the next chapter. 

Parsing source-code
-------------------

Let's assume a simple header file, containing some declarations:

.. code-block:: c++

  #ifndef Path_h_
  #define Path_h_

  //. A Vertex is a 2D point.
  struct Vertex
  {
    Vertex(double xx, double yy): x(xx), y(yy) {}
    double x; //.< the x coordinate
    double y; //.< the y coordinate
  };

  //. Path is the basic abstraction
  //. used for drawing (curved) paths.
  class Path
  {
  public:
    virtual ~Path() {}
    //. Draw this path.
    virtual void draw() = 0;
    // temporarily commented out...
    // bool intersects(const Path &);
  private:
  };

  #endif

        

Process this with:

  synopsis -p Cxx -f HTML -o Paths Path.h

to generate an html document in the directory specified using the -o option, i.e. Paths.

The above represents the simplest way to use synopsis. A simple command is used to parse a source-file and to generate a document from it. The parser to be used is selected using the -p option, and the formatter with the -f option.

If no formatter is specified, synopsis dumps its internal representation to the specified output file. Similarly, if no parser is specified, the input is interpreted as an IR dump. Thus, the processing can be split into multiple synopsis invocations.

Each processor (including parsers and formatters) provides a number of parameters that can be set from the command line. For example the Cxx parser has a parameter base_path to specify a prefix to be stripped off of file names as they are stored in synopsis' internal representation. Parser-specific options can be given that are passed through to the parser processor. To pass such an option, use the -Wp, prefix. For example, to set the parser's base_path option, use:

  synopsis -p Cxx -Wp,--base-path=<prefix> -f HTML -o Paths Path.h

Emulating a compiler
--------------------

Whenever the code to be parsed includes system headers, the parser needs to know about their location(s), and likely also about system macro definitions that may be in effect. For example, parsing:

.. code-block:: c++

  #include <vector>
  #include <string>

  typedef std::vector<std::string> option_list;
      

requires the parser to know where to find the vector and string headers.

Synopsis will attempt to emulate a compiler for the current programming language. By default, synopsis -p Cxx will try to locate c++ or similar, to query system flags. However, the compiler can be specified via the --emulate-compiler option, e.g. synopsis -p Cxx -Wp,--emulate-compiler=/usr/local/gcc4/bin/g++.

All languages that use the Cpp processor to preprocess the input accept the emulate-compiler argument, and pass it down to the Cpp parser. See the section called “The Cpp Parser” for a detailed discussion of this process.

Using comments for documentation
--------------------------------

Until now the generated document didn't contain any of the text from comments in the source code. To do that the comments have to be translated first. This translation consists of a filter that picks up a particular kind of comment, for example only lines starting with ``//.``, or javadoc-style comments such as ``/**...*/``, as well as some translator that converts the comments into actual documentation, possibly using some inline markup, such as Javadoc or ReST.

The following source code snippet contains java-style comments, with javadoc-style markup. Further, an embedded processing instruction wants some declarations to be grouped.

.. code-block:: c++

  #ifndef Bezier_h_
  #define Bezier_h_

  #include "Path.h"
  #include <vector>

  namespace Paths
  {

  /**
    * The Bezier class. It implements a Bezier curve
    * for the given order.
    */
  template <size_t Order>
  class Bezier : public Path
  {
  public:
    /** Create a new Bezier.*/
    Bezier();

    /** @group Manipulators {*/

    /**
      * Add a new control point.
      * @param p A point
      */
    void add_control_point(const Vertex &);

    /**
      * Remove the control point at index i.
      * @param i An index
      */
    void remove_control_point(size_t i);
    /** }*/
    virtual void draw();
  private:
    /** The data...*/
    std::vector<Vertex> controls_;
  };

  }

  #endif

        

The right combination of comment processing options for this code would be::

  synopsis -p Cxx --cfilter=java --translate=javadoc -lComments.Grouper ...

The --cfilter option allows to specify a filter to select document comments, and the --translate option sets the kind of markup to expect. The -l option is somewhat more generic. It is a linker to which (almost) arbitrary post-processors can be attached. Here we pass the Comments.Grouper processor that injects Group nodes into the IR that cause the grouped declarations to be documented together. 

