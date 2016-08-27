/**
 * @file  detail/ordered_trie_node.hpp
 * @brief Node class definition
 *
 * This file is subject to the terms and conditions defined in
 * file 'LICENSE.txt', which is part of this source code package.
 *  
 */

#ifndef DETAIL_ORDERED_TRIE_NODE_HPP
#define DETAIL_ORDERED_TRIE_NODE_HPP

#include "ordered_trie_builtin_serialise.hpp"
#include "ordered_trie_varint.hpp"

#include <boost/optional.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/range.hpp>

#include <stdexcept>

namespace ordered_trie {
namespace detail {

/**
 * Provide read-only view over information associated to
 * a trie node. 
 */
template<typename T = Void>
class Node
{
public:

  constexpr static size_t max_label_size = 8;
  using metadata_type = T;

  /**
   * Max encoding size of a single node 
   */
  static constexpr size_t max_encoding_size ();

  /**
   * Default null instance.
   */
  explicit Node () = default;
  
  /**
   * Ctor.
   */
  explicit Node (const std::uint8_t *address,
		 const std::uint64_t base_rank,
		 const std::uint8_t *children_base) noexcept;

  /**
   * Pointer to beginning of label
   */
  const std::uint8_t* label_begin () const;

  /**
   * Label size
   */
  const std::uint8_t label_size () const;

  /**
   * Get ranking score
   */
  std::uint64_t rank () const;

  /**
   * Returns true iff this is a leaf node
   */
  bool is_leaf () const;

  /**
   * Pointer to serialisation of descendant sub-trie begins
   * (undefined for leaf nodes).
   */
  const std::uint8_t* first_child () const;

  /**
   * Address of node's data serialisation
   * (nullptr for invalid node). 
   */  
  const std::uint8_t* data () const {return m_data;}

  /**
   * Overload operator== comparing node's address
   */
  bool operator== (const Node &other) const;

  /**
   * Overload of operator< comparing node's address
   */
  bool operator< (const Node &other) const;

  /**
   * Pointer to first byte after node serialisation
   */
  static const std::uint8_t* skip (const std::uint8_t*);

private:

