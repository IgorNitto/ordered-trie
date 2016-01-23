#ifndef ORDERED_TRIE_DETAIL_CONCEPTS_H
#define ORDERED_TRIE_DETAIL_CONCEPTS_H

/**
 * @file detail_concepts.h
 */

#include "detail_trie_impl.h"

namespace ordered_trie { namespace detail {

/**
 * Represent location of a single char in the trie,
 * reachable by matching a given string starting from
 * the root node.
 */
class Locus
{
public:

  bool match (char) const;

  const Node &base () const;

  size_t label_offset () const;

private:
  
};

/**
 * Represent a root to internal node path
 */
class Path
{
public:

  class SiblingsRange;
  class PathRange;

  explicit Path (const Node &);

  SiblingsRange ordered_children () const;

  PathRange enumerate_from (const Path&) const
 
  size_t score () const;

  const Node &base () const;

private:
};

class MakeTrie
{
  BuildTrie ()
    :
  {};

  
}

};

}} // namespace ordered_trie { namespace detail {

#endif
