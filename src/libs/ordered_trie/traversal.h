/**
 * @file  traversal.h
 * @brief
 */

#ifndef ORDERED_TRIE_TRAVERSAL_H
#define ORDERED_TRIE_TRAVERSAL_H

#include <boost/iterator.hpp>
#include <boost/range.hpp>

namespace ordered_trie {

/* Fwd declare */
template<typename> class SiblingsIterator;

/**
 * Forward range of siblings node
 */
template<typename Encoding>
using SiblingsRange =
  boost::iterator_range<SiblingsIterator<Encoding>>;

/**
 * Forward iterator running through a sequence 
 * of siblings node
 */
template<typename Encoding>
class SiblingsIterator: 
  public boost::iterator_facade<
    /* CRTP       */ SiblingsIterator<Encoding>,
    /* value_type */ typename Encoding::View,
    /* category   */ boost::forward_traversal_tag,
    /* reference  */ typename Encoding::View
   >
{
public:
  using self_t        = SiblingsIterator<Encoding>;
  using View          = typename Encoding::View;

  /**
   * Ctor from pointer to root node
   */
  explicit SiblingsIterator (const std::uint8_t*);

  /**
   * Take range iterating through children
   */
  inline SiblingsRange<Encoding> children () const;

  /**
   * Take iterator to first children
   */
  inline self_t first_children () const;

private:
  friend class boost::iterator_core_access;

  inline View dereference () const;  
  inline bool equal (const self_t &other) const;
  inline void increment ();

private:

  template<typename T> friend
  auto make_siblings_range (const std::uint8_t*)
    -> SiblingsRange<T>;

private:
  View                m_pointed;
  const std::uint8_t *m_children_base;
};

/**
 * Create range of siblings node from pointer to beginning
 * of encoding
 */
template<typename Encoding>
inline auto make_siblings_range (const std::uint8_t *base)
  -> SiblingsRange<Encoding>;

} // namespace ordered_trie {

#include "traversal.inl"

#endif // ORDERED_TRIE_TRAVERSAL_H
