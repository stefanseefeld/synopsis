//
// Copyright (C) 2000 Stefan Seefeld
// Copyright (C) 2000 Stephen Davies
// All rights reserved.
// Licensed to the public under the terms of the GNU LGPL (>= 2),
// see the file COPYING for details.
//

#include <Python.h>
#include <Synopsis/Trace.hh>
#include "Translator.hh"
#include "Filter.hh"
#include "exception.hh"
#include <map>
#include <set>
#include <iostream>

using namespace Synopsis;

// The compiler firewalled private stuff
struct Translator::Private
{
  //. Constructor
  Private(Translator* s) : m_syn(s)
  {
    PyObject *qname_module  = PyImport_ImportModule("Synopsis.QualifiedName");
    if (!qname_module) throw py_error_already_set();
    m_qname_factory = PyObject_GetAttrString(qname_module, "QualifiedCxxName");
    if (!m_qname_factory) throw py_error_already_set();
    Py_DECREF(qname_module);
    m_cxx = PyString_InternFromString("C++");
    Py_INCREF(Py_None);
    add((ASG::Declaration*)0, Py_None);
    Py_INCREF(Py_None);
    add((Types::Type*)0, Py_None);
  }
  ~Private()
  {
    Py_DECREF(m_qname_factory);
  }

  //. Reference to parent synopsis object
  Translator* m_syn;
  PyObject* m_qname_factory;
  //. Interned string for "C++"
  PyObject* m_cxx;
  //. Returns the string for "C++" as a borrowed reference
  PyObject* cxx()
  {
    return m_cxx;
  }
  // Sugar
  typedef std::map<void*, PyObject*> ObjMap;
  // Maps from C++ objects to PyObjects
  ObjMap obj_map;
  // A set of builtin declarations to not to store in global namespace
  std::set<ASG::Declaration*> builtin_decl_set;
  
  // Note that these methods always succeed
  
  //. Return the PyObject for the given ASG::SourceFile
  PyObject* py(ASG::SourceFile*);
  //. Return the PyObject for the given ASG::Include
  PyObject* py(ASG::Include*);
  //. Return the PyObject for the given ASG::Declaration
  PyObject* py(ASG::Declaration*);
  //. Return the PyObject for the given ASG::Inheritance
  PyObject* py(ASG::Inheritance*);
  //. Return the PyObject for the given ASG::Parameter
  PyObject* py(ASG::Parameter*);
  //. Return the PyObject for the given Types::Type
  PyObject* py(Types::Type*);
  //. Return the PyObject for the given string
  PyObject* py(const std::string &);
  
  //. Add the given pair
  void add(void* cobj, PyObject* pyobj)
  {
    if (!pyobj) throw py_error_already_set();
    obj_map.insert(ObjMap::value_type(cobj, pyobj));
  }

  // Convert a vector to a List
  template <class T>
  PyObject* List(const std::vector<T*>& vec)
  {
    PyObject* list = PyList_New(vec.size());
    int index = 0;
    typename std::vector<T*>::const_iterator iter = vec.begin();
    while (iter != vec.end())
      PyList_SET_ITEM(list, index++, py(*iter++));
    return list;
  }

  // Convert a vector to a Tuple
  template <class T>
  PyObject* Tuple(const std::vector<T*>& vec)
  {
    PyObject* tuple = PyTuple_New(vec.size());
    int index = 0;
    typename std::vector<T*>::const_iterator iter = vec.begin();
    while (iter != vec.end())
      PyTuple_SET_ITEM(tuple, index++, py(*iter++));
    return tuple;
  }

  PyObject* QualifiedName(const QName& name)
  {
    PyObject* tuple = PyTuple_New(name.size());
    int index = 0;
    QName::const_iterator iter = name.begin();
    while (iter != name.end())
      PyTuple_SET_ITEM(tuple, index++, py(*iter++));
    PyObject *qname = PyObject_CallFunctionObjArgs(m_qname_factory, tuple, NULL);
    Py_DECREF(tuple);
    return qname;
  }

  // Convert a string vector to a List
  PyObject* List(const std::vector<std::string> &vec)
  {
    PyObject* list = PyList_New(vec.size());
    int index = 0;
    std::vector<std::string>::const_iterator iter = vec.begin();
    while (iter != vec.end())
      PyList_SET_ITEM(list, index++, py(*iter++));
    return list;
  }

  // Convert a string vector to a Tuple
  PyObject* Tuple(const std::vector<std::string> &vec)
  {
    PyObject* tuple = PyTuple_New(vec.size());
    int index = 0;
    std::vector<std::string>::const_iterator iter = vec.begin();
    while (iter != vec.end())
      PyTuple_SET_ITEM(tuple, index++, py(*iter++));
    return tuple;
  }
};

