/**
 * @file detail_traversal.inl
 */

namespace ordered_trie {

/*********************************************************************/

template<typename Parameters>
SiblingsIterator<Parameters>::
SiblingsIterator (const std::uint8_t *base)
 : m_pointed (base)
{
  m_children_base =
    m_pointed.data () +
    m_pointed.size () +
    m_pointed.children_offset ();
}

/*********************************************************************/

template<typename Parameters>
inline auto SiblingsIterator<Parameters>::children () const
  -> SiblingsRange<Parameters>
{
  return make_siblings_range<Parameters> (m_children_base);
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
inline auto children (const std::uint8_t *base)
  -> SiblingsRange<Parameters>
{
  using boost::make_iterator_range;
  using iterator = SiblingsIterator<Parameters>;

  iterator start_it {base};
  iterator end_it {start_it.m_children_base};

  return make_iterator_range (start_it, end_it);
}

/********************************************************************/


  /* Ctor (mostly for internal use) */
  TrieCursor (NodeView            address,
	      std::unit64_t       rank_offset,
	      const std::uint8_t *children_base)
    : m_node_ref (node_ref)
  {
    m_rank = rank_offset + m_node_ref.rank ();
    m_children_base =
      children_base + m_node_ref.children_offset ();
  }

  inline std::uint64_t rank () const
  {
    return m_rank;
  }

  inline boost::string_ref label () const
  {
    return m_node_ref.label ();
  }

  inline SiblingsRange children () const
  {
  }

  inline bool is_leaf () const
  {
    return m_node_ref.is_leaf ();
  }

  inline auto metadata () const
    -> typename Parameters::metadata_type
  {
    using result_type = typename Parameters::metadata_type;

    return Serialise<result_type>::deserialise (
	     m_node_ref.payload ());
  }

} // namespace ordered_trie {
