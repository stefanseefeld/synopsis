// $Id: swalker.cc,v 1.78 2003/12/03 03:43:27 stefan Exp $
//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <iostream>
#include <string>
#include <typeinfo>
#include <sstream>
#include <algorithm>

#include <occ/ptree.h>
#include <occ/parse.h>

#include "swalker.hh"
#include "strace.hh"
#include "type.hh"
#include "ast.hh"
#include "builder.hh"
#include "decoder.hh"
#include "dumper.hh"
//#include "link_map.hh"
#include "linkstore.hh"
#include "lookup.hh"
#include "filter.hh"

using namespace AST;

#ifdef DO_TRACE
int STrace::slevel = 0, STrace::dlevel = 0;
std::ostringstream* STrace::stream = 0;
STrace::string_list STrace::my_list;
std::ostream& STrace::operator <<(Ptree* p)
{
  std::ostream& out = operator <<("-");
  p->Display2(out);
  return out;
}
#endif

SWalker *SWalker::g_swalker = 0;

SWalker::SWalker(FileFilter* filter, Parser* parser,
                 Builder* builder, Program* program)
  : Walker(parser),
    my_parser(parser),
    my_builder(builder),
    my_filter(filter),
    my_program(program),
    my_decoder(new Decoder(my_builder)),
    my_declaration(0),
    my_template(0),
    my_filename_ptr(0),
    my_file(0),
    my_lineno(0),
    my_links(0),
    my_store_decl(false),
    my_type_formatter(new TypeFormatter()),
    my_function(0),
    my_type(0),
    my_scope(0),
    my_postfix_flag(Postfix_Var)
{
  g_swalker = this; // FIXME: is this needed?
  my_builder->set_swalker(this);
  my_lookup = my_builder->lookup();
}

// Destructor
SWalker::~SWalker()
{
  delete my_decoder;
  delete my_type_formatter;
  if (my_links) delete my_links;
}

// The name returned is just the node's text if the node is a leaf. Otherwise,
// the ToString method of Ptree is used, which is rather expensive since it
// creates a temporary write buffer and reifies the node tree into it.
std::string
SWalker::parse_name(Ptree *node) const
{
  // STrace trace("SWalker::parse_name");
  if (node && node->IsLeaf())
    return std::string(node->GetPosition(), node->GetLength());
  return node->ToString();
}

void
SWalker::set_store_links(LinkStore* links)
{
  my_links = links;
}

int SWalker::line_of_ptree(Ptree* node)
{
  update_line_number(node);
  return my_lineno;
}

// Updates the line number stored in this SWalker instance, and the filename
// stored in the Builder instance at my_builder. The filename is only set if
// the actual char* changed (which only happens when the preprocessor places
// another #line directive)
void SWalker::update_line_number(Ptree* ptree)
{
  // Ask the Parser for the linenumber of the ptree. This used to be
  // expensive until I hacked buffer.cc to cache the last line number found.
  // Now it's okay as long as you are looking for lines sequentially.
  char* fname;
  int fname_len;
  my_lineno = my_parser->LineNumber(ptree->LeftMost(), fname, fname_len);
  if (fname != my_filename_ptr)
  {
    my_filename_ptr = fname;
    my_file = my_filter->get_sourcefile(fname, fname_len);
    my_builder->set_file(my_file);
  }
}

AST::Comment *
make_Comment(SourceFile* file, int line, Ptree* first, bool suspect=false)
{
  return new AST::Comment(file, line, first->ToString(), suspect);
}

Leaf* make_Leaf(char* pos, int len)
{
    return new Leaf(pos, len);
}

// Adds the given comments to the given declaration. If my_links is set,
// then syntax highlighting information is also stored.
void
SWalker::add_comments(AST::Declaration* decl, Ptree* node)
{
  if (!node) return;
  
  AST::Comment::vector comments;

  // First, make sure that node is a list of comments
  if (node->What() == ntDeclaration)
    node = static_cast<PtreeDeclaration*>(node)->GetComments();

  // Loop over all comments in the list
  for (Ptree* next = node->Rest(); node && !node->IsLeaf(); next = node->Rest())
  {
    Ptree* first = node->First();
    if (!first || !first->IsLeaf())
    {
      node = next;
      continue;
    }
    update_line_number(node);
    // Make sure comment is in same file!
    if (decl && (my_file != decl->file()))
    {
      node = next;
      // Empty list of comments to add: an #include in the middle is not
      // allowed!
      comments.clear();
      continue;
    }

    // Check if comment is continued, eg: consecutive C++ comments
    while (next && next->First() && next->First()->IsLeaf())
    {
      if (!strncmp(first->GetPosition() + first->GetLength() - 2, "*/", 2))
        break;
      if (strncmp(next->First()->GetPosition(), "//", 2))
        break;
      char* next_pos = next->First()->GetPosition();
      char* start_pos = node->First()->GetPosition();
      char* curr_pos = start_pos + node->First()->GetLength();
      // Must only be whitespace between current comment and next
      // and only one newline
      int newlines = 0;
      while (curr_pos < next_pos && strchr(" \t\r\n", *curr_pos))
        if (*curr_pos == '\n' && newlines > 0)
          break;
        else if (*curr_pos++ == '\n')
          ++newlines;
      if (curr_pos < next_pos)
        break;
      // Current comment stretches to end of next
      int len = int(next_pos - start_pos + next->First()->GetLength());
      //node->SetCar(first = new Leaf(start_pos, len));
      node->SetCar(first = make_Leaf(start_pos, len));
      // Skip the combined comment
      next = next->Rest();
    }

    // all comments that are not immediately (i.e. separated
    // by a single new line) followed by a declaration are
    // marked as 'suspect'
    bool suspect = false;
    char* pos = first->GetPosition() + first->GetLength();
    while (*pos && strchr(" \t\r", *pos)) ++pos;
    if (*pos == '\n')
    {
      ++pos;
      // Found only allowed \n
      while (*pos && strchr(" \t\r", *pos)) ++pos;
      if (*pos == '\n' || !strncmp(pos, "/*", 2)) suspect = true;
    }

    if (decl)
    {
      AST::Comment* comment = make_Comment(my_file, 0, first, suspect);
      comments.push_back(comment);
    }
    if (my_links) my_links->long_span(first, "file-comment");
    // Set first to nil so we dont accidentally do them twice (eg:
    // when parsing expressions)
    node->SetCar(nil);
    node = next;
  }

  // Now add the comments, if applicable
  if (decl)
    for (AST::Comment::vector::iterator i = comments.begin();
         i != comments.end();
         ++i)
      decl->comments().push_back(*i);
}

