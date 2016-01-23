/**
 * @file detail_iterator.inl
 */

namespace ordered_trie {

/*********************************************************************/

template<typename Node>
SiblingsIterator<Node>::
SiblingsIterator (const std::uint8_t *base)
 : m_pointed (base)
{
}

/*********************************************************************/

template<typename Parameters>
inline auto SiblingsIterator<Parameters>::first_child () const
  -> SiblingsIterator<Parameters>
{
  return SiblingsIterator<Parameters> {m_children_base};
}

/*********************************************************************/

template<typename Parameters>
inline auto SiblingsIterator<Parameters>::dereference () const
  -> NodeView
{
  return m_pointed;
}

/*********************************************************************/

template<typename Parameters>
inline bool SiblingsIterator<Parameters>::equal (
  const SiblingsIterator<Parameters> &other) const
{
  return (other.m_pointed).data () == m_pointed.data ();
}

/*********************************************************************/
  
template<typename Parameters>
inline void SiblingsIterator<Parameters>::increment ()
{
  m_pointed = View {m_pointed.data () + m_pointed.size ()};
  m_children_base += m_pointed.children_offset ();
}

/********************************************************************/

template<typename Parameters>
inline auto make_siblings_range (const std::uint8_t *base)
  -> SiblingsRange<Parameters>
{
  using boost::make_iterator_range;
  using iterator = SiblingsIterator<Parameters>;

  iterator start_it {base};
  iterator end_it {start_it.m_children_base};

  return make_iterator_range (start_it, end_it);
}

} // namespace ordered_trie {
