/**
 * @file  test_serialise_trie.cpp
 * @brief Testing trie serialisation
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_serialise_trie

#include "detail_make_ordered_trie.h"
#include "detail_encoding.h"
#include "serialise.h"
#include "traversal.h"

#include <boost/test/unit_test.hpp>
#include <boost/variant.hpp>

#include <string>
#include <cassert>
#include <iostream>

using namespace ordered_trie;

/******************************************************************************/

template<typename Encoding>
void print_child (std::ostream &os,
		  const std::uint8_t *subtree_ptr,
		  std::string   prefix    = {},
		  std::uint32_t base_rank = 0)
{
  const auto siblings =
    make_siblings_range<Encoding> (subtree_ptr);

  size_t children_offset = 0;

  for (auto node_it  = std::begin (siblings);
            node_it != std::end (siblings);
          ++node_it)
  {
    const auto node_view = *node_it;

    const auto suggestion = 
    [&] ()
    {   
      const auto label = node_view.label ();

      auto result = prefix;
      result.append (label.data (), label.size ());

      return result;
    } ();

    base_rank = node_view.rank ();
    children_offset = node_view.children_offset ();

    os << "Label: " << suggestion << " "
       << "Rank: " << base_rank << " "
       << "Child offset: " << children_offset << " "
       << "Is leaf: " << node_view.is_leaf () << " "
       << "size: "  << node_view.size () << " "
       << std::endl;
  }
}

/******************************************************************************/

template<typename SiblingsRange>
void dfs_print (std::ostream        &os,
		const SiblingsRange &siblings,
		std::string          prefix    = {},
		std::uint32_t        base_rank = 0)
{
  for (auto node_it  = std::begin (siblings);
            node_it != std::end (siblings);
          ++node_it)
  {
    const auto node_view = *node_it;

    const auto suggestion = 
    [&] ()
    {   
      const auto label = node_view.label ();

      auto result = prefix;
      result.append (label.data (), label.size ());

      return result;
    } ();

    base_rank += node_view.rank ();

    if (node_view.is_leaf ())
    {
      os << "Label: " << suggestion << " "
	 << "Rank: " << base_rank << std::endl;
    }
    else
    {
      os << "Internal node : " << node_view.label ()
	 << " size: " << node_view.size ()
	 << " offset : "<< node_view.children_offset ()
	 << std::endl;

      dfs_print (os,
		 node_it.children (),
		 suggestion,
		 base_rank);
    }
  }
}

/******************************************************************************/

template<typename Encoding>
void dfs_print (std::ostream &os,
		detail::ConcreteNode<Encoding> root)
{
  const auto base_rank = root.rank ();
  const std::string prefix (root.label ().begin (),
			    root.label ().end ());

  const auto subtree_data = root.move_to_vector ();

  if (subtree_data.empty ())
  {
    os << "Label: " << prefix    << " "
       << "Rank: "  << base_rank << std::endl;
  }
  else
  {
    const auto subtree_ptr = subtree_data.data ();

    const auto siblings_range =
      make_siblings_range<Encoding> (subtree_ptr);

    dfs_print (std::cerr, siblings_range, prefix, base_rank);
  }
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
	//	Node::Leaf ('a', 3, {}),
	  	 
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
    print_child<Encoding> (std::cerr, root.subtree_data ());
    print_child<Encoding> (std::cerr, root.subtree_data () + 7 + 6);
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

#if 0

BOOST_AUTO_TEST_CASE (test_serialise_trie_2)
{
  {
    std::cerr << "[IGNORE] Test trie serialisation " << std::endl;

    const std::vector<std::string> keys    = {"a"};
    const std::vector<std::uint32_t> ranks = {0};

    const auto trie_encoding =
      serialise_trie<NodeSerialiser<Node>> (keys, ranks);

    dfs_print (std::cerr,
	       make_siblings_range<Node> (
		reinterpret_cast<const std::uint8_t*> (trie_encoding.c_str ()))
              );
  }

  {
    std::cerr << "[IGNORE] Test trie serialisation " << std::endl;

    const std::vector<std::string> keys =
      {"aaa", "bbb", "ccc"};

    const std::vector<std::uint32_t> ranks =
      {3, 2, 1};

    const auto trie_encoding =
      serialise_trie<NodeSerialiser<Node>> (keys, ranks);

    const auto siblings_range = make_siblings_range<Node> (
      reinterpret_cast<const std::uint8_t*> (trie_encoding.c_str ())
    );

    dfs_print (std::cerr, siblings_range);
  }

  {
    std::cerr << "[IGNORE] Test trie serialisation " << std::endl;

    const std::vector<std::string> keys =
      {"a", "aa", "aaa", "aaaa", "aaaaa", "aaaaaaaaaaaaa"};

    const std::vector<std::uint32_t> ranks =
      {3, 2, 1, 0, 4, 7};

    const auto trie_encoding =
      serialise_trie<NodeSerialiser<Node>> (keys, ranks);

    const auto siblings_range = make_siblings_range<Node> (
      reinterpret_cast<const std::uint8_t*> (trie_encoding.c_str ())
    );

    dfs_print (std::cerr, siblings_range);
  }

  {
    std::cerr << "[IGNORE] Test trie serialisation " << std::endl;

    const std::vector<std::string> keys =
      {"a", "aa", "aaa", "aaaa", "aaaaa", "aaaaaaaaaaaaa",
                         "aaab", "aaaab", "aaaaaaab"};

    const std::vector<std::uint32_t> ranks =
      {3, 2, 1, 0, 4, 7, 8, 9, 10};

    const auto trie_encoding =
      serialise_trie<NodeSerialiser<Node>> (keys, ranks);

    const auto siblings_range = make_siblings_range<Node> (
      reinterpret_cast<const std::uint8_t*> (trie_encoding.c_str ())
    );

    dfs_print (std::cerr, siblings_range);
  }

  {
    std::cerr << "[IGNORE] Test trie serialisation " << std::endl;

    const std::vector<std::string> keys =
      {"a",   "aa",  "aaa", "aab",
       "aac", "aad", "aae", "aaf"};

    const std::vector<std::uint32_t> ranks =
      {3, 2, 1, 0, 7, 6, 5, 4, 3};

    const auto trie_encoding =
      serialise_trie<NodeSerialiser<Node>> (keys, ranks);

    const auto siblings_range = make_siblings_range<Node> (
      reinterpret_cast<const std::uint8_t*> (trie_encoding.c_str ())
    );

    dfs_print (std::cerr, siblings_range);
  }
}

#endif
