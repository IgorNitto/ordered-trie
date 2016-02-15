/**
 * @file  test_node_encoding.cpp
 * @brief Unit test for variable length encoding
 */

#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_node_encoding

#include "detail_trie_impl.h"
#include "detail_payload_types.h"

#include <boost/test/unit_test.hpp>
#include <boost/range.hpp>

#include <cassert>
#include <iostream>
#include <iterator>
#include <tuple>

using namespace ordered_trie;

namespace
{

/*
 * @TODO: Move this into the library
 */
bool operator==(Void, Void) {return true;}

template<typename T>
struct Completion
{
  Completion (const std::string   &text_,
	      const std::uint64_t  rank_,
	      const T             &metadata_ = {})
    : text (text_)
    , rank (rank_)
    , metadata (metadata_)
  {}

  bool operator== (const Completion &rhs) const
  {
    return (text == rhs.text &&
	    rank == rhs.rank &&
	    metadata == rhs.metadata);
  }

  std::string   text;
  std::uint64_t rank;
  T             metadata;
};

/***********************************************************/
/**
 * Depth-first visit input trie and outputting the pair
 * (suggestion, score) when encounter a leaf.
 */

template<typename Node, typename OutIt>
void dfs_visit (OutIt, const Node&, std::string = {});

template<typename SiblingsRange, typename OutIt>
void dfs_visit_children (OutIt               out_it,
			 const SiblingsRange &siblings,
			 std::string          prefix = {})
{
  for (const auto node : siblings)
  {
    dfs_visit (out_it, node, prefix);
  }
}

template<typename Node, typename OutIt>
void dfs_visit (OutIt        out_it,
		const Node  &node,
		std::string  prefix)
{
  using metadata_type = typename Node::metadata_type;
  const auto label = node.label ();
  prefix.append (label.data (), label.size ());

  if (!node.is_leaf ())
  {
    dfs_visit_children (
      out_it, node.children (), prefix);
  }
  else
  {
    *out_it++ = Completion<metadata_type> (
       prefix,
       node.rank (),
       node.metadata ());
  }
}

template<typename T>
struct Trait
{
  using node_type = typename boost::range_value<T>::type;
  using metadata_type = typename node_type::metadata_type;
};

template<typename Siblings>
auto dfs_visit_children (const Siblings siblings)
  -> std::vector<Completion<typename Trait<Siblings>::metadata_type>>
{
  using metadata_t = typename Trait<Siblings>::metadata_type;
  std::vector<Completion<metadata_t>> result;
  dfs_visit_children (back_inserter (result), siblings);
  return result;
}

} // namespace {

using namespace detail::v0;

BOOST_AUTO_TEST_CASE (test_make_trie_1)
{
  const auto trie =
    MakeTrie<Void> ({{"a", 0u}}).move_to_trie ();

  const auto node = *(trie.root ().children ().begin ());
  BOOST_CHECK_EQUAL (node.label (), "a");
  BOOST_CHECK (node.is_leaf ());    
  BOOST_CHECK_EQUAL (node.rank (), 0u);
}

BOOST_AUTO_TEST_CASE (test_make_trie_2)
{
  const std::vector<Completion<Void>> suggestions =
  {
    {"abbb", 1u, {}},
    {"b", 20u, {}},
    {"bcc", 20u, {}},
    {"aaaa", 30u, {}}
  };

  std::vector<MakeTrie<Void>> leaves;
  for (const auto &s : suggestions)
  {
    leaves.emplace_back (s.text, s.rank, s.metadata);
  }

  const auto trie = MakeTrie<Void> (leaves).move_to_trie ();
  const auto root = trie.root ();
  const auto root_siblings = root.children ();

  BOOST_CHECK (dfs_visit_children (root_siblings) == suggestions);
}

BOOST_AUTO_TEST_CASE (test_make_trie_3)
{
  const std::vector<Completion<Void>> suggestions =
  {
    {"ac", 1u, {}},
    {"ab", 2u, {}},
    {"ba", 2u, {}},
    {"bd", 4u, {}},
    {"a", 3u, {}}
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

  const auto root = trie.root ();
  const auto root_siblings = root.children ();
  
  BOOST_CHECK (dfs_visit_children (root_siblings) ==
               suggestions);
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

BOOST_AUTO_TEST_CASE (test_make_trie_5)
{
  const auto trie = MakeTrie<Void> ({
    {"a", {
       {"", 10},
       {"", 11}
    }}
  }).move_to_trie ();

  const auto root = trie.root ();

  for (const auto c : dfs_visit_children (root.children ()))
  {
    std::cerr << c.text << " " << c.rank << std::endl;
  }
}
