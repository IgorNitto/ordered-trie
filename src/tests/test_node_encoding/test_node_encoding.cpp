/**
 * @file  test_node_encoding.cpp
 * @brief Unit test for variable length encoding
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_node_encoding

#include "detail_encoding.h"
#include "detail_payload_types.h"

#include <boost/test/unit_test.hpp>

#include <cassert>
#include <iostream>

using namespace ordered_trie;

BOOST_AUTO_TEST_CASE (test_node_encoding)
{
  using Encoder = encoders::Default<Void>;

  std::vector<std::uint8_t> out;

  Encoder::serialise_internal (out, 1, 0, std::string {"blah"});
  Encoder::serialise_internal (out, 2, 1000, std::string {"mew"});
  Encoder::serialise_internal (out, 2, 1000, std::string {"zzz"});
  Encoder::serialise_leaf (out, 4, 100, std::string {"zzzz"}, {});

  std::cerr << "[IGNORE] Deserialise" << std::endl;

  auto base_ptr = &out [0];

  std::cerr << "[IGNORE] Output size " << out.size () << std::endl;
    
  for (size_t j=0; j < 4; ++j)
  {
    const auto view = Encoder::View {base_ptr};
    std::cerr << view.size () << std::endl;

    std::cerr << view.rank () << std::endl
	      << view.is_leaf () << std::endl
	      << view.children_offset () << std::endl
	      << view.label () << std::endl;

    base_ptr += view.size ();
  }

  BOOST_CHECK_EQUAL (base_ptr, out.data () + out.size ());
}
