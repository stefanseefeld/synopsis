//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Python.h>
#include <iostream>
#include <string>
#include <typeinfo>
#include <sstream>
#include <algorithm>

#include <Synopsis/Buffer.hh>
#include <Synopsis/PTree.hh>
#include <Synopsis/PTree/Writer.hh>
#include <Synopsis/PTree/Display.hh>

#include "swalker.hh"
#include "strace.hh"
#include "type.hh"
#include "ast.hh"
#include "builder.hh"
#include "decoder.hh"
#include "dumper.hh"
#include "linkstore.hh"
#include "lookup.hh"
#include "filter.hh"

using namespace Synopsis;
using namespace AST;

#ifdef DO_TRACE
int STrace::slevel = 0, STrace::dlevel = 0;
std::ostringstream* STrace::stream = 0;
STrace::string_list STrace::m_list;
std::ostream& STrace::operator <<(PTree::Node *p)
{
  std::ostream& out = operator <<("-");
  PTree::display(p, out, true);
  return out;
}
#endif

SWalker *SWalker::g_swalker = 0;

SWalker::SWalker(FileFilter* filter, Builder* builder, Buffer* buffer)
  : Walker(buffer),
    my_builder(builder),
    my_filter(filter),
    my_buffer(buffer),
    my_decoder(new Decoder(my_builder)),
    my_declaration(0),
    my_template(0),
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
SWalker::parse_name(PTree::Node *node) const
{
  return PTree::reify(node);
}

void
SWalker::set_store_links(LinkStore* links)
{
  my_links = links;
}

int SWalker::line_of_ptree(PTree::Node *node)
{
  update_line_number(node);
  return my_lineno;
}

// Updates the line number stored in this SWalker instance, and the filename
// stored in the Builder instance at my_builder. The filename is only set if
// the actual char* changed (which only happens when the preprocessor places
// another #line directive)
void SWalker::update_line_number(PTree::Node *ptree)
{
  // Ask the Parser for the linenumber of the ptree. This used to be
  // expensive until I hacked buffer.cc to cache the last line number found.
  // Now it's okay as long as you are looking for lines sequentially.
  std::string filename;
  my_lineno = my_buffer->origin(ptree->begin(), filename);
  if (filename != my_filename)
  {
    my_filename = filename;
    my_file = my_filter->get_sourcefile(my_filename.c_str());
    my_builder->set_file(my_file);
  }
}

AST::Comment *
make_Comment(SourceFile* file, int line, PTree::Node *first, bool suspect=false)
{
  return new AST::Comment(file, line, PTree::reify(first), suspect);
}

PTree::Atom *make_Leaf(const char *pos, size_t len)
{
    return new PTree::Atom(pos, len);
}

