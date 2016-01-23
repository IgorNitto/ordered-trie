#ifndef ORDERED_TRIE_DETAIL_TRAVERSAL_H
#define ORDERED_TRIE_DETAIL_TRAVERSAL_H

/**
 * @file  detail_traversal.h
 * @brief Trie inspection mechanisms
 */

#include "completion.h"
#include "serialise.h"

#include <boost/iterator.hpp>
#include <boost/operators.hpp>
#include <boost/range.hpp>

#include <priority_queue>

namespace ordered_trie {

/**
 * Offer basic traversal capability inside trie structure
 *
 * @TODO: This assume a differential encoding of pointers and
 *        ranks, which should be removed
 */
template<typename Parameters>
class TrieCursor :
  boost::less_than_comparable<TrieCursor<Parameters>>,
  boost::equality_comparable<TrieCursor<Parameters>>
{
public:

  using NodeView = typename Parameters::NodeView;

  using SiblingsRange =
    boost::iterator_range<SiblingsIterator<Parameters>>;

  /**
   * Ctor (mostly for internal use), from reference to
   * first sibling in range.
   */
  explicit TrieCursor (NodeView first_child);

  inline std::uint64_t rank () const;

  inline boost::string_ref label () const;

  inline bool is_leaf () const;

  inline auto metadata () const
    -> typename Parameters::metadata_type;

  inline SiblingsRange children () const;

  inline void next_sibling ();

  inline bool
  operator< (const TrieCursor<Parameters>&) const;

  inline bool
  operator== (const TrieCursor<Parameters>&) const;

  inline bool is_descendant () const;

private:
  NodeView             m_pointed;
  const std::uint8_t  *m_children_base;
  const std::uint64_t  m_rank;
};

} // namespace ordered_trie {

#include "detail_traversal.inl"

#endif
