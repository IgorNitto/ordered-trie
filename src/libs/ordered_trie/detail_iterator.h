#ifndef ORDERED_TRIE_DETAIL_ITERATOR_H
#define ORDERED_TRIE_DETAIL_ITERATOR_H

/**
 * @file  detail_iterator.h
 * @brief Auxiliary iterators
 */

#include <boost/iterator.hpp>
#include <boost/range.hpp>

#include <queue>
#include <tuple>

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
  explicit SiblingsIterator (const Node &);

private:
  friend class boost::iterator_core_access;

  inline Node dereference () const;
  inline bool equal (const SiblingsIterator<Node>&) const;
  inline void increment ();

private:
  Node m_pointed;
};
 
/**
 * Iterate over set of leaves contained in subtrie
 * rooted at given node in order of increasing score
 * and return a completion object for each of them
 */
template<typename Node>
class OrderedLeavesIterator
  : public boost::iterator_facade< 
     /* CRTP       */ OrderedLeavesIterator<Node>,
     /* value_type */ Node,
     /* category   */ boost::forward_traversal_tag,
     /* reference  */ Node>
{
public:
  
  explicit OrderedLeavesIterator (const Node &root);

  static auto end (const Node &root)
    -> OrderedLeavesIterator<Node>;
  
private:

  friend class boost::iterator_core_access;

  inline Node dereference () const;  
  inline bool equal (const OrderedLeavesIterator<Node> &) const;
  inline void increment ();
  OrderedLeavesIterator () noexcept;
    
private:
  class SearchNode;
  inline void push_leftmost_path (SearchNode);
  inline void next_leaf ();
  
  Node m_root;
 
  std::priority_queue<
    SearchNode,
    std::vector<SearchNode>,
    std::greater<SearchNode>> m_frontier;
};

} // namespace ordered_trie {

#include "detail_iterator.inl"

#endif
