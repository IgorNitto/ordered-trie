/**
 * @file  detail_trie_impl.inl
 * @brief
 */

#include "detail_iterator.h"
#include "detail_varint.h"
#include "serialise.h"

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/iterator_range_core.hpp>
#include <boost/range/size.hpp>

#include <iostream>
#include <stdexcept>

namespace ordered_trie { namespace detail {

namespace v0 {

/***********************************************************/
/*
 * The first byte of node's encoding is a bit packed header
 * encoding the sizes in byte of the subsequent parts of the 
 * node plus a single bit marking leaf nodes:
 *
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
template<typename T>
auto make_siblings_range (const Node<T> first)
  -> typename Node<T>::SiblingsRange
{
  // Dummy sentinel node marking end of siblings range
  Node<T> siblings_end;
  siblings_end.m_data = first.children_ptr ();

  return boost::make_iterator_range (
    SiblingsIterator<Node<T>> (first),
    SiblingsIterator<Node<T>> (siblings_end));
}

/***********************************************************/
/*
 * Helper functionalities for node's header decoding 
 */
const std::uint8_t* label_begin (const std::uint8_t *data)
{
  const auto offset_encoding =
    static_cast<VarInt64::wordsize_t> (
      ((*data) & OFFSET_MASK) >> BIT_OFFSET);

  const auto rank_encoding =
    static_cast<VarInt32::wordsize_t> (
      ((*data) & RANK_MASK) >> BIT_RANK);

  return data + 
    VarInt32::codeword_size (rank_encoding) +
    VarInt64::codeword_size (offset_encoding) +
    1;
}

const std::uint8_t* label_end (const std::uint8_t *data)
{
  const auto label_size = (*data) & LABEL_MASK;
  return label_begin (data) + label_size;
}

template<typename T>
const std::uint8_t* node_end (const std::uint8_t *data)
{
  auto result = label_end (data);

  if ((*data) & IS_LEAF_MASK)
  {
    return Serialise<T>::next (result);
  }

  return result;
}

/***********************************************************/
template<typename T>
Node<T>::Node (const std::uint8_t *data) noexcept
  : Node<T> (data, 0, node_end<T> (data))
{}

/***********************************************************/
template<typename T>
Node<T>::Node (const std::uint8_t *data,
	       const std::uint64_t base_score,
	       const std::uint8_t *base_children) noexcept
  : m_data (data)
  , m_score (base_score)
  , m_children (base_children) {}

/***********************************************************/
template<typename T>
auto Node<T>::label () const -> boost::string_ref
{
  const std::uint8_t label_size = (*m_data) & LABEL_MASK;
  
  /* Thanks type system, I am on my own now... */

  return {
    reinterpret_cast<const char*> (label_begin (m_data)),
    label_size};
}

/***********************************************************/
template<typename T>
auto Node<T>::rank () const -> std::uint64_t
{
  const auto offset_encoding =
    static_cast<VarInt64::wordsize_t> (
      ((*m_data) & OFFSET_MASK) >> BIT_OFFSET);

  const auto rank_encoding =
    static_cast<VarInt32::wordsize_t> (
      ((*m_data) & RANK_MASK) >> BIT_RANK);

  const auto ptr =
    m_data + 1 + VarInt64::codeword_size (offset_encoding);

  return m_score + VarInt32::deserialise (ptr, rank_encoding);
}

/***********************************************************/
template<typename T>				   
bool Node<T>::is_leaf () const
{
  return (*m_data) & IS_LEAF_MASK;
}

/***********************************************************/
template<typename T>
T Node<T>::metadata () const
{
  return Serialise<T>::deserialise (label_end (m_data));
}

/***********************************************************/
template<typename T>
void Node<T>::advance_to_sibling ()
{
  m_score = rank ();
  m_children += children_offset ();
  m_data = node_end<T> (m_data);
}
  
/***********************************************************/
template<typename T>
auto Node<T>::children () const -> Node<T>::SiblingsRange
{
  if (!is_leaf ())
  {
    const auto *ptr = children_ptr ();
    const Node first {ptr, rank (), node_end<T> (ptr)};
    return make_siblings_range (first);
  }
  else
  {
    return boost::make_iterator_range (
      SiblingsIterator<Node> (*this),
      SiblingsIterator<Node> (*this));
  }
}

/***********************************************************/
template<typename T>
bool Node<T>::operator== (const Node<T> &other) const
{
  return (m_data == other.m_data);
}

/***********************************************************/
template<typename T>
std::uint64_t Node<T>::children_offset () const
{
  const auto offset_encoding =
    static_cast<VarInt64::wordsize_t> (
      ((*m_data) & OFFSET_MASK) >> BIT_OFFSET);

  return VarInt64::deserialise (
    m_data + 1, offset_encoding);
}

/***********************************************************/
template<typename T>
const std::uint8_t* Node<T>::children_ptr () const
{
  return m_children + children_offset ();
}

/***********************************************************/
template<typename T>
MakeTrie<T>::MakeTrie (std::string label,
		       std::size_t rank,
		       T           metadata)
  : m_label (std::move (label))
  , m_rank (rank)
  , m_metadata (std::move (metadata))
{
  BOOST_ASSERT_MSG (label.size () < max_label_size,
		    "Exceeded maximum label size");
}

/***********************************************************/
template<typename T>
MakeTrie<T>::MakeTrie (std::string label,
		       std::vector<MakeTrie<T>> children)
  : m_label (std::move (label))
{
  BOOST_ASSERT_MSG (label.size () < max_label_size,
		    "Exceeded maximum label size");

  add_children (std::move (children));
}

/***********************************************************/
template<typename T>
MakeTrie<T>::MakeTrie (std::vector<MakeTrie<T>> siblings)
{    
  /*
   * Prepend an auxiliary root node with base score and
   * children offset both set to 0
   */
  m_subtree_serialised.push_back (0);

  if (!siblings.empty ())
  { 
    boost::sort (siblings,
      [] (const MakeTrie<T> &lhs, const MakeTrie<T> &rhs)
      {
	return lhs.min_score () < rhs.min_score ();
      });

    serialise_siblings (m_subtree_serialised, siblings, 0);
  }
  else
  {
    m_subtree_serialised.front () |= (1 << BIT_IS_LEAF);
  }
}

/***********************************************************/
template<typename T>
std::uint64_t MakeTrie<T>::min_score () const {return m_rank;}

/***********************************************************/
template<typename T>
const std::string &MakeTrie<T>::label () const {return m_label;}

/***********************************************************/
template<typename T>
TrieImpl<T> MakeTrie<T>::move_to_trie ()
{
  return TrieImpl<T> {std::move (m_subtree_serialised)};
}

/***********************************************************/
template<typename T>
void MakeTrie<T>::serialise_header (
  std::vector<std::uint8_t> &output,
  const size_t               children_offset) const
{
  BOOST_ASSERT_MSG (m_label.size () < max_label_size,
                    "Exceeded maximum label size");

  output.reserve (output.size () + max_node_size ());
  output.push_back (0);

  const auto header_off = output.size () - 1;

  /* Stream out header fields: offset, rank, label */
  const auto offset_encoding =
    VarInt64::serialise (output, children_offset);

  const auto rank_encoding =
    VarInt32::serialise (output, m_rank);

  const auto label_size =
    static_cast<std::uint8_t> (m_label.size ());

  const auto is_leaf = static_cast<bool> (m_metadata);

  /* Fill in header byte fields */
  output [header_off] =
      label_size
    | (is_leaf << BIT_IS_LEAF)
    | (static_cast<std::uint8_t> (offset_encoding) << BIT_OFFSET)
    | (static_cast<std::uint8_t> (rank_encoding) << BIT_RANK);

  /* Append label bytes */
  std::copy (m_label.begin (),
	       m_label.end (),
	       back_inserter (output));

  /* Only on leaves: append metadata encoding */
  if (m_metadata)
  {
    Serialise<T>::serialise (
      output,
	m_metadata.get ());
  }    
}

/***********************************************************/
template<typename T>
void MakeTrie<T>::serialise_siblings (
  std::vector<std::uint8_t> &output,
  std::vector<MakeTrie<T>>  siblings,
  const std::uint64_t       base_rank)
{
  /*
   * Estimate total space taken by the encoding
   */
  const auto estimated_encoding_size = [&]
  {
    auto result = siblings.size () * max_node_size ();

    for (const auto &node : siblings)
    {
      result += node.m_subtree_serialised.size ();
    }

    return result;
  } ();

  /*
   * First part of serialisation consists of concatenation
   * of all node's header
   */
  const auto initial_size = output.size ();
  const auto first_node = std::begin (siblings);
	  auto prev_node  = first_node;
	  auto prev_rank  = first_node->m_rank;

  output.reserve (initial_size + estimated_encoding_size);

  for (auto this_node  = std::next (first_node);
	      this_node != std::end (siblings); 
	    ++this_node)
  {
    const auto children_offset =
	prev_node->m_subtree_serialised.size ();

    const auto current_rank = this_node->m_rank;

    BOOST_ASSERT_MSG (current_rank >= prev_rank,
			"Rank values not in increasing order");

    this_node->m_rank -= prev_rank;

    this_node->serialise_header (
      output,
	children_offset);

    prev_node = this_node;
    prev_rank = current_rank;
  }

  /*
   * We can serialise the first children only after
   * all other siblings, as this requires to know
   * total size of their headers.
   */
  const size_t total_headers_size = output.size () - initial_size;

  /*
   * Append first node, then perform a rotate to move its encoding
   * before all the other siblings
   */
  first_node->m_rank -= base_rank;
  first_node->serialise_header (
    output,
    total_headers_size);

  const auto pivot = output.begin () + initial_size;

  std::rotate (
    pivot,
    pivot + total_headers_size,
    output.end ());

  /*
   * Second part of the serialisation consists of
   * concatenation of all sub-tries serialisation
   */
  for (auto &&node: siblings)
  {
    output.insert (
      output.end (),
	node.m_subtree_serialised.begin (),
	node.m_subtree_serialised.end ());

    node.m_subtree_serialised.clear ();
  }
}

/***********************************************************/
template<typename T> 
void MakeTrie<T>::add_children (
  std::vector<MakeTrie<T>> siblings,
  const bool called_from_root)
{
  BOOST_ASSERT (!m_metadata);
  BOOST_ASSERT_MSG (!siblings.empty (),
		      "Internal node with empty children");

  /*
   * If there is a single children node, check possibility to
   * collapse label if their size can fit into single label
   */
  if (siblings.size () == 1)
  {
    auto &child = siblings.front ();

    if ((child.m_label.size () + m_label.size ()) < max_label_size)
    {
	/* append label and copy metadata */

	m_label.insert (m_label.end (),
			child.m_label.begin (),
			child.m_label.end ());

	m_subtree_serialised = std::move (child.m_subtree_serialised);
	m_rank               = child.m_rank;
	m_metadata           = std::move (child.m_metadata);
	return;
    }
  }

  /*
   * Proceed appending serialisation of children.
   */
  boost::sort (siblings,
    [] (const MakeTrie<T> &lhs, const MakeTrie<T> &rhs)
    {
	return lhs.min_score () < rhs.min_score ();
    });

  m_rank = called_from_root ? 0 : siblings.begin ()->m_rank;

  serialise_siblings (
    m_subtree_serialised,
    siblings,
    m_rank);
}

/***********************************************************/
template<typename T>
constexpr size_t MakeTrie<T>::max_node_size ()
{
  return
    2 * VarInt64::max_codeword_size ()  +
    max_label_size                      +
    Serialise<T>::estimated_max_size () +
    1;
}

/***********************************************************/
/**
 * Trie implementation
 */
template<typename T>
TrieImpl<T>::TrieImpl (std::vector<std::uint8_t> data)
  : m_data (std::move (data))
  , m_root (m_data.data ())
{}

template<typename T>
const Node<T> &TrieImpl<T>::root () const
{
  return m_root;
}

template<typename T>
constexpr const char* TrieImpl<T>::version_id ()
{
  return "1.0.0";
}

} // namespace v0 {

}} // namespace ordered_trie { namespace detail {
