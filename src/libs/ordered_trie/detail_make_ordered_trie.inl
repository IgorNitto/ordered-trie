/**
 * @file  detail_make_ordered_trie.inl
 * @brief
 */

#include "detail_trie_impl.inl"

#include <boost/range.hpp>
#include <boost/range/size.hpp>

namespace ordered_trie { namespace detail {

template<typename SuggestionsRange,
	 typename ScoresRange,
	 typename MetaDataRange>
make_ordered_trie (const SuggestionsRange &suggestions,
		   const ScoresRange      &scores,
		   const MetaDataRange    &metadata)
  ->TrieImpl<typename boost::range_type<MetaDataRange>::type>
{
  using MetaDataType =
    typename boost::range_value<SuggestionsRange>::type;
  using SuggestionType =
    typename boost::range_value<SuggestionsRange>::type;
  using BuilderNode = MakeTrie<MetaDataType>;
  using TrieLevel   = std::vector<BuilderNode>;
    
  auto scores_it = std::begin (scores);
  auto metadata_it = std::begin (metadata);

  std::vector<TrieLevel> levels;

  /*
   * Auxiliary lambda to merge the bottm level of trie
   * until only it has depth target_depth
   */
  const auto merge_levels =
    [&levels] (const size_t target_depth)
    {
      BOOST_ASSERT (target_depth >= 1);
      BOOST_ASSERT (target_depth <= levels.size ());
            
      while (levels.size () > target_depth)
      {
	auto &current_level = level.back ();
	auto &father_level = level[level.size () - 2];

	father_level.back ().add_children (std::move (current_level));
	levels.pop_back ();
      }
    };
  
  if (!boost::empty (suggestions))
  {
    auto suggestion_prev = *std::begin (suggestions);
    
    for (const auto suggestion: suggestions)
    {
      BOOST_ASSERT_MSG (scores_it != std::end (scores),
			"Scores and suggestions range of differing"
			" sizes");

      BOOST_ASSERT_MSG (metadata_it != std::end (metadata),
			"Metadata and suggestions range of differing"
			" sizes");
      
      /*
       * Determing longest common prefix (lcp) length between
       * current and previous suggestion and merge all trie levels
       * after lcp.
       */
      size_t lcp_length = 0;
      if (!levels.empty ())
      {
	const auto mismatch_it = // NOTE: check behaviour on different lengths
	  std::match (suggestion.begin (), suggestion.end (),
		      suggestion_prev.begin());

	lcp_length =
	  std::distance (suggestion.begin (), mismatch_it);
	
	merge_levels (lcp_length + 1);
      }

      /*
       * Extend currently built trie adding one node per character
       * plus one dummy leaf containing metadata and score 
       */
      for (auto idx = lcp_length; idx < suggestion.size(); ++idx)
      {
	const auto c = suggestion[idx];
	levels.push_back ({MakeTrie<MetaDataType> {c, {}}});
      }

      levels.push_back ({
	MakeTrie<MetaDataType> {c, *scores_it, *metadata_it}});
			  
      suggestion_prev = suggestion;
      ++metadata_it;
      ++scores_it;
    }      
  }

  BOOST_ASSERT_MSG (scores_it == std::end (scores),
		    "Scores and suggestions range of differing"
		    " sizes");

  BOOST_ASSERT_MSG (metadata_it == std::end (metadata),
		    "Metadata and suggestions range of differing"
		    " sizes");

  /*
   * Complete trie construction by merging remaining levels
   */
  merge_levels (1);
  return BuilderNode::move_to_trie (
    BuilderNode {std::move (levels.back ())});
}

}} // namespace ordered_trie { namespace detail {
