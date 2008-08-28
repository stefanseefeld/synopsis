//
// Copyright (C) 2005 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "ASGTranslator.hh"
#include <Synopsis/Trace.hh>
#include <Synopsis/PTree/Writer.hh> // for PTree::reify
#include <Support/Path.hh>

using namespace Synopsis;

namespace
{
  ScopedName sname(std::string const &name) { return ScopedName(name);}

class UnknownSymbol : public std::runtime_error
{
public:
  explicit UnknownSymbol(std::string const &n)
    : std::runtime_error("Unknown symbol: " + n) {}
};

}
#define qname(arg) qname_(Python::Tuple(arg))

ASGTranslator::ASGTranslator(std::string const &filename,
			     std::string const &base_path, bool primary_file_only,
			     Synopsis::IR ir, bool v, bool d)
  : asg_kit_("C"),
    sf_kit_("C"),
    declarations_(ir.declarations()),
    types_(ir.types()),
    files_(ir.files()),
    raw_filename_(filename),
    base_path_(base_path),
    primary_file_only_(primary_file_only),
    lineno_(0),
    verbose_(v),
    debug_(d),
    buffer_(0),
    declaration_(0),
    defines_class_or_enum_(false)
{
  Trace trace("ASGTranslator::ASGTranslator", Trace::TRANSLATION);

  Python::Module qn = Python::Module::import("Synopsis.QualifiedName");
  qname_ = qn.attr("QualifiedCxxName");

  // determine canonical filenames
  Path path = Path(raw_filename_).abs();
  std::string long_filename = path.str();
  path.strip(base_path_);
  std::string short_filename = path.str();

  SourceFile file = files_.get(short_filename);
  if (file)
    file_ = file;
  else
  {
    file_ = sf_kit_.create_source_file(short_filename, long_filename);
    files_.set(short_filename, file_);
  }
  file_.set_primary(true);

  Python::Object define = types_.attr("__setitem__");
  types_.set(qname(sname("char")), asg_kit_.create_builtin_type_id(sname("char")));
  types_.set(qname(sname("short")), asg_kit_.create_builtin_type_id(sname("short")));
  types_.set(qname(sname("int")), asg_kit_.create_builtin_type_id(sname("int")));
  types_.set(qname(sname("long")), asg_kit_.create_builtin_type_id(sname("long")));
  types_.set(qname(sname("unsigned")), asg_kit_.create_builtin_type_id(sname("unsigned")));
  types_.set(qname(sname("unsigned long")), asg_kit_.create_builtin_type_id(sname("unsigned long")));
  types_.set(qname(sname("float")), asg_kit_.create_builtin_type_id(sname("float")));
  types_.set(qname(sname("double")), asg_kit_.create_builtin_type_id(sname("double")));
  types_.set(qname(sname("void")), asg_kit_.create_builtin_type_id(sname("void")));
  types_.set(qname(sname("...")), asg_kit_.create_builtin_type_id(sname("...")));
  types_.set(qname(sname("long long")), asg_kit_.create_builtin_type_id(sname("long long")));
  types_.set(qname(sname("long double")), asg_kit_.create_builtin_type_id(sname("long double")));
  // some GCC extensions...
  types_.set(qname(sname("__builtin_va_list")), asg_kit_.create_builtin_type_id(sname("__builtin_va_list")));
}

void ASGTranslator::translate(PTree::Node *ptree, Buffer &buffer)
{
  Trace trace("ASGTranslator::translate", Trace::TRANSLATION);
  buffer_ = &buffer;
  ptree->accept(this);
}

void ASGTranslator::visit(PTree::CommentedAtom *node)
{
  // The only purpose of this method is to filter
  // out those atoms that are used as end markers.
  // They can be recognized by having length() == 0.
  if (node->length() == 0)
  {
    bool visible = update_position(node);
    ASG::Builtin eos = asg_kit_.create_builtin(file_, lineno_,
                                               "EOS", sname(std::string("EOS")));
    add_comments(eos, node->get_comments());
    if (visible) declare(eos);

  }
//   else
//     visit(static_cast<PTree::Atom *>(node));
}

