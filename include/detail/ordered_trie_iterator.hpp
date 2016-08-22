#ifndef DETAIL_ORDERED_TRIE_ITERATOR_HPP
#define DETAIL_ORDERED_TRIE_ITERATOR_HPP

/**
 * @file  detail_iterator.h
 * @brief Auxiliary logic to build iterators and
 *        visit sequence of nodes.
 */

#include <boost/iterator.hpp>
#include <boost/range.hpp>

#include <queue>
#include <stack>
#include <tuple>

namespace ordered_trie {
namespace detail {

/**
 * Iterate over a sequence of children nodes
 *
 * Note: this is not standard conformant
 */
template<typename Node>
class SiblingsIterator
{
public:

  /**
   * Default ctor
   */
  explicit SiblingsIterator ()
    : m_current ()
    , m_end (nullptr)
  {
  }

  /**
   * Ctor
   */
  explicit SiblingsIterator (Node first,
			     const std::uint8_t *end)
    : m_current (first)
    , m_end (end)
  {
  }
    
  /**
   * Advance to next sibling
   */
  SiblingsIterator &operator++ ()
  {
    const auto next_addr = Node::skip (m_current.data ());

    if (next_addr < m_end)
    {
      m_current = Node {next_addr,
			m_current.rank (),
                        m_current.first_child ()};
    }
    else
    {
      m_current = Node {};
    }

    return (*this);
  }
  
  /**
   * Get reference to underlying node
   */
  const Node& operator* () const
  {
    return m_current;
  }

  /**
   * Pointer to underlying node
   */
  const Node* operator-> () const
  {
    return &m_current;
  }

  /**
   * Boolean conversion to test if visit has terminated
   */
  operator bool () const
  {
    return m_current.data ();
  }
  
  /**
   * Get pointer to end of siblings encoding
   */
  const std::uint8_t *end_siblings_range () const
  {
    return m_end;
  }
  
private:
  Node m_current;
  const std::uint8_t *m_end;
};

/**
 * Get iterator over children of current node
 */
template<typename Node>
SiblingsIterator<Node> visit_children (const Node &node)
{
  if (node.is_leaf ())
  {
    return SiblingsIterator<Node> {};
  }

  Node first_child
  {
    node.first_child (),
    node.rank (),
    Node::skip (node.first_child ())
  };

  return SiblingsIterator<Node> {
    first_child,
    first_child.first_child ()};
}
 
/**
 * Find first sibling node satisfying given predicate
 */
template<typename SiblingsIterator, typename Predicate>
auto find_sibling (SiblingsIterator &&siblings_it,
		   const Predicate  &pred)
  -> SiblingsIterator
{
  while (siblings_it)
  {
    if (pred (*siblings_it))
    {
      break;
    }

    ++siblings_it;
  }

  return std::move (siblings_it);
}
 
/**
 * Iterate subtree leaves by increasing rank.
 *
 * Note: this is not a standard-compliant iterator.
 */
template<typename Node>
class OrderedLeavesIterator
{
public:

  explicit OrderedLeavesIterator () {}

  explicit OrderedLeavesIterator (
    const SiblingsIterator<Node> &siblings_range)
  {
    if (siblings_range)
    {
      m_frontier.push (siblings_range);
      advance_to_leaf ();
    }
  }

  operator bool () const
  {
    return !m_frontier.empty ();
  }

  auto operator* () const -> const Node&
  {
    return *(m_frontier.top ());
  }

  auto operator-> () const -> const Node*
  {
    return &(*(m_frontier.top ()));
  }

  void operator++ ()
  {
    auto current = m_frontier.top ();
    m_frontier.pop ();
    ++current;
    
    if (current)
    {
      m_frontier.push (current);
    }

    advance_to_leaf ();
  }

private:  

  void advance_to_leaf ()
  {
    while (static_cast<bool> (*this) &&
           (!(m_frontier.top ()->is_leaf ())))
    {
      auto current = m_frontier.top ();
      m_frontier.pop ();
      push_leftmost_path (current);
    }
  }

  void push_leftmost_path (SiblingsIterator<Node> visitor)
  {
    while (1)
    {
      if (visitor->is_leaf ())
      {
	m_frontier.push (visitor);
	return;
      }
      else
      {
	auto tail = visitor;
	++tail;

	if (tail)
	{
	  m_frontier.push (tail);
	}

	visitor = visit_children (*visitor);
      }
    }
  }

private:

  using Entry = SiblingsIterator<Node>;

  struct OrderByRank
  {
    bool operator () (const Entry &lhs,
		      const Entry &rhs) const
    {
      const auto lhs_rank = lhs->rank ();
      const auto rhs_rank = rhs->rank ();

      if (lhs_rank != rhs_rank)
      {
	return lhs_rank > rhs_rank;
      }

      return !(*lhs < *rhs);
    }
  };

  std::priority_queue
  <
    Entry,
    std::vector<Entry>,
    OrderByRank
  > m_frontier;
};

/**
 * Visit downward path to a given descendant calling input
 * functor on each node except source.
 */
template<typename Node, typename F>
void traverse_descending_path (
  Node source,
  const Node &destination,
  F&& f)
{
  BOOST_ASSERT (source < destination);

  if (source == destination)
  {
    return;
  }

  while (1)
  {
    auto children_it = visit_children (source);

    if (destination.data () < children_it.end_siblings_range ())
    {
      f (destination);
      return;
    }
    else
    {
      do
      {
	source = *children_it;
	++children_it;
      } 
      while (children_it &&
            (children_it->first_child () <= destination.data ()));

      f (source);
    }
  }
}

/**
 * Perform an ordered visit of subtrie descending from given root
 */
template<typename Node, typename F>
void ordered_visit (SiblingsIterator<Node> siblings_range, F&& f)
{
  OrderedLeavesIterator<Node> visit {siblings_range};

  while (visit)
  {
    f (*visit);
    ++visit;
  }
}

} // namespace detail { 
} // namespace ordered_trie {

#endif
