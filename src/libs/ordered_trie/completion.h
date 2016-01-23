#ifndef ORDERED_TRIE_COMPLETIONS_H
#define ORDERED_TRIE_COMPLETIONS_H

/**
 * @file  completion.h
 */

#include "serialise.h"

#include <boost/string_ref.hpp>

namespace ordered_trie {

/**
 * Exposes suggestion string with associated
 * integral score and metadata.
 */
template<typename Parameters>
class Completion
{
public:

  using Suggestion = typename Parameters::suggestion_type;
  using MetaData   = typename Parameters::metadata_type;

  /**
   * Suggestion string
   */
  const Suggestion &suggestion () const;

  /**
   * Integral score associated to suggestion
   */
  std::uint64_t score () const;

  /**
   * Metadata object associated to this completion
   */
  const MetaData &metadata () const;

  /* Ctor, mostly for internal use */
  Completion (typename Parameters::NodeView leaf,
	      std::uint64_t     score,
	      boost::string_ref suggestion);

private:
  std::uint64_t m_score;
  Suggestion m_suggestion;  
  MetaData   m_metadata; 
};

} // namespace ordered_trie

#endif