void ASGTranslator::visit(PTree::List *node)
{
  if (node->car()) node->car()->accept(this);
  if (node->cdr()) node->cdr()->accept(this);
}

void ASGTranslator::visit(PTree::Declarator *declarator)
{
  Trace trace("ASGTranslator::visit(PTree::Declarator *)", Trace::TRANSLATION);
  trace << declarator;
  if (!declaration_ || !PTree::first(declarator)) return; // empty

  bool visible = update_position(declarator);
  PTree::Encoding name = declarator->encoded_name();
  PTree::Encoding type = declarator->encoded_type();
  if (type.is_function())
  {
    trace << "declare function " << name << " (" << type << ')' 
	  << raw_filename_ << ':' << lineno_;

    ASG::TypeIdList parameter_types;
    ASG::TypeId return_type = lookup_function_types(type, parameter_types);
    ASG::Function::Parameters parameters;
    PTree::Node *p = PTree::rest(declarator);
    while (p && p->car() && *p->car() != '(') p = PTree::rest(p);
    translate_parameters(PTree::second(p), parameter_types, parameters);

    size_t length = (name.front() - 0x80);
    ScopedName sname(std::string(name.begin() + 1, name.begin() + 1 + length));
    ASG::Modifiers pre;
    ASG::Modifiers post;
    ASG::Function function = asg_kit_.create_function(file_, lineno_,
                                                      "function",
                                                      pre,
                                                      return_type,
                                                      post,
                                                      sname,
                                                      sname.get(0));
    function.parameters().extend(parameters);
    if (declaration_) add_comments(function, declaration_->get_comments());
    add_comments(function, declarator->get_comments());
    if (visible) declare(function);
  }
  else
  {
    ASG::TypeId t = lookup(type);
    size_t length = (name.front() - 0x80);
    ScopedName sname;
    if (scope_.size()) sname = scope_.top().name();
    sname.append(std::string(name.begin() + 1, name.begin() + 1 + length));

    std::string vtype;
    if (scope_.size())
    {
      vtype = scope_.top().type();
      if (vtype == "class" || vtype == "struct" || vtype == "union")
        vtype = "data member";
      else if (vtype == "function")
	vtype = "local variable";
    }
    else
      vtype += "variable";
    ASG::Variable variable = asg_kit_.create_variable(file_, lineno_,
                                                      vtype, sname, t, false);
    if (declaration_) add_comments(variable, declaration_->get_comments());
    add_comments(variable, declarator->get_comments());
    if (visible) declare(variable);
  }
}

void ASGTranslator::visit(PTree::Declaration *declaration)
{
  Trace trace("ASGTranslator::visit(PTree::Declaration *)", Trace::TRANSLATION);
  // Cache the declaration while traversing the individual declarators;
  // the comments are passed through.
  declaration_ = declaration;
  visit(static_cast<PTree::List *>(declaration));
  declaration_ = 0;
}

void ASGTranslator::visit(PTree::FunctionDefinition *def)
{
  Trace trace("ASGTranslator::visit(PTree::FunctionDefinition *)", Trace::TRANSLATION);
  // Cache the declaration while traversing the individual declarators;
  // the comments are passed through.
  declaration_ = def;
  // Only traverse declaration-specifier-seq and declarator, but not body
  if (PTree::first(def)) PTree::first(def)->accept(this);
  PTree::second(def)->accept(this);
  declaration_ = 0;
}

