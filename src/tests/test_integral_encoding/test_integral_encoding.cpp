/**
 * @file  test_integral_encoding.cpp
 * @brief Unit test for variable length encoding
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_integral_encoding

#include "detail_varint.h"

#include <boost/test/unit_test.hpp>

#include <cassert>
#include <iostream>

using namespace ordered_trie;

BOOST_AUTO_TEST_CASE (test_encoding_32)
{
  using detail::VarInt32;

  const std::vector<size_t> input =
  {
    0,
    0,
    0x1,
    0xF,
    0x100,
    0x101,
    0x1FF,
    0xFFFF,
    0x100000,
    0x10FFFFF,
    0xFFFFFFFF
  };

  const std::vector<VarInt32::wordsize_t> codewords =
  {
    VarInt32::EMPTY,
    VarInt32::EMPTY,
    VarInt32::UINT8,
    VarInt32::UINT8,
    VarInt32::UINT16,
    VarInt32::UINT16,
    VarInt32::UINT16,
    VarInt32::UINT16,
    VarInt32::UINT32,
    VarInt32::UINT32,
    VarInt32::UINT32
  };

  std::vector<std::uint8_t> out;

  for (size_t j=0; j < input.size (); ++j)
  {
    BOOST_CHECK_EQUAL (VarInt32::serialise (out, input[j]),
		       codewords [j]);
    std::cout << "[IGNORE] Writing " << input [j] << std::endl;
  }

  const auto *read_offset = out.data ();
  for (size_t j=0; j < codewords.size (); ++j)
  {

    const auto c = VarInt32::deserialise (read_offset, codewords [j]);
    read_offset += VarInt32::codeword_size (codewords [j]);
    std::cout << "[IGNORE] Reading " << c << std::endl;

    BOOST_CHECK_EQUAL (c, input [j]);
  }
  
  BOOST_CHECK_EQUAL (read_offset, out.data () + out.size ());
}

BOOST_AUTO_TEST_CASE (test_encoding_64)
{
  using detail::VarInt64;

  const std::vector<size_t> input =
  {
    0,
    0,
    0x1,
    0xF,
    0x100,
    0x101,
    0x1FF,
    0xFFFF,
    0x100000,
    0x10FFFFF,
    0xFFFFFFFF,
    0x100000000,
    0xFFFFFFFFFFFFFFFF
  };

  const std::vector<VarInt64::wordsize_t> codewords =
  {
    VarInt64::EMPTY,
    VarInt64::EMPTY,
    VarInt64::UINT8,
    VarInt64::UINT8,
    VarInt64::UINT16,
    VarInt64::UINT16,
    VarInt64::UINT16,
    VarInt64::UINT16,
    VarInt64::UINT64,
    VarInt64::UINT64,
    VarInt64::UINT64,
    VarInt64::UINT64,
    VarInt64::UINT64
  };

  std::vector<std::uint8_t> out;

  for (size_t j=0; j < input.size (); ++j)
  {
    BOOST_CHECK_EQUAL (VarInt64::serialise (out, input[j]),
		       codewords [j]);
    std::cout << "[IGNORE] Writing " << input [j] << std::endl;
  }

  auto read_offset = out.data ();
  for (size_t j=0; j < codewords.size (); ++j)
  {
    const auto c = VarInt64::deserialise (read_offset, codewords [j]);
    read_offset += VarInt64::codeword_size (codewords [j]);
    std::cout << "[IGNORE] Reading " << c << std::endl;

    BOOST_CHECK_EQUAL (c, input [j]);
  }

  BOOST_CHECK_EQUAL (read_offset, out.data () + out.size ());
}