  static const std::uint8_t* rank_address (const std::uint8_t*);
  static const std::uint8_t* label_begin (const std::uint8_t*);
  static const std::uint8_t  label_size (const std::uint8_t*);
  static const std::uint8_t* metadata_address (const std::uint8_t*);

private:
  const std::uint8_t *m_data = nullptr;
  std::uint64_t m_cumulative_rank = 0;
  const std::uint8_t *m_children = nullptr;
};

/**
 * Serialise node representation from basic components
 */
template<typename T>
void serialise_node (std::vector<std::uint8_t> &output,
		     const std::string         &label,
		     const std::uint64_t        rank,
		     const size_t               children_offset,
		     const boost::optional<T>  &metadata);


/***********************************************************
 * ordered_trie_node.hpp implementation
 ***********************************************************/
  
/*
 * The first byte of node's encoding is a bit packed header
 * encoding the sizes in byte of the subsequent parts of the 
 * node plus a single bit marking leaf nodes:
 *
 * @code
 * {
 *    rank_encoding   : 2; //< Length of the rank component
 *    offset_encoding : 2; //< Length of the offset component
 *    is_leaf         : 1; //< Is leaf node
 *    label_size      : 3; //< Length of the node label
 * };
 * @endcode
 *
 * Note: C bit fields are avoided for portability
 *
 * @TODO: Sub-optimal in case of leaves encoding
 *        (offset redundant in that case)
 */

enum NodeHeaderOffset
{
  BIT_LABEL   = 0,
  BIT_IS_LEAF = 3,
  BIT_OFFSET  = 4,
  BIT_RANK    = 6
};

enum NodeHeaderMask
{
  LABEL_MASK   = ((1 << BIT_IS_LEAF) - 1),
  IS_LEAF_MASK = (1 << BIT_IS_LEAF),
  OFFSET_MASK  = ((1 << BIT_OFFSET) | (1 << (BIT_OFFSET + 1))),
  RANK_MASK    = ((1 << BIT_RANK) | (1 << (BIT_RANK + 1)))
};

/***********************************************************/
/* static */
template<typename T>
const std::uint8_t* Node<T>::label_begin (const std::uint8_t *data)
{
  const auto offset_encoding =
    static_cast<OffsetEncoder::wordsize_t> (
      ((*data) & OFFSET_MASK) >> BIT_OFFSET);
    
  return OffsetEncoder::skip (data + 1, offset_encoding);
}

/***********************************************************/
template<typename T>
/* static */
const std::uint8_t
Node<T>::label_size (const std::uint8_t *data)
{
  return (*data) & LABEL_MASK;
}
 
/***********************************************************/
template<typename T>
const std::uint8_t* Node<T>::label_begin () const
{
  return label_begin (data ());
}

/***********************************************************/
template<typename T>
const std::uint8_t Node<T>::label_size () const
{
  return label_size (data ());
}

/*********************************************************************/
template<typename T>
constexpr size_t Node<T>::max_encoding_size ()
{
  return
    OffsetEncoder::max_codeword_size () +
    RankEncoder::max_codeword_size ()   +
    max_label_size                      +
    Serialise<T>::estimated_max_size () +
    1;
}

/***********************************************************/
template<typename T>
/* static */
const std::uint8_t*
Node<T>::rank_address (const std::uint8_t *data)
{
  return label_begin (data) + label_size (data);
}

/***********************************************************/
template<typename T>
/* static */
const std::uint8_t*
Node<T>::metadata_address (const std::uint8_t *data)
{
  const auto rank_encoding =
    static_cast<RankEncoder::wordsize_t> (
      ((*data) & RANK_MASK) >> BIT_RANK);

  return RankEncoder::skip (rank_address (data), rank_encoding);
}

/***********************************************************/
template<typename T>
/* static */
const std::uint8_t*
Node<T>::skip (const std::uint8_t *data)
{
  if (*data & IS_LEAF_MASK)
  {
    return Serialise<T>::skip (metadata_address (data));
  }

  return metadata_address (data);
}

/***********************************************************/
template<typename T>
Node<T>::Node (const std::uint8_t *data,
	       const std::uint64_t base_rank,
	       const std::uint8_t *children_base) noexcept
  : m_data (data)
{
  const auto offset_encoding =
    static_cast<OffsetEncoder::wordsize_t> (
      ((*m_data) & OFFSET_MASK) >> BIT_OFFSET);

  const auto rank_encoding =
    static_cast<RankEncoder::wordsize_t> (
      ((*m_data) & RANK_MASK) >> BIT_RANK);

  m_children = children_base + OffsetEncoder::
    deserialise (m_data + 1, offset_encoding);

  m_cumulative_rank = base_rank + RankEncoder::
    deserialise (rank_address (m_data), rank_encoding);
}

/***********************************************************/
template<typename T>
auto Node<T>::rank () const -> std::uint64_t
{
  return m_cumulative_rank;
}

/***********************************************************/
template<typename T>				   
bool Node<T>::is_leaf () const
{
  return (*m_data) & IS_LEAF_MASK;
}

/***********************************************************/
template<typename T>
bool Node<T>::operator== (const Node<T> &other) const
{
  return (m_data == other.m_data);
}

/***********************************************************/
template<typename T>
bool Node<T>::operator< (const Node<T> &other) const
{
  return (m_data < other.m_data);
}

/***********************************************************/
template<typename T>
const std::uint8_t* Node<T>::first_child () const
{
  return m_children;
}

/***********************************************************/

template<typename T>
void serialise_node (std::vector<std::uint8_t> &output,
		     const std::string         &label,
		     const std::uint64_t        rank,
		     const size_t               children_offset,
		     const boost::optional<T>  &metadata)
{
  if (label.size () >= Node<T>::max_label_size)
  {
    throw std::length_error ("Exceeded maximum label size");
  }
  
  output.reserve (output.size () + Node<T>::max_encoding_size ());
  output.push_back (0);

  const auto header_off = output.size () - 1;

  /* Stream out header fields: offset, rank, label */
  const auto offset_encoding =
    OffsetEncoder::serialise (output, children_offset);

  /* Append label bytes */
  std::copy (label.begin (),
	     label.end (),
	     back_inserter (output));

  const auto label_size =
    static_cast<std::uint8_t> (label.size ());

  const auto rank_encoding =
    RankEncoder::serialise (output, rank);

  const auto is_leaf = static_cast<bool> (metadata);

  /* Only on leaves: append metadata encoding */
  if (metadata)
  {
    Serialise<T>::serialise (output, metadata.get ());
  }

  /* Fill in header byte fields */
  output[header_off] =
      label_size
    | (is_leaf << BIT_IS_LEAF)
    | (static_cast<std::uint8_t> (offset_encoding) << BIT_OFFSET)
    | (static_cast<std::uint8_t> (rank_encoding) << BIT_RANK);
}

} // namespace detail  
} // namespace ordered_trie

#endif
