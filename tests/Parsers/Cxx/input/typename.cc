template <std::size_t Bits, typename ::boost::uint_t<Bits>::fast TruncPoly>
class crc_optimal
{
   // Implementation type
   typedef detail::mask_uint_t<Bits> masking_type;

public:
   // Type
   typedef typename masking_type::fast  value_type;

   // Constants for the template parameters
   static const std::size_t  bit_count = Bits;
};
