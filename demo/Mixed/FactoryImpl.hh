#ifndef _FactoryImpl_hh
#define _FactoryImpl_hh

#include <Storage.hh>

class FactoryImpl : public virtual POA_Storage::Factory
{
public:
  FactoryImpl();
  virtual ~FactoryImpl();
  virtual Storage::Storable_ptr create_uninitialized();
};

#endif
