#ifndef _StorableImpl_hh
#define _StorableImpl_hh

#include <Storage.hh>

class StorableImpl : public virtual Storage_POA::Storable
{
public:
  StorableImpl();
  virtual ~StorableImpl();
  virtual void store(ObjectState_ptr);
  virtual void load(ObjectState_ptr);
};

#endif
