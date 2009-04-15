//
// Copyright (C) 2008 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Python.h>
#include <Support/Path.hh>

#include "Walker.hh"
#include "SXRGenerator.hh"
#include "SXRBuffer.hh"
#include "ASG.hh"
#include "Types.hh"
#include "TypeIdFormatter.hh"
#include "Builder.hh"
#include "STrace.hh"
#include "Filter.hh"

#include <fstream>
#include <sstream>
#include <iomanip>
#include <map>

#include <Synopsis/PTree.hh>
#include <Synopsis/Buffer.hh>

namespace 
{

char const *context_names[] =
{
  "reference",
  "definition",
  "SPAN",
  "IMPL",
  "UDIR",
  "UDEC",
  "call"
};

}

SXRGenerator::SXRGenerator(FileFilter* filter, Walker *walker)
  : filter_(filter),
    buffer_(walker->buffer()),
    walker_(walker)

{
  // Check size of array here to prevent later segfaults
  assert(sizeof(context_names)/sizeof(context_names[0]) == NumContext);
}

SXRGenerator::~SXRGenerator()
{
  for (SXRDict::iterator i = buffers_.begin(); i != buffers_.end(); ++i)
  {
    i->second->write();
    delete i->second;
  }
}

void SXRGenerator::xref_macro_calls()
{
  for (SXRDict::iterator f = buffers_.begin(); f != buffers_.end(); ++f)
  {
    ASG::SourceFile *file = f->first;
    SXRBuffer *sxr = f->second;
    ASG::SourceFile::Lines &lines = file->macro_calls();
    for (ASG::SourceFile::Lines::iterator l = lines.begin(); l != lines.end(); ++l)
    {
      long lineno = l->first;
      ASG::SourceFile::Line &line = l->second;
      for (ASG::SourceFile::Line::iterator e = line.begin(); e != line.end(); ++e)
      {
        ASG::SourceFile::MacroCall const &call = *e;
        if (!call.continuation)
          sxr->insert_xref(lineno, call.column, call.name.size(), call.name, "definition",
                           "global scope", "macro call", false);
        
      }
    }
  }
}

Walker* SXRGenerator::walker()
{
  return walker_;
}

int SXRGenerator::map_column(ASG::SourceFile *file, int line, char const *ptr)
{
  char const *line_start = ptr;
  while (line_start > buffer_->ptr() && *line_start != '\n') --line_start;
  int col = ptr - (line_start + 1);
  // Resolve macro maps
  return file->map_column(line, col);
}

void SXRGenerator::xref(PTree::Node *node, Context context, QName const &name, std::string const &desc, ASG::Declaration const *decl)
{
  walker_->update_line_number(node);
  ASG::SourceFile* file = walker_->current_file();

  // Dont store records for included files
  if (!filter_->should_xref(file))
    return;

  // Get info for storing a syntax record
  int begin_line = walker_->line_of_ptree(node);
  int begin_col = map_column(file, begin_line, node->begin());

  if (begin_col < 0) return; // inside macro

  std::string filename;
  unsigned long end_line = buffer_->origin(node->end(), filename);

  if (begin_line == end_line)
  {
    int len = node->end() - node->begin();
    store_xref(file, begin_line, begin_col, len, context, name, desc, false);
  }
  else
  {
    // Generate one xref plus continuations for each additional line.
    int end_col = map_column(file, end_line, node->end());
    for (int line = begin_line; line < end_line; ++line, begin_col = 0)
      store_xref(file, line, begin_col, -1, context, name, desc, line != begin_line);
    store_xref(file, end_line, 0, end_col, context, name, desc, true);
  }
}

//. A class which acts as a Types Visitor to store the correct link to a given
//. type
class TypeStorer : public Types::Visitor
{
  // Variables to pass to link()
  SXRGenerator *sxr_;
  PTree::Node *node;
  SXRGenerator::Context context;
public:
  //. Constructor
  TypeStorer(SXRGenerator* sxr, PTree::Node *n, SXRGenerator::Context c)
    : sxr_(sxr), node(n), context(c)
  {}

  //. Returns a suitable description for the given type
  std::string describe(Types::Type* type)
  {
    std::string desc;
    try
    {
      return Types::declared_cast<ASG::Declaration>(type)->type();
    }
    catch (Types::wrong_type_cast const &)
    {
      return sxr_->walker()->type_formatter()->format(type);
    }
  }

  // Visitor methods
  void visit_base(Types::Base* base)
  {
    sxr_->span(node, "keyword");
  }
  void visit_dependent(Types::Dependent*)
  {
  }

