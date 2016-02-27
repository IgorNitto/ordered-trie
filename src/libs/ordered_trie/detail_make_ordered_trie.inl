/**
 * @file  detail_make_ordered_trie.inl
 * @brief
 */

#include <boost/range.hpp>
#include <boost/range/size.hpp>

namespace ordered_trie { namespace detail {

template<typename SuggestionsRange,
	 typename ScoresRange,
	 typename MetadataRange>
auto make_ordered_trie (const SuggestionsRange &suggestions,
			const ScoresRange      &scores,
			const MetadataRange    &metadata)
  ->TrieImpl<typename boost::range_value<MetadataRange>::type>
{
  using MetadataType =
    typename boost::range_value<MetadataRange>::type;
  using BuilderNode = MakeTrie<MetadataType>;
  using TrieLevel   = std::vector<BuilderNode>;
    
  auto scores_it = std::begin (scores);
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
  return BuilderNode {std::move (levels.back ())}.move_to_trie();
}

}} // namespace ordered_trie { namespace detail {
