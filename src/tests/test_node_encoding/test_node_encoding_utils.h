#include "detail_make_ordered_trie.h"
#include "detail_trie_impl.h"
#include "detail_payload_types.h"

#include <boost/test/unit_test.hpp>
#include <boost/range.hpp>

#include <cassert>
#include <iostream>
#include <iterator>
#include <tuple>

using namespace ordered_trie;
using namespace ordered_trie::detail;

namespace ordered_trie { namespace test_utils {

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

  auto as_tuple () const
  {
    return std::make_tuple (text, rank, metadata);
  }
  
  bool operator== (const Completion &rhs) const
  {
    return as_tuple () == rhs.as_tuple ();
  }

  bool operator!= (const Completion &rhs) const
  {
    return as_tuple () != rhs.as_tuple ();
  }

  bool operator< (const Completion &rhs) const
  {
    return as_tuple () < rhs.as_tuple ();
  }

  std::string   text;
  std::uint64_t rank;
  T             metadata;
};

std::ostream& operator<< (std::ostream &os,
			  const Completion<Void> c)
{
  return (os << c.text << " " << c.rank << " " << std::endl);
}

/***********************************************************/
/*
 * Helper function to adapt make_ordered_trie working on a
 * sequence of Completion object
 */
template<typename T>
auto make_from_completions (const std::vector<Completion<T>> &in)
  -> TrieImpl<T>
{
  auto sorted = in;
  std::sort (sorted.begin (), sorted.end ());

  std::vector<std::string> suggestions;
  std::vector<std::uint64_t> scores;
  std::vector<T> metadata;

  for (const auto &v : sorted)
  {
    suggestions.push_back (v.text);
    scores.push_back (v.rank);
    metadata.push_back (v.metadata);
  }

  return detail::make_ordered_trie (suggestions,
				    scores,
				    metadata);
}

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
auto dfs_visit (const TrieImpl<T> &trie)
  -> std::vector<Completion<T>>
{
  std::vector<Completion<T>> result;
  dfs_visit_children (back_inserter (result),
                      trie.root ().children ());
  return result;
}

}}
