/**
 * @file  serialise.h
 * @brief Serialisation scheme for metadata
 */

#ifndef ORDERED_TRIE_SERIALISE_H
#define ORDERED_TRIE_SERIALISE_H

#include <string>

#include <boost/utility/string_ref.hpp>

namespace ordered_trie {

/** 
 * Payload data serialisation/deserialisation API.
 * This has to be specialised for any user defined
 * metadata type.
 */
template<typename T> struct Serialise
{
  /**
   * Unique version name associated to this serialisation format
   */
  constexpr static char* format_id ();

  /**
   * Append serialisation to output
   */
  static inline void
  serialise (std::vector<std::uint8_t> &output,
	     const T &input);

  /**
   * Deserialise object starting at given input address
   */
  static inline auto
  deserialise (const std::uint8_t *start) -> T;

  /**
   * Advance pointer to first byte past end of encoding
   */
  static inline auto
  next (const std::uint8_t *start) -> const std::uint8_t*;

  /**
   * Upper bound on encoding size of input
   */
  static inline auto
  serialised_size (const T&) -> size_t;

  /**
   * Expected maximum encoding length of a generic metadata
   */
  static inline auto constexpr
  estimated_max_size () -> size_t;
};

} // namespace ordered_trie {

#endif // ORDERED_TRIE_SERIALISE_H

