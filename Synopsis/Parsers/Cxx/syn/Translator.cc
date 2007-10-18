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
#include "filter.hh"

#include <map>
#include <set>
#include <iostream>
#include <stdexcept>

using namespace Synopsis;

#define assertObject(pyo) if (!pyo) PyErr_Print(); assert(pyo)

// This func is called so you can breakpoint on it
void nullObj()
{
  std::cout << "Null ptr." << std::endl;
  if (PyErr_Occurred()) PyErr_Print();
  throw std::runtime_error("internal error");
}

// The compiler firewalled private stuff
struct Translator::Private
{
  //. Constructor
  Private(Translator* s) : m_syn(s)
  {
    m_cxx = PyString_InternFromString("C++");
    Py_INCREF(Py_None);
    add((AST::Declaration*)0, Py_None);
    Py_INCREF(Py_None);
    add((Types::Type*)0, Py_None);
  }
  //. Reference to parent synopsis object
  Translator* m_syn;
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
  std::set<AST::Declaration*> builtin_decl_set;
  
  // Note that these methods always succeed
  
  //. Return the PyObject for the given AST::SourceFile
  PyObject* py(AST::SourceFile*);
  //. Return the PyObject for the given AST::Include
  PyObject* py(AST::Include*);
  //. Return the PyObject for the given AST::Declaration
  PyObject* py(AST::Declaration*);
  //. Return the PyObject for the given AST::Inheritance
  PyObject* py(AST::Inheritance*);
  //. Return the PyObject for the given AST::Parameter
  PyObject* py(AST::Parameter*);
  //. Return the PyObject for the given Types::Type
  PyObject* py(Types::Type*);
  //. Return the PyObject for the given string
  PyObject* py(const std::string &);
  
  //. Add the given pair
  void add(void* cobj, PyObject* pyobj)
  {
    if (!pyobj) { nullObj();}
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
PyObject* Translator::Private::List(const std::vector<AST::Declaration*>& vec)
{
  std::vector<PyObject*> objects;
  std::vector<AST::Declaration*>::const_iterator iter = vec.begin();
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

PyObject* Translator::Private::py(AST::SourceFile* file)
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
      throw "Translator::Private::py(AST::SourceFile*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Translator::Private::py(AST::Include* incl)
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
      throw "Translator::Private::py(AST::Include*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}

PyObject* Translator::Private::py(AST::Declaration* decl)
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
        throw "Translator::Private::py(AST::Declaration*)";
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

PyObject* Translator::Private::py(AST::Inheritance* decl)
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
      throw "Translator::Private::py(AST::Inheritance*)";
    }
  }
  PyObject* obj = iter->second;
  Py_INCREF(obj);
  return obj;
}
PyObject* Translator::Private::py(AST::Parameter* decl)
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
      throw "Translator::Private::py(AST::Parameter*)";
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

