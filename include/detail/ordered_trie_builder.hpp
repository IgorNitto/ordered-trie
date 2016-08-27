/**
 * @file  detail_ordered_trie_builder.hpp
 *
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 * 
 */

#ifndef ORDERED_TRIE_DETAIL_ORDERED_TRIE_BUILDER_HPP
#define ORDERED_TRIE_DETAIL_ORDERED_TRIE_BUILDER_HPP

#include "ordered_trie_builtin_serialise.hpp"
#include "ordered_trie_node.hpp"
#include "../ordered_trie_serialise.hpp"

#include <boost/optional.hpp>
#include <boost/range.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/utility/string_ref.hpp>

#include <tuple>
#include <unordered_map>
#include <vector>

namespace ordered_trie {
namespace detail {

/**
 * Auxiliary class defining a recursive serialisation process
 */
template<typename T = Void>
class MakeTrie
{
public:
  using metadata_type = T;

  /**
   * Make empty trie
   */
  MakeTrie () = default;

  /**
   * Make leaf node
   */
  MakeTrie (std::string label,
	    std::size_t rank,
	    T           metadata);

  /**
   * Make internal node
   */
  MakeTrie (std::string           label,
	    std::vector<MakeTrie> children);
  
  /**
   * Make root from children nodes
   */
  explicit MakeTrie (std::vector<MakeTrie> children,
		     std::vector<std::uint8_t> header = {});

  /**
   * Minimum scores associated to any leave in the subtrie
   */
  std::uint64_t min_score () const;
  
  /**
   * Label attached to root of contained subtrie
   */
  const std::string &label () const;

  /**
   * Add children nodes
   */
  void add_children (std::vector<MakeTrie> siblings);

  /**
   * Extract subtrie data from this instance
   */
  std::vector<std::uint8_t> move_to_trie ();

private:
 
  void serialise_header (
    std::vector<std::uint8_t> &output,
    const size_t               children_offset) const;