// Adds the given comments to the given declaration. If my_links is set,
// then syntax highlighting information is also stored.
void
SWalker::add_comments(AST::Declaration* decl, PTree::Node *node)
{
  if (!node) return;
  
  AST::Comment::vector comments;

  // First, make sure that node is a list of comments
  if (PTree::type_of(node) == Token::ntDeclaration)
    node = static_cast<PTree::Declaration*>(node)->get_comments();

  // Loop over all comments in the list
  for (PTree::Node *next = PTree::rest(node); node && !node->is_atom(); next = PTree::rest(node))
  {
    PTree::Node *first = PTree::first(node);
    if (!first || !first->is_atom())
    {
      node = next;
      continue;
    }
    update_line_number(node);

    // Check if comment is continued, eg: consecutive C++ comments
    while (next && PTree::first(next) && PTree::first(next)->is_atom())
    {
      if (!strncmp(first->position() + first->length() - 2, "*/", 2))
        break;
      if (strncmp(PTree::first(next)->position(), "//", 2))
        break;
      const char *next_pos = PTree::first(next)->position();
      const char *start_pos = PTree::first(node)->position();
      const char *curr_pos = start_pos + PTree::first(node)->length();
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
      int len = int(next_pos - start_pos + PTree::first(next)->length());
      //node->SetCar(first = new Leaf(start_pos, len));
      node->set_car(first = make_Leaf(start_pos, len));
      // Skip the combined comment
      next = PTree::rest(next);
    }

    // all comments that are not immediately (i.e. separated
    // by a single new line) followed by a declaration are
    // marked as 'suspect'
    bool suspect = false;
    const char *pos = first->position() + first->length();
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
      AST::Comment* comment = make_Comment(my_file, my_lineno, first, suspect);
      comments.push_back(comment);
    }
    if (my_links) my_links->long_span(first, "file-comment");
    // Set first to 0 so we dont accidentally do them twice (eg:
    // when parsing expressions)
    node->set_car(0);
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
void SWalker::add_comments(AST::Declaration* decl, PTree::CommentedAtom *node)
{
  if (node) add_comments(decl, node->get_comments());
}
void SWalker::add_comments(AST::Declaration* decl, PTree::Declaration* node)
{
  if (node) add_comments(decl, node->get_comments());
}
void SWalker::add_comments(AST::Declaration* decl, PTree::Declarator* node)
{
  if (node) add_comments(decl, node->get_comments());
}
void SWalker::add_comments(AST::Declaration* decl, PTree::NamespaceSpec* node)
{
  if (node) add_comments(decl, node->get_comments());
}
void SWalker::find_comments(PTree::Node *node)
{
  PTree::Node *leaf, *parent;
  leaf = FindLeftLeaf(node, parent);
  if (leaf) add_comments(0, dynamic_cast<PTree::CommentedAtom *>(leaf));
}

PTree::Node *SWalker::translate_arg_decl_list(bool, PTree::Node *, PTree::Node *)
{
  STrace trace("SWalker::translate_arg_decl_list NYI");
  return 0;
}

PTree::Node *SWalker::translate_initialize_args(PTree::Declarator*, PTree::Node *)
{
  STrace trace("SWalker::translate_initialize_args NYI");
  return 0;
}

PTree::Node *SWalker::translate_assign_initializer(PTree::Declarator*, PTree::Node *)
{
  STrace trace("SWalker::translate_assign_initializer NYI");
  return 0;
}

//Class* SWalker::MakeClassMetaobject(Ptree*, Ptree*, Ptree*) { STrace trace("SWalker::MakeClassMetaobject NYI"); return 0; }
//PTree::Node *SWalker::TranslateClassSpec(Ptree*, Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateClassSpec NYI"); return 0; }
//PTree::Node *SWalker::TranslateClassBody(Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateClassBody NYI"); return 0; }
//PTree::Node *SWalker::TranslateTemplateInstantiation(Ptree*, Ptree*, Ptree*, Class*) { STrace trace("SWalker::TranslateTemplateInstantiation NYI"); return 0; }

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
SWalker::translate(PTree::Node *node)
{
  STrace trace("SWalker::translate");
  try
  {
    Walker::translate(node);
  }
  // Debug and non-debug modes handle these very differently
#ifdef DEBUG
  catch (const TranslateError& e)
  {
    if (e.node) node = e.node;
    std::string filename;
    unsigned long line = my_buffer->origin(node->begin(),
					   filename);
    LOG("Warning: An exception occurred:" << " (" << filename << ":" << line << ")");
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
    std::string filename;
    unsigned long line = my_buffer->origin(node->begin(), filename);
    std::cout << " (" << filename << ":" << line << ")" << std::endl;
  }
  catch (...)
  {
    std::cout << "Warning: An unknown exception occurred: " << std::endl;
    std::cout << "At: ";
    std::string filename;
    unsigned long line = my_buffer->origin(node->begin(), filename);
    std::cout << " (" << filename << ":" << line << ")" << std::endl;
  }
#endif
}

// Default translate, usually means a literal
void SWalker::visit(PTree::Atom *node)
{
  STrace trace("SWalker::visit(PTree::Atom *)");
  // Determine type of node
  std::string s = PTree::reify(node);
  const char *str = s.c_str();
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
        else if (num_type == "double") num_type = "long double";
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
  else if (*str == '/' && !node->is_atom())
  {
    // Assume comment. Must be a list of comments!
    AST::Declaration* decl;
    update_line_number(node);
    decl = my_builder->add_tail_comment(my_lineno);
    add_comments(decl, dynamic_cast<PTree::CommentedAtom *>(node));
  }
  else
  {
#ifdef DEBUG
    STrace trace("SWalker::TranslatePtree");
    LOG("Warning: Unknown Ptree "<<PTree::type_of(node));
    nodeLOG(node);
    //*((char*)0) = 1; // force breakpoint, or core dump :)
#endif
  }
}

// As various nodes may through due to incomplete symbol
// lookup support, we need an 'exception firewall' here to
// be able to traverse the following nodes
void SWalker::visit(PTree::List *node)
{
  for (PTree::Node *i = node; i; i = i->cdr())
    if (i->car())
      try { i->car()->accept(this);}
      catch (const TranslateError &) {}
}

//. NamespaceSpec
void SWalker::visit(PTree::NamespaceSpec *node)
{
  STrace trace("SWalker::visit(PTree::NamespaceSpec *)");
  
  PTree::Node *pNamespace = PTree::first(node);
  PTree::Node *pIdentifier = PTree::second(node);
  PTree::Node *pBody = PTree::third(node);

  if (my_links) my_links->span(pNamespace, "file-keyword");
  else update_line_number(node);

  // Start the namespace
  AST::Namespace* ns;
  if (pIdentifier)
  {
    ns = my_builder->start_namespace(parse_name(pIdentifier), NamespaceNamed);
    ns->set_file(my_file);
  }
  else ns = my_builder->start_namespace(my_file->filename(), NamespaceAnon);

  // Add comments
  add_comments(ns, dynamic_cast<PTree::NamespaceSpec*>(node));
  if (my_links && PTree::first(pIdentifier)) my_links->link(pIdentifier, ns);

  // Translate the body
  translate(pBody);

  // End the namespace
  my_builder->end_namespace();
}

//. [ : (public|private|protected|0) <name> {, ...} ]
std::vector<Inheritance*> SWalker::translate_inheritance_spec(PTree::Node *node)
{
  STrace trace("SWalker::translate_inheritance_spec");
  std::vector<Inheritance*> ispec;
  Types::Type *type;
  while (node)
  {
    node = node->cdr(); // skip : or ,
    // the attributes
    std::vector<std::string> attributes(PTree::length(node->car()) - 1);
    for (int i = 0; i != PTree::length(node->car()) - 1; ++i)
    {
      attributes[i] = parse_name(PTree::nth(node->car(), i));
      if (my_links) my_links->span(PTree::nth(node->car(), i), "file-keyword");
    }
    // look up the parent type
    PTree::Node *name = PTree::last(node->car())->car();
    if (name->is_atom())
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
      my_decoder->init(name->encoded_name());
      type = my_decoder->decodeType();
    }
    if (my_links) my_links->link(name, type);

    node = node->cdr();
    // add it to the list
    ispec.push_back(new AST::Inheritance(type, attributes));
  }
  return ispec;
}

