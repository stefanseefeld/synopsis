#ifndef _StorableImpl_hh
#define _StorableImpl_hh

#include <Storage.hh>

class StorableImpl : public virtual POA_Storage::Storable
{
public:
  StorableImpl();
  virtual ~StorableImpl();
  virtual void store(Storage::ObjectState_ptr);
  virtual void load(Storage::ObjectState_ptr);
};

#endif