// -- These methods implement add_comments for various node types that store
// comment pointers
void SWalker::add_comments(AST::Declaration* decl, CommentedLeaf* node)
{
  if (node) add_comments(decl, node->GetComments());
}
void SWalker::add_comments(AST::Declaration* decl, PtreeDeclaration* node)
{
  if (node) add_comments(decl, node->GetComments());
}
void SWalker::add_comments(AST::Declaration* decl, PtreeDeclarator* node)
{
  if (node) add_comments(decl, node->GetComments());
}
void SWalker::add_comments(AST::Declaration* decl, PtreeNamespaceSpec* node)
{
  if (node) add_comments(decl, node->GetComments());
}
void SWalker::find_comments(Ptree* node)
{
  Ptree* leaf, *parent;
  leaf = FindLeftLeaf(node, parent);
  if (leaf) add_comments(0, dynamic_cast<CommentedLeaf*>(leaf));
}

Ptree* SWalker::TranslateArgDeclList(bool, Ptree*, Ptree*)
{
    STrace trace("SWalker::TranslateArgDeclList NYI");
    return 0;
}
Ptree* SWalker::TranslateInitializeArgs(PtreeDeclarator*, Ptree*)
{
    STrace trace("SWalker::TranslateInitializeArgs NYI");
    return 0;
}
Ptree* SWalker::TranslateAssignInitializer(PtreeDeclarator*, Ptree*)
{
    STrace trace("SWalker::TranslateAssignInitializer NYI");
    return 0;
}
//Class* SWalker::MakeClassMetaobject(Ptree*, Ptree*, Ptree*) { STrace trace("SWalker::MakeClassMetaobject NYI"); return 0; }
//Ptree* SWalker::TranslateClassSpec(Ptree*, Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateClassSpec NYI"); return 0; }
//Ptree* SWalker::TranslateClassBody(Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateClassBody NYI"); return 0; }
//Ptree* SWalker::TranslateTemplateInstantiation(Ptree*, Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateTemplateInstantiation NYI"); return 0; }

// Format the given parameters. my_type_formatter is used to format the given
// list of parameters into a string, suitable for use as the name of a
// Function object.
std::string SWalker::format_parameters(AST::Parameter::vector& params)
{
  // TODO: Tell formatter to expand typedefs! Eg: this function uses a typedef
  // in implementation, but not for declaration in the class!!!!!!
  AST::Parameter::vector::iterator iter = params.begin(), end = params.end();
  if (iter == end) return "()";
  // Set scope for formatter
  AST::Scope* scope = my_builder->scope();
  if (scope) my_type_formatter->push_scope(scope->name());
  else my_type_formatter->push_scope(ScopedName());
  // Format the parameters one at a time
  std::ostringstream buf;
  buf << "(" << my_type_formatter->format((*iter++)->type());
  while (iter != end)
    buf << "," << my_type_formatter->format((*iter++)->type());
  buf << ")";
  my_type_formatter->pop_scope();
  return buf.str();
}

// Overrides Walker::Translate to catch any exceptions
void
SWalker::Translate(Ptree* node)
{
  STrace trace("SWalker::Translate");
  try
  {
    Walker::Translate(node);
  }
  // Debug and non-debug modes handle these very differently
#ifdef DEBUG
  catch (const TranslateError& e)
  {
    if (e.node) node = e.node;
    char* fname;
    int fname_len;
    int lineno = my_parser->LineNumber(node->LeftMost(), fname, fname_len);
    std::ostringstream buf;
    buf << " (" << std::string(fname, fname_len) << ":" << lineno << ")";
    LOG("Warning: An exception occurred:" << buf.str());
    LOG("- " << e.str());
  }
  catch (const std::exception& e)
  {
    LOG("Warning: An exception occurred: " << e.what());
    nodeLOG(node);
  }
  catch (...)
  {
    LOG("Warning: An exception occurred (unknown) at:");
    nodeLOG(node);
  }
#else
  catch (const TranslateError& e)
  {
    // This error usually means that the syntax highlighting failed, and
    // can be safely ignored
  }
  catch (const std::exception& e)
  {
    std::cout << "Warning: An exception occurred: " << e.what() << std::endl;
    std::cout << "At: ";
    char* fname;
    int fname_len;
    int lineno = my_parser->LineNumber(node->LeftMost(), fname, fname_len);
    std::cout << " (" << std::string(fname, fname_len) << ":" << lineno << ")" << std::endl;
  }
  catch (...)
  {
    std::cout << "Warning: An unknown exception occurred: " << std::endl;
    std::cout << "At: ";
    char* fname;
    int fname_len;
    int lineno = my_parser->LineNumber(node->LeftMost(), fname, fname_len);
    std::cout << " (" << std::string(fname, fname_len) << ":" << lineno << ")" << std::endl;
  }
#endif
}

// Default translate, usually means a literal
Ptree* SWalker::TranslatePtree(Ptree* node)
{
  // Determine type of node
  char* str = node->ToString();
  if (*str >= '0' && *str <= '9' || *str == '.')
  {
    // Assume whole node is a number
    if (my_links) my_links->span(node, "file-number");
    // TODO: decide if Long, Float, Double, etc
    const char* num_type = (*str == '.' ? "double" : "int");
    while (*++str)
    {
      if (*str >= '0' && *str <= '9') {}
      else if (*str == 'e' || *str == 'E')
      {
        // Might be followed by + or -
        ++str;
        if (*str == '+' || *str == '-') ++str;
      }
      else if (*str == '.') num_type = "double";
      else if (*str == 'f' || *str == 'F')
      {
        num_type = "float";
        break;
      }
      else if (*str == 'l' || *str == 'L')
      {
        if (num_type == "int") num_type = "long";
        else if (num_type == "long") num_type = "long long";
        else if (num_type == "unsigned") num_type = "unsigned long";
        else if (num_type == "float") num_type = "long double";
        else
          std::cout << "Unknown num type: " << num_type << std::endl;
      }
      else if (*str == 'u' || *str == 'U')
      {
        if (num_type == "int") num_type = "unsigned";
        else if (num_type == "long") num_type = "unsigned long";
        else
          std::cout << "Unknown num type: " << num_type << std::endl;
      }
      else break;// End of numeric constant
    }
    my_type = my_lookup->lookupType(num_type);
  }
  else if (*str == '\'')
  {
    // Whole node is a char literal
    if (my_links) my_links->span(node, "file-string");
    my_type = my_lookup->lookupType("char");
  }
  else if (*str == '"')
  {
    // Assume whole node is a string
    if (my_links) my_links->span(node, "file-string");
    my_type = my_lookup->lookupType("char");
    Types::Type::Mods pre, post;
    pre.push_back("const");
    post.push_back("*");
    my_type = new Types::Modifier(my_type, pre, post);
  }
  else if (*str == '/' && !node->IsLeaf())
  {
    // Assume comment. Must be a list of comments!
    AST::Declaration* decl;
    update_line_number(node);
    decl = my_builder->add_tail_comment(my_lineno);
    add_comments(decl, node);
  }
  else
  {
#ifdef DEBUG
    STrace trace("SWalker::TranslatePtree");
    LOG("Warning: Unknown Ptree "<<node->What());
    nodeLOG(node);
    //*((char*)0) = 1; // force breakpoint, or core dump :)
#endif
  }
  return 0;
}