Translator::Translator(FileFilter* filter, PyObject *ast)
  : m_ast(ast), m_filter(filter)
{
  Trace trace("Translator::Translator", Trace::TRANSLATION);
  m_ast_module  = PyImport_ImportModule("Synopsis.AST");
  assertObject(m_ast_module);
  m_sf_module  = PyImport_ImportModule("Synopsis.SourceFile");
  assertObject(m_sf_module);
  m_type_module = PyImport_ImportModule("Synopsis.Type");
  assertObject(m_type_module);
  
  m_declarations = PyObject_GetAttrString(m_ast, "declarations");
  assertObject(m_declarations);
  m_dictionary = PyObject_GetAttrString(m_ast, "types");
  assertObject(m_dictionary);
  
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
  
  Py_DECREF(m_type_module);
  Py_DECREF(m_ast_module);
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

void Translator::translate(AST::Scope* scope)//, PyObject* ast)
{
  AST::Declaration::vector globals, &decls = scope->declarations();
  AST::Declaration::vector::iterator it = decls.begin();
  
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
  PyObject* pyfiles = PyObject_GetAttrString(m_ast, "files");
  assertObject(pyfiles);
  assert(PyDict_Check(pyfiles));
  
  AST::SourceFile::vector all_sourcefiles;
  m_filter->get_all_sourcefiles(all_sourcefiles);
  AST::SourceFile::vector::iterator file_iter = all_sourcefiles.begin();
  while (file_iter != all_sourcefiles.end())
  {
    AST::SourceFile* file = *file_iter++;
    
    PyObject* decls, *new_decls, *incls, *new_incls;
    PyObject* pyfile = m->py(file);
    
    // Add the declarations if it's a main file
    if (file->is_main())
    {
      decls = PyObject_GetAttrString(pyfile, "declarations");
      assertObject(decls);
      PyObject_CallMethod(decls, "extend", "O", new_decls = m->List(file->declarations()));
      // TODO: add the includes
      Py_DECREF(new_decls);
      Py_DECREF(decls);
    }
    
    // Add the includes
    incls = PyObject_GetAttrString(pyfile, "includes");
    assertObject(incls);
    PyObject_CallMethod(incls, "extend", "O", new_incls = m->List(file->includes()));
    Py_DECREF(new_incls);
    Py_DECREF(incls);
    
    // Add to the AST
    PyObject* pyfilename = PyObject_GetAttrString(pyfile, "name");
    PyDict_SetItem(pyfiles, pyfilename, pyfile);
    Py_DECREF(pyfilename);
    Py_DECREF(pyfile);
  }
  
  Py_DECREF(pyfiles);
}

void Translator::set_builtin_decls(const AST::Declaration::vector& builtin_decls)
{
  // Insert Base*'s into a set for faster lookup
  AST::Declaration::vector::const_iterator it = builtin_decls.begin();
  while (it != builtin_decls.end()) m->builtin_decl_set.insert(*it++);
}

PyObject *Translator::Base(Types::Base* type)
{
  Trace trace("Translator::Base", Trace::TRANSLATION);
  PyObject *name, *base;
  base = PyObject_CallMethod(m_type_module, "Base", "OO",
                             m->cxx(), name = m->Tuple(type->name()));
  PyObject_SetItem(m_dictionary, name, base);
  Py_DECREF(name);
  return base;
}

PyObject *Translator::Dependent(Types::Dependent* type)
{
  Trace trace("Translator::Dependent", Trace::TRANSLATION);
  PyObject *name, *base;
  base = PyObject_CallMethod(m_type_module, "Dependent", "OO",
                             m->cxx(), name = m->Tuple(type->name()));
  PyObject_SetItem(m_dictionary, name, base);
  Py_DECREF(name);
  return base;
}

PyObject *Translator::Unknown(Types::Named* type)
{
  Trace trace("Translator::Unknown", Trace::TRANSLATION);
  PyObject *name, *unknown;
  unknown = PyObject_CallMethod(m_type_module, "Unknown", "OO",
                                m->cxx(), name = m->Tuple(type->name()));
  PyObject_SetItem(m_dictionary, name, unknown);
  Py_DECREF(name);
  return unknown;
}

PyObject *Translator::Declared(Types::Declared* type)
{
  Trace trace("Translator::Declared", Trace::TRANSLATION);
  PyObject *name, *declared, *decl;
  declared = PyObject_CallMethod(m_type_module, "Declared", "OOO",
                                 m->cxx(), name = m->Tuple(type->name()), 
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
  templ = PyObject_CallMethod(m_type_module, "Template", "OOOO",
                              m->cxx(), name = m->Tuple(type->name()), 
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
  modifier = PyObject_CallMethod(m_type_module, "Modifier", "OOOO",
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
  array = PyObject_CallMethod(m_type_module, "Array", "OOO",
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
  parametrized = PyObject_CallMethod(m_type_module, "Parametrized", "OOO",
                                     m->cxx(), templ = m->py(type->template_type()), 
                                     params = m->List(type->parameters()));
  Py_DECREF(templ);
  Py_DECREF(params);
  return parametrized;
}

PyObject *Translator::FuncPtr(Types::FuncPtr* type)
{
  Trace trace("Translator::FuncType", Trace::TRANSLATION);
  PyObject *func, *ret, *pre, *params;
  func = PyObject_CallMethod(m_type_module, "Function", "OOOO",
                             m->cxx(), ret = m->py(type->return_type()), 
                             pre = m->List(type->pre()),
                             params = m->List(type->parameters()));
  Py_DECREF(ret);
  Py_DECREF(pre);
  Py_DECREF(params);
  return func;
}

void Translator::addComments(PyObject* pydecl, AST::Declaration* cdecl)
{
  Trace trace("Translator::addComments", Trace::TRANSLATION);
  PyObject *annotations = PyObject_GetAttrString(pydecl, "annotations");
  PyObject *comments = m->List(cdecl->comments());
  int size = PyList_GET_SIZE(comments);
  if (size)
  {
    PyObject *last = PyList_GetItem(comments, size -1);
    if (!PyString_Size(last))
      PyList_SetItem(comments, size -1, Py_None);
  }

  PyDict_SetItemString(annotations, "comments", comments);
  // Also set the accessability..
  PyObject_CallMethod(pydecl, "set_accessibility", "i", int(cdecl->access()));
  Py_DECREF(annotations);
  Py_DECREF(comments);
}

//. merge the file with an existing (python) object if it exists
PyObject *Translator::SourceFile(AST::SourceFile* file)
{
  // don't construct, but instead find existing python object !
  Trace trace("Translator::SourceFile", Trace::TRANSLATION);
  PyObject *files = PyObject_GetAttrString(m_ast, "files");
  assertObject(files);
  PyObject *pyfile = PyDict_GetItemString(files, const_cast<char *>(file->filename().c_str()));
  if (!pyfile) // the file wasn't found, create it now
  {
    PyObject *filename, *full_filename;
    pyfile = PyObject_CallMethod(m_sf_module, "SourceFile", "OOO",
				 filename = m->py(file->filename()),
				 full_filename = m->py(file->full_filename()),
				 m->cxx());
    assertObject(pyfile);
    Py_DECREF(filename);
    Py_DECREF(full_filename);
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

PyObject *Translator::Include(AST::Include* include)
{
  Trace trace("Translator::Include", Trace::TRANSLATION);
  PyObject *pyinclude, *target;
  pyinclude = PyObject_CallMethod(m_sf_module, "Include", "Oii",
                                  target = m->py(include->target()),
                                  include->is_macro() ? 1 : 0,
                                  include->is_next() ? 1 : 0);
  assertObject(pyinclude);
  Py_DECREF(target);
  return pyinclude;
}

PyObject *Translator::Declaration(AST::Declaration* decl)
{
  Trace trace("Translator::Declaration", Trace::TRANSLATION);
  PyObject *pydecl, *file, *type, *name;
  pydecl = PyObject_CallMethod(m_ast_module, "Declaration", "OiOO",
                               file = m->py(decl->file()), decl->line(),
                               type = m->py(decl->type()), name = m->Tuple(decl->name()));
  assertObject(pydecl);
  addComments(pydecl, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return pydecl;
}

PyObject *Translator::Builtin(AST::Builtin* decl)
{
  Trace trace("Translator::Builtin", Trace::TRANSLATION);
  PyObject *pybuiltin, *file, *type, *name;
  pybuiltin = PyObject_CallMethod(m_ast_module, "Builtin", "OiOO",
                                  file = m->py(decl->file()), decl->line(),
                                  type = m->py(decl->type()), name = m->Tuple(decl->name()));
  assertObject(pybuiltin);
  addComments(pybuiltin, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return pybuiltin;
}

PyObject *Translator::Macro(AST::Macro* decl)
{
  Trace trace("Translator::Macro", Trace::TRANSLATION);
  PyObject *pymacro, *file, *type, *name, *params, *text;
  if (decl->parameters()) params = m->List(*decl->parameters());
  else
  {
    params = Py_None;
    Py_INCREF(Py_None);
  }
  pymacro = PyObject_CallMethod(m_ast_module, "Macro", "OiOOOO",
                                file = m->py(decl->file()), decl->line(),
                                type = m->py(decl->type()), name = m->Tuple(decl->name()),
                                params, text = m->py(decl->text()));
  assertObject(pymacro);
  addComments(pymacro, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(params);
  Py_DECREF(text);
  return pymacro;
}

PyObject *Translator::Forward(AST::Forward* decl)
{
  Trace trace("Translator::Forward", Trace::TRANSLATION);
  PyObject *forward, *file, *type, *name;
  forward = PyObject_CallMethod(m_ast_module, "Forward", "OiOO",
                                file = m->py(decl->file()), decl->line(),
                                type = m->py(decl->type()), name = m->Tuple(decl->name()));
  addComments(forward, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  return forward;
}

PyObject *Translator::Scope(AST::Scope* decl)
{
  Trace trace("Translator::Scope", Trace::TRANSLATION);
  PyObject *scope, *file, *type, *name;
  scope = PyObject_CallMethod(m_ast_module, "Scope", "OiOO",
                              file = m->py(decl->file()), decl->line(),
                              type = m->py(decl->type()), name = m->Tuple(decl->name()));
  PyObject *decls = PyObject_CallMethod(scope, "declarations", 0);
  PyObject_CallMethod(decls, "extend", "O", m->List(decl->declarations()));
  addComments(scope, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(decls);
  return scope;
}

PyObject *Translator::Namespace(AST::Namespace* decl)
{
  Trace trace("Translator::Namespace", Trace::TRANSLATION);
  PyObject *module, *file, *type, *name;
  module = PyObject_CallMethod(m_ast_module, "Module", "OiOO",
                               file = m->py(decl->file()), decl->line(),
                               type = m->py(decl->type()), name = m->Tuple(decl->name()));
  PyObject *decls = PyObject_CallMethod(module, "declarations", 0);
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

PyObject *Translator::Inheritance(AST::Inheritance* decl)
{
  Trace trace("Translator::Inheritance", Trace::TRANSLATION);
  PyObject *inheritance, *parent, *attrs;
  inheritance = PyObject_CallMethod(m_ast_module, "Inheritance", "sOO",
                                    "inherits", parent = m->py(decl->parent()), 
                                    attrs = m->List(decl->attributes()));
  Py_DECREF(parent);
  Py_DECREF(attrs);
  return inheritance;
}

PyObject *Translator::Class(AST::Class* decl)
{
  Trace trace("Translator::Class", Trace::TRANSLATION);
  PyObject *clas, *file, *type, *name;
  clas = PyObject_CallMethod(m_ast_module, "Class", "OiOO",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), name = m->Tuple(decl->name()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, clas);
  PyObject *new_decls, *new_parents;
  PyObject *decls = PyObject_CallMethod(clas, "declarations", 0);
  PyObject_CallMethod(decls, "extend", "O", new_decls = m->List(decl->declarations()));
  PyObject *parents = PyObject_CallMethod(clas, "parents", 0);
  PyObject_CallMethod(parents, "extend", "O", new_parents = m->List(decl->parents()));
  if (decl->template_type())
  {
    PyObject* ttype;
    PyObject_CallMethod(clas, "set_template", "O", ttype = m->py(decl->template_type()));
    Py_DECREF(ttype);
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

PyObject *Translator::Typedef(AST::Typedef* decl)
{
  Trace trace("Translator::Typedef", Trace::TRANSLATION);
  // FIXME: what to do about the declarator?
  PyObject *tdef, *file, *type, *name, *alias;
  tdef = PyObject_CallMethod(m_ast_module, "Typedef", "OiOOOi",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), name = m->Tuple(decl->name()),
                             alias = m->py(decl->alias()), decl->constructed());
  addComments(tdef, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(name);
  Py_DECREF(alias);
  return tdef;
}

PyObject *Translator::Enumerator(AST::Enumerator* decl)
{
  Trace trace("Translator::Enumerator", Trace::TRANSLATION);
  PyObject *enumor, *file, *name, *type;
  if (decl->type() == "dummy") // work around a hack with another hack ;-)
  {
     std::vector<std::string> eos;
     eos.push_back("EOS");
     enumor = PyObject_CallMethod(m_ast_module, "Builtin", "OiOO",
                                  file = m->py(decl->file()), decl->line(),
                                  type = m->py("EOS"), name = m->Tuple(eos));
  }
  else
     enumor = PyObject_CallMethod(m_ast_module, "Enumerator", "OiOs",
                                  file = m->py(decl->file()), decl->line(),
                                  name = m->Tuple(decl->name()), decl->value().c_str());
  addComments(enumor, decl);
  Py_DECREF(file);
  Py_DECREF(name);
  return enumor;
}

PyObject *Translator::Enum(AST::Enum* decl)
{
  Trace trace("Translator::Enum", Trace::TRANSLATION);
  PyObject *enumor, *file, *enums, *name;
  enumor = PyObject_CallMethod(m_ast_module, "Enum", "OiOO",
                               file = m->py(decl->file()), decl->line(),
                               name = m->Tuple(decl->name()), enums = m->List(decl->enumerators()));
  addComments(enumor, decl);
  Py_DECREF(file);
  Py_DECREF(enums);
  Py_DECREF(name);
  return enumor;
}

PyObject *Translator::Variable(AST::Variable* decl)
{
  Trace trace("Translator::Variable", Trace::TRANSLATION);
  PyObject *var, *file, *type, *name, *vtype;
  var = PyObject_CallMethod(m_ast_module, "Variable", "OiOOOi",
                            file = m->py(decl->file()), decl->line(),
                            type = m->py(decl->type()), name = m->Tuple(decl->name()),
                            vtype = m->py(decl->vtype()), decl->constructed());
  addComments(var, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(vtype);
  Py_DECREF(name);
  return var;
}

PyObject *Translator::Const(AST::Const* decl)
{
  Trace trace("Translator::Const", Trace::TRANSLATION);
  PyObject *cons, *file, *type, *name, *ctype;
  cons = PyObject_CallMethod(m_ast_module, "Const", "OiOOOs",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), ctype = m->py(decl->ctype()),
                             name = m->Tuple(decl->name()), decl->value().c_str());
  if (PyErr_Occurred()) PyErr_Print();
  addComments(cons, decl);
  Py_DECREF(file);
  Py_DECREF(type);
  Py_DECREF(ctype);
  Py_DECREF(name);
  return cons;
}

PyObject *Translator::Parameter(AST::Parameter* decl)
{
  Trace trace("Translator::Parameter", Trace::TRANSLATION);
  PyObject *param, *pre, *post, *type, *value, *name;
  param = PyObject_CallMethod(m_ast_module, "Parameter", "OOOOO",
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

PyObject *Translator::Function(AST::Function* decl)
{
  Trace trace("Translator::Function", Trace::TRANSLATION);
  PyObject *func, *file, *type, *name, *pre, *ret, *post, *realname;
  func = PyObject_CallMethod(m_ast_module, "Function", "OiOOOOOO",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), pre = m->List(decl->premodifier()),
                             ret = m->py(decl->return_type()), post = m->List(decl->postmodifier()),
                             name = m->Tuple(decl->name()), realname = m->py(decl->realname()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, func);
  PyObject* new_params;
  PyObject* params = PyObject_CallMethod(func, "parameters", 0);
  PyObject_CallMethod(params, "extend", "O", new_params = m->List(decl->parameters()));
  if (decl->template_type())
  {
    PyObject* ttype;
    PyObject_CallMethod(func, "set_template", "O", ttype = m->py(decl->template_type()));
    Py_DECREF(ttype);
  }
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

PyObject *Translator::Operation(AST::Operation* decl)
{
  Trace trace("Translator::Operation", Trace::TRANSLATION);
  PyObject *oper, *file, *type, *name, *pre, *ret, *post, *realname;
  oper = PyObject_CallMethod(m_ast_module, "Operation", "OiOOOOOO",
                             file = m->py(decl->file()), decl->line(),
                             type = m->py(decl->type()), pre = m->List(decl->premodifier()),
                             ret = m->py(decl->return_type()), post = m->List(decl->postmodifier()),
                             name = m->Tuple(decl->name()), realname = m->py(decl->realname()));
  // This is necessary to prevent inf. loops in several places
  m->add(decl, oper);
  PyObject* new_params;
  PyObject* params = PyObject_CallMethod(oper, "parameters", 0);
  PyObject_CallMethod(params, "extend", "O", new_params = m->List(decl->parameters()));
  if (decl->template_type())
  {
    PyObject* ttype;
    PyObject_CallMethod(oper, "set_template", "O", ttype = m->py(decl->template_type()));
    Py_DECREF(ttype);
  }
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

void Translator::visit_declaration(AST::Declaration* decl)
{
  // Assume this is a dummy declaration
  if (m_filter->should_store(decl)) m->add(decl, Declaration(decl));
}
void Translator::visit_builtin(AST::Builtin* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Builtin(decl));
}
void Translator::visit_macro(AST::Macro* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Macro(decl));
}
void Translator::visit_scope(AST::Scope* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Scope(decl));
  //else
  //	m->add(decl, Forward(new AST::Forward(decl)));
}
void Translator::visit_namespace(AST::Namespace* decl)
{
  // Namespaces are always included, because the Linker knows to combine
  // them always. The exception are "local namespaces", those created to
  // handle scopes created by braces in code.
  if (decl->type() != "local") m->add(decl, Namespace(decl));
}
void Translator::visit_class(AST::Class* decl)
{
  // Add if the class is in the main file, *or* if it has any members
  // declared in the main file (eg: forward declared nested classes which
  // are fully defined in this main file)
  if (m_filter->should_store(decl)) m->add(decl, Class(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Translator::visit_forward(AST::Forward* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Forward(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Translator::visit_typedef(AST::Typedef* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Typedef(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Translator::visit_variable(AST::Variable* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Variable(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Translator::visit_const(AST::Const* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Const(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Translator::visit_enum(AST::Enum* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Enum(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Translator::visit_enumerator(AST::Enumerator* decl)
{
  m->add(decl, Enumerator(decl));
}
void Translator::visit_function(AST::Function* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Function(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}
void Translator::visit_operation(AST::Operation* decl)
{
  if (m_filter->should_store(decl)) m->add(decl, Operation(decl));
  //else
  //    m->add(decl, Forward(new AST::Forward(decl)));
}

void Translator::visit_inheritance(AST::Inheritance* decl)
{
  m->add(decl, Inheritance(decl));
}
void Translator::visit_parameter(AST::Parameter* decl)
{
  m->add(decl, Parameter(decl));
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
