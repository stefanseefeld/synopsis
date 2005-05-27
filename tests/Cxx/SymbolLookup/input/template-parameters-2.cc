template <class T> class myarray { /* ... */ };
template <class T> func() {}
template <class K, class V, template<class T> class C = myarray>
class Map
{
  C<K> key;
  C<V> value;
};