void ASGTranslator::visit(PTree::ClassSpec *class_spec)
{
  Trace trace("ASGTranslator::visit(PTree::ClassSpec *)", Trace::TRANSLATION);
  
  bool visible = update_position(class_spec);

  size_t size = PTree::length(class_spec);
  if (size == 2) // forward declaration ?
  {
    // [ class|struct <name> ]
    PTree::Encoding t = class_spec->encoded_name();
    try 
    {
      ASG::TypeId tt = lookup(t);
      // The type is known, thus nothing to do. Move on.
      return;
    }
    catch (UnknownSymbol const &e) {}
    std::string type = PTree::reify(PTree::first(class_spec));
    std::string name = PTree::reify(PTree::second(class_spec));
    ScopedName sname(name);

    ASG::Forward forward = asg_kit_.create_forward(file_, lineno_,
                                                   type, sname);
    add_comments(forward, class_spec->get_comments());
    if (visible)
    {
      declare(forward);
      declare_type(sname, forward);
    }
    else
      declare_type(sname);
      
    defines_class_or_enum_ = true;
    return;
  }

  std::string type = PTree::reify(PTree::first(class_spec));
  std::string name;
  PTree::ClassBody *body = 0;

  if (size == 4) // struct definition
  {
    // [ class|struct <name> <inheritance> [{ body }] ]

    name = PTree::reify(PTree::second(class_spec));
    body = static_cast<PTree::ClassBody *>(PTree::nth(class_spec, 3));
  }
  else if (size == 3) // anonymous struct definition
  {
    // [ struct [nil nil] [{ ... }] ]

    PTree::Encoding ename = class_spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    name = std::string(ename.begin() + 1, ename.begin() + 1 + length);
    body = static_cast<PTree::ClassBody *>(PTree::nth(class_spec, 2));
  }

  ScopedName sname(name);
  ASG::Class class_ = asg_kit_.create_class(file_, lineno_, type, sname);
  add_comments(class_, class_spec->get_comments());
  if (visible)
  {
    declare(class_);
    declare_type(sname, class_);
  }
  else
    declare_type(sname);

  scope_.push(class_);
  defines_class_or_enum_ = false;
  body->accept(this);
  scope_.pop();
  defines_class_or_enum_ = true;
}

void ASGTranslator::visit(PTree::EnumSpec *enum_spec)
{
  Trace trace("ASGTranslator::visit(PTree::EnumSpec *)", Trace::TRANSLATION);

  bool visible = update_position(enum_spec);
  std::string name;
  
  if (!PTree::second(enum_spec)) //anonymous
  {
    PTree::Encoding ename = enum_spec->encoded_name();
    size_t length = (ename.front() - 0x80);
    name = std::string(ename.begin() + 1, ename.begin() + 1 + length);
  }
  else
    name = PTree::reify(PTree::second(enum_spec));

  ASG::Enumerators enumerators;
  PTree::Node *enode = PTree::second(PTree::third(enum_spec));

  PTree::Encoding t = enum_spec->encoded_name();
  try 
  {
    ASG::TypeId tt = lookup(t);
    // The type is known. If we find a body, this is a parse error:
//     if (enode)
//       throw std::runtime_error("redefinition of 'enum " + name + "'");

    // Else it may be an elaborate specifier, or a forward declaration.
    // Nothing to do. Move on.
    return;
  }
  catch (UnknownSymbol const &e) {}

  ASG::Enumerator enumerator;
  while (enode)
  {
    // quite a costly way to update the line number...
    update_position(enode);
    PTree::Node *penumor = PTree::first(enode);
    if (penumor->is_atom())
    {
      // Just a name
      ScopedName sname(PTree::reify(penumor));
      enumerator = asg_kit_.create_enumerator(file_, lineno_, sname, "");
      add_comments(enumerator, static_cast<PTree::CommentedAtom *>(penumor)->get_comments());
    }
    else
    {
      // Name = Value
      ScopedName sname(PTree::reify(PTree::first(penumor)));
      std::string value;
      if (PTree::length(penumor) == 3)
        value = PTree::reify(PTree::third(penumor));
      enumerator = asg_kit_.create_enumerator(file_, lineno_, sname, value);
      add_comments(enumerator, static_cast<PTree::CommentedAtom *>(PTree::first(penumor))->get_comments());
    }
    enumerators.append(enumerator);
    enode = PTree::rest(enode);
    // Skip comma
    if (enode && enode->car() && *enode->car() == ',') enode = PTree::rest(enode);
  }
  // Add a dummy enumerator at the end to absorb trailing comments.
  PTree::Node *close = PTree::third(PTree::third(enum_spec));
  ASG::Builtin eos = asg_kit_.create_builtin(file_, lineno_,
                                             "EOS", sname(std::string("EOS")));
  add_comments(eos, static_cast<PTree::CommentedAtom *>(close)->get_comments());
  enumerators.append(eos);
  
  // Create ASG.Enum object
  ASG::Enum enum_ = asg_kit_.create_enum(file_, lineno_, name, enumerators);

  if (visible)
  {
    declare(enum_);
    declare_type(ScopedName(name), enum_);
  }
  else
    declare_type(ScopedName(name));

  defines_class_or_enum_ = true;
}