  void visit_named(Types::Named* named)
  {
    // All other nameds get stored
    sxr_->xref(node, context, named->name(), describe(named));
  }
  void visit_declared(Types::Declared* declared)
  {
    // All other nameds get stored
    sxr_->xref(node, context, declared->name(), describe(declared), declared->declaration());
  }
  void visit_modifier(Types::Modifier* mod)
  {
    // We recurse on the mod's alias, but dont link the const bit
    if (mod->pre().size() && mod->pre().front() == "const")
      if (!node->is_atom() && PTree::first(node) && *PTree::first(node) == "const")
      {
        sxr_->span(PTree::first(node), "keyword");
        node = PTree::first(PTree::last(node));
      }
    mod->alias()->accept(this);
  }
  void visit_parameterized(Types::Parameterized* param)
  {
    // Sometimes there's a typename at the front..
    if (PTree::first(node)->is_atom() && PTree::first(node) && *PTree::first(node) == "typename")
      node = PTree::second(node);
    // Some modifiers nest the identifier..
    while (!PTree::first(node)->is_atom())
      node = PTree::first(node);
    // For qualified template names the ptree is:
    //  [ std :: [ vector [ < ... , ... > ] ] ]
    // If the name starts with :: (global scope), skip it
    if (PTree::first(node) && *PTree::first(node) == "::")
      node = PTree::rest(node);
    // Skip the qualifieds (and just link the final name)
    while (PTree::second(node) && *PTree::second(node) == "::")
      if (PTree::third(node)->is_atom())
        node = PTree::rest(PTree::rest(node));
      else
        node = PTree::third(node);
    // Do template
    sxr_->xref(PTree::first(node), param->template_id());
    // Do params
    node = PTree::second(node);
    typedef Types::Type::vector::iterator iterator;
    iterator iter = param->parameters().begin();
    iterator end = param->parameters().end();
    // Could be leaf if eg: [SomeId const] node is now "const"
    while (node && !node->is_atom() && iter != end)
    {
      // Skip '<' or ','
      if (!(node = PTree::rest(node)))
        break;
      if (node->car() && node->car()->car() && !node->car()->car()->is_atom() && node->car()->car()->car())
        sxr_->xref(node->car()->car()->car(), *iter);
      ++iter;
      node = PTree::rest(node);
    }
  }
  // Other types ignored, for now
};

// Store if type is named
void SXRGenerator::xref(PTree::Node *node, Types::Type* type, Context context)
{
  ASG::SourceFile* file = walker_->current_file();
  if (!type || !filter_->should_xref(file))
    return;
  TypeStorer storer(this, node, context);
  type->accept(&storer);
}

void SXRGenerator::xref(PTree::Node *node, ASG::Declaration const *decl)
{
  ASG::SourceFile* file = walker_->current_file();
  if (!decl || !filter_->should_xref(file))
    return;
  xref(node, Definition, decl->name(), decl->type(), decl);
}

void SXRGenerator::store_span(unsigned int line, unsigned int col, int len, char const *type)
{
  // TODO: desc maps to href title...
  ASG::SourceFile* file = walker_->current_file();
  if (!filter_->should_xref(file)) return;
  SXRBuffer *sxr = get_buffer(file);
  sxr->insert_span(line, col, len, type);
}

void SXRGenerator::span(PTree::Node *node, char const *desc)
{
  int line = walker_->line_of_ptree(node);
  ASG::SourceFile* file = walker_->current_file();
  if (!filter_->should_xref(file))
    return;
  int col = map_column(file, line, node->begin());
  if (col < 0)
    return; // inside macro
  int len = node->end() - node->begin();
  
  store_span(line, col, len, desc);
}

void SXRGenerator::long_span(PTree::Node *node, char const *desc)
{
  // Find left edge
  int left_line = walker_->line_of_ptree(node);
  ASG::SourceFile* file = walker_->current_file();
  if (!filter_->should_xref(file))
    return;
  int left_col = map_column(file, left_line, node->begin());
  if (left_col < 0)
    return; // inside macro
  int len = node->end() - node->begin();
  
  // Find right edge
  std::string filename;
  unsigned long right_line = buffer_->origin(node->end(), filename);
  
  if (right_line == left_line)
    // Same line, so normal output
    store_span(left_line, left_col, len, desc);
  else
  {
    // Must output one for each line
    int right_col = map_column(file, right_line, node->end());
    for (int line = left_line; line < right_line; line++, left_col = 0)
      store_span(line, left_col, -1, desc);
    // Lasg line is a bit different
    store_span(right_line, 0, right_col, desc);
  }
}

// Store a link in the Syntax File
void SXRGenerator::store_xref(ASG::SourceFile* file,
                              int line, int col, int len, Context context,
                              QName const &qname, std::string const &desc,
                              bool continuation)
{
  SXRBuffer *sxr = get_buffer(file);
  std::vector<ASG::Scope*> scopes;
  Types::Named* vtype;
  QName name;
  if (walker_->builder()->mapName(qname, scopes, vtype))
  {
    for (size_t i = 0; i < scopes.size(); i++)
    {
      if (ASG::Namespace* ns = dynamic_cast<ASG::Namespace*>(scopes[i]))
        if (ns->type() == "function")
        {
          // Restart description at function scope
          name.clear();
          continue;
        }
      // Add the name to the short name
      name.push_back(scopes[i]->name().back());
    }
    // Add the final type name to the short name
    name.push_back(vtype->name().back());
  }
  else
  {
    STrace trace("SXRGenerator::xref");
    LOG("WARNING: couldnt map name " << qname);
    name = qname;
  }
  ASG::Scope *scope = walker_->builder()->scope();
  std::string from = join(scope->name(), "::");
  std::string type = context_names[context];
  std::string title = desc + " " + join(name, "::");
  sxr->insert_xref(line, col, len, join(qname, "::"), type, from, title, continuation);
}

SXRBuffer *SXRGenerator::get_buffer(ASG::SourceFile* file)
{
  SXRBuffer *buf = 0;
  if (buffers_.find(file) == buffers_.end())
  {
    std::string filename = filter_->get_sxr_filename(file);
    Synopsis::makedirs(Synopsis::Path(filename).dirname());
    buf = new SXRBuffer(filename.c_str(), file->abs_name(), file->name());
    buffers_.insert(std::make_pair(file, buf));
  }
  else
    buf = buffers_[file];
  return buf;
}
