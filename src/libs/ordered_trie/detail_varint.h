/**
 * @file  integral_encoding.h
 * @brief Variable-length integer encoding functions
 */

#ifndef ORDERED_TRIE_INTEGRAL_ENCODING_H
#define ORDERED_TRIE_INTEGRAL_ENCODING_H

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <vector>

namespace ordered_trie { namespace detail {

/**
 * Simple variable length encoder for 32-bit unsigned integers
 */
struct VarInt32
{
  static_assert (std::is_same<unsigned char, std::uint8_t>::value,
		 "Require char type of exactly 8 bits");

  /**
   * Codeword sizes 
   */
  enum wordsize_t : std::uint8_t
  {
    EMPTY  = 0,
    UINT8  = 1,
    UINT16 = 2,
    UINT32 = 3
  };

  /**
   * Read memory word of size @p byte_size from input address
   */
  inline static std::uint64_t
  deserialise (const std::uint8_t *in, wordsize_t byte_size)
  {
    std::uint64_t result = 0;

    const std::size_t length = codeword_size (byte_size);

    /* Pointer aliasing with char is allowed */

    auto *dest = reinterpret_cast<unsigned char*> (&result);
    memcpy (dest, in, length);

    return result;
  }

  /**
   * Serialise @p in at end of vector @p out 
   * returning the actual encoding size.
   *
   * @throws logic_error if input integer exceed 32 bits
   *         boundaries.
   */
  inline static wordsize_t
  serialise (std::vector<std::uint8_t> &out,
	     const std::uint64_t        in)
  {
    wordsize_t byte_size;

    const auto *in_ptr = reinterpret_cast<const char*> (&in);

    if (in == 0u)
    {
      return wordsize_t::EMPTY;
    }
    else if (in <= std::numeric_limits<std::uint8_t>::max ())
    {
      byte_size = wordsize_t::UINT8;
      std::copy (in_ptr, in_ptr + 1, back_inserter (out));
    }
    else if (in <= std::numeric_limits<std::uint16_t>::max ())
    {
      byte_size = wordsize_t::UINT16;
      std::copy (in_ptr, in_ptr + 2, back_inserter (out));
    }
    else if (in <= std::numeric_limits<std::uint32_t>::max ())
    {
      byte_size = wordsize_t::UINT32;
      std::copy (in_ptr, in_ptr + 4, back_inserter (out));
    }
    else
    {
      throw std::logic_error ("Input value exceeding 32 bits boundary");
    }

    return byte_size;
  }

  constexpr static size_t max_codeword_size ()
  {
    return sizeof(std::uint32_t);
  }

  inline static size_t codeword_size (wordsize_t in)
  {
    switch (in)
    {
      case EMPTY:  return 0u;
      case UINT8:  return sizeof (std::uint8_t);
      case UINT16: return sizeof (std::uint16_t);
      case UINT32: return sizeof (std::uint32_t);
    }

    throw std::runtime_error ("Unexpected codeword");
  }
};

/**
 * Simple variable length encoder for 64-bit unsigned integers
 */
struct VarInt64
{
  static_assert (std::is_same<unsigned char, std::uint8_t>::value,
		 "Require char type of exactly 8 bits");

  /* Enumerate serialisable integer types  */
  enum wordsize_t : std::uint8_t
  {
    EMPTY  = 0,
    UINT8  = 1,
    UINT16 = 2,
    UINT64 = 3
  };

  /**
   * Read memory word of size @p byte_size from input address
   */
  inline static std::uint64_t 
  deserialise (const std::uint8_t *in, wordsize_t byte_size)
  {
    std::uint64_t result = 0u;

    const std::size_t length = codeword_size (byte_size);

    /* Pointer aliasing with char is allowed */

    auto *dest = reinterpret_cast<char*> (&result);
    memcpy (dest, in, length);

    return result;
  }

  /**
   * Write encoding of @p in at address @p out,
   * returning the actual encoding size.
   */
  inline static wordsize_t
  serialise (std::vector<std::uint8_t> &out, std::uint64_t in)
  {
    wordsize_t byte_size;

    const auto *in_ptr = reinterpret_cast<const char*> (&in);

    if (in == 0u)
    {
      return wordsize_t::EMPTY;
    }
    else if (in <= std::numeric_limits<std::uint8_t>::max ())
    {
      byte_size = wordsize_t::UINT8;
      std::copy (in_ptr, in_ptr + 1, back_inserter (out));
    }
    else if (in <= std::numeric_limits<std::uint16_t>::max ())
    {
      byte_size = wordsize_t::UINT16;
      std::copy (in_ptr, in_ptr + 2, back_inserter (out));
    }
    else
    {
      byte_size = wordsize_t::UINT64;
      std::copy (in_ptr, in_ptr + 8, back_inserter (out));
    }

    return byte_size;
  }

  constexpr static size_t max_codeword_size ()
  {
    return sizeof (std::uint64_t);
  }

  inline static size_t codeword_size (wordsize_t in)
  {
    switch (in)
    {
      case EMPTY:  return 0u;
      case UINT8:  return sizeof (std::uint8_t);
      case UINT16: return sizeof (std::uint16_t);
      case UINT64: return sizeof (std::uint64_t);
    };

    throw std::runtime_error ("Unrecognized codeword");
  };
};

}} // namespace ordered_trie { namespace detail {

#endif // ORDERED_TRIE_INTEGRAL_ENCODING_H