void ASGTranslator::visit(PTree::Typedef *typed)
{
  Trace trace("ASGTranslator::visit(PTree::Typedef *)", Trace::TRANSLATION);
  defines_class_or_enum_ = false;

  bool visible = update_position(typed);

  // the second child node may be an inlined class spec, i.e.
  // typedef struct {...} type;
  PTree::second(typed)->accept(this);
  for (PTree::Node *d = PTree::third(typed); d; d = PTree::tail(d, 2))
  {
    if(PTree::type_of(d->car()) != Token::ntDeclarator)  // is this check necessary
      continue;
    
    PTree::Declarator *declarator = static_cast<PTree::Declarator *>(d->car());
    PTree::Encoding name = declarator->encoded_name();
    PTree::Encoding type = declarator->encoded_type();
    trace << "declare type " << name << " (" << type << ')' 
	  << raw_filename_ << ':' << lineno_;
    assert(name.is_simple_name());
    size_t length = (name.front() - 0x80);
    ScopedName sname(std::string(name.begin() + 1, name.begin() + 1 + length));
    ASG::TypeId alias = lookup(type);
    ASG::Declaration declaration = asg_kit_.create_typedef(file_, lineno_,
                                                           "typedef",
                                                           sname,
                                                           alias, defines_class_or_enum_);
    add_comments(declaration, declarator->get_comments());
    if (visible)
    {
      declare(declaration);
      declare_type(sname, declaration);
    }
    else
      declare_type(sname);
  }
  defines_class_or_enum_ = false;
}

void ASGTranslator::translate_parameters(PTree::Node *node,
					 ASG::TypeIdList types,
					 ASG::Function::Parameters &parameters)
{
  Trace trace("ASGTranslator::translate_parameters", Trace::TRANSLATION);
  if (PTree::length(node) == 1 && *node->car() == "void") return;
  while (node)
  {
    // A parameter has a type, possibly a name and possibly a value.
    // Treat the value as a string, i.e. don't analyse the expression further.
    std::string name, value;
    ASG::Modifiers premods, postmods;
    if (*node->car() == ',')
      node = node->cdr();
    PTree::Node *parameter = PTree::first(node);

    ASG::TypeId type = types.get(0);
    types.del(0); // pop one value
    if (PTree::length(parameter) == 3)
    {
      PTree::Declarator *decl = static_cast<PTree::Declarator*>(PTree::nth(parameter, 2));
      name = PTree::reify(decl->name());
      value = PTree::reify(decl->initializer());

      // Skip keywords (eg: register) which are Leaves
      PTree::Node *atom = PTree::nth(parameter, 0);
      if (atom) premods.append(PTree::reify(atom));
    }
    ASG::Parameter p = asg_kit_.create_parameter(premods, type, postmods,
                                                 name, value);
    parameters.append(p);
    node = PTree::rest(node);
  }
}

void ASGTranslator::add_comments(ASG::Declaration declarator, PTree::Node *c)
{
  Trace trace("ASGTranslator::add_comments", Trace::TRANSLATION);
  if (!declarator || !c) return;
  
  Python::List comments;

  // Loop over all comments in the list
  // If the last comment is separated from the declaration by more than a single '\n',
  // we add a None to the list.
  bool suspect = false;
  for (PTree::Node *next = PTree::rest(c); c && !c->is_atom(); next = PTree::rest(c))
  {
    PTree::Node *first = PTree::first(c);
    if (!first || !first->is_atom())
    {
      c = next;
      continue;
    }
    // Check if comment is continued, eg: consecutive C++ comments
    while (next && PTree::first(next) && PTree::first(next)->is_atom())
    {
      if (!strncmp(first->position() + first->length() - 2, "*/", 2))
        break;
      if (strncmp(PTree::first(next)->position(), "//", 2))
        break;
      char const *next_pos = PTree::first(next)->position();
      char const *start_pos = PTree::first(c)->position();
      char const *curr_pos = start_pos + PTree::first(c)->length();
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
      c->set_car(first = new PTree::Atom(start_pos, len));
      // Skip the combined comment
      next = PTree::rest(next);
    }
    suspect = false;
    char const *pos = first->position() + first->length();
    while (*pos && strchr(" \t\r", *pos)) ++pos;
    if (*pos == '\n')
    {
      ++pos;
      // Found only allowed \n
      while (*pos && strchr(" \t\r", *pos)) ++pos;
      if (*pos == '\n' || !strncmp(pos, "/*", 2)) suspect = true;
    }
    Python::Object comment = PTree::reify(first);
    comments.append(comment);

//     if (links_) links_->long_span(first, "file-comment");
    // Set first to 0 so we dont accidentally do them twice (eg:
    // when parsing expressions)
    c->set_car(0);
    c = next;
  }
  if (suspect) comments.append(Python::Object());
  declarator.annotations().set("comments", comments);
}

