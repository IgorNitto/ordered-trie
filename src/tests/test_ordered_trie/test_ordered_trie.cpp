#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ordered_trie

#include "detail_encoding.h"
#include "detail_make_ordered_trie.h"
#include "detail_varint.h"
#include "serialise.h"
#include "traversal.h"

#include <boost/test/unit_test.hpp>

#include <cassert>
#include <iostream>

using namespace ordered_trie;

/***********************************************************/

namespace
{

template<typename Node>
void dsf_print (std::ostream&, const Node&, std::string);

template<typename SiblingsRange>
void dfs_print_children (std::ostream        &os,
			 const SiblingsRange &siblings,
			 std::string          prefix = {})
{
  for (const auto node : siblings)
  {
    dsf_print (os, node, prefix);
  }
}

template<typename Node>
void dfs_print (std::ostream &os,
		const Node   &node,
		std::string   prefix = {})
{
  const auto label = node.label ();
  prefix.append (label.data (), label.size ());

  os << node.rank () << "\t" 
     << prefix << std::endl;

  if (!node.is_leaf ())
  {
    dfs_print_siblings (
      os, node.children (), prefix);
  }
}

} // namespace {

/***********************************************************/

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
  }

  const auto *read_offset = out.data ();
  for (size_t j=0; j < codewords.size (); ++j)
  {

    const auto c = VarInt32::deserialise (read_offset, codewords [j]);
    read_offset += VarInt32::codeword_size (codewords [j]);
    BOOST_CHECK_EQUAL (c, input [j]);
  }
  
  BOOST_CHECK_EQUAL (read_offset, out.data () + out.size ());
}

/***********************************************************/

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
  }

  auto read_offset = out.data ();
  for (size_t j=0; j < codewords.size (); ++j)
  {
    const auto c = VarInt64::deserialise (read_offset, codewords [j]);
    read_offset += VarInt64::codeword_size (codewords [j]);

    BOOST_CHECK_EQUAL (c, input [j]);
  }

  BOOST_CHECK_EQUAL (read_offset, out.data () + out.size ());
}

/***********************************************************/

BOOST_AUTO_TEST_CASE (test_node_encoding)
{
  using Encoder = encoders::Default<Void>;

  std::vector<std::uint8_t> out;

  Encoder::serialise_internal (out, 1, 0, std::string {"blah"});
  Encoder::serialise_internal (out, 2, 1000, std::string {"mew"});
  Encoder::serialise_internal (out, 2, 1000, std::string {"zzz"});
  Encoder::serialise_leaf (out, 4, 100, std::string {"zzzz"}, {});

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

/******************************************************************************/
template<typename Encoding>
void test_serialise_and_visit ()
{
  using namespace ordered_trie::detail;
  using Node = ConcreteNode<Encoding>;

  {
    std::cerr << "[IGNORE] Test trie encoding" << std::endl;

    auto root =
      Node::Internal ("",
      {
	Node::Leaf ('a', 0, {}),
      });

    dfs_print (std::cerr, std::move (root));
  }

  {
    std::cerr << "[IGNORE] Test trie encoding" << std::endl;

    auto root =
      Node::Internal ("",
      {
	Node::Leaf ('a', 2, {}), 
	Node::Leaf ('b', 1, {}),
	Node::Leaf ('c', 3, {})
      });

    dfs_print (std::cerr, std::move (root));
  }

  {
    std::cerr << "[IGNORE] Test trie encoding" << std::endl;

    auto root =
      Node::Internal ("",
      {
	Node::Leaf ('a', 3, {}),
	  	 
 	Node::Internal ("a",
	{
	  Node::Leaf ('c', 1, {}), 
	  Node::Leaf ('b', 2, {}) 
	}),
	  
	Node::Internal ("b",
	{
	  Node::Leaf ('a', 2, {}),
	  Node::Leaf ('d', 4, {}) 
	})

      });

    dfs_print (std::cerr, root);
  }

  {
    std::cerr << "[IGNORE] Test trie encoding" << std::endl;

    auto root =
      Node::Internal ("",
      {
	Node::Internal ("aaa",
	{
	  Node::Internal ("aaa",
	  {
	    Node::Internal ("aaa",
	    {
	      Node::Leaf ('a', 10, {})
	    })
	  })
	})
      });

    dfs_print (std::cerr, std::move (root));
  }
}

/******************************************************************************/

BOOST_AUTO_TEST_CASE (test_serialise_children)
{
  test_serialise_and_visit<encoders::Default<Void>> ();
}
