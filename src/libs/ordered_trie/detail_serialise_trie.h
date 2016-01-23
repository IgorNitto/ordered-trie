/**
 * @file  detail_make_trie.h
 * @brief Auxiliary tools for trie serialisation algorithm
 */

#ifndef ORDERED_TRIE_DETAIL_MAKE_TRIE_H
#define ORDERED_TRIE_DETAIL_MAKE_TRIE_H

#include "detail_concrete_node.h"

namespace ordered_trie { namespace detail {

/**
 * Helper class for trie serialisation process for
 * creating and holding subtrie serialisation
 */ 
template<typename Parameters> class SerialiseTrie
{
public:

  using self_t = SerialiseTrie<Parameters>;

  /**
   * Leaf node ctor
   */ 
  SerialiseTrie (const std::string        &label,
		 const typename::MetaData &metadata,
		 std::size_t               rank);

  /**
   * Internal node ctor
   */
  SerialiseTrie (
    const std::string         &label,
    const std::vector<self_t> &children);

  /**
   * Get subtree data
   */
  inline const std::uint8_t *subtree_data () const
  {
    return m_subtree_serialised.data ();
  };

  /**
   * Release ownership of serialised subtree
   */
  inline std::vector<std::uint8_t> move_subtree ()
  {
    m_subtree_serialised.shrink_to_fit ();
    return m_subtree_serialised;
  }

  /**
   * Append sequence of children to serialised subtree,
   * transforming a leaf into an internal node
   */
  template<typename FwdRange>
  void append_children (FwdRange &children);

private:

  /*
   * Serialise range of children nodes.
   * The input siblings are given as pair of iterator to the
   * first and last siblings node.
   */
  template<typename FwdRange>
  static inline std::uint64_t
  serialise_siblings (std::vector<std::uint8_t> &output,
		      FwdRange                  &siblings);

private:
  ConcreteNode<Parameters::MetaData> m_root;
  std::vector<std::uint8_t> m_subtree_serialised;
};
-
}} // namespace detail { namespace serialise {

#include "detail_make_ordered_trie.inl"

#endif