  static void serialise_siblings (
    std::vector<std::uint8_t> &output,
    std::vector<MakeTrie<T>>  siblings,
    const std::uint64_t       base_rank = 0);

private:
  std::string m_label;
  std::uint64_t m_rank = 0;
  boost::optional<T> m_metadata;
  std::vector<std::uint8_t> m_subtree_serialised;
};

/**
 * Construct trie serialisation from given (suggestions, rank)
 * pairs ordered by increasing lexicographic order of suggestion.
 */
template<typename OrderedPairs>
auto make_serialised_ordered_trie (
  const OrderedPairs &suggestion_ranks)
  -> std::vector<std::uint8_t>;

/**
 * @overload transforming input score in each pair
 * by mean of the given input functor to score component.
 */
template<typename OrderedPairs,
         typename ScoreTransform>
auto make_serialised_ordered_trie (
  const OrderedPairs   &suggestion_ranks,
  const ScoreTransform &score_transform)
  -> std::vector<std::uint8_t>;

/**
 * Score serialisation
 */
template<typename FwdRange, typename Cmp>
auto serialise_scores (
  std::vector<std::uint8_t> &output,
  const FwdRange            &completions,
  const Cmp                 &scores_cmp);

/***********************************************************
 *
 * ordered_trie_builder.hpp - Inlined implementation       
 *
 ***********************************************************/


/***********************************************************/
template<typename T>
MakeTrie<T>::MakeTrie (std::string label,
		       std::size_t rank,
		       T           metadata)
  : m_label (std::move (label))
  , m_rank (rank)
  , m_metadata (std::move (metadata))
{
  if (label.size () >= Node<T>::max_label_size)
  {
    throw std::length_error ("Exceeded maximum label size");
  }
}

/***********************************************************/
template<typename T>
MakeTrie<T>::MakeTrie (std::string label,
		       std::vector<MakeTrie<T>> children)
  : m_label (std::move (label))
{
  if (label.size () >= Node<T>::max_label_size)
  {
    throw std::length_error ("Exceeded maximum label size");
  }

  if (!children.empty ())
  {
    add_children (std::move (children));
  }
}

/***********************************************************/
template<typename T>
MakeTrie<T>::MakeTrie (std::vector<MakeTrie<T>> siblings,
		       std::vector<std::uint8_t> serialised)
  : m_subtree_serialised (std::move (serialised))
{    
  /*
   * Append an auxiliary root node with base score and
   * children offset both set to 0
   */
  m_subtree_serialised.push_back (1 << BIT_IS_LEAF);

  if (!siblings.empty ())
  {
    add_children (std::move (siblings));
  }
}

/***********************************************************/
template<typename T>
std::uint64_t MakeTrie<T>::min_score () const {return m_rank;}

/***********************************************************/
template<typename T>
const std::string &MakeTrie<T>::label () const {return m_label;}

/***********************************************************/
template<typename T>
std::vector<std::uint8_t> MakeTrie<T>::move_to_trie ()
{
  return std::move (m_subtree_serialised);
}

/***********************************************************/
template<typename T>
void MakeTrie<T>::serialise_header (
  std::vector<std::uint8_t> &output,
  const size_t               children_offset) const
{
  serialise_node (output,
		  m_label,
		  m_rank,
		  children_offset,
		  m_metadata);
}

/***********************************************************/
template<typename T>
void MakeTrie<T>::serialise_siblings (
  std::vector<std::uint8_t> &output,
  std::vector<MakeTrie<T>>   siblings,
  const std::uint64_t        base_rank)
{
  /*
   * Estimate total space taken by the encoding
   */
  const auto estimated_encoding_size = [&]
  {
    auto result = siblings.size () *
                  Node<T>::max_encoding_size ();

    for (const auto &node : siblings)
    {
      result += node.m_subtree_serialised.size ();
    }

    return result;
  } ();

  /*
   * First part of serialisation consists of concatenation
   * of all node's header
   */
  const auto initial_size = output.size ();
  const auto first_node = std::begin (siblings);
	  auto prev_node  = first_node;
	  auto prev_rank  = first_node->m_rank;

  output.reserve (initial_size + estimated_encoding_size);

  for (auto this_node  = std::next (first_node);
	      this_node != std::end (siblings); 
	    ++this_node)
  {
    const auto children_offset =
	prev_node->m_subtree_serialised.size ();

    const auto current_rank = this_node->m_rank;

    if (current_rank < prev_rank)
    {
      throw std::logic_error (
	"Rank values not in increasing order");
    }
    
    this_node->m_rank -= prev_rank;

    this_node->serialise_header (
      output,
      children_offset);

    prev_node = this_node;
    prev_rank = current_rank;
  }

  /*
   * We can serialise the first children only after
   * all other siblings, as this requires to know
   * total size of their headers.
   */
  const size_t total_headers_size = output.size () - initial_size;

  /*
   * Append first node, then perform a rotate to move its encoding
   * before all the other siblings
   */
  first_node->m_rank -= base_rank;
  first_node->serialise_header (
    output,
    total_headers_size);

  const auto pivot = output.begin () + initial_size;

  std::rotate (
    pivot,
    pivot + total_headers_size,
    output.end ());

  /*
   * Second part of the serialisation consists of
   * concatenation of all sub-tries serialisation
   */
  for (auto &&node: siblings)
  {
    output.insert (
      output.end (),
	node.m_subtree_serialised.begin (),
	node.m_subtree_serialised.end ());

    node.m_subtree_serialised.clear ();
  }
}

/***********************************************************/
template<typename T> 
void MakeTrie<T>::add_children (
  std::vector<MakeTrie<T>> siblings)
{
  if (m_metadata)
  {
    throw std::logic_error (
      "Attempting to add children to a leaf");
  }
  
  const bool called_from_root =
    m_label.empty () && !m_subtree_serialised.empty ();

  if (siblings.empty ())
  {
    /* Nothing to do, children must be added later */
    return;
  }

  if (called_from_root)
  {
    if (m_subtree_serialised.back () != (1 << BIT_IS_LEAF))
    {
      throw std::logic_error (
        "Attempting to add children to non-leaf root node");
    }

    m_subtree_serialised.back () = 0;
  }

  /*
   * If there is a single children node, check possibility to
   * collapse label if their size can fit into single label
   */
  if ((siblings.size () == 1) && !called_from_root)
  {
    auto &child = siblings.front ();

    if ((child.m_label.size () + m_label.size ()) <
         Node<T>::max_label_size)
    {
	/* append label and copy metadata */

	m_label.insert (m_label.end (),
			child.m_label.begin (),
			child.m_label.end ());

	m_subtree_serialised = std::move (child.m_subtree_serialised);
	m_rank               = child.m_rank;
	m_metadata           = std::move (child.m_metadata);
	return;
    }
  }

  /*
   * Proceed appending serialisation of children.
   */
  boost::sort (siblings,
    [] (const MakeTrie<T> &lhs, const MakeTrie<T> &rhs)
    {
	return lhs.min_score () < rhs.min_score ();
    });

  m_rank = called_from_root ? 0 : siblings.begin ()->m_rank;

  serialise_siblings (
    m_subtree_serialised,
    siblings,
    m_rank);
}

/**********************************************************************/
template<typename FwdRange, typename Cmp>
auto serialise_scores (
  std::vector<std::uint8_t> &output,
  const FwdRange            &completions,
  const Cmp                 &scores_cmp)
{
  using Score = decltype ((*completions.begin ()).second);
  
  /*
   * Create sorted/uniqued sequence of scores
   */
  std::vector<Score> scores_sequence;
  scores_sequence.reserve (boost::size (completions));

  for (const auto &c: completions)
  {
    scores_sequence.push_back (c.second);
  }
  
  std::sort (scores_sequence.begin (),
	     scores_sequence.end (),
	     scores_cmp);

  scores_sequence.erase (
    std::unique (scores_sequence.begin (),
		 scores_sequence.end ()),
    scores_sequence.end ());

  /*
   * Map every score to its offset in the serialised scores sequence
   */
  std::unordered_map<Score, size_t> score_ranks;
  const auto estimated_size =
    Serialise<Score>::estimated_max_size () *
    scores_sequence.size ();
  
  output.reserve (output.size () + estimated_size);
  score_ranks.reserve (scores_sequence.size ());

  for (const auto &score : scores_sequence)
  {
    score_ranks[score] = output.size ();
    ordered_trie::serialise (output, score);
  }
  
  return score_ranks;
}

/*********************************************************************/
template<typename SuggestionsRange,
	 typename ScoresRange,
	 typename MetadataRange>
std::vector<std::uint8_t>
make_serialised_ordered_trie (
  const SuggestionsRange   &suggestions,
  const ScoresRange        &scores,
  const MetadataRange      &metadata)
{
  using MetadataType =
    typename boost::range_value<MetadataRange>::type;
  
  using BuilderNode = MakeTrie<MetadataType>;
  using TrieLevel   = std::vector<BuilderNode>;

  auto scores_it   = std::begin (scores);
  auto metadata_it = std::begin (metadata);

  std::vector<TrieLevel> levels;

  /*
   * Merge levels of the partially constructed trie
   * bottom up until there are only target_depth levels
   */
  const auto merge_levels =
    [&levels] (const size_t target_depth)
    {
      BOOST_ASSERT (target_depth >= 1);
      BOOST_ASSERT (target_depth <= levels.size ());

      while (levels.size () > target_depth)
      {
	auto &current_level = levels.back ();
	auto &father_level = levels[levels.size () - 2];
	BOOST_ASSERT (!current_level.empty ());
	BOOST_ASSERT (!father_level.empty ());
	
	father_level.back ().add_children (std::move (current_level));
	levels.pop_back ();
      }
    };
  
  if (!boost::empty (suggestions))
  {
    auto suggestion_prev = *std::begin (suggestions);
    
    for (const auto suggestion: suggestions)
    {
      if (scores_it == std::end (scores))
      {
	throw std::length_error (
	  "Scores and suggestions range of differing sizes");
      }

      if (metadata_it == std::end (metadata))
      {
	throw std::length_error (
	  "Metadata and suggestions range of differing sizes");
      }
    
      /*
       * Determing longest common prefix (lcp) length between
       * current and previous suggestion and merge all trie levels
       * after lcp.
       */
      size_t lcp_length = 0;
      if (!levels.empty ())
      {
	auto first_it = suggestion.begin ();
	auto second_it = suggestion_prev.begin ();
	
	while (first_it != suggestion.end () &&
	       second_it != suggestion_prev.end () &&
	       *first_it == *second_it)
	{
	  ++first_it;
	  ++second_it;
	  ++lcp_length;
	}

	merge_levels (lcp_length + 1);
      }

      /*
       * Extend currently built trie adding one node per character
       * plus one dummy leaf containing metadata and score 
       */
      levels.resize (suggestion.size () + 1);
      
      for (auto idx = lcp_length; idx < suggestion.size (); ++idx)
      {
	const auto c = suggestion[idx];
	levels[idx].push_back ({std::string (1, c), {}});
      }

      levels.back ().push_back ({"", *scores_it, *metadata_it});

      BOOST_ASSERT (std::all_of (levels.begin (),
				 levels.end (),
				 [] (const auto &x) {return !x.empty ();}));
      
      suggestion_prev = suggestion;
      ++metadata_it;
      ++scores_it;
    }      
  }
  else
  {
    return BuilderNode {{}}.move_to_trie ();
  }
  
  if (scores_it != std::end (scores))
  {
    throw std::length_error (
      "Scores and suggestions range of differing sizes");
  }

  if (metadata_it != std::end (metadata))
  {
    throw std::length_error (
      "Metadata and suggestions range of differing sizes");
  }

  /*
   * Complete trie construction by merging remaining levels
   */
  merge_levels (1);
  return BuilderNode {std::move (levels.back ())}.move_to_trie ();
}

/*************************************************************/

template<typename OrderedPairs,
	 typename ScoreTransform>
std::vector<std::uint8_t>
make_serialised_ordered_trie (const OrderedPairs   &completions,
			      const ScoreTransform &score_transform)
{
  using boost::adaptors::transformed;

  /* 
   * Append trie serialisation
   */
  const auto suggestions_range = completions |
    transformed ([] (const auto &t)
    {
      return std::get<0> (t);
    });

  const auto scores_range = completions | 
    transformed ([&score_transform] (const auto &t)
    {
      return score_transform (std::get<1> (t));
    });

  const auto metadata_range = completions |
    transformed ([] (const auto &t)
    {
      return Void {};
    });

  return make_serialised_ordered_trie (
    suggestions_range,
    scores_range,
    metadata_range);				
}

/*************************************************************/
    
template<typename OrderedPairs>
auto make_serialised_ordered_trie (
  const OrderedPairs &suggestion_ranks)
  -> std::vector<std::uint8_t>
{ 
  return make_serialised_ordered_trie (
    suggestion_ranks,
    [] (const auto &score)
    {
      return score;
    });
}

} // namespace detail {
} // namespace ordered_trie {

#endif
