/**
 * @file  detail_make_ordered_trie.h
 * @brief Trie serialisation algorithm
 */

#ifndef ORDERED_TRIE_DETAIL_MAKE_ORDERED_TRIE_H
#define ORDERED_TRIE_DETAIL_MAKE_ORDERED_TRIE_H

#include "detail_trie_impl.h"

#include <boost/range.hpp>

namespace ordered_trie { namespace detail {

template<typename SuggestionsRange,
	 typename ScoresRange,
	 typename MetaDataRange>
auto make_ordered_trie (const SuggestionsRange &suggestions,
			const ScoresRange      &scores,
			const MetaDataRange    &metadata)
  -> TrieImpl<typename boost::range_value<MetaDataRange>::type>;

}} // namespace ordered_trie { namespace detail {

#include "detail_make_ordered_trie.inl"

#endif
