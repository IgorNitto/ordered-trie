#ifndef DETAIL_ORDERED_TRIE_VARINT_HPP
#define DETAIL_ORDERED_TRIE_VARINT_HPP

/**
 * @file  integral_encoding.h
 * @brief Variable-length integer encoding functions
 */

#include "ordered_trie_builtin_serialise.hpp"

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
struct RankEncoder
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
    UINT64 = 3
  };

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
    using ordered_trie::serialise;
    
    wordsize_t byte_size;

    if (in == 0u)
    {
      return wordsize_t::EMPTY;
    }
    else if (in <= std::numeric_limits<std::uint8_t>::max ())
    {
      byte_size = wordsize_t::UINT8;
      serialise (out, static_cast<std::uint8_t> (in));
    }
    else if (in <= std::numeric_limits<std::uint16_t>::max ())
    {
      byte_size = wordsize_t::UINT16;
      serialise (out, static_cast<std::uint16_t> (in));
    }
    else
    {
      byte_size = wordsize_t::UINT64;
      auto higher = (in >> 31);

      serialise (
        out,
        static_cast<std::uint32_t> (((higher > 0) << 31) | in));

      while (higher)
      {
	const auto remainder = higher >> 7;

	out.push_back (static_cast<std::uint8_t> (higher)
		       | ((remainder > 0) << 7));

	higher = remainder;
      }
    }

    return byte_size;
  }

  /**
   * Read memory word of size @p byte_size from input address
   */
  inline static std::uint64_t
  deserialise (const std::uint8_t *in, wordsize_t byte_size)
  {
    using ordered_trie::deserialise;
        
    switch (byte_size)
    {
      case EMPTY:  return 0;
      case UINT8:  return deserialise<std::uint8_t> (in);
      case UINT16: return deserialise<std::uint16_t> (in);
      case UINT64:
      {
	std::uint64_t result = deserialise<std::uint32_t> (in);
	auto *extent_ptr = Serialise<std::uint32_t>::skip (in);

	size_t bit_offset = 31;
	auto highest_bit = result & (1 << 31);
	result &= (~(1 << 31));

	while (highest_bit)
	{
	  const std::uint64_t next_byte = *extent_ptr++;
	  highest_bit = next_byte & (1 << 7);

	  result |= (next_byte & (~(1 << 7))) << bit_offset;

	  bit_offset += 7;
	}

	return result;
      }
    }

    throw std::logic_error ("Invalid codeword");
  }

  constexpr static size_t max_codeword_size ()
  {
    /*
     * Longest codeword consists of a 32-bits integer
     * followed by the byte-aligned encoding of a
     * 32-bits integer
     */
    return sizeof (std::uint32_t) +
           (sizeof (std::uint32_t) * 8 + 6) / 7; 
  }

  static const std::uint8_t* skip (const std::uint8_t *in,
				   wordsize_t codeword_size)
  {
    using ordered_trie::deserialise;
    
    switch (codeword_size)
    {
      case EMPTY:  return in;
      case UINT8:  return in + sizeof (std::uint8_t);
      case UINT16: return in + sizeof (std::uint16_t);
      case UINT64:
      {
	in += sizeof (std::uint32_t) - 1;

	while (*in & (1 << 7))
	{
	  ++in;
	}

	return in + 1;	
      }
    }

    throw std::runtime_error ("Unexpected codeword");
  }
};

/**
 * An integer encoding is currently used for internal tree pointers.
 * Note: not portable across platforms with different endianness.
 */
struct OffsetEncoder
{
  static_assert (std::is_same<unsigned char, std::uint8_t>::value,
		 "Require char type of exactly 8 bits");

  /* Enumerate serialisable integer types */
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
    using ordered_trie::deserialise;
    
    switch (byte_size)
    {
      case EMPTY:  return 0;
      case UINT8:  return deserialise<std::uint8_t> (in);
      case UINT16: return deserialise<std::uint16_t> (in);
      case UINT64: return deserialise<std::uint64_t> (in);
    }

    throw std::logic_error ("Invalid codeword");
  }

  /**
   * Write encoding of @p in at address @p out, returning the
   * actual encoding size.
   */
  inline static wordsize_t
  serialise (std::vector<std::uint8_t> &out, std::uint64_t in)
  {
    using ordered_trie::serialise;
    
    if (in == 0u)
    {
      return wordsize_t::EMPTY;
    }
    else if (in <= std::numeric_limits<std::uint8_t>::max ())
    {
      serialise (out, static_cast<std::uint8_t> (in));
      return wordsize_t::UINT8;
    }
    else if (in <= std::numeric_limits<std::uint16_t>::max ())
    {
      serialise (out, static_cast<std::uint16_t> (in));

      return wordsize_t::UINT16;
    }
    
    serialise (out, static_cast<std::uint64_t> (in));
    return wordsize_t::UINT64;      
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

  static const std::uint8_t* skip (const std::uint8_t *in,
				   wordsize_t codeword)
  {
    return in + codeword_size (codeword);
  }
};

}} // namespace ordered_trie { namespace detail {

#endif