//. NamespaceSpec
//. [ namespace <identifier> [{ body }] ]
Ptree*
SWalker::TranslateNamespaceSpec(Ptree* def)
{
  STrace trace("SWalker::TranslateNamespaceSpec");
  
  Ptree* pNamespace = def->First();
  Ptree* pIdentifier = def->Second();
  Ptree* pBody = def->Third();

  if (my_links) my_links->span(pNamespace, "file-keyword");
  else update_line_number(def);

  // Start the namespace
  AST::Namespace* ns;
  if (pIdentifier)
  {
    ns = my_builder->start_namespace(parse_name(pIdentifier), NamespaceNamed);
    ns->set_file(my_file);
  }
  else ns = my_builder->start_namespace(my_file->filename(), NamespaceAnon);

  // Add comments
  add_comments(ns, dynamic_cast<PtreeNamespaceSpec*>(def));
  if (my_links && Ptree::First(pIdentifier)) my_links->link(pIdentifier, ns);

  // Translate the body
  Translate(pBody);

  // End the namespace
  my_builder->end_namespace();
  return 0;
}

//. [ : (public|private|protected|nil) <name> {, ...} ]
std::vector<Inheritance*> SWalker::TranslateInheritanceSpec(Ptree *node)
{
  STrace trace("PyWalker::TranslateInheritanceSpec");
  std::vector<Inheritance*> ispec;
  Types::Type *type;
  while (node)
  {
    node = node->Cdr(); // skip : or ,
    // the attributes
    std::vector<std::string> attributes(node->Car()->Length() - 1);
    for (int i = 0; i != node->Car()->Length() - 1; ++i)
    {
      attributes[i] = parse_name(node->Car()->Nth(i));
      if (my_links) my_links->span(node->Car()->Nth(i), "file-keyword");
    }
    // look up the parent type
    Ptree* name = node->Car()->Last()->Car();
    if (name->IsLeaf())
    {
      try
      {
        type = my_lookup->lookupType(parse_name(name));
      }
      catch (const TranslateError)
      {
        // Ignore error, and put an Unknown in, instead
        ScopedName uname;
        uname.push_back(parse_name(name));
        type = new Types::Unknown(uname);
      }
    }
    else
    {
      char* encname = name->GetEncodedName();
      my_decoder->init(encname);
      type = my_decoder->decodeType();
    }
    if (my_links) my_links->link(name, type);

    node = node->Cdr();
    // add it to the list
    ispec.push_back(new AST::Inheritance(type, attributes));
  }
  return ispec;
}

Ptree*
SWalker::TranslateClassSpec(Ptree* node)
{
  // REVISIT: figure out why this method is so long
  STrace trace("SWalker::TranslateClassSpec");
  enum { SizeForwardDecl = 2, SizeAnonClass = 3, SizeClass = 4 };

  AST::Parameter::vector* is_template = my_template;
  my_template = 0;

  int size = Ptree::Length(node);

  if (size == SizeForwardDecl)
  {
    // Forward declaration
    // [ class|struct <name> ]
    std::string name = parse_name(node->Second());
    if (is_template)
      LOG("Templated class forward declaration " << name);
    my_builder->add_forward(my_lineno, name, is_template);
    if (my_links)
    { // highlight the comments, at least
      PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
      add_comments(0, cspec->GetComments());
    }
    return 0;
  }
  Ptree* pClass = node->First();
  Ptree* pName = 0, *pInheritance = 0;
  Ptree* pBody = 0;
  if (size == SizeClass)
  {
    // [ class|struct <name> <inheritance> [{ body }] ]
    pName = node->Nth(1);
    pInheritance = node->Nth(2);
    pBody = node->Nth(3);
  }
  else if (size == SizeAnonClass)
    // An anonymous struct. OpenC++ encodes us a unique
    // (may be qualified if nested) name
    // [ struct [nil nil] [{ ... }] ]
    pBody = node->Nth(2);
  else
    throw nodeERROR(node, "Class node has bad length: " << size);

  if (my_links) my_links->span(pClass, "file-keyword");
  else update_line_number(node);

  // Create AST.Class object
  AST::Class *clas;
  std::string type = parse_name(pClass);
  char* encname = node->GetEncodedName();
  my_decoder->init(encname);
  if (encname[0] == 'T')
  {
    // Specialization
    // TODO: deal with this.
    // Eg: /usr/include/g++-3/std/straits.h
    // search: /^struct string_char_traits <char> {/
    // encname: "T\222string_char_traits\201c"
    LOG("Specialization?");
    nodeLOG(node);
    LOG("encname:"<<make_code(encname));
    Types::Parameterized* param = dynamic_cast<Types::Parameterized*>(my_decoder->decodeTemplate());
    // If a non-type param was found, it's name will be '*'
    for (size_t i = 0; i < param->parameters().size(); i++)
      if (Types::Dependent* dep = dynamic_cast<Types::Dependent*>(param->parameters()[i]))
      {
        if (dep->name().size() == 1 && dep->name()[0] == "*")
        {
          // Find the value of this parameter
          std::string name = parse_name(pName->Second()->Second()->Nth(i*2));
          dep->name()[0] = name;
        }
      }

    my_type_formatter->push_scope(my_builder->scope()->name());
    std::string name = my_type_formatter->format(param);
    my_type_formatter->pop_scope();
    clas = my_builder->start_class(my_lineno, type, name, is_template);
    // TODO: figure out spec stuff, like what to do with vars, link to
    // original template, etc.
  }
  else if (encname[0] == 'Q')
  {
    ScopedName names;
    my_decoder->decodeQualName(names);
    clas = my_builder->start_class(my_lineno, type, names);
  }
  else
  {
    std::string name = my_decoder->decodeName();
    clas = my_builder->start_class(my_lineno, type, name, is_template);
  }
  if (my_links && pName) my_links->link(pName, clas);
  LOG("Translating class '" << clas->name() << "'");

  // Translate the inheritance spec, if present
  if (pInheritance)
  {
    clas->parents() = TranslateInheritanceSpec(pInheritance);
    my_builder->update_class_base_search();
  }

  // Add comments
  PtreeClassSpec* cspec = static_cast<PtreeClassSpec*>(node);
  add_comments(clas, cspec->GetComments());

  // Push the impl stack for a cache of func impls
  my_func_impl_stack.push_back(FuncImplVec());

  // Translate the body of the class
  TranslateBlock(pBody);

  // Translate any func impls inlined in the class
  FuncImplVec& vec = my_func_impl_stack.back();
  FuncImplVec::iterator iter = vec.begin();
  while (iter != vec.end()) TranslateFuncImplCache(*iter++);
  my_func_impl_stack.pop_back();

  my_builder->end_class();
  return 0;
}

