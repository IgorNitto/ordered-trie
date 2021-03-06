/**
 * @file  ordered_trie_serialise.hpp
 * @brief Serialisation scheme
 *
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 *
 */

#ifndef ORDERED_TRIE_SERIALISE_HPP
#define ORDERED_TRIE_SERIALISE_HPP

#include <cstddef>
#include <cstdint>
#include <vector>

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
	     T &&input);

  /**
   * Deserialise object starting at given input address
   */
  static inline auto
  deserialise (const std::uint8_t *start) -> T;

  /**
   * Advance pointer to first byte past end of encoding
   */
  static inline auto
  skip (const std::uint8_t *start) -> const std::uint8_t*;

  /**
   * Expected maximum encoding length of a generic metadata
   */
  static inline auto constexpr
  estimated_max_size () -> size_t;
};

} // namespace ordered_trie {

#endif