void SWalker::visit(PTree::ClassSpec *node)
{
  // REVISIT: figure out why this method is so long
  STrace trace("SWalker::visit(PTree::ClassSpec*)");
  enum { SizeForwardDecl = 2, SizeAnonClass = 3, SizeClass = 4 };

  AST::Parameter::vector* is_template = my_template;
  my_template = 0;

  int size = PTree::length(node);

  if (size == SizeForwardDecl)
  {
    // Forward declaration
    // [ class|struct <name> ]
    std::string name = parse_name(PTree::second(node));
    if (is_template)
      LOG("Templated class forward declaration " << name);
    my_builder->add_forward(my_lineno, name, is_template);
    if (my_links)
    { // highlight the comments, at least
      PTree::ClassSpec* cspec = static_cast<PTree::ClassSpec*>(node);
      add_comments(0, cspec->get_comments());
    }
    return;
  }
  PTree::Node *pClass = PTree::first(node);
  PTree::Node *pName = 0, *pInheritance = 0;
  PTree::ClassBody *pBody = 0;
  if (size == SizeClass)
  {
    // [ class|struct <name> <inheritance> [{ body }] ]
    pName = PTree::nth(node, 1);
    pInheritance = PTree::nth(node, 2);
    pBody = dynamic_cast<PTree::ClassBody *>(PTree::nth(node, 3));
  }
  else if (size == SizeAnonClass)
    // An anonymous struct. OpenC++ encodes us a unique
    // (may be qualified if nested) name
    // [ struct [nil nil] [{ ... }] ]
    pBody = dynamic_cast<PTree::ClassBody *>(PTree::nth(node, 2));
  else
    throw nodeERROR(node, "Class node has bad length: " << size);

  if (my_links) my_links->span(pClass, "file-keyword");
  else update_line_number(node);

  // Create AST.Class object
  AST::Class *clas;
  std::string type = parse_name(pClass);
  PTree::Encoding enc = node->encoded_name();
  my_decoder->init(enc);
  if (enc.at(0) == 'T')
  {
    // Specialization
    // TODO: deal with this.
    // Eg: /usr/include/g++-3/std/straits.h
    // search: /^struct string_char_traits <char> {/
    // encname: "T\222string_char_traits\201c"
    LOG("Specialization?");
    nodeLOG(node);
    LOG("encname:"<< enc);
    Types::Parameterized* param = dynamic_cast<Types::Parameterized*>(my_decoder->decodeTemplate());
    // If a non-type param was found, it's name will be '*'
    for (size_t i = 0; i < param->parameters().size(); i++)
      if (Types::Dependent* dep = dynamic_cast<Types::Dependent*>(param->parameters()[i]))
      {
        if (dep->name().size() == 1 && dep->name()[0] == "*")
        {
          // Find the value of this parameter
          std::string name = parse_name(PTree::nth(PTree::second(PTree::second(pName)), i*2));
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
  else if (enc.at(0) == 'Q')
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
    clas->parents() = translate_inheritance_spec(pInheritance);
    my_builder->update_class_base_search();
  }

  // Add comments
  PTree::ClassSpec* cspec = static_cast<PTree::ClassSpec*>(node);
  add_comments(clas, cspec->get_comments());

  // Push the impl stack for a cache of func impls
  my_func_impl_stack.push_back(FuncImplVec());

  // Translate the body of the class
  translate(pBody);

  // Translate any func impls inlined in the class
  FuncImplVec& vec = my_func_impl_stack.back();
  FuncImplVec::iterator iter = vec.begin();
  while (iter != vec.end()) translate_func_impl_cache(*iter++);
  my_func_impl_stack.pop_back();

  my_builder->end_class();
}

PTree::TemplateDecl *
SWalker::translate_template_class(PTree::TemplateDecl *def, PTree::ClassSpec *node)
{
  STrace trace("SWalker::translate_template_class");
  AST::Parameter::vector* old_params = my_template;
  update_line_number(def);
  my_builder->start_template();
  try
  {
    translate_template_params(PTree::third(def));
    visit(node);
  }
  catch (...)
  {
    my_builder->end_template();
    my_template = old_params;
    throw;
  }
  my_builder->end_template();
  my_template = old_params;
}

void SWalker::translate_template_params(PTree::Node *params)
{
  STrace trace("SWalker::translate_template_params");
  my_template = new AST::Parameter::vector;
  AST::Parameter::vector& templ_params = *my_template;
  // Declare some default parameter values - these should not be modified!
  std::string name, value;
  AST::Parameter::Mods pre_mods, post_mods;
  while (params)
  {
    PTree::Node *param = PTree::first(params);
    nodeLOG(param);
    if (*PTree::first(param) == "class" || *PTree::first(param) == "typename")
    {
      // Ensure that there is an identifier (it is optional!)
      if (param->cdr() && PTree::second(param))
      {
        // This parameter specifies a type, add as dependent
        Types::Dependent* dep = my_builder->create_dependent(parse_name(PTree::second(param)));
        my_builder->add(dep);
        AST::Parameter::Mods paramtype;
        paramtype.push_back(parse_name(PTree::first(param)));
        templ_params.push_back(new AST::Parameter(paramtype, dep, post_mods, name, value));
      }
      else
      {
        // Add a parameter, but with no name
        AST::Parameter::Mods paramtype;
        paramtype.push_back(parse_name(PTree::first(param)));
        templ_params.push_back(new AST::Parameter(paramtype, 0, post_mods, name, value));
      }
    }
    else if (*PTree::first(param) == "template")
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
      PTree::Node *p = PTree::second(param);
      while (p && p->car() && p->car()->is_atom() && (*p->car() == '*' || *p->car() == '&'))
        p = PTree::rest(p);
      std::string name = parse_name(p);
      Types::Dependent* dep = my_builder->create_dependent(name);
      my_builder->add(dep);
      // Figure out the type of the param
      my_decoder->init(PTree::second(param)->encoded_type());
      Types::Type* param_type = my_decoder->decodeType();
      templ_params.push_back(new AST::Parameter(pre_mods, param_type, post_mods, name, value));
    }
    // Skip comma
    params = PTree::rest(PTree::rest(params));
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

PTree::TemplateDecl *
SWalker::translate_template_function(PTree::TemplateDecl *def, PTree::Node *node)
{
  STrace trace("SWalker::translate_template_function");
  nodeLOG(def);
  nodeLOG(node);
  PTree::Declaration *decl = dynamic_cast<PTree::Declaration *>(node);
  if (!decl)
  {
    LOG("Warning: Unknown node type in template");
    nodeLOG(def);
    return 0;
  }

  LOG("Encoded name is: " << node->encoded_name());

  AST::Parameter::vector* old_params = my_template;
  update_line_number(def);
  my_builder->start_template();
  try
  {
    translate_template_params(PTree::third(def));
    visit(decl);
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
void SWalker::visit(PTree::LinkageSpec *node)
{
  STrace trace("SWalker::visit(LinkageSpec*)");
  translate(PTree::third(node));
}

//. Block
void SWalker::visit(PTree::Block *node)
{
  STrace trace("SWalker::visit(PTree::Block *");
  PTree::Node *rest = PTree::second(node);
  while (rest)
  {
    translate(rest->car());
    rest = rest->cdr();
  }
  PTree::Node *close = PTree::third(node);
  AST::Declaration *decl;
  decl = my_builder->add_tail_comment(my_lineno);
  add_comments(decl, dynamic_cast<PTree::CommentedAtom *>(close));
}

//. Brace
void SWalker::visit(PTree::Brace *node)
{
  STrace trace("SWalker::visit(PTree::Brace *)");
  PTree::Node *rest = PTree::second(node);
  while (rest)
  {
    translate(rest->car());
    rest = rest->cdr();
  }
  PTree::Node *close = PTree::third(node);
  AST::Declaration *decl;
  decl = my_builder->add_tail_comment(my_lineno);
  add_comments(decl, dynamic_cast<PTree::CommentedAtom *>(close));
}

//. TemplateDecl
void SWalker::visit(PTree::TemplateDecl *node)
{
  STrace trace("SWalker::visit(PTree::TemplateDecl*)");
  PTree::Node *body = PTree::nth(node, 4);
  PTree::ClassSpec *class_spec = get_class_template_spec(body);
  if(class_spec) translate_template_class(node, class_spec);
  else translate_template_function(node, body);
}

//. A typeof(expr) expression evaluates to the type of 'expr'. This is a GNU
//. GCC extension!
//. Since the OCC parser can't resolve the type properly, we try to do it here
//. and modify the type of the declarations to set it
PTree::Node *SWalker::translate_typeof(PTree::Node *spec, PTree::Node *declarations)
{
  STrace trace("SWalker::translate_typeof");
  nodeLOG(spec);
  PTree::Encoding enc = PTree::third(spec)->encoded_name();
  LOG("The name is: " << enc);
  LOG("The type is: " << PTree::third(spec)->encoded_type());
  // Find the type referred to by the expression
  if (!my_decoder->isName(enc))
  {
    LOG("typeof is not a simple name: ");
    nodeLOG(spec);
    return 0;
  }
  std::string name = my_decoder->decodeName(enc);
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
      PTree::Node *declarator = PTree::first(declarations);
      declarations = PTree::rest(declarations);
      
      if (PTree::type_of(declarator) == Token::ntDeclarator)
        ((PTree::Declarator*)declarator)->set_encoded_type("PFv_v");
      else
        LOG("declarator is " << PTree::type_of(declarator));
    }
  }
  else
  {
    LOG("unknown decl type");
  }
  nodeLOG(declarations);
  return 0;
}

void SWalker::visit(PTree::Declaration *node)
{
  STrace trace("SWalker::visit(PTree::Declaration*)");
  // Link any comments added because we are inside a function body
  if (my_links) find_comments(node);

  update_line_number(node);

  my_declaration = node;
  my_store_decl = true;
  PTree::Node *decls = PTree::third(node);

  // Typespecifier may be a class {} etc.
  translate_type_specifier(PTree::second(node));
  // Or it might be a typeof()
  if (PTree::second(node) && PTree::type_of(PTree::second(node)) == Token::ntTypeofExpr)
    translate_typeof(PTree::second(node), decls);

  if (PTree::is_a(decls, Token::ntDeclarator))
  {
    // A single declarator is probably a function impl, but could also be
    // the declarator in an if or switch condition
    PTree::Encoding enc = decls->encoded_type();
    if (!enc.empty())
    {
      // A function may be const, skip the C
      PTree::Encoding::iterator i = enc.begin();
      while (*i == 'C') ++i;
      if (*i != 'F')
      {
        // Not a function
        translate_declarator(decls);
        my_declaration = 0;
        return;
      }
    }
    translate_function_implementation(node);
  }
  else
    // if it is a function prototype or a variable declaration.
    if (!decls->is_atom())        // if it is not ";"
      translate_declarators(decls);
  my_declaration = 0;
}

PTree::Node *
SWalker::translate_declarators(PTree::Node *decls)
{
  STrace trace("SWalker::translate_declarators");
  PTree::Node *rest = decls, *p;
  while (rest != 0)
  {
    p = rest->car();
    if (PTree::is_a(p, Token::ntDeclarator))
    {
      translate_declarator(p);
      my_store_decl = false;
    } // if. There is no else..?
    rest = rest->cdr();
    // Skip comma
    if (rest != 0) rest = rest->cdr();
  }
  return 0;
}

//. translate_declarator
//. Function proto:
//.   [ { * | & }* name ( [params] ) ]
//. param:
//.   [ [types] { [ { * | & }* name ] { = value } } ]
PTree::Node *
SWalker::translate_declarator(PTree::Node *decl)
{
  // REVISIT: Figure out why this method is so HUGE!
  STrace trace("SWalker::translate_declarator");
  // Insert code from occ.cc here
  PTree::Encoding encname = decl->encoded_name();
  PTree::Encoding enctype = decl->encoded_type();
  if (encname.empty() || enctype.empty())
  {
    std::cout << "encname or enctype empty !" << std::endl;
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
      return translate_function_declarator(decl, is_const);
    else
      return translate_variable_declarator(decl, is_const);
  }
  catch (const TranslateError& e)
  {
    e.set_node(decl);
    throw;
  }
  return 0;
}


PTree::Node *
SWalker::translate_function_declarator(PTree::Node *decl, bool is_const)
{
  STrace trace("SWalker::translate_function_declarator");
  AST::Parameter::vector* is_template = my_template;
  my_template = 0;

  code_iter& iter = my_decoder->iter();
  PTree::Encoding encname = decl->encoded_name();
  
  // This is a function. Skip the 'F'
  ++iter;
  
  // Create parameter objects
  PTree::Node *p_params = PTree::rest(decl);
  while (p_params && p_params->car() && *p_params->car() != '(')
    p_params = PTree::rest(p_params);
  if (!p_params)
  {
    std::cout << "Warning: error finding params!" << std::endl;
    return 0;
  }
  std::vector<AST::Parameter*> params;
  translate_parameters(PTree::second(p_params), params);
  my_param_cache = params;
  
  // Figure out the return type:
  while (*iter++ != '_') {} // in case of decoding error this is needed
  Types::Type* returnType = my_decoder->decodeType();
  
  // Figure out premodifiers
  std::vector<std::string> premod;
  PTree::Node *p = PTree::first(my_declaration);
  while (p)
  {
    premod.push_back(PTree::reify(p->car()));
    p = PTree::rest(p);
  }

  AST::Function* func = 0;
  // Find name:
  if (encname.at(0) == 'Q')
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
    translate_function_name(encname, realname, returnType);
    // Name is same as realname, but with parameters added
    std::string name = realname + format_parameters(params);
    // Append const after params if this is a const function
    if (is_const) name += "const";
    // Create AST::Function object
    func = my_builder->add_function(my_lineno, name, premod, returnType, realname, is_template);
    func->parameters() = params;
  }
  add_comments(func, my_declaration);
  add_comments(func, dynamic_cast<PTree::Declarator*>(decl));

  // if storing links, find name
  if (my_links)
  {
    // Store for use by TranslateFunctionImplementation
    my_function = func;
    
    // Do decl type first
    if (my_store_decl && PTree::second(my_declaration))
      my_links->link(PTree::second(my_declaration), returnType);

    p = decl;
    while (p && p->car()->is_atom() && (*p->car() == '*' || *p->car() == '&'))
      p = PTree::rest(p);
    if (p)
      // p should now be at the name
      my_links->link(p->car(), func);
  }
  return 0;
}

PTree::Node*
SWalker::translate_variable_declarator(PTree::Node *decl, bool is_const)
{
  STrace trace("translate_variable_declarator");
  // Variable declaration. Restart decoding
  PTree::Encoding encname = decl->encoded_name();
  PTree::Encoding enctype = decl->encoded_type();
  my_decoder->init(enctype);
  // Get type
  Types::Type* type = my_decoder->decodeType();
  std::string name;
  if (my_decoder->isName(encname))
    name = my_decoder->decodeName(encname);
  else if (encname.at(0) == 'Q')
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
  //if (my_declaration->get_comments()) add_comments(var, my_declaration->get_comments());
  //if (decl->get_comments()) add_comments(var, decl->get_comments());
  add_comments(var, my_declaration);
  add_comments(var, dynamic_cast<PTree::Declarator*>(decl));

  // if storing links, find name
  if (my_links)
  {
    // Do decl type first
    if (my_store_decl && PTree::second(my_declaration))
      my_links->link(PTree::second(my_declaration), type);
    
    PTree::Node *p = decl;
    while (p && p->car()->is_atom() && 
           (*p->car() == '*' || *p->car() == '&' || *p->car() == "const"))
    {
      // Link the const keyword
      if (*p->car() == "const")
        my_links->span(p->car(), "file-keyword");
      p = PTree::rest(p);
    }
    if (p)
    {
      // p should now be at the name
      my_links->link(p->car(), var);
      
      // Next might be '=' then expr
      p = PTree::rest(p);
      if (p && p->car() && *p->car() == '=')
      {
        p = PTree::rest(p);
        if (p && p->car())
          translate(p->car());
      }
    }
  }
  return 0;
}

// Fills the vector of Parameter types by parsing p_params.
void
SWalker::translate_parameters(PTree::Node *p_params, std::vector<AST::Parameter*>& params)
{
  STrace trace("SWalker::translate_parameters");
  while (p_params)
  {
    // A parameter has a type, possibly a name and possibly a value
    std::string name, value;
    AST::Parameter::Mods premods, postmods;
    if (*p_params->car() == ',')
      p_params = p_params->cdr();
    PTree::Node *param = PTree::first(p_params);
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
    if (PTree::length(param) > 1)
    {
      // There is a parameter
      int type_ix, value_ix = -1, len = PTree::length(param);
      if (len >= 4 && *PTree::nth(param, len-2) == '=')
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
      if (my_links && !param->is_atom() && PTree::nth(param, type_ix))
        my_links->link(PTree::nth(param, type_ix), type);
      // Skip keywords (eg: register) which are Leaves
      for (int ix = 0; ix < type_ix && PTree::nth(param, ix)->is_atom(); ix++)
      {
        PTree::Node *leaf = PTree::nth(param, ix);
        premods.push_back(parse_name(leaf));
      }
      // Find name
      if (PTree::Node *pname = PTree::nth(param, type_ix+1))
      {
        if (PTree::last(pname) && !PTree::last(pname)->is_atom() && 
	    PTree::first(PTree::last(pname)) &&
            *PTree::first(PTree::last(pname)) == ')' && PTree::length(pname) >= 4)
        {
          // Probably a function pointer type
          // pname is [* [( [* convert] )] ( [params] )]
          // set to [( [* convert] )] from example
          pname = PTree::nth(pname, PTree::length(pname) - 4);
          if (pname && !pname->is_atom() && PTree::length(pname) == 3)
          {
            // set to [* convert] from example
            pname = PTree::second(pname);
            if (pname && PTree::second(pname) && PTree::second(pname)->is_atom())
              name = parse_name(PTree::second(pname));
          }
        }
        else if (!pname->is_atom() && PTree::last(pname) && PTree::last(pname)->car())
        {
          // * and & modifiers are stored with the name so we must skip them
          PTree::Node *last = PTree::last(pname)->car();
          if (*last != '*' && *last != '&')
            // The last node is the name:
            name = PTree::reify(last);
        }
      }
      // Find value
      if (value_ix >= 0) value = PTree::reify(PTree::nth(param, value_ix));
    }
    // Add the AST.Parameter type to the list
    params.push_back(new AST::Parameter(premods, type, postmods, name, value));
    p_params = PTree::rest(p_params);
  }
}

void SWalker::translate_function_name(const PTree::Encoding &encname, std::string& realname, Types::Type*& returnType)
{
  STrace trace("SWalker::translate_function_name");
  if (my_decoder->isName(encname))
  {
    if (encname.at(1) == '@')
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
  else if (encname.at(0) == 'Q')
  {
    // If a function declaration has a scoped name, then it is not
    // declaring a new function in that scope and can be ignored in
    // the context of synopsis.
    // TODO: maybe needed for syntax stuff?
    return;
  }
  else if (encname.at(0) == 'T')
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
PTree::Node*
SWalker::translate_type_specifier(PTree::Node *tspec)
{
  STrace trace("SWalker::translate_type_specifier");
  PTree::Node *class_spec = get_class_or_enum_spec(tspec);
  if (class_spec) translate(class_spec);
  return 0;
}

void SWalker::visit(PTree::Typedef *node)
{
  STrace trace("SWalker::visit(Typedef*)");
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  /* PTree::Node *tspec = */
  translate_type_specifier(PTree::second(node));
  my_declaration = node;
  my_store_decl = true;
  for (PTree::Node *declarator = PTree::third(node); declarator; declarator = PTree::tail(declarator, 2))
    translate_typedef_declarator(declarator->car());
}

void SWalker::translate_typedef_declarator(PTree::Node *node)
{
  STrace trace("SWalker::translate_typedef_declarator");
  if (PTree::type_of(node) != Token::ntDeclarator) return;
  PTree::Encoding encname = node->encoded_name();
  PTree::Encoding enctype = node->encoded_type();
  if (encname.empty() || enctype.empty()) return;

  update_line_number(node);

  // Get type of declarator
  my_decoder->init(enctype);
  Types::Type* type = my_decoder->decodeType();
  // Get name of typedef
  std::string name = my_decoder->decodeName(encname);
  // Create typedef object
  AST::Typedef* tdef = my_builder->add_typedef(my_lineno, name, type, false);
  add_comments(tdef, dynamic_cast<PTree::Declarator*>(node));
  // if storing links, find name
  if (my_links)
  {
    if (my_store_decl && PTree::second(my_declaration))
      my_links->link(PTree::second(my_declaration), type);

    PTree::Node *p = node;
    // function pointer: [( [* f] )]
    if (p && !p->car()->is_atom() && *p->car()->car() == '(')
      p = PTree::rest(p->car())->car();

    while (p && p->car()->is_atom() && (*p->car() == '*' || *p->car() == '&'))
      p = PTree::rest(p);

    if (p)
      // p should now be at the name
      my_links->link(p->car(), tdef);
  }
}

PTree::Node*
SWalker::translate_function_implementation(PTree::Node *node)
{
  STrace trace("SWalker::translate_function_implementation");
  my_function = 0;
  my_params.clear();
  translate_declarator(PTree::third(node));
  if (!my_filter->should_visit_function_impl(my_file)) return 0;
  if (!my_function)
  {
    std::cerr << "Warning: function was null!" << std::endl;
    return 0;
  }

  FuncImplCache cache;
  cache.func = my_function;
  cache.params = my_param_cache;
  cache.body = PTree::nth(node, 3);

  if (dynamic_cast<AST::Class*>(my_builder->scope()))
    my_func_impl_stack.back().push_back(cache);
  else translate_func_impl_cache(cache);
  return 0;
}

void
SWalker::translate_func_impl_cache(const FuncImplCache &cache)
{
  STrace trace("SWalker::translate_func_impl_cache");
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
    const_cast<PTree::Node *>(cache.body)->accept(this);
  }
  catch (...)
  {
    LOG("Cleaning up func impl cache");
    my_builder->end_function_impl();
    throw;
  }
  my_builder->end_function_impl();
}

void SWalker::visit(PTree::AccessSpec *node)
{
  STrace trace("SWalker::visit(PTree::AccessSpec*)");
  AST::Access axs = AST::Default;
  switch (PTree::type_of(PTree::first(node)))
  {
    case Token::PUBLIC:
      axs = AST::Public;
      break;
    case Token::PROTECTED:
      axs = AST::Protected;
      break;
    case Token::PRIVATE:
      axs = AST::Private;
      break;
  }
  my_builder->set_access(axs);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
}

/* Enum Spec
 *  
 */
void SWalker::visit(PTree::EnumSpec *node)
{
  STrace trace("SWalker::visit(PTree::EnumSpec*)");
  //update_line_number(spec);
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  if (!PTree::second(node))
  {
    return; /* anonymous enum */
  }
  std::string name = PTree::reify(PTree::second(node));

  update_line_number(node);
  int enum_lineno = my_lineno;
  // Parse enumerators
  std::vector<AST::Enumerator*> enumerators;
  PTree::Node *penum = PTree::second(PTree::third(node));
  AST::Enumerator *enumor;
  while (penum)
  {
    update_line_number(penum);
    PTree::Node *penumor = PTree::first(penum);
    if (penumor->is_atom())
    {
      // Just a name
      enumor = my_builder->add_enumerator(my_lineno, PTree::reify(penumor), "");
      add_comments(enumor, static_cast<PTree::CommentedAtom *>(penumor)->get_comments());
      if (my_links) my_links->link(penumor, enumor);
    }
    else
    {
      // Name = Value
      std::string name = PTree::reify(PTree::first(penumor));
      std::string value;
      if (PTree::length(penumor) == 3)
        value = PTree::reify(PTree::third(penumor));
      enumor = my_builder->add_enumerator(my_lineno, name, value);
      add_comments(enumor, dynamic_cast<PTree::CommentedAtom *>(PTree::first(penumor)));
      if (my_links) my_links->link(PTree::first(penumor), enumor);
    }
    enumerators.push_back(enumor);
    penum = PTree::rest(penum);
    // Skip comma
    if (penum && penum->car() && *penum->car() == ',') penum = PTree::rest(penum);
  }
  PTree::Node *close = PTree::third(PTree::third(node));
  enumor = new AST::Enumerator(my_file, my_lineno, "dummy", my_dummyname, "");
  add_comments(enumor, static_cast<PTree::CommentedAtom *>(close));
  enumerators.push_back(enumor);
  
  // Create AST.Enum object
  AST::Enum* theEnum = my_builder->add_enum(enum_lineno,name,enumerators);
  add_comments(theEnum, my_declaration);
  if (my_declaration)
  {
    // Enum declared inside declaration. Comments for the declaration
    // belong to the enum. This is policy. #TODO review policy
    //my_declaration->SetComments(0); ?? typedef doesn't have comments?
  }
  if (my_links) my_links->link(PTree::second(node), theEnum);
}

void SWalker::visit(PTree::Using *node)
{
  STrace trace("SWalker::visit(PTree::Using*)");
  if (my_links) my_links->span(PTree::first(node), "file-keyword");
  bool is_namespace = false;
  PTree::Node *p = PTree::rest(node);
  if (*PTree::first(p) == "namespace")
  {
    if (my_links) my_links->span(PTree::first(p), "file-keyword");
    // Find namespace to alias
    p = PTree::rest(p);
    is_namespace = true;
  }
  // Find name that we are looking up, and make a new ptree list for linking it
  p = p->car(); // p now points to the 'PTree::Name' child of 'PTree::Using'
  PTree::Node *p_name = PTree::snoc(0, p->car());
  ScopedName name;
  if (*PTree::first(p) == "::")
    // Eg; "using ::memcpy;" Indicate global scope with empty first
    name.push_back("");
  else
  {
    name.push_back(parse_name(PTree::first(p)));
    p = PTree::rest(p);
  }
  while (p && *PTree::first(p) == "::")
  {
    p_name = PTree::snoc(p_name, p->car()); // Add '::' to p_name
    p = PTree::rest(p);
    name.push_back(parse_name(PTree::first(p)));
    p_name = PTree::snoc(p_name, p->car()); // Add identifier to p_name
    p = PTree::rest(p);
  }

  // Resolve and link name
  try
  {
    Types::Named* type = my_lookup->lookupType(name);
    if (my_links) my_links->link(p_name, type);
    if (is_namespace)
    {
      // Check for '=' alias
      // Huh ? '=' isn't valid within a 'using' directive or declaration
      if (p && *PTree::first(p) == "=")
      {
        p = PTree::rest(p);
        std::string alias = parse_name(PTree::first(p));
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
}
