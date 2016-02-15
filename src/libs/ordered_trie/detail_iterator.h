#ifndef ORDERED_TRIE_DETAIL_ITERATOR_H
#define ORDERED_TRIE_DETAIL_ITERATOR_H

/**
 * @file  detail_iterator.h
 * @brief Some helper iterators
 */

#include <boost/iterator.hpp>
#include <boost/range.hpp>

namespace ordered_trie {

/**
 * Forward iterator ranging over a sequence of siblings node.
 */
template<typename Node>
class SiblingsIterator:
  public boost::iterator_facade<
    /* CRTP       */ SiblingsIterator<Node>,
    /* value_type */ Node,
    /* category   */ boost::forward_traversal_tag,
    /* reference  */ Node
   >
{
public:
  /**
   * Ctor from pointer to node
   */
  explicit SiblingsIterator (const Node &node)
    : m_pointed (node)
  {
  }

private:
  friend class boost::iterator_core_access;

  inline Node dereference () const
  {
    return m_pointed;
  }

  inline bool equal (const SiblingsIterator<Node> &other) const
  {
    return (m_pointed == other.m_pointed);
  }

  inline void increment ()
  {
    m_pointed.advance_to_sibling ();
  }

private:
  Node m_pointed;
};

#if 0

/**
 * Create range of siblings node from pointer to
 * beginning of first sibling's encoding
 */
template<typename Parameters>
inline auto make_siblings_range (const std::uint8_t *base)
  -> SiblingsRange<Parameters>;

/**
 * Iterate over set of leaves contained in subtrie
 * rooted at given node in order of increasing score
 * and return a completion object for each of them
 */
template<typename Parameters>
class CompletionsIterator
  : public boost::iterator_facade< 
     /* CRTP       */ CompletionsIterator<Parameters>,
     /* value_type */ Completion<Parameters>,
     /* category   */ boost::forward_traversal_tag,
     /* reference  */ Completion<Parameters>  
    >
{
public:
  
  explicit CompletionsIterator (const std::uint8_t *in);

private:
  friend class boost::core_iterator_access;

  inline Completions<Parameters> dereference () const;  
  inline bool equal (const CompletionsIterator<Parameters>&) const;
  inline void increment ();

private:
  struct SearchPoint;
  std::priority_queue<SearchPoint> m_frontier;
};

#endif

} // namespace ordered_trie {

#endif
