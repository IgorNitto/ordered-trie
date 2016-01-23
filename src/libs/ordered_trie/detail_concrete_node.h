/**
 * @file  detail_concrete_node.h
 * @brief Plain representation of single node components
 */

#ifndef ORDERED_TRIE_DETAIL_CONCRETE_NODE_H
#define ORDERED_TRIE_DETAIL_CONCRETE_NODE_H

#include <boost/assert.hpp>
#include <boost/container/static_vector.hpp>
#include <boost/optional.hpp>
#include <boost/utility/string_ref.hpp>

namespace ordered_trie {

/**
 * Plain representation of node components, templatized
 * over the associated metadata type (@p MetaData) and the
 * length of the associated label (@p N).
 */ 
template<typename MetaData> struct ConcreteNode
{
  /* Helper container for stack allocated small string */

  static constexpr int max_label_size = 8;

  using LabelString =
    boost::container::static_vector<char, max_label_size>;

  /**
   * String attached to descending edge from father node
   */
  LabelString label;

  /**
   * Ranking score
   */
  std::uint64_t rank;

  /**
   * Optional payload metadata, only defined for leaf nodes 
   */
  boost::optional<MetaData> metadata;

  /**
   * Relative offset of children encoding
   */
  std::size_t children_offset;

  /**
   * Test for leaf node (defined as every node 
   * having associated metadata)
   */
  inline bool is_leaf () const
  {
    return static_cast<bool> (m_payload_data);
  };
};

} // namespace ordered_trie {

#endif 
