/**
 * @file  detail_payload_types.h
 * @brief Serialisation scheme for metadata
 */

#ifndef ORDERED_TRIE_DETAIL_PAYLOAD_TYPES_H
#define ORDERED_TRIE_DETAIL_PAYLOAD_TYPES_H

#include "serialise.h"

namespace ordered_trie {

/**
 * Define empty metadata
 */
struct Void
{
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

} // namespace ordered_trie {

#endif // ORDERED_TRIE_PAYLOAD_TYPES_H
