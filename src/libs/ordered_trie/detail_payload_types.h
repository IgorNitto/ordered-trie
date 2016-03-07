/**
 * @file  detail_payload_types.h
 * @brief Serialisation scheme for metadata
 */

#ifndef ORDERED_TRIE_DETAIL_PAYLOAD_TYPES_H
#define ORDERED_TRIE_DETAIL_PAYLOAD_TYPES_H

#include "serialise.h"

namespace ordered_trie {

namespace detail {

template<typename T>
struct SerialiseIntegral
{
  static_assert (std::is_integral<T>::value, "Not integral");

  constexpr static
  inline void serialise (std::vector<std::uint8_t> &out,
			 const T value)
  {
    auto size = out.size ();
    out.resize (out.size () + sizeof (T));
    auto *out_ptr = reinterpret_cast<T*> (&out[size]);
    *out_ptr = value;
  }

  constexpr static
  inline T deserialise (const std::uint8_t *in)
  {
    return *(reinterpret_cast<const T*> (in));
  }

  constexpr static
  inline size_t serialised_size (const T&)
  {
    return sizeof (T);
  }

  constexpr static
  inline const std::uint8_t* next (const std::uint8_t *in)
  {
    return in + sizeof (T);
  }

  constexpr static
  inline size_t estimated_max_size ()
  {
    return sizeof (T);
  }  
};

} // namespace detail {

/**
 * Empty metadata type
 */
struct Void
{
  bool operator==(Void) const {return true;}
  bool operator<(Void) const {return false;}
};

template<> struct Serialise<Void>
{
  constexpr static const char* format_id ()
  {
    return "VOID";
  }

  template<typename Output>
  constexpr static
  inline void serialise (Output&&, const Void&)
  {
  }

  template<typename Input>
  constexpr static
  inline Void deserialise (const Input&)
  {
    return {};
  }

  constexpr static
  inline size_t serialised_size (const Void&)
  {
    return 0u;
  }

  constexpr static
  inline const std::uint8_t* next (const std::uint8_t *in)
  {
    return in;
  }

  constexpr static
  inline size_t estimated_max_size ()
  {
    return 0u;
  }
};

/**
 * Serialise specialisations for integral types
 */
template<> struct Serialise<int> :
  public detail::SerialiseIntegral<int>
{
};

template<> struct Serialise<std::int64_t> :
  public detail::SerialiseIntegral<std::int64_t>
{
};

template<> struct Serialise<std::uint64_t> :
  public detail::SerialiseIntegral<std::uint64_t>
{
};

} // namespace ordered_trie {

#endif // ORDERED_TRIE_PAYLOAD_TYPES_H
