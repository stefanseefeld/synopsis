#ifndef _StorableImpl_hh
#define _StorableImpl_hh

#include <Storage.hh>

//. Storable implementation. This is an implementation of the IDL interface
//. Storage::Storable
class StorableImpl : public virtual POA_Storage::Storable
{
public:
  //. Constructor. You know what a constructor is
  StorableImpl();
protected:
  //. Destructor. Note its virtuality
  virtual ~StorableImpl();
public:
  //. Store the given object state. The _ptr of the type means that this is a
  //. CORBA object reference, since this may be called via a remote interface
  virtual void store(Storage::ObjectState_ptr);
  //. Loads the given object state.
  virtual void load(Storage::ObjectState_ptr);

private:
  //. Private data structure
  struct Private;
  //. Private data. The 'Private' type is only defined in the .cc file.
  Private _m;
};

#endif
