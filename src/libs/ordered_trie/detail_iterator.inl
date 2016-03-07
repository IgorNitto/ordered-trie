/**
 * @file  detail_iterator.inl
 */

#include <boost/iterator.hpp>
#include <boost/range.hpp>

#include <queue>
#include <tuple>

namespace ordered_trie {

/**********************************************************************/
  
template<typename Node>
SiblingsIterator<Node>::SiblingsIterator (const Node &node)
  : m_pointed (node)
{}

template<typename Node>
inline auto SiblingsIterator<Node>::dereference () const
 -> Node
{
  return m_pointed;
}

template<typename Node>
inline bool
SiblingsIterator<Node>::equal (
  const SiblingsIterator<Node> &other) const
{
  return (m_pointed == other.m_pointed);
}

template<typename Node>
inline void
SiblingsIterator<Node>::increment ()
{
  m_pointed.advance_to_sibling ();
}

/**********************************************************************/
template<typename Node>
OrderedLeavesIterator<Node>::OrderedLeavesIterator () noexcept
{
}
  
template<typename Node>
OrderedLeavesIterator<Node>::OrderedLeavesIterator (const Node &root)
  : m_root (root)
{
  SiblingsIterator<Node> first {root};
  m_frontier.push (SearchNode {first, std::next (first)});
  next_leaf ();
}

template<typename Node>
inline Node
OrderedLeavesIterator<Node>::dereference () const
{
  return *(m_frontier.top ().base ());
}

template<typename Node>
inline bool
OrderedLeavesIterator<Node>::equal (
  const OrderedLeavesIterator<Node> &other) const
{
  if (!(m_root == other.m_root))
  {
    return false;
  }

  if (m_frontier.empty ())
  {
    return other.m_frontier.empty ();
  }

  if (other.m_frontier.empty ())
  {
    return m_frontier.empty ();
  }

  return (m_frontier.top ().base () ==
	  other.m_frontier.top ().base ());
}

template<typename Node>
inline void
OrderedLeavesIterator<Node>::push_leftmost_path (SearchNode node)
{
  if (node.is_last ())
  {
    return;
  }
      
  while (!node.base ()->is_leaf ())
  {
    const auto children = node.base ()->children ();
    node.advance ();

    if (!node.is_last ())
    {
      m_frontier.push (node);
    }
      
    if (!boost::empty (children))
    {
      node = SearchNode {children};	
    }
    else
    {
      return;
    }
  }

  m_frontier.push (node);
}
  
template<typename Node>
inline void
OrderedLeavesIterator<Node>::increment ()
{
  if (m_frontier.empty ())
  {
    throw std::logic_error ("Incrementing an end iterator");
  }

  auto current = m_frontier.top ();
  m_frontier.pop ();
  current.advance ();

  if (!current.is_last ())
  {
    m_frontier.push (current);
  }

  next_leaf ();
}

template<typename Node>
inline void
OrderedLeavesIterator<Node>::next_leaf ()
{  
  /*
   * Rstore invariant of having either an empty m_frontier
   * or m_frontier.top ().base ()->is_leaf ()
   */    
  while (!m_frontier.empty () &&
	 !m_frontier.top ().base ()->is_leaf ())
  {
    auto current = m_frontier.top ();
    m_frontier.pop ();    
    push_leftmost_path (std::move (current));
  }
}

template<typename Node>
auto 
OrderedLeavesIterator<Node>::end (const Node &root)
  -> OrderedLeavesIterator<Node>
{
  OrderedLeavesIterator<Node> result;
  result.m_root = root;
  return result;
}
 
/**********************************************************************/

template<typename Node>
class OrderedLeavesIterator<Node>::SearchNode
{
public:

  using self_type = OrderedLeavesIterator<Node>::SearchNode;
  using siblings_iterator = SiblingsIterator<Node>;
  using siblings_range = typename Node::SiblingsRange;
  
  explicit SearchNode (const siblings_range &in)
    : m_base (std::begin (in))
    , m_end (std::end (in))
  {
    BOOST_ASSERT (m_base != m_end);
    m_rank = m_base->rank ();
  }

  explicit SearchNode (const siblings_iterator &base,
		       const siblings_iterator &end)
    : m_base (base)
    , m_end (end)
  {
    BOOST_ASSERT (m_base != m_end);
    m_rank = m_base->rank ();
  }

  bool operator> (const self_type &rhs) const
  {
    if (m_rank != rhs.m_rank)
    {
      return m_rank > rhs.m_rank;
    }

    return *m_base < *rhs.m_base;
  }

  void advance ()
  {
    BOOST_ASSERT (m_base != m_end);

    ++m_base;

    if (m_base != m_end)
    {
      m_rank = m_base->rank ();
    }
  }

  bool is_last () const
  {
    return (m_base == m_end);
  }

  const siblings_iterator &operator-> () const
  {
    return m_base;
  }
  
  const siblings_iterator &base () const
  {
    return m_base;
  }

private:

  siblings_iterator m_base;
  siblings_iterator m_end;
  std::uint64_t m_rank;  
};
  
/**********************************************************************/

} // namespace ordered_trie {