bool ASGTranslator::update_position(PTree::Node *node)
{
  Trace trace("ASGTranslator::update_position", Trace::TRANSLATION);

  std::string filename;
  lineno_ = buffer_->origin(node->begin(), filename);

  if (filename != raw_filename_)
  {
    if (primary_file_only_)
      // raw_filename_ remains the primary file's name
      // and all declarations from elsewhere are ignored
      return false;
    raw_filename_ = filename;

    // determine canonical filenames
    Path path = Path(filename).abs();
    std::string long_filename = path.str();
    path.strip(base_path_);
    std::string short_filename = path.str();

    SourceFile file = files_.get(short_filename);
    if (file)
      file_ = file;
    else
    {
      file_ = sf_kit_.create_source_file(short_filename, long_filename);
      files_.set(short_filename, file_);
    }
  }
  return true;
}

ASG::TypeId ASGTranslator::lookup(PTree::Encoding const &name)
{
  Trace trace("ASGTranslator::lookup", Trace::SYMBOLLOOKUP);
  trace << name;
  name_ = name;
  ASG::TypeId type;
  decode_type(name.begin(), type);
  return type;
}

ASG::TypeId ASGTranslator::lookup_function_types(PTree::Encoding const &name,
                                                 ASG::TypeIdList &parameters)
{
  Trace trace("ASGTranslator::lookup_function_types", Trace::SYMBOLLOOKUP);
  trace << name;
  name_ = name;

  PTree::Encoding::iterator i = name.begin();
  assert(*i == 'F');
  ++i;
  while (true)
  {
    ASG::TypeId parameter;
    i = decode_type(i, parameter);
    if (parameter)
      parameters.append(parameter);
    else
      break;
  }
  ++i; // skip over '_'
  ASG::TypeId return_type;
  i = decode_type(i, return_type);
  return return_type;
}

void ASGTranslator::declare(ASG::Declaration declaration)
{
  if (scope_.size())
    scope_.top().declarations().append(declaration);
  else
    declarations_.append(declaration);    
  file_.declarations().append(declaration);
}

ASG::TypeId ASGTranslator::declare_type(ScopedName name,
                                        ASG::Declaration declaration)
{
  Trace trace("ASGTranslator::declare_type", Trace::SYMBOLLOOKUP);
  trace << name;
  ASG::TypeId type = asg_kit_.create_declared_type_id(name, declaration);
  types_.set(qname(name), type);
  return type;
}

ASG::TypeId ASGTranslator::declare_type(ScopedName name)
{
  Trace trace("ASGTranslator::declare_type(unknown)", Trace::SYMBOLLOOKUP);
  trace << name;
  ASG::TypeId type = asg_kit_.create_unknown_type_id(name);
  types_.set(qname(name), type);
  return type;
}

// This is almost a verbatim copy of the Decoder::decode
// methods from Synopsis/Parsers/Cxx/syn/decoder.cc
// with some minor modifications to disable the C++ specific things.
// FIXME: this ought to be part of SymbolLookup::Type.
PTree::Encoding::iterator ASGTranslator::decode_name(PTree::Encoding::iterator i,
                                                     std::string &name)
{
  Trace trace("ASGTranslator::decode_name", Trace::PARSING);
  size_t length = *i++ - 0x80;
  name = std::string(length, '\0');
  std::copy(i, i + length, name.begin());
  i += length;
  return i;
}

