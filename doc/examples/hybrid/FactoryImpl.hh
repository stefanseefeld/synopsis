#ifndef FactoryImpl_hh_
#define FactoryImpl_hh_

#include <Storage.hh>

class FactoryImpl : public virtual POA_Storage::Factory
{
public:
  FactoryImpl();
  virtual ~FactoryImpl();
  virtual Storage::Storable_ptr create_uninitialized();
};

#endif
