#ifndef DETAIL_ORDERED_TRIE_BUILTIN_SERIALISE_HPP
#define DETAIL_ORDERED_TRIE_BUILTIN_SERIALISE_HPP

/**
 * @file  ordered_trie_builtin_serialise.hpp
 * @brief 
 */

#include "../ordered_trie_serialise.hpp"

#include <typeinfo>
#include <cstring>
#include <string>

namespace ordered_trie {
namespace detail {

template<typename T>
struct SerialiseArithmetic
{
  static_assert (std::is_arithmetic<T>::value, "Not integral");

  static inline void serialise (std::vector<std::uint8_t> &out,
				const T value)
  {
    auto size = out.size ();
    out.resize (out.size () + sizeof (T));
    memcpy (&out[size], &value, sizeof (T));
  }

  static const std::string& format_id ()
  {
    static const auto name =
      std::string {"FIXED_INT_"} + typeid (T).name ();

    return name;
  }
  
  static inline T deserialise (const std::uint8_t *in)
  {
    T result {};
    auto *dest = reinterpret_cast<unsigned char*> (&result);
    memcpy (dest, in, sizeof (T));
    return result;
  }

  static inline const std::uint8_t* skip (const std::uint8_t *in)
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
  inline const std::uint8_t* skip (const std::uint8_t *in)
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
 * Serialise specialisations for integral types and double
 */
template<>
struct Serialise<std::int8_t>
  : public detail::SerialiseArithmetic<std::int8_t> {};

template<>
struct Serialise<std::uint8_t>
  : public detail::SerialiseArithmetic<std::uint8_t> {};

template<>
struct Serialise<std::int16_t>
  : public detail::SerialiseArithmetic<std::int16_t> {};

template<>
struct Serialise<std::uint16_t>
  : public detail::SerialiseArithmetic<std::uint16_t> {};

template<>
struct Serialise<std::int32_t>
  : public detail::SerialiseArithmetic<std::int32_t> {};

template<>
struct Serialise<std::uint32_t>
  : public detail::SerialiseArithmetic<std::uint32_t> {};

template<>
struct Serialise<std::int64_t>
  : public detail::SerialiseArithmetic<std::int64_t> {};

template<>
struct Serialise<std::uint64_t>
  : public detail::SerialiseArithmetic<std::uint64_t> {};

template<>
struct Serialise<double>
  : public detail::SerialiseArithmetic<double> {};
 
/**
 * serialise/deserialise facade
 */
template<typename T>
void serialise (std::vector<std::uint8_t> &input,
		const T& value)
{
  Serialise<T>::serialise (input, value);
}
 
template<typename T>
T deserialise (const std::uint8_t *input)
{
  return Serialise<T>::deserialise (input);
}

} // namespace ordered_trie {

#endif