PTree::Encoding::iterator ASGTranslator::decode_type(PTree::Encoding::iterator i,
                                                     ASG::TypeId &type)
{
  Trace trace("ASGTranslator::decode_type", Trace::PARSING);
  ASG::Modifiers premod, postmod;
  std::string name;
  ASG::TypeId base;

  // Loop forever until broken
  while (i != name_.end() && !name.length() && !base)
  {
    int c = *i++;
    switch (c)
    {
      case 'P':
	postmod.insert(0, "*");
	break;
//       case 'R':
// 	postmod.insert(0, "&");
// 	break;
      case 'S':
	premod.append("signed");
	break;
      case 'U':
	premod.append("unsigned");
	break;
      case 'C':
	premod.append("const");
	break;
      case 'V':
	premod.append("volatile");
	break;
      case 'A':
      {
	std::string array("[");
	while (*i != '_') array.push_back(*i++);
	array.push_back(']');
	++i;
	postmod.append(array);
	  break;
      }
//       case '*':
//       {
// 	ScopedName n;
// 	n.push_back("*");
// 	base = new Types::Dependent(n);
// 	break;
//       }
      case 'i':
	name = "int";
	break;
      case 'v':
	name = "void";
	break;
//       case 'b':
// 	name = "bool";
// 	break;
      case 's':
	name = "short";
	break;
      case 'c':
	name = "char";
	break;
//       case 'w':
// 	name = "wchar_t";
// 	break;
      case 'l':
	name = "long";
	break;
      case 'j':
	name = "long long";
	break;
      case 'f':
	name = "float";
	break;
      case 'd':
	name = "double";
	break;
      case 'r':
	name = "long double";
	break;
      case 'e':
	name = "...";
	break;
      case '?':
	name = "int"; // in C, no return type spec defaults to int
	break;
//       case 'Q':
// 	base = decodeQualType();
// 	break;
      case '_':
	--i;
	type = ASG::TypeId();
	return i; // end of func params
      case 'F':
	i = decode_func_ptr(i, base, postmod);
	break;
//       case 'T':
// 	base = decodeTemplate();
// 	break;
//       case 'M':
// 	// Pointer to member. Format is same as for named types
// 	name = decodeName() + "::*";
// 	break;
      default:
	if (c > 0x80)
	{
	  --i;
	  i = decode_name(i, name);
	  break;
	}
	// FIXME
	// 	    std::cerr << "\nUnknown char decoding '"<<m_string<<"': "
	// 		 << char(c) << " " << c << " at "
	// 		 << (m_iter - m_string.begin()) << std::endl;
    } // switch
  } // while
  if (!base && !name.length())
  {
    // FIXME
    // 	std::cerr << "no type or name found decoding " << m_string << std::endl;
    type = ASG::TypeId();
    return i;
  }
  if (!base)
  {
    base = types_.get(qname(sname(name)));
    if (!base) throw UnknownSymbol(name);
  }
  if (premod.empty() && postmod.empty())
    type = base;
  else
    type = asg_kit_.create_modifier_type_id(base, premod, postmod);

  return i;
}

PTree::Encoding::iterator ASGTranslator::decode_func_ptr(PTree::Encoding::iterator i,
                                                         ASG::TypeId &type,
                                                         ASG::Modifiers &postmod)
{
  Trace trace("ASGTranslator::decode_func_ptr", Trace::PARSING);
  // Function ptr. Encoded same as function
  ASG::Modifiers premod;
  // Move * from postmod to funcptr's premod. This makes the output be
  // "void (*convert)()" instead of "void (convert)()*"
  if (postmod.size() > 0 && postmod.get(0) == "*")
  {
    premod.append(postmod.get(0));
    postmod.erase(postmod.begin());
  }
  ASG::TypeIdList parameters;
  while (true)
  {
    ASG::TypeId parameter;
    i = decode_type(i, parameter);
    if (parameter)
      parameters.append(parameter);
    else
      break;
  }
  ++i; // skip over '_'
  i = decode_type(i, type);
  type = asg_kit_.create_function_type_id(type, premod, parameters);
  return i;
}