// Convert a Declaration vector to a List. Declarations can return 0
// from py(), if they are not in the main file.
template <>
PyObject* Translator::Private::List(const std::vector<ASG::Declaration*>& vec)
{
  std::vector<PyObject*> objects;
  std::vector<ASG::Declaration*>::const_iterator iter = vec.begin();
  while (iter != vec.end())
  {
    PyObject* obj = py(*iter++);
    if (obj != 0) objects.push_back(obj);
  }

  PyObject* list = PyList_New(objects.size());
  int index = 0;
  std::vector<PyObject*>::const_iterator piter = objects.begin();
  while (piter != objects.end())
    PyList_SET_ITEM(list, index++, *piter++);
  return list;
}

PyObject* Translator::Private::py(ASG::SourceFile* file)
{
  ObjMap::iterator iter = obj_map.find(file);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    add(file, m_syn->SourceFile(file));
    iter = obj_map.find(file);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Translator::Private::py(ASG::SourceFile*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Translator::Private::py(ASG::Include* incl)
{
  ObjMap::iterator iter = obj_map.find(incl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    add(incl, m_syn->Include(incl));
    iter = obj_map.find(incl);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Translator::Private::py(ASG::Include*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Translator::Private::py(ASG::Declaration* decl)
{
  ObjMap::iterator iter = obj_map.find(decl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    decl->accept(m_syn);
    iter = obj_map.find(decl);
    if (iter == obj_map.end())
    {
      // Probably means it's not in the main file
      return 0;
      /*
        std::cout << "Fatal: Still not PyObject after converting." << std::endl;
        throw "Translator::Private::py(ASG::Declaration*)";
      */
    }
    // Force addition of type to dictionary
    PyObject* declared = py(decl->declared());
    Py_DECREF(declared);
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Translator::Private::py(ASG::Inheritance* decl)
{
  ObjMap::iterator iter = obj_map.find(decl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    decl->accept(m_syn);
    iter = obj_map.find(decl);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Translator::Private::py(ASG::Inheritance*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}
PyObject* Translator::Private::py(ASG::Parameter* decl)
{
  ObjMap::iterator iter = obj_map.find(decl);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    decl->accept(m_syn);
    iter = obj_map.find(decl);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Translator::Private::py(ASG::Parameter*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Translator::Private::py(Types::Type* type)
{
  ObjMap::iterator iter = obj_map.find(type);
  if (iter == obj_map.end())
  {
    // Need to convert object first
    type->accept(m_syn);
    iter = obj_map.find(type);
    if (iter == obj_map.end())
    {
      std::cout << "Fatal: Still not PyObject after converting." << std::endl;
      throw "Translator::Private::py(Types::Type*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Translator::Private::py(const std::string &str)
{
  PyObject* pystr = PyString_FromStringAndSize(str.data(), str.size());
  //PyString_InternInPlace(&pystr);
  return pystr;
}

Translator::Translator(FileFilter* filter, PyObject *ir)
  : m_ir(ir), m_filter(filter)
{
  Trace trace("Translator::Translator", Trace::TRANSLATION);
  m_asg_module  = PyImport_ImportModule("Synopsis.ASG");
  if (!m_asg_module) throw py_error_already_set();
  m_sf_module  = PyImport_ImportModule("Synopsis.SourceFile");
  if (!m_sf_module) throw py_error_already_set();
  PyObject *asg = PyObject_GetAttrString(m_ir, "asg");
  m_declarations = PyObject_GetAttrString(asg, "declarations");
  if (!m_declarations) throw py_error_already_set();
  m_dictionary = PyObject_GetAttrString(asg, "types");
  if (!m_dictionary) throw py_error_already_set();
  Py_DECREF(asg);
  
  m = new Private(this);
}

Translator::~Translator()
{
  Trace trace("Translator::~Translator", Trace::TRANSLATION);
  /*
    PyObject *file = PyObject_CallMethod(scopes.back(), "declarations", 0);
    size_t size = PyList_Size(file);
    for (size_t i = 0; i != size; i++)
    PyObject_CallMethod(declarations, "append", "O", PyList_GetItem(file, i));
  */
  
  Py_DECREF(m_declarations);
  Py_DECREF(m_dictionary);
  
  Py_DECREF(m_asg_module);
  Py_DECREF(m_sf_module);
  
  // Deref the objects we created
  Private::ObjMap::iterator iter = m->obj_map.begin();
  Private::ObjMap::iterator end = m->obj_map.end();
  while (iter != end)
  {
    PyObject* obj = iter->second;
    PyObject* repr = PyObject_Repr(obj);
    //std::cout << (obj->ob_refcnt-1) << " " << PyString_AsString(repr) << std::endl;
    Py_DECREF(repr);
    Py_DECREF(obj);
    iter->second = 0;
    ++iter;
  }
  //std::cout << "Total: " << m->obj_map.size()<<" objects." << std::endl;
  delete m;
}

void Translator::translate(ASG::Scope* scope)//, PyObject* asg)
{
  ASG::Declaration::vector globals, &decls = scope->declarations();
  ASG::Declaration::vector::iterator it = decls.begin();
  
  // List all declarations not in the builtin_decl_set
  for (; it != decls.end(); ++it)
    if (m->builtin_decl_set.find(*it) == m->builtin_decl_set.end())
      globals.push_back(*it);
  
  // Translate those declarations
  PyObject* list;
  PyObject_CallMethod(m_declarations, "extend", "O",
                      list = m->List(globals));
  Py_DECREF(list);

  // Translate the sourcefiles, making sure the declarations list is done
  // for each
  PyObject* pyfiles = PyObject_GetAttrString(m_ir, "files");
  if (!pyfiles) throw py_error_already_set();
  assert(PyDict_Check(pyfiles));
  
  ASG::SourceFile::vector files;
  m_filter->get_all_sourcefiles(files);
  for (ASG::SourceFile::vector::iterator i = files.begin(); i != files.end(); ++i)
  {
    ASG::SourceFile* file = *i;
    
    PyObject* decls, *new_decls, *incls, *new_incls;
    PyObject* pyfile = m->py(file);
    
    // Add the declarations if it's a main file
    if (file->is_primary())
    {
      decls = PyObject_GetAttrString(pyfile, "declarations");
      if (!decls) throw py_error_already_set();
      PyObject_CallMethod(decls, "extend", "O", new_decls = m->List(file->declarations()));
      // TODO: add the includes
      Py_DECREF(new_decls);
      Py_DECREF(decls);
    }
    
    // Add the includes
    incls = PyObject_GetAttrString(pyfile, "includes");
    if (!incls) throw py_error_already_set();
    PyObject_CallMethod(incls, "extend", "O", new_incls = m->List(file->includes()));
    Py_DECREF(new_incls);
    Py_DECREF(incls);
    
    // Add to the ASG
    PyObject* pyfilename = PyObject_GetAttrString(pyfile, "name");
    PyDict_SetItem(pyfiles, pyfilename, pyfile);
    Py_DECREF(pyfilename);
    Py_DECREF(pyfile);
  }
  
  Py_DECREF(pyfiles);
}

void Translator::set_builtin_decls(const ASG::Declaration::vector& builtin_decls)
{
  // Insert Base*'s into a set for fasger lookup
  ASG::Declaration::vector::const_iterator it = builtin_decls.begin();
  while (it != builtin_decls.end()) m->builtin_decl_set.insert(*it++);
}

PyObject *Translator::Base(Types::Base* type)
{
  Trace trace("Translator::Base", Trace::TRANSLATION);
  PyObject *name, *base;
  base = PyObject_CallMethod(m_asg_module, "BuiltinTypeId", "OO",
                             m->cxx(), name = m->QualifiedName(type->name()));
  PyObject_SetItem(m_dictionary, name, base);
  Py_DECREF(name);
  return base;
}

PyObject *Translator::Dependent(Types::Dependent* type)
{
  Trace trace("Translator::Dependent", Trace::TRANSLATION);
  PyObject *name, *base;
  base = PyObject_CallMethod(m_asg_module, "DependentTypeId", "OO",
                             m->cxx(), name = m->QualifiedName(type->name()));
  PyObject_SetItem(m_dictionary, name, base);
  Py_DECREF(name);
  return base;
}

PyObject *Translator::Unknown(Types::Named* type)
{
  Trace trace("Translator::Unknown", Trace::TRANSLATION);
  PyObject *name, *unknown;
  unknown = PyObject_CallMethod(m_asg_module, "UnknownTypeId", "OO",
                                m->cxx(), name = m->QualifiedName(type->name()));
  PyObject_SetItem(m_dictionary, name, unknown);
  Py_DECREF(name);
  return unknown;
}

PyObject *Translator::Declared(Types::Declared* type)
{
  Trace trace("Translator::Declared", Trace::TRANSLATION);
  PyObject *name, *declared, *decl;
  declared = PyObject_CallMethod(m_asg_module, "DeclaredTypeId", "OOO",
                                 m->cxx(), name = m->QualifiedName(type->name()), 
                                 decl = m->py(type->declaration()));
  // Skip zero-length names (eg: dummy declarators/enumerators)
  if (type->name().size())
    PyObject_SetItem(m_dictionary, name, declared);
  Py_DECREF(name);
  Py_DECREF(decl);
  return declared;
}

PyObject *Translator::Template(Types::Template* type)
{
  Trace trace("Translator::Template", Trace::TRANSLATION);
  PyObject *name, *templ, *decl, *params;
  templ = PyObject_CallMethod(m_asg_module, "TemplateId", "OOOO",
                              m->cxx(), name = m->QualifiedName(type->name()), 
                              decl = m->py(type->declaration()),
                              params = m->List(type->parameters()));
  PyObject_SetItem(m_dictionary, name, templ);
  Py_DECREF(name);
  Py_DECREF(decl);
  Py_DECREF(params);
  return templ;
}

PyObject *Translator::Modifier(Types::Modifier* type)
{
  Trace trace("Translator::Modifier", Trace::TRANSLATION);
  PyObject *modifier, *alias, *pre, *post;
  modifier = PyObject_CallMethod(m_asg_module, "ModifierTypeId", "OOOO",
                                 m->cxx(), alias = m->py(type->alias()),
                                 pre = m->List(type->pre()), post = m->List(type->post()));
  Py_DECREF(alias);
  Py_DECREF(pre);
  Py_DECREF(post);
  return modifier;
}

PyObject *Translator::Array(Types::Array *type)
{
  Trace trace("Translator::Array", Trace::TRANSLATION);
  PyObject *array, *alias, *sizes;
  array = PyObject_CallMethod(m_asg_module, "ArrayTypeId", "OOO",
                              m->cxx(), alias = m->py(type->alias()), 
                              sizes = m->List(type->sizes()));
  Py_DECREF(alias);
  Py_DECREF(sizes);
  return array;
}

PyObject *Translator::Parameterized(Types::Parameterized* type)
{
  Trace trace("Translator::Parametrized", Trace::TRANSLATION);
  PyObject *parametrized, *templ, *params;
  parametrized = PyObject_CallMethod(m_asg_module, "ParametrizedTypeId", "OOO",
                                     m->cxx(), templ = m->py(type->template_id()), 
                                     params = m->List(type->parameters()));
  Py_DECREF(templ);
  Py_DECREF(params);
  return parametrized;
}

PyObject *Translator::FuncPtr(Types::FuncPtr* type)
{
  Trace trace("Translator::FuncType", Trace::TRANSLATION);
  PyObject *func, *ret, *pre, *params;
  func = PyObject_CallMethod(m_asg_module, "FunctionTypeId", "OOOO",
                             m->cxx(), ret = m->py(type->return_type()), 
                             pre = m->List(type->pre()),
                             params = m->List(type->parameters()));
  Py_DECREF(ret);
  Py_DECREF(pre);
  Py_DECREF(params);
  return func;
}

void Translator::addComments(PyObject* pydecl, ASG::Declaration* cdecl)
{
  Trace trace("Translator::addComments", Trace::TRANSLATION);
  PyObject *annotations = PyObject_GetAttrString(pydecl, "annotations");
  PyObject *comments = m->List(cdecl->comments());
  int size = PyList_GET_SIZE(comments);
  if (size)
  {
    PyObject *lasg = PyList_GetItem(comments, size -1);
    if (!PyString_Size(lasg))
      PyList_SetItem(comments, size -1, Py_None);
  }

  PyDict_SetItemString(annotations, "comments", comments);
  // Also set the accessability..
  PyObject_SetAttrString(pydecl, "accessibility", PyInt_FromLong(int(cdecl->access())));
  Py_DECREF(annotations);
  Py_DECREF(comments);
}

//. merge the file with an existing (python) object if it exists
PyObject *Translator::SourceFile(ASG::SourceFile* file)
{
  // don't construct, but instead find existing python object !
  Trace trace("Translator::SourceFile", Trace::TRANSLATION);
  PyObject *files = PyObject_GetAttrString(m_ir, "files");
  if (!files) throw py_error_already_set();
  PyObject *pyfile = PyDict_GetItemString(files, const_cast<char *>(file->name().c_str()));
  if (!pyfile) // the file wasn't found, create it now
  {
    PyObject *name, *abs_name;
    pyfile = PyObject_CallMethod(m_sf_module, "SourceFile", "OOO",
				 name = m->py(file->name()),
				 abs_name = m->py(file->abs_name()),
				 m->cxx());
    if (!pyfile) throw py_error_already_set();
    Py_DECREF(name);
    Py_DECREF(abs_name);
  }
  else Py_INCREF(pyfile);

  // FIXME: why is this here ? Why can't we assume the preprocessor got it
  //        right ??

//   // update the 'main' attribute to reflect the right value (this may
//   // differ from the original one if the preprocessor sees a different
//   // 'extra include' list from the one passed to the cxx parser
//   PyObject_CallMethod(pyfile, "set_is_main", "i", (int)file->is_main());

  Py_DECREF(files);
  return pyfile;
}

PyObject *Translator::Include(ASG::Include* include)
{
  Trace trace("Translator::Include", Trace::TRANSLATION);
  PyObject *pyinclude, *target;
  pyinclude = PyObject_CallMethod(m_sf_module, "Include", "Oii",
                                  target = m->py(include->target()),
                                  include->is_macro() ? 1 : 0,
                                  include->is_next() ? 1 : 0);
  if (!pyinclude) throw py_error_already_set();
  Py_DECREF(target);
  return pyinclude;
}

PyObject *Translator::Declaration(ASG::Declaration* decl)
{
  Trace trace("Translator::Declaration", Trace::TRANSLATION);
  PyObject *pydecl, *file, *type, *name;
  pydecl = PyObject_CallMethod(m_asg_module, "Declaration", "OiOO",
                               file = m->py(decl->file()), decl->line(),
                               type = m->py(decl->type()), name = m->QualifiedName(decl->name()));
  if (!pydecl) throw py_error_already_set();
  addComments(pydecl, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return pydecl;
}

PyObject *Translator::Builtin(ASG::Builtin* decl)
{
  Trace trace("Translator::Builtin", Trace::TRANSLATION);
  PyObject *pybuiltin, *file, *type, *name;
  pybuiltin = PyObject_CallMethod(m_asg_module, "Builtin", "OiOO",
                                  file = m->py(decl->file()), decl->line(),
                                  type = m->py(decl->type()), name = m->QualifiedName(decl->name()));
  if (!pybuiltin) throw py_error_already_set();
  addComments(pybuiltin, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return pybuiltin;
}

PyObject *Translator::Macro(ASG::Macro* decl)
{
  Trace trace("Translator::Macro", Trace::TRANSLATION);
  PyObject *pymacro, *file, *type, *name, *params, *text;
  if (decl->parameters()) params = m->List(*decl->parameters());
  else
  {
    params = Py_None;
    Py_INCREF(Py_None);
  }
  pymacro = PyObject_CallMethod(m_asg_module, "Macro", "OiOOOO",
                                file = m->py(decl->file()), decl->line(),
                                type = m->py(decl->type()), name = m->QualifiedName(decl->name()),
                                params, text = m->py(decl->text()));
  if (!pymacro) throw py_error_already_set();
  addComments(pymacro, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(params);
  Py_DECREF(text);
  return pymacro;
}

PyObject *Translator::Forward(ASG::Forward* decl)
{
  Trace trace("Translator::Forward", Trace::TRANSLATION);
  PyObject *forward, *file, *type, *name;
  forward = PyObject_CallMethod(m_asg_module, "Forward", "OiOO",
                                file = m->py(decl->file()), decl->line(),
                                type = m->py(decl->type()), name = m->QualifiedName(decl->name()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, forward);
  if (decl->template_id())
  {
    PyObject* ttype = m->py(decl->template_id());
    PyObject_SetAttrString(forward, "template", ttype);
    Py_DECREF(ttype);
  }
  if (decl->is_template_specialization())
  {
    PyObject_SetAttrString(forward, "is_template_specialization", Py_True);
  }
  addComments(forward, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return forward;
}

PyObject *Translator::Scope(ASG::Scope* decl)
{
  Trace trace("Translator::Scope", Trace::TRANSLATION);
  PyObject *scope, *file, *type, *name;
  scope = PyObject_CallMethod(m_asg_module, "Scope", "OiOO",
                              file = m->py(decl->file()), decl->line(),
                              type = m->py(decl->type()), name = m->QualifiedName(decl->name()));
  PyObject *decls = PyObject_GetAttrString(scope, "declarations");
  PyObject_CallMethod(decls, "extend", "O", m->List(decl->declarations()));
  addComments(scope, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(decls);
  return scope;
}

PyObject *Translator::Namespace(ASG::Namespace* decl)
{
  Trace trace("Translator::Namespace", Trace::TRANSLATION);
  PyObject *module, *file, *type, *name;
  module = PyObject_CallMethod(m_asg_module, "Module", "OiOO",
                               file = m->py(decl->file()), decl->line(),
                               type = m->py(decl->type()), name = m->QualifiedName(decl->name()));
  PyObject *decls = PyObject_GetAttrString(module, "declarations");
  PyObject *new_decls = m->List(decl->declarations());
  PyObject_CallMethod(decls, "extend", "O", new_decls);
  addComments(module, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(decls);
  Py_DECREF(new_decls);
  return module;
}

PyObject *Translator::Inheritance(ASG::Inheritance* decl)
{
  Trace trace("Translator::Inheritance", Trace::TRANSLATION);
  PyObject *inheritance, *parent, *attrs;
  inheritance = PyObject_CallMethod(m_asg_module, "Inheritance", "sOO",
                                    "inherits", parent = m->py(decl->parent()), 
                                    attrs = m->List(decl->attributes()));
  Py_DECREF(parent);
  Py_DECREF(attrs);
  return inheritance;
}

PyObject *Translator::Class(ASG::Class* decl)
{
  Trace trace("Translator::Class", Trace::TRANSLATION);
  PyObject *clas, *file, *type, *name;
  clas = PyObject_CallMethod(m_asg_module, "Class", "OiOO",
                             file = m->py(decl->file()),
                             decl->line(),
                             type = m->py(decl->type()),
                             name = m->QualifiedName(decl->name()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, clas);
  PyObject *new_decls, *new_parents;
  PyObject *decls = PyObject_GetAttrString(clas, "declarations");
  PyObject_CallMethod(decls, "extend", "O", new_decls = m->List(decl->declarations()));
  PyObject *parents = PyObject_GetAttrString(clas, "parents");
  PyObject_CallMethod(parents, "extend", "O", new_parents = m->List(decl->parents()));
  if (decl->is_template_specialization())
  {
    PyObject_SetAttrString(clas, "is_template_specialization", Py_True);
  }
  addComments(clas, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(decls);
  Py_DECREF(parents);
  Py_DECREF(new_decls);
  Py_DECREF(new_parents);
  return clas;
}

PyObject *Translator::ClassTemplate(ASG::ClassTemplate *decl)
{
  Trace trace("Translator::ClassTemplate", Trace::TRANSLATION);
  PyObject *clas, *file, *type, *name;
  clas = PyObject_CallMethod(m_asg_module, "ClassTemplate", "OiOO",
                             file = m->py(decl->file()),
                             decl->line(),
                             type = m->py(decl->type()),
                             name = m->QualifiedName(decl->name()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, clas);
  PyObject *new_decls, *new_parents;
  PyObject *decls = PyObject_GetAttrString(clas, "declarations");
  PyObject_CallMethod(decls, "extend", "O", new_decls = m->List(decl->declarations()));

  PyObject* ttype = m->py(decl->template_id());
  PyObject_SetAttrString(clas, "template", ttype);
  Py_DECREF(ttype);

  PyObject *parents = PyObject_GetAttrString(clas, "parents");
  PyObject_CallMethod(parents, "extend", "O", new_parents = m->List(decl->parents()));
  if (decl->is_template_specialization())
  {
    PyObject_SetAttrString(clas, "is_template_specialization", Py_True);
  }
  addComments(clas, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(decls);
  Py_DECREF(parents);
  Py_DECREF(new_decls);
  Py_DECREF(new_parents);
  return clas;
}

PyObject *Translator::Typedef(ASG::Typedef* decl)
{
  Trace trace("Translator::Typedef", Trace::TRANSLATION);
  // FIXME: what to do about the declarator?
  PyObject *tdef, *file, *type, *name, *alias;
  tdef = PyObject_CallMethod(m_asg_module, "Typedef", "OiOOOi",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), name = m->QualifiedName(decl->name()),
                             alias = m->py(decl->alias()), decl->constructed());
  addComments(tdef, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(alias);
  return tdef;
}

PyObject *Translator::Enumerator(ASG::Enumerator* decl)
{
  Trace trace("Translator::Enumerator", Trace::TRANSLATION);
  PyObject *enumor, *file, *name, *type;
  if (decl->type() == "dummy") // work around a hack with another hack ;-)
  {
     std::vector<std::string> eos;
     eos.push_back("EOS");
     enumor = PyObject_CallMethod(m_asg_module, "Builtin", "OiOO",
                                  file = m->py(decl->file()), decl->line(),
                                  type = m->py("EOS"), name = m->QualifiedName(eos));
  }
  else
     enumor = PyObject_CallMethod(m_asg_module, "Enumerator", "OiOs",
                                  file = m->py(decl->file()), decl->line(),
                                  name = m->QualifiedName(decl->name()), decl->value().c_str());
  addComments(enumor, decl);
  Py_DECREF(file);
  Py_DECREF(name);
  return enumor;
}

PyObject *Translator::Enum(ASG::Enum* decl)
{
  Trace trace("Translator::Enum", Trace::TRANSLATION);
  PyObject *enumor, *file, *enums, *name;
  enumor = PyObject_CallMethod(m_asg_module, "Enum", "OiOO",
                               file = m->py(decl->file()), decl->line(),
                               name = m->QualifiedName(decl->name()), enums = m->List(decl->enumerators()));
  addComments(enumor, decl);
  Py_DECREF(file);
  Py_DECREF(enums);
  Py_DECREF(name);
  return enumor;
}

PyObject *Translator::Variable(ASG::Variable* decl)
{
  Trace trace("Translator::Variable", Trace::TRANSLATION);
  PyObject *var, *file, *type, *name, *vtype;
  var = PyObject_CallMethod(m_asg_module, "Variable", "OiOOOi",
                            file = m->py(decl->file()), decl->line(),
                            type = m->py(decl->type()), name = m->QualifiedName(decl->name()),
                            vtype = m->py(decl->vtype()), decl->constructed());
  addComments(var, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(vtype);
  Py_DECREF(name);
  return var;
}

PyObject *Translator::Const(ASG::Const* decl)
{
  Trace trace("Translator::Const", Trace::TRANSLATION);
  PyObject *cons, *file, *type, *name, *ctype;
  cons = PyObject_CallMethod(m_asg_module, "Const", "OiOOOs",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), ctype = m->py(decl->ctype()),
                             name = m->QualifiedName(decl->name()), decl->value().c_str());
  if (PyErr_Occurred()) PyErr_Print();
  addComments(cons, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(ctype);
  Py_DECREF(name);
  return cons;
}

PyObject *Translator::Parameter(ASG::Parameter* decl)
{
  Trace trace("Translator::Parameter", Trace::TRANSLATION);
  PyObject *param, *pre, *post, *type, *value, *name;
  param = PyObject_CallMethod(m_asg_module, "Parameter", "OOOOO",
                              pre = m->List(decl->premodifier()), type = m->py(decl->type()), 
                              post = m->List(decl->postmodifier()),
                              name = m->py(decl->name()), value = m->py(decl->value()));
  Py_DECREF(pre);
  Py_DECREF(post);
  Py_DECREF(type);
  Py_DECREF(value);
  Py_DECREF(name);
  return param;
}

PyObject *Translator::Function(ASG::Function* decl)
{
  Trace trace("Translator::Function", Trace::TRANSLATION);
  PyObject *func, *file, *type, *name, *pre, *ret, *post, *realname;
  char const *class_ = decl->template_id() ? "FunctionTemplate" : "Function";
  func = PyObject_CallMethod(m_asg_module, const_cast<char *>(class_), "OiOOOOOO",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), pre = m->List(decl->premodifier()),
                             ret = m->py(decl->return_type()), post = m->List(decl->postmodifier()),
                             name = m->QualifiedName(decl->name()), realname = m->py(decl->realname()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, func);
  if (decl->template_id())
  {
    PyObject* ttype = m->py(decl->template_id());
    PyObject_SetAttrString(func, "template", ttype);
    Py_DECREF(ttype);
  }
  PyObject* new_params;
  PyObject* params = PyObject_GetAttrString(func, "parameters");
  PyObject_CallMethod(params, "extend", "O", new_params = m->List(decl->parameters()));
  addComments(func, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(pre);
  Py_DECREF(ret);
  Py_DECREF(post);
  Py_DECREF(realname);
  Py_DECREF(params);
  Py_DECREF(new_params);
  return func;
}

PyObject *Translator::Operation(ASG::Operation* decl)
{
  Trace trace("Translator::Operation", Trace::TRANSLATION);
  PyObject *oper, *file, *type, *name, *pre, *ret, *post, *realname;
  char const *class_ = decl->template_id() ? "OperationTemplate" : "Operation";
  oper = PyObject_CallMethod(m_asg_module, const_cast<char *>(class_), "OiOOOOOO",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), pre = m->List(decl->premodifier()),
                             ret = m->py(decl->return_type()), post = m->List(decl->postmodifier()),
                             name = m->QualifiedName(decl->name()), realname = m->py(decl->realname()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, oper);
  if (decl->template_id())
  {
    PyObject* ttype = m->py(decl->template_id());
    PyObject_SetAttrString(oper, "template", ttype);
    Py_DECREF(ttype);
  }
  PyObject* new_params;
  PyObject* params = PyObject_GetAttrString(oper, "parameters");
  PyObject_CallMethod(params, "extend", "O", new_params = m->List(decl->parameters()));
  addComments(oper, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(pre);
  Py_DECREF(ret);
  Py_DECREF(post);
  Py_DECREF(realname);
  Py_DECREF(params);
  Py_DECREF(new_params);
  return oper;
}

PyObject *Translator::UsingDirective(ASG::UsingDirective* u)
{
  Trace trace("Translator::UsingDirective", Trace::TRANSLATION);
  PyObject *dir, *file, *type, *name;
  dir = PyObject_CallMethod(m_asg_module, "UsingDirective", "OiOO",
                            file = m->py(u->file()), u->line(),
                            type = m->py(u->type()), name = m->QualifiedName(u->name()));
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return dir;
}

PyObject *Translator::UsingDeclaration(ASG::UsingDeclaration* u)
{
  Trace trace("Translator::UsingDeclaration", Trace::TRANSLATION);
  PyObject *decl, *file, *type, *name, *alias;
  decl = PyObject_CallMethod(m_asg_module, "UsingDeclaration", "OiOOO",
                             file = m->py(u->file()), u->line(),
                             type = m->py(u->type()), name = m->QualifiedName(u->name()),
                             alias = m->QualifiedName(u->target()->name()));
  Py_DECREF(alias);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return decl;
}

void Translator::visit_declaration(ASG::Declaration* decl)
{
  // Assume this is a dummy declaration
  if (m_filter->should_store(decl)) m->add(decl, Declaration(decl));
}
void Translator::visit_builtin(ASG::Builtin* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Builtin(decl));
}
void Translator::visit_macro(ASG::Macro* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Macro(decl));
}
void Translator::visit_scope(ASG::Scope* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Scope(decl));
  //else
  //	m->add(decl, Forward(new ASG::Forward(decl)));
}
void Translator::visit_namespace(ASG::Namespace* decl)
{
  // Namespaces are always included, because the Linker knows to combine
  // them always. The exception are "local namespaces", those created to
  // handle scopes created by braces in code.
  if (decl->type() != "local") m->add(decl, Namespace(decl));
}
void Translator::visit_class(ASG::Class* decl)
{
  // Add if the class is in the main file, *or* if it has any members
  // declared in the main file (eg: forward declared nested classes which
  // are fully defined in this main file)
  if (m_filter->should_store(decl)) m->add(decl, Class(decl));
  //else
  //    m->add(decl, Forward(new ASG::Forward(decl)));
}
void Translator::visit_class_template(ASG::ClassTemplate* decl)
{
  // Add if the class is in the main file, *or* if it has any members
  // declared in the main file (eg: forward declared nested classes which
  // are fully defined in this main file)
  if (m_filter->should_store(decl)) m->add(decl, ClassTemplate(decl));
}
void Translator::visit_forward(ASG::Forward* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Forward(decl));
  //else
  //    m->add(decl, Forward(new ASG::Forward(decl)));
}
void Translator::visit_typedef(ASG::Typedef* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Typedef(decl));
  //else
  //    m->add(decl, Forward(new ASG::Forward(decl)));
}
void Translator::visit_variable(ASG::Variable* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Variable(decl));
  //else
  //    m->add(decl, Forward(new ASG::Forward(decl)));
}
void Translator::visit_const(ASG::Const* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Const(decl));
  //else
  //    m->add(decl, Forward(new ASG::Forward(decl)));
}
void Translator::visit_enum(ASG::Enum* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Enum(decl));
  //else
  //    m->add(decl, Forward(new ASG::Forward(decl)));
}
void Translator::visit_enumerator(ASG::Enumerator* decl)
{
  m->add(decl, Enumerator(decl));
}
void Translator::visit_function(ASG::Function* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Function(decl));
  //else
  //    m->add(decl, Forward(new ASG::Forward(decl)));
}
void Translator::visit_operation(ASG::Operation* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Operation(decl));
  //else
  //    m->add(decl, Forward(new ASG::Forward(decl)));
}

void Translator::visit_inheritance(ASG::Inheritance* decl)
{
  m->add(decl, Inheritance(decl));
}
void Translator::visit_parameter(ASG::Parameter* decl)
{
  m->add(decl, Parameter(decl));
}
void Translator::visit_using_directive(ASG::UsingDirective* u)
{
  m->add(u, UsingDirective(u));
}
void Translator::visit_using_declaration(ASG::UsingDeclaration* u)
{
  m->add(u, UsingDeclaration(u));
}

//
// Types::Visitor methods
//
/*void Translator::visitType(Types::Type* type) {
  m->add(type, this->Type(type));
  }*/
void Translator::visit_unknown(Types::Unknown* type)
{
  m->add(type, Unknown(type));
}
void Translator::visit_dependent(Types::Dependent* type)
{
  m->add(type, Dependent(type));
}
void Translator::visit_modifier(Types::Modifier* type)
{
  m->add(type, Modifier(type));
}
void Translator::visit_array(Types::Array *type)
{
  m->add(type, Array(type));
}
/*void Translator::visitNamed(Types::Named* type) {
  m->add(type, Named(type));
  }*/
void Translator::visit_base(Types::Base* type)
{
  m->add(type, Base(type));
}
void Translator::visit_declared(Types::Declared* type)
{
  if (!m_filter->should_store(type->declaration()))
    m->add(type, Unknown(type));
  else m->add(type, Declared(type));
}
void Translator::visit_template_type(Types::Template* type)
{
  if (!m_filter->should_store(type->declaration()))
    m->add(type, Unknown(type));
  else m->add(type, Template(type));
}
void Translator::visit_parameterized(Types::Parameterized* type)
{
  m->add(type, Parameterized(type));
}
void Translator::visit_func_ptr(Types::FuncPtr* type)
{
  m->add(type, FuncPtr(type));
}
