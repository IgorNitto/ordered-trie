/**
 * @file  test_node_encoding.cpp
 * @brief Unit test for variable length encoding
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_node_encoding

#include "detail_make_ordered_trie.h"
#include "detail_trie_impl.h"
#include "detail_payload_types.h"
#include "test_node_encoding_utils.h"

#include <boost/test/unit_test.hpp>
#include <boost/range.hpp>

#include <cassert>
#include <iostream>
#include <iterator>
#include <tuple>

using namespace ordered_trie;
using namespace ordered_trie::detail;
using namespace ordered_trie::test_utils;

BOOST_AUTO_TEST_CASE (test_make_trie_1)
{
  {
    const auto trie =
      MakeTrie<Void> ({{"a", 0u, {}}}).move_to_trie ();

    const auto node = *(trie.root ().children ().begin ());
    BOOST_CHECK_EQUAL (node.label (), "a");
    BOOST_CHECK (node.is_leaf ());    
    BOOST_CHECK_EQUAL (node.rank (), 0u);
  }

  {
    const auto trie =
      make_from_completions<Void> ({{"a", 0u}});

    const auto node = *(trie.root ().children ().begin ());
    BOOST_CHECK_EQUAL (node.label (), "a");
    BOOST_CHECK (node.is_leaf ());    
    BOOST_CHECK_EQUAL (node.rank (), 0u);
  }
}

BOOST_AUTO_TEST_CASE (test_make_trie_2_simplified)
{
  const std::vector<Completion<Void>> suggestions =
  {
    {"a", 1u, {}},
    {"b", 20u, {}},
    {"c", 20u, {}},
    {"d", 30u, {}}
  };

  {
    const auto trie = make_from_completions (suggestions);
    BOOST_CHECK (dfs_visit (trie) == suggestions);
  }
}

BOOST_AUTO_TEST_CASE (test_make_trie_2)
{
    const std::vector<Completion<Void>> suggestions =
    {
      {"abbb", 1u, {}},
      {"b", 20u, {}},
      {"bcc", 20u, {}},
      {"aaaa", 30u, {}},
    };

    std::vector<MakeTrie<Void>> leaves;
    for (const auto &s : suggestions)
    {
      leaves.emplace_back (s.text, s.rank, s.metadata);
    }

    const auto trie = MakeTrie<Void> (leaves).move_to_trie ();
    BOOST_CHECK (dfs_visit (trie) == suggestions);
}

BOOST_AUTO_TEST_CASE (test_make_ordered_trie_2)
{
  const std::vector<Completion<Void>> suggestions =
  {
    {"abbb", 1u, {}},
    {"aaaa", 30u, {}},
    {"b", 20u, {}},
    {"bcc", 20u, {}},
  };

  const auto trie = make_from_completions (suggestions);
  const auto output = dfs_visit (trie);
}

BOOST_AUTO_TEST_CASE (test_make_trie_3)
{
  const std::vector<Completion<Void>> suggestions =
  {
    {"ac", 1u, {}},
    {"ab", 2u, {}},
    {"ba", 2u, {}},
    {"bd", 4u, {}},
    {"a",  3u, {}}
  };

  const auto trie = MakeTrie<Void> ({
    {"a", 3, {}},
    {"a", {
      {"c", 1, {}}, 
      {"b", 2, {}}}
    },
    {"b", {
      {"a", 2, {}},
      {"d", 4, {}}}
    }}
  ).move_to_trie ();

  BOOST_CHECK (dfs_visit (trie) == suggestions);
}

BOOST_AUTO_TEST_CASE (test_make_ordered_trie_3)
{
  const std::vector<Completion<Void>> suggestions =
  {
    {"ac", 1u, {}},
    {"ab", 2u, {}},
    {"a",  3u, {}},
    {"ba", 2u, {}},
    {"bd", 4u, {}},
  };

  const auto trie = make_from_completions (suggestions);
  const auto output = dfs_visit (trie);

  BOOST_CHECK_EQUAL (output.size (), suggestions.size ());

  BOOST_CHECK_EQUAL_COLLECTIONS (
    output.begin (), output.end (),
    suggestions.begin (), suggestions.end ());
}

BOOST_AUTO_TEST_CASE (test_make_trie_4)
{
  const auto trie = MakeTrie<Void> ({
    {"aa", {
       {"aa", {
	  {"aa", {
	      {"a", 10, {}}}}}}}}
  }).move_to_trie ();

  const auto root = trie.root ();
  const auto first = root.children ().begin ();

  BOOST_CHECK_EQUAL ((*first).label (), "aaaaaaa");
  BOOST_CHECK_EQUAL ((*first).rank (), 10u);
  BOOST_CHECK (std::next (first) == root.children ().end ());
}

BOOST_AUTO_TEST_CASE (test_ordered_leaves_iterator_1)
{
  {
    const std::vector<Completion<Void>> suggestions =
    {
      {"aa", 1u, {}},
      {"ab", 2u, {}},
      {"ba", 1u, {}},
      {"bb", 2u, {}}
    };

    const auto trie = make_from_completions (suggestions);
    const auto output = ordered_visit (trie);

    std::cerr << output.back ().data () - trie.root ().data () << std::endl;
    
    std::cerr << label (trie.root (), output.back ()) << std::endl;
    
    for (const auto node : output)
    {
      std::cerr << node.rank () << ","
		<< node.label () << ","
		<< label (trie.root (), node) << ","
		<< node.is_leaf () << std::endl;
    }
  }

  {
    std::cerr << std::endl;
    const std::vector<Completion<Void>> suggestions =
    {
      {"ac", 1u, {}},
      {"ab", 2u, {}},
      {"a",  3u, {}},
      {"ba", 2u, {}},
      {"bd", 4u, {}},
    };

    const auto trie = make_from_completions (suggestions);
    const auto output = ordered_visit (trie);

    for (const auto node : output)
    {
      std::cerr << node.rank () << ","
	        << node.label () << ","
		<< label (trie.root (), node) << std::endl;
    }
  }

  {
    std::cerr << std::endl;
    const std::vector<Completion<int>> suggestions =
    {
      {"a", 1u, 2},
    };

    const auto trie = make_from_completions (suggestions);
    const auto output = ordered_visit (trie);

    for (const auto node : output)
    {
      std::cerr << node.rank () << ","
		<< node.label () << ","
		<< label (trie.root (), node) << ","
	        << node.metadata () << std::endl;
    }

  }

  {
    const std::vector<Completion<int>> suggestions =
    {
      {"abbb", 1u, 1},
      {"b", 20u, 2},
      {"bcc", 20u, 3},
      {"aaaa", 30u, 4},
    };

    const auto trie = make_from_completions (suggestions);
    const auto output = ordered_visit (trie);

    for (const auto node : output)
    {
      std::cerr << node.rank () << ","
		<< node.label () << ","
		<< label (trie.root (), node) << ","
	        << node.metadata () << std::endl;
    }
  }
}
