//
// Copyright (C) 2008 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#ifndef SXRGenerator_hh_
#define SXRGenerator_hh_

#include "ASG.hh"
#include <iostream>
#include "SXRBuffer.hh"
#include <string>

namespace Synopsis
{
class Buffer;
class Parser;
namespace PTree { class Node;}
}
class Walker;
class FileFilter;

//. Stores link information about the file. Link info is stored in two files
//. with two purposes.
//.
//. The first file stores all links and non-link spans, in a
//. simple text file with one record per line and with spaces as field
//. separators. The fields themselves are encoded using URL-style %FF encoding
//. of non alpha-numeric characters (including spaces, brackers, commas etc).
//. The purpose of this file is for syntax-hightlighting of source files.
//.
//. The second file stores only cross-reference information, which is a subset
//. of the first file.
class SXRGenerator
{
public:
    //. Enumeration of record types
    enum Context
    {
        Reference,	        //.< General name reference
        Definition,         //.< Definition of the declaration
        Span,               //.< Non-declarative span of text
        Implementation,     //.< Implementation of a declaration
        UsingDirective,     //.< Referenced in a using directive
        UsingDeclaration,   //.< Referenced in a using declaration
        FunctionCall,       //.< Called as a function
        NumContext          //.< Marker used to check size of array
    };

  //. Constructor.
  //. @param filter the filter to use to decide whether to output syntax and
  //. xref records
  //. @param swalker the Walker object we are linking for
  SXRGenerator(FileFilter* filter, Walker* walker);

  //. Destructor. Closes all opened file streams
  ~SXRGenerator();

  //. Store links for all macro calls back to their definitions.
  void xref_macro_calls();

  //. Store a link for the given Ptree node. If a decl is given, store an
  //. xref too
  void xref(Synopsis::PTree::Node *node, Context, QName const &name, std::string const &desc, ASG::Declaration const *decl = 0);

  //. Store a Definition link for the given Ptree node using the ASG node
  void xref(Synopsis::PTree::Node *node, ASG::Declaration const *decl);

  //. Store a link for the given node using the given Context, which defaults
  //. to a Reference
  void xref(Synopsis::PTree::Node *node, Types::Type*, Context = Reference);

  //. Store a span for the given Ptree node
  void span(Synopsis::PTree::Node *node, char const *desc);

  //. Store a long (possibly multi-line) span
  void long_span(Synopsis::PTree::Node *node, char const *desc);

  //. Returns the Walker
  Walker* walker();

private:
  //. The type of a map of streams
  typedef std::map<ASG::SourceFile *, SXRBuffer *> SXRDict;
  
  void store_span(unsigned int line, unsigned int col, int len, char const *type);
  void store_xref(ASG::SourceFile*,
                  int line, int col, int len, Context context,
                  QName const &name, std::string const &desc, bool continuation);

  SXRBuffer *get_buffer(ASG::SourceFile*);

  //. Computes ptr's column in the original source. This requires looking for
  //. macro call expansion that may have displaced ptr in the preprocessed file.
  int map_column(ASG::SourceFile *file, int line, char const *ptr);

  //. The filter
  FileFilter* filter_;

  //. The Buffer object
  Synopsis::Buffer *buffer_;

  //. The Walker object
  Walker* walker_;

  //. A map of streams for each file
  SXRDict buffers_;
};

#endif
