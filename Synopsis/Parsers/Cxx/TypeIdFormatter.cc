//
// Copyright (C) 2008 Stefan Seefeld
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include "TypeIdFormatter.hh"

TypeIdFormatter::TypeIdFormatter ()
  : fptr_id_(0)
{
  scope_stack_.push_back(QName());
}

void TypeIdFormatter::push_scope(const QName& scope)
{
  scope_stack_.push_back(scope_);
  scope_ = scope;
}

void TypeIdFormatter::pop_scope()
{
  scope_ = scope_stack_.back();
  scope_stack_.pop_back();
}

std::string TypeIdFormatter::colonate(const QName& name)
{
  std::string str;
  QName::const_iterator n = name.begin();
  QName::const_iterator s = scope_.begin();
  // Skip identical scopes
  while (n != name.end() && s != scope_.end() && *n == *s) ++n, ++s;
  // If name == scope, just return last eg: struct S { S* ptr; };
  if (n == name.end())
    return name.back();
  // Join the rest in name with colons
  str = *n++;
  while (n != name.end())
    str += "::" + *n++;
  return str;
}

std::string TypeIdFormatter::format(const Types::Type* type, const std::string** id)
{
  if (!type)
    return "(unknown)";
  const std::string** save = 0;
  if (id)
  {
    save = fptr_id_;
    fptr_id_ = id;
  }
  const_cast<Types::Type*>(type)->accept(this);
  if (id)
    fptr_id_ = save;
  return type_;
}

void TypeIdFormatter::visit_type(Types::Type* type)
{
  type_ = "(unknown)";
}

void TypeIdFormatter::visit_unknown(Types::Unknown* type)
{
  type_ = colonate(type->name());
}

void TypeIdFormatter::visit_modifier(Types::Modifier* type)
{
  // Premods
  std::string pre = "";
  Types::Type::Mods::iterator iter = type->pre().begin();
  while (iter != type->pre().end())
    if (*iter == "*" || *iter == "&")
      pre += *iter++;
    else
      pre += *iter++ + " ";
  // Alias
  type_ = pre + format(type->alias());
  // Postmods
  iter = type->post().begin();
  while (iter != type->post().end())
    if (*iter == "*" || *iter == "&")
      type_ += *iter++;
    else
      type_ += " " + *iter++;
}

void TypeIdFormatter::visit_named(Types::Named* type)
{
  type_ = colonate(type->name());
}

void TypeIdFormatter::visit_base(Types::Base* type)
{
  type_ = colonate(type->name());
}

void TypeIdFormatter::visit_declared(Types::Declared* type)
{
  type_ = colonate(type->name());
}

void TypeIdFormatter::visit_template_type(Types::Template* type)
{
  type_ = colonate(type->name());
}

void TypeIdFormatter::visit_parameterized(Types::Parameterized* type)
{
  std::string str;
  if (type->template_id())
    str = colonate(type->template_id()->name()) + "<";
  else
    str = "(unknown)<";
  if (type->parameters().size())
  {
    str += format(type->parameters().front());
    Types::Type::vector::iterator iter = type->parameters().begin();
    while (++iter != type->parameters().end())
      str += "," + format(*iter);
  }
  type_ = str + ">";
}

void TypeIdFormatter::visit_func_ptr(Types::FuncPtr* type)
{
  std::string str = format(type->return_type()) + "(";
  Types::Type::Mods::iterator i_pre = type->pre().begin();
  while (i_pre != type->pre().end())
    str += *i_pre++;
  if (fptr_id_)
  {
    str += **fptr_id_;
    *fptr_id_ = 0;
  }
  str += ")(";
  if (type->parameters().size())
  {
    str += format(type->parameters().front());
    Types::Type::vector::iterator iter = type->parameters().begin();
    while (++iter != type->parameters().end())
      str += "," + format(*iter);
  }
  type_ = str + ")";
}
