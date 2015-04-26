/**
 * @file  traversal.inl
 * @brief
 */

namespace ordered_trie {

/*********************************************************************/

template<typename Encoding>
SiblingsIterator<Encoding>::
SiblingsIterator (const std::uint8_t *base)
 : m_pointed (base)
{
  m_children_base =
    m_pointed.data () +
    m_pointed.size () +
    m_pointed.children_offset ();
}

/*********************************************************************/

template<typename Encoding>
inline auto SiblingsIterator<Encoding>::children () const
  -> SiblingsRange<Encoding>
{
  return make_siblings_range<Encoding> (m_children_base);
}

/*********************************************************************/

template<typename Encoding>
inline auto SiblingsIterator<Encoding>::first_children () const
  -> SiblingsIterator<Encoding>
{
  return SiblingsIterator<Encoding> {m_children_base};
}

/*********************************************************************/

template<typename Encoding>
inline auto SiblingsIterator<Encoding>::dereference () const
  -> View
{
  return m_pointed;
}

/*********************************************************************/

template<typename Encoding>
inline bool SiblingsIterator<Encoding>::equal (
  const SiblingsIterator<Encoding> &other) const
{
  return (other.m_pointed).data () == m_pointed.data ();
}

/*********************************************************************/
  
template<typename Encoding>
inline void SiblingsIterator<Encoding>::increment ()
{
  m_pointed = View {m_pointed.data () + m_pointed.size ()};
  m_children_base += m_pointed.children_offset ();
}

/********************************************************************/

template<typename Encoding>
inline auto make_siblings_range (const std::uint8_t *base)
  -> SiblingsRange<Encoding>
{
  using boost::make_iterator_range;
  using iterator = SiblingsIterator<Encoding>;

  iterator start_it {base};
  iterator end_it {start_it.m_children_base};

  return make_iterator_range (start_it, end_it);
}

} // namespace ordered_trie {