Ptree*
SWalker::TranslateTemplateClass(Ptree* def, Ptree* node)
{
  STrace trace("SWalker::TranslateTemplateClass");
  AST::Parameter::vector* old_params = my_template;
  update_line_number(def);
  my_builder->start_template();
  try
  {
    TranslateTemplateParams(def->Third());
    TranslateClassSpec(node);
  }
  catch (...)
  {
    my_builder->end_template();
    my_template = old_params;
    throw;
  }
  my_builder->end_template();
  my_template = old_params;
  return 0;
}

void SWalker::TranslateTemplateParams(Ptree* params)
{
  STrace trace("SWalker::TranslateTemplateParams");
  my_template = new AST::Parameter::vector;
  AST::Parameter::vector& templ_params = *my_template;
  // Declare some default parameter values - these should not be modified!
  std::string name, value;
  AST::Parameter::Mods pre_mods, post_mods;
  while (params)
  {
    Ptree* param = params->First();
    nodeLOG(param);
    if (param->First()->Eq("class") || param->First()->Eq("typename"))
    {
      // Ensure that there is an identifier (it is optional!)
      if (param->Cdr() && param->Second())
      {
        // This parameter specifies a type, add as dependent
        Types::Dependent* dep = my_builder->create_dependent(parse_name(param->Second()));
        my_builder->add
          (dep);
        AST::Parameter::Mods paramtype;
        paramtype.push_back(parse_name(param->First()));
        templ_params.push_back(new AST::Parameter(paramtype, dep, post_mods, name, value));
      }
      else
      {
        // Add a parameter, but with no name
        AST::Parameter::Mods paramtype;
        paramtype.push_back(parse_name(param->First()));
        templ_params.push_back(new AST::Parameter(paramtype, 0, post_mods, name, value));
      }
    }
    else if (param->First()->Eq("template"))
    {
      // A non-type parameter that is templatized
      // eg: template< class A, template<class T> class B = foo > C;
      // FIXME.
      LOG("templated non-type template parameter!");
      nodeLOG(param);
    }
    else
    {
      // This parameter specifies a value or something
      // FIXME can do a lot more here..
      LOG("non-type template parameter! approximating..");
      nodeLOG(param);
      Ptree* p = param->Second();
      while (p && p->Car() && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&')))
        p = Ptree::Rest(p);
      std::string name = parse_name(p);
      Types::Dependent* dep = my_builder->create_dependent(name);
      my_builder->add(dep);
      // Figure out the type of the param
      my_decoder->init(param->Second()->GetEncodedType());
      Types::Type* param_type = my_decoder->decodeType();
      templ_params.push_back(new AST::Parameter(pre_mods, param_type, post_mods, name, value));
    }
    // Skip comma
    params = Ptree::Rest(params->Rest());
  }
  /*
    Types::Template* templ = new Types::Template(decl->name(), decl, templ_params);
    if (AST::Class* clas = dynamic_cast<AST::Class*>(decl))
    clas->set_template_type(templ);
    else if (AST::Function* func = dynamic_cast<AST::Function*>(decl))
    func->set_template_type(templ);
    std::ostrstream buf;
    buf << "template " << decl->type() << std::ends;
    decl->set_type(buf.str());
  */
}

Ptree*
SWalker::TranslateTemplateFunction(Ptree* def, Ptree* node)
{
  STrace trace("SWalker::TranslateTemplateFunction");
  nodeLOG(def);
  nodeLOG(node);
  if (node->What() != ntDeclaration)
  {
    LOG("Warning: Unknown node type in template");
    nodeLOG(def);
    return 0;
  }

  LOG("What is: " << node->What());
  LOG("Encoded name is: " << make_code(node->GetEncodedName()));

  AST::Parameter::vector* old_params = my_template;
  update_line_number(def);
  my_builder->start_template();
  try
  {
    TranslateTemplateParams(def->Third());
    TranslateDeclaration(node);
  }
  catch (...)
  {
    my_builder->end_template();
    my_template = old_params;
    throw;
  }
  my_builder->end_template();
  my_template = old_params;
  return 0;
}

//. Linkage Spec
//. [ extern ["C++"] [{ body }] ]
Ptree*
SWalker::TranslateLinkageSpec(Ptree* node)
{
  STrace trace("SWalker::TranslateLinkageSpec");
  Translate(node->Third());
  return 0;
}

//. Block
//. [ { [ <statement>* ] } ]
Ptree*
SWalker::TranslateBlock(Ptree* block)
{
  STrace trace("SWalker::TranslateBlock");
  Ptree* rest = Ptree::Second(block);
  while (rest != nil)
  {
    Translate(rest->Car());
    rest = rest->Cdr();
  }
  Ptree* close = Ptree::Third(block);
  AST::Declaration* decl;
  decl = my_builder->add_tail_comment(my_lineno);
  add_comments(decl, dynamic_cast<CommentedLeaf*>(close));
  return 0;
}

//. Brace
//. [ { [ <statement>* ] } ]
Ptree*
SWalker::TranslateBrace(Ptree* brace)
{
  STrace trace("SWalker::TranslateBrace");
  Ptree* rest = Ptree::Second(brace);
  while (rest != nil)
  {
    Translate(rest->Car());
    rest = rest->Cdr();
  }
  Ptree* close = Ptree::Third(brace);
  AST::Declaration* decl;
  decl = my_builder->add_tail_comment(my_lineno);
  add_comments(decl, dynamic_cast<CommentedLeaf*>(close));
  return 0;
}

//. TemplateDecl
//. [ template < [types] > [decl] ]
Ptree*
SWalker::TranslateTemplateDecl(Ptree* def)
{
  STrace trace("SWalker::TranslateTemplateDecl");
  Ptree* body = Ptree::Nth(def, 4);
  Ptree* class_spec = GetClassTemplateSpec(body);
  if(class_spec->IsA(ntClassSpec))
    TranslateTemplateClass(def, class_spec);
  else TranslateTemplateFunction(def, body);
  return 0;
}

//. A typeof(expr) expression evaluates to the type of 'expr'. This is a GNU
//. GCC extension!
//. Since the OCC parser can't resolve the type properly, we try to do it here
//. and modify the type of the declarations to set it
Ptree* SWalker::TranslateTypeof(Ptree* spec, Ptree* declarations)
{
  STrace trace("SWalker::TranslateTypeof");
  nodeLOG(spec);
  char* encname = spec->Third()->GetEncodedName();
  LOG("The name is: " << make_code(encname));
  LOG("The type is: " << make_code(spec->Third()->GetEncodedType()));
  // Find the type referred to by the expression
  if (!my_decoder->isName(encname))
  {
    LOG("typeof is not a simple name: ");
    nodeLOG(spec);
    return 0;
  }
  std::string name = my_decoder->decodeName(encname);
  LOG("name is " << name);
  Types::Type* type = my_lookup->lookupType(name, true);
  // Find the declaration it refers to
  Types::Declared* declared = dynamic_cast<Types::Declared*>(type);
  if (!declared) return 0;
  LOG("Looked up " << declared->name());
  AST::Declaration* decl = declared->declaration();
  if (!decl) return 0;
  LOG("Declaration is " << decl->name());
  // TODO: make this a visitor and support different things
  if (/*AST::Function* func =*/ dynamic_cast<AST::Function*>(decl))
  {
    LOG("decl is a function.");
    while (declarations)
    {
      Ptree* declarator = declarations->First();
      declarations = declarations->Rest();
      
      if (declarator->What() == ntDeclarator)
        ((PtreeDeclarator*)declarator)->SetEncodedType("PFv_v");
      else
        LOG("declarator is " << declarator->What());
    }
  }
  else
  {
    LOG("unknown decl type");
  }
  nodeLOG(declarations);
  return 0;
}

//. Translates a declaration, either variable, typedef or function
//. Variables:
//.  [ [modifiers] name [declarators] ; ]
//. Function prototype:
//.  [ [modifiers] name [declarators] ; ]
//. Function impl:
//.  [ [modifiers] name declarator [ { ... } ] ]
//. Typedef:
//.  ?
//. Class definition:
//.  [ [modifiers] [class foo ...] [declarators]? ; ]
Ptree* SWalker::TranslateDeclaration(Ptree* def)
{
  STrace trace("SWalker::TranslateDeclaration");
  // Link any comments added because we are inside a function body
  if (my_links) find_comments(def);

  update_line_number(def);

  my_declaration = def;
  my_store_decl = true;
  Ptree* decls = Ptree::Third(def);

  // Typespecifier may be a class {} etc.
  TranslateTypespecifier(Ptree::Second(def));
  // Or it might be a typeof()
  if (Ptree::Second(def) && Ptree::Second(def)->What() == ntTypeofExpr)
    TranslateTypeof(Ptree::Second(def), decls);

  if (decls->IsA(ntDeclarator))
  {
    // A single declarator is probably a function impl, but could also be
    // the declarator in an if or switch condition
    if (const char* encoded_type = decls->GetEncodedType())
    {
      // A function may be const, skip the C
      while (*encoded_type == 'C') encoded_type++;
      if (*encoded_type != 'F')
      {
        // Not a function
        TranslateDeclarator(decls);
        my_declaration = 0;
        return 0;
      }
    }
    TranslateFunctionImplementation(def);
  }
  else
    // if it is a function prototype or a variable declaration.
    if (!decls->IsLeaf())        // if it is not ";"
      TranslateDeclarators(decls);
  my_declaration = 0;
  return 0;
}

//. [ [ declarator { = <expr> } ] , ... ]
Ptree*
SWalker::TranslateDeclarators(Ptree* decls)
{
  STrace trace("SWalker::TranslateDeclarators");
  Ptree* rest = decls, *p;
  while (rest != nil)
  {
    p = rest->Car();
    if (p->IsA(ntDeclarator))
    {
      TranslateDeclarator(p);
      my_store_decl = false;
    } // if. There is no else..?
    rest = rest->Cdr();
    // Skip comma
    if (rest != nil) rest = rest->Cdr();
  }
  return 0;
}

//. TranslateDeclarator
//. Function proto:
//.   [ { * | & }* name ( [params] ) ]
//. param:
//.   [ [types] { [ { * | & }* name ] { = value } } ]
Ptree*
SWalker::TranslateDeclarator(Ptree* decl)
{
  // REVISIT: Figure out why this method is so HUGE!
  STrace trace("SWalker::TranslateDeclarator");
  // Insert code from occ.cc here
  char* encname = decl->GetEncodedName();
  char* enctype = decl->GetEncodedType();
  if (!encname || !enctype)
  {
    std::cout << "encname or enctype null!" << std::endl;
    return 0;
  }

  try
  {
    // Decide if this is a function or variable
    my_decoder->init(enctype);
    code_iter& iter = my_decoder->iter();
    bool is_const = false;
    while (*iter == 'C')
    {
      ++iter;
      is_const = true;
    }
    if (*iter == 'F')
      return TranslateFunctionDeclarator(decl, is_const);
    else
      return TranslateVariableDeclarator(decl, is_const);
  }
  catch (const TranslateError& e)
  {
    e.set_node(decl);
    throw;
  }
  return 0;
}


Ptree*
SWalker::TranslateFunctionDeclarator(Ptree* decl, bool is_const)
{
  STrace trace("SWalker::TranslateFunctionDeclarator");
  AST::Parameter::vector* is_template = my_template;
  my_template = 0;

  code_iter& iter = my_decoder->iter();
  char* encname = decl->GetEncodedName();
  
  // This is a function. Skip the 'F'
  ++iter;
  
  // Create parameter objects
  Ptree *p_params = decl->Rest();
  while (p_params && !p_params->Car()->Eq('('))
    p_params = Ptree::Rest(p_params);
  if (!p_params)
  {
    std::cout << "Warning: error finding params!" << std::endl;
    return 0;
  }
  std::vector<AST::Parameter*> params;
  TranslateParameters(p_params->Second(), params);
  my_param_cache = params;
  
  // Figure out the return type:
  while (*iter++ != '_') {} // in case of decoding error this is needed
  Types::Type* returnType = my_decoder->decodeType();
  
  // Figure out premodifiers
  std::vector<std::string> premod;
  Ptree* p = Ptree::First(my_declaration);
  while (p)
  {
    premod.push_back(p->Car()->ToString());
    p = Ptree::Rest(p);
  }

  AST::Function* func = 0;
  // Find name:
  if (encname[0] == 'Q')
  {
    // The name is qualified, which introduces a bit of difficulty
    std::vector<std::string> names;
    my_decoder->init(encname);
    my_decoder->decodeQualName(names);
    names.back() += format_parameters(params);
    // A qual name must already be declared, so find it:
    try
    {
      Types::Named* named_type = my_lookup->lookupType(names, true);
      func = Types::declared_cast<AST::Function>(named_type);
    }
    catch (const Types::wrong_type_cast &)
    {
      throw ERROR("Qualified function name wasn't a function:" << names);
    }
    // expand param info, since we now have names for them
    std::vector<AST::Parameter*>::iterator piter = func->parameters().begin();
    std::vector<AST::Parameter*>::iterator pend = func->parameters().end();
    std::vector<AST::Parameter*>::iterator new_piter = params.begin();
    while (piter != pend)
    {
      AST::Parameter* param = *piter++, *new_param = *new_piter++;
      if (!param->name().size() && new_param->name().size())
        param->set_name(new_param->name());
    }
  }
  else
  {
    // Decode the function name
    std::string realname;
    TranslateFunctionName(encname, realname, returnType);
    // Name is same as realname, but with parameters added
    std::string name = realname + format_parameters(params);
    // Append const after params if this is a const function
    if (is_const) name += "const";
    // Create AST::Function object
    func = my_builder->add_function(my_lineno, name, premod, returnType, realname, is_template);
    func->parameters() = params;
  }
  add_comments(func, my_declaration);
  add_comments(func, dynamic_cast<PtreeDeclarator*>(decl));

  // if storing links, find name
  if (my_links)
  {
    // Store for use by TranslateFunctionImplementation
    my_function = func;
    
    // Do decl type first
    if (my_store_decl && my_declaration->Second())
      my_links->link(my_declaration->Second(), returnType);

    p = decl;
    while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&')))
      p = Ptree::Rest(p);
    if (p)
      // p should now be at the name
      my_links->link(p->Car(), func);
  }
  return 0;
}

Ptree*
SWalker::TranslateVariableDeclarator(Ptree* decl, bool is_const)
{
  STrace trace("TranslateVariableDeclarator");
  // Variable declaration. Restart decoding
  char* encname = decl->GetEncodedName();
  char* enctype = decl->GetEncodedType();
  my_decoder->init(enctype);
  // Get type
  Types::Type* type = my_decoder->decodeType();
  std::string name;
  if (my_decoder->isName(encname))
    name = my_decoder->decodeName(encname);
  else if (*encname == 'Q')
  {
    LOG("Scoped name in variable decl!");
    nodeLOG(decl);
    return 0;
  }
  else
  {
    LOG("Unknown name in variable decl!");
    nodeLOG(decl);
    return 0;
  }

  // TODO: implement sizes support
  std::vector<size_t> sizes;
  std::string var_type = my_builder->scope()->type();
  if (var_type == "class" || var_type == "struct" || var_type == "union")
    var_type = "data member";
  else
  {
    if (var_type == "function")
      var_type = "local";
    var_type += " variable";
  }
  AST::Variable* var = my_builder->add_variable(my_lineno, name, type, false, var_type);
  //if (my_declaration->GetComments()) add_comments(var, my_declaration->GetComments());
  //if (decl->GetComments()) add_comments(var, decl->GetComments());
  add_comments(var, my_declaration);
  add_comments(var, dynamic_cast<PtreeDeclarator*>(decl));

  // if storing links, find name
  if (my_links)
  {
    // Do decl type first
    if (my_store_decl && my_declaration->Second())
      my_links->link(my_declaration->Second(), type);
    
    Ptree* p = decl;
    while (p && p->Car()->IsLeaf() && 
           (p->Car()->Eq('*') || p->Car()->Eq('&') || p->Car()->Eq("const")))
    {
      // Link the const keyword
      if (p->Car()->Eq("const"))
        my_links->span(p->Car(), "file-keyword");
      p = Ptree::Rest(p);
    }
    if (p)
    {
      // p should now be at the name
      my_links->link(p->Car(), var);
      
      // Next might be '=' then expr
      p = p->Rest();
      if (p && p->Car() && p->Car()->Eq('='))
      {
        p = p->Rest();
        if (p && p->Car())
          Translate(p->Car());
      }
    }
  }
  return 0;
}

// Fills the vector of Parameter types by parsing p_params.
void
SWalker::TranslateParameters(Ptree* p_params, std::vector<AST::Parameter*>& params)
{
  while (p_params)
  {
    // A parameter has a type, possibly a name and possibly a value
    std::string name, value;
    AST::Parameter::Mods premods, postmods;
    if (p_params->Car()->Eq(','))
      p_params = p_params->Cdr();
    Ptree* param = p_params->First();
    // The type is stored in the encoded type string already
    Types::Type* type = my_decoder->decodeType();
    if (!type)
    {
      std::cout << "Premature end of decoding!" << std::endl;
      break; // 0 means end of encoding
    }
    // Discover contents. Ptree may look like:
    //[register iostate [* a] = [0 + 2]]
    //[register iostate [nil] = 0]
    //[register iostate [nil]]
    //[iostate [nil] = 0]
    //[iostate [nil]]   etc
    if (param->Length() > 1)
    {
      // There is a parameter
      int type_ix, value_ix = -1, len = param->Length();
      if (len >= 4 && param->Nth(len-2)->Eq('='))
      {
        // There is an =, which must be followed by the value
        value_ix = len-1;
        type_ix = len-4;
      }
      else
      {
        // No =, so last is name and second last is type
        type_ix = len-2;
      }
      // Link type
      if (my_links && !param->IsLeaf() && param->Nth(type_ix))
        my_links->link(param->Nth(type_ix), type);
      // Skip keywords (eg: register) which are Leaves
      for (int ix = 0; ix < type_ix && param->Nth(ix)->IsLeaf(); ix++)
      {
        Ptree* leaf = param->Nth(ix);
        premods.push_back(parse_name(leaf));
      }
      // Find name
      if (Ptree* pname = param->Nth(type_ix+1))
      {
        if (pname->Last() && !pname->Last()->IsLeaf() && pname->Last()->First() &&
            pname->Last()->First()->Eq(')') && pname->Length() >= 4)
        {
          // Probably a function pointer type
          // pname is [* [( [* convert] )] ( [params] )]
          // set to [( [* convert] )] from example
          pname = pname->Nth(pname->Length() - 4);
          if (pname && !pname->IsLeaf() && pname->Length() == 3)
          {
            // set to [* convert] from example
            pname = pname->Second();
            if (pname && pname->Second() && pname->Second()->IsLeaf())
              name = parse_name(pname->Second());
          }
        }
        else if (!pname->IsLeaf() && pname->Last() && pname->Last()->Car())
        {
          // * and & modifiers are stored with the name so we must skip them
          Ptree* last = pname->Last()->Car();
          if (!last->Eq('*') && !last->Eq('&'))
            // The last node is the name:
            name = last->ToString();
        }
      }
      // Find value
      if (value_ix >= 0) value = param->Nth(value_ix)->ToString();
    }
    // Add the AST.Parameter type to the list
    params.push_back(new AST::Parameter(premods, type, postmods, name, value));
    p_params = Ptree::Rest(p_params);
  }
}

void SWalker::TranslateFunctionName(char* encname, std::string& realname, Types::Type*& returnType)
{
  STrace trace("SWalker::TranslateFunctionName");
  if (my_decoder->isName(encname))
  {
    if (encname[1] == '@')
    {
      // conversion operator
      my_decoder->init(encname);
      my_decoder->iter() += 2;
      returnType = my_decoder->decodeType();
      realname = "("+my_type_formatter->format(returnType)+")";
    }
    else
    {
      // simple name
      realname = my_decoder->decodeName(encname);
      // operator names are missing the 'operator', add it back
      char c = realname[0];
      if (c == '+' || c == '-' || c == '*' || c == '/' || c == '%'
          || c == '^' || c == '&' || c == '!' || c == '=' || c == '<'
          || c == '>' || c == ',' || c == '(' || c == '['
          || (c == '~' && realname[1] == 0))
        realname = "operator"+realname;
    }
  }
  else if (*encname == 'Q')
  {
    // If a function declaration has a scoped name, then it is not
    // declaring a new function in that scope and can be ignored in
    // the context of synopsis.
    // TODO: maybe needed for syntax stuff?
    return;
  }
  else if (*encname == 'T')
  {
    // Template specialisation.
    // blah<int, int> is T4blah2ii ---> realname = foo<int,int>
    my_decoder->init(encname);
    code_iter& iter = ++my_decoder->iter();
    realname = my_decoder->decodeName()+"<";
    code_iter tend = iter + (*iter - 0x80u);
    iter++; // For some reason, putting this in prev line causes error with 3.2
    bool first = true;
    // Append type names to realname
    while (iter <= tend)
    {
      /*Types::Type* type = */my_decoder->decodeType();
      if (!first) realname+=",";
      else first=false;
      realname += "type"; //type->ToString();
    }
    realname += ">";
  }
  else
    std::cout << "Warning: Unknown function name: " << encname << std::endl;
}

//. Class or Enum
Ptree*
SWalker::TranslateTypespecifier(Ptree* tspec)
{
  STrace trace("SWalker::TranslateTypespecifier");
  Ptree *class_spec = GetClassOrEnumSpec(tspec);
  if (class_spec) Translate(class_spec);
  return 0;
}

Ptree*
SWalker::TranslateTypedef(Ptree* node)
{
  STrace trace("SWalker::TranslateTypedef");
  if (my_links) my_links->span(node->First(), "file-keyword");
  /* Ptree *tspec = */
  TranslateTypespecifier(node->Second());
  my_declaration = node;
  my_store_decl = true;
  for (Ptree *declarator = node->Third(); declarator; declarator = declarator->ListTail(2))
    TranslateTypedefDeclarator(declarator->Car());
  return 0;
}

void SWalker::TranslateTypedefDeclarator(Ptree* node)
{
  if (node->What() != ntDeclarator) return;
  char* encname = node->GetEncodedName();
  char* enctype = node->GetEncodedType();
  if (!encname || !enctype) return;

  update_line_number(node);

  // Get type of declarator
  my_decoder->init(enctype);
  Types::Type* type = my_decoder->decodeType();
  // Get name of typedef
  std::string name = my_decoder->decodeName(encname);
  // Create typedef object
  AST::Typedef* tdef = my_builder->add_typedef(my_lineno, name, type, false);
  add_comments(tdef, dynamic_cast<PtreeDeclarator*>(node));

  // if storing links, find name
  if (my_links)
  {
    if (my_store_decl && my_declaration->Second())
      my_links->link(my_declaration->Second(), type);
    Ptree* p = node;
    while (p && p->Car()->IsLeaf() && (p->Car()->Eq('*') || p->Car()->Eq('&')))
      p = Ptree::Rest(p);
    if (p)
      // p should now be at the name
      my_links->link(p->Car(), tdef);
  }
}

Ptree*
SWalker::TranslateFunctionImplementation(Ptree* node)
{
  STrace trace("SWalker::TranslateFunctionImplementation");
  my_function = 0;
  my_params.clear();
  TranslateDeclarator(node->Third());
  if (!my_filter->should_visit_function_impl(my_file)) return 0;
  if (!my_function)
  {
    std::cerr << "Warning: function was null!" << std::endl;
    return 0;
  }

  FuncImplCache cache;
  cache.func = my_function;
  cache.params = my_param_cache;
  cache.body = node->Nth(3);

  if (dynamic_cast<AST::Class*>(my_builder->scope()))
    my_func_impl_stack.back().push_back(cache);
  else TranslateFuncImplCache(cache);
  return 0;
}

void
SWalker::TranslateFuncImplCache(const FuncImplCache& cache)
{
  STrace trace("SWalker::TranslateFuncImplCache");
  // We create a dummy namespace with the name of the function. Any
  // declarations in the function are added to this dummy namespace. Once we
  // are done, we remove it from the parent scope (its not much use in the
  // documents)
  std::vector<std::string> name = cache.func->name();
  name.back() = "`"+name.back();
  my_builder->start_function_impl(name);
  try
  {
    // Add parameters
    std::vector<AST::Parameter*>::const_iterator iter, end;
    iter = cache.params.begin();
    end = cache.params.end();
    while (iter != end)
    {
      AST::Parameter* param = *iter++;
      // Make sure the parameter is named
      if (param->name().size())
        my_builder->add_variable(my_lineno, param->name(), param->type(), false, "parameter");
    }
    // Add 'this' if method
    my_builder->add_this_variable();
    // Translate the function body
    TranslateBlock(cache.body);
  }
  catch (...)
  {
    LOG("Cleaning up func impl cache");
    my_builder->end_function_impl();
    throw;
  }
  my_builder->end_function_impl();
}

Ptree*
SWalker::TranslateAccessSpec(Ptree* spec)
{
  STrace trace("SWalker::TranslateAccessSpec");
  AST::Access axs = AST::Default;
  switch (spec->First()->What())
  {
    case PUBLIC:
      axs = AST::Public;
      break;
    case PROTECTED:
      axs = AST::Protected;
      break;
    case PRIVATE:
      axs = AST::Private;
      break;
  }
  my_builder->set_access(axs);
  if (my_links) my_links->span(spec->First(), "file-keyword");
  return 0;
}

/* Enum Spec
 *  [ enum [name] [{ [name [= value] ]* }] ]
 */
Ptree*
SWalker::TranslateEnumSpec(Ptree *spec)
{
  //update_line_number(spec);
  if (my_links) my_links->span(spec->First(), "file-keyword");
  if (!spec->Second())
  {
    return 0; /* anonymous enum */
  }
  std::string name = spec->Second()->ToString();

  update_line_number(spec);
  int enum_lineno = my_lineno;
  // Parse enumerators
  std::vector<AST::Enumerator*> enumerators;
  Ptree* penum = spec->Third()->Second();
  AST::Enumerator* enumor;
  while (penum)
  {
    update_line_number(penum);
    Ptree* penumor = penum->First();
    if (penumor->IsLeaf())
    {
      // Just a name
      enumor = my_builder->add_enumerator(my_lineno, penumor->ToString(), "");
      add_comments(enumor, static_cast<CommentedLeaf*>(penumor)->GetComments());
      if (my_links) my_links->link(penumor, enumor);
    }
    else
    {
      // Name = Value
      std::string name = penumor->First()->ToString(), value;
      if (penumor->Length() == 3)
        value = penumor->Third()->ToString();
      enumor = my_builder->add_enumerator(my_lineno, name, value);
      add_comments(enumor, dynamic_cast<CommentedLeaf*>(penumor->First()));
      if (my_links) my_links->link(penumor->First(), enumor);
    }
    enumerators.push_back(enumor);
    penum = Ptree::Rest(penum);
    // Skip comma
    if (penum && penum->Car() && penum->Car()->Eq(',')) penum = Ptree::Rest(penum);
  }
  Ptree* close = spec->Third()->Third();
  enumor = new AST::Enumerator(my_file, my_lineno, "dummy", my_dummyname, "");
  add_comments(enumor, static_cast<CommentedLeaf*>(close));
  enumerators.push_back(enumor);
  
  // Create AST.Enum object
  AST::Enum* theEnum = my_builder->add_enum(enum_lineno,name,enumerators);
  add_comments(theEnum, my_declaration);
  if (my_declaration)
  {
    // Enum declared inside declaration. Comments for the declaration
    // belong to the enum. This is policy. #TODO review policy
    //my_declaration->SetComments(nil); ?? typedef doesn't have comments?
  }
  if (my_links) my_links->link(spec->Second(), theEnum);
  return 0;
}


Ptree*
SWalker::TranslateUsing(Ptree* node)
{
  STrace trace("SWalker::TranslateUsing");
  // [ using Foo :: x ; ]
  // [ using namespace Foo ; ]
  // [ using namespace Foo = Bar ; ]
  if (my_links) my_links->span(node->First(), "file-keyword");
  bool is_namespace = false;
  Ptree *p = node->Rest();
  if (p->First()->Eq("namespace"))
  {
    if (my_links) my_links->span(p->First(), "file-keyword");
    // Find namespace to alias
    p = p->Rest();
    is_namespace = true;
  }
  // Find name that we are looking up, and make a new ptree list for linking it
  Ptree *p_name = Ptree::Snoc(nil, p->Car());
  ScopedName name;
  if (p->First()->Eq("::"))
    // Eg; "using ::memcpy;" Indicate global scope with empty first
    name.push_back("");
  else
  {
    name.push_back(parse_name(p->First()));
    p = p->Rest();
  }
  while (p->First()->Eq("::"))
  {
    p_name = Ptree::Snoc(p_name, p->Car()); // Add '::' to p_name
    p = p->Rest();
    name.push_back(parse_name(p->First()));
    p_name = Ptree::Snoc(p_name, p->Car()); // Add identifier to p_name
    p = p->Rest();
  }
  // Resolve and link name
  try
  {
    Types::Named* type = my_lookup->lookupType(name);
    if (my_links) my_links->link(p_name, type);
    if (is_namespace)
    {
      // Check for '=' alias
      if (p->First()->Eq("="))
      {
        p = p->Rest();
        std::string alias = parse_name(p->First());
        my_builder->add_aliased_using_namespace(type, alias);
      }
      else my_builder->add_using_namespace(type);
    }
    else
      // Let builder do all the work
      my_builder->add_using_declaration(type);
  }
  catch (const TranslateError& e)
  {
    LOG("Oops!");
    e.set_node(node);
    throw;
  }
  return 0;
}
