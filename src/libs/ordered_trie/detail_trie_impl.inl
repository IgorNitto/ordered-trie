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
class Node
{
public:

  using SiblingsRange =
    boost::iterator_range<SiblingsIterator<Node<T>>>;

  /************************************************/

  explicit Node (const std::uint8_t *data) noexcept
    : Node<T> (data, 0, node_end (data))
  {
  }

  /************************************************/

  boost::string_ref label () const
  {
    const auto label_size = (*m_data) & LABEL_MASK;
    return {label_data (), label_size};
  }

  /************************************************/

  std::uint64_t rank () const
  {
    return m_score;
  }

  /************************************************/

  bool is_leaf () const
  {
    return (*m_data) & IS_LEAF_MASK;
  }

  /************************************************/

  T metadata () const
  {
    return Serialise<T>::deserialise (header_end ());
  }

  /************************************************/

  Node<T> next_sibling ()
  {
    return Node<T> {
      node_end (m_data),
      m_score,
      m_children};
  }

  /************************************************/

  SiblingsRange children () const
  {
    Node<T> first {m_children_base,
	           m_score,
	           node_end (m_children_base)};

    Node<T> siblings_end;
    siblings_end.m_data = first.m_children_base;

    return boost::make_iterator_range (
       SiblingsIterator<Node<T>> (first),
       SiblingsIterator<Node<T>> (siblings_end));
  }

  /************************************************/

  bool operator== (const Node<T> &other) const
  {
    return (m_data == other.m_data);
  }

private:

  Node () {}

  /************************************************/

  Node (const std::uint8_t *data,
	const std::uint64_t base_score,
	const std::uint8_t *base_children) noexcept
  : m_data (data)
  , m_score (base_score)
  , m_children (base_children)
  {
    auto ptr = m_data + 1;

    const auto offset_encoding =
      static_cast<VarInt64::wordsize_t> (
        ((*m_data) & OFFSET_MASK) >> BIT_OFFSET);

    m_children += VarInt64::deserialise (
      ptr, offset_encoding);

    ptr += VarInt64::codeword_size (offset_encoding);

    const auto rank_encoding =
      static_cast<VarInt32::wordsize_t> (
        ((*m_data) & RANK_MASK) >> BIT_RANK);

    m_score += VarInt32::deserialise (
      ptr, rank_encoding);
  }

  /**************************************************/
  /*
   * Helper functionalities for node's header decoding 
   */

  static const std::uint8_t* label_begin (const std::uint8_t *data)
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

  static const std::uint8_t* label_end (const std::uint8_t *data)
  {
    const auto label_size = (*data) & LABEL_MASK;
    return label_begin (data) + label_size;
  }

  static const std::uint8_t* node_end (const std::uint8_t *data)
  {
    auto result = label_end (data);

    if ((*data) & IS_LEAF_MASK)
    {
      return Serialise<T>::next (result);
    }

    return result;
  }

private:

  const std::uint8_t *m_data;
  std::uint64_t       m_score;
  const std::uint8_t *m_children;
};

/***********************************************************/

template<typename T>
class MakeTrie
{
public:

  static constexpr size_t max_label_size = 8;

  /*********************************************************/

  MakeTrie () {};

  MakeTrie (std::string label,
	    std::size_t rank,
	    T           metadata = {})
  : m_label (std::move (label))
  , m_metadata (std::move (metadata))
  , m_rank (rank)
  {}

  MakeTrie (std::string         label,
	    std::vector<self_t> children)
  : m_label (std::move (label))
  {
    add_children (std::move (children));
  }


  /*********************************************************/
  static void
  serialise_trie (std::vector<std::uint8_t> &output,
		  std::vector<MakeTrie<T>>   siblings)
  {
    
  }

private:

  /*********************************************************/
  void serialise_header (
    std::vector<std::uint8_t> &output,
    const size_t               children_offset) const
  {
    BOOST_ASSERT_MSG (m_label.size () < max_label_size,
                      "Exceeded maximum label size");
 
    output.reserve (output.size () + max_encoding_size ());
    output.push_back (0);
    const auto header_off = output.size () - 1;

    /* Stream out header fields: offset, rank, label */
    const auto offset_encoding =
      VarInt64::serialise (output, children_offset);

    const auto rank_encoding =
      VarInt32::serialise (output, m_rank);

    const auto label_size =
      static_cast<std::uint8_t> (m_label.size ());

    /* Fill in header byte fields */
    output [header_off] =
        label_size
      | (is_leaf << BIT_IS_LEAF)
      | (static_cast<std::uint8_t> (offset_encoding) << BIT_OFFSET)
      | (static_cast<std::uint8_t> (rank_encoding) << BIT_RANK);

    /* Append label bytes */
    std::copy (m_label.begin (), m_label.end (),
	       back_inserter (output));

    /* Only on leaves: append metadata encoding */
    if (m_metadata)
    {
      Serialise<T>::serialise (
        output,
	m_metadata.get ());
    }    
  }

  /*********************************************************/
  static void
  serialise_siblings (std::vector<std::uint8_t> &output,
		      std::vector<self_t>       siblings,
		      const std::uint64_t       base_rank = 0)
  {
    /*
     * Estimate total space taken by the encoding
     */
    const auto estimated_encoding_size = [&]
    {
      auto result =
        boost::size (siblings) * max_node_size ();

      for (const auto &node : siblings)
      {
	result += node.m_subtree_serialise.size ();
      }

      return result;
    } ();

    /*
     * First part of serialisation consists of concatenation
     * of all node's header
     */
    const auto initial_size = output.size ();
    const auto first_node = std::begin (siblings);
    const auto min_rank   = first_node->m_rank;
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

      this_node->m_rank -= prev_rank;

      this_node->serialise_header (
        output,
	children_offset);

      prev_node = this_node;
      prev_rank = current_rank;
    }

    /*
     * We can serialise the first children only after
     * all the other siblings, as this requires to know
     * total size of their headers
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
     * Second part of the serialisation consists of,
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
 
  /*********************************************************/
  void add_children (std::vector<self_t> siblings)
  {
    BOOST_ASSERT (!m_metadata);
    BOOST_ASSERT (!siblings.empty (),
		  "Internal node with empty children");

    /*
     * If there is a single children node, check possibility to
     * collapse label if their size can fit into single label
     */
    if (boost::size (children) == 1)
    {
      auto &child = children.front ();

      if ((child.m_label.size () + m_label.size ()) < max_label_size)
      {
	/* append label and copy metadata */

	m_label.insert (m_label.end (),
			child.label ().begin (),
			child.label ().end ());

	m_subtree_serialised = std::move (child.m_subtree_serialised);
	m_rank               = child.m_rank;
	m_metadata           = std::move (child.m_metadata);
	return;
      }
    }

    /*
     * Proceed appending serialisation of children.
     */
    boost::sort (siblings);

    m_rank = siblings.begin ()->m_rank;

    serialise_siblings (
      m_subtree_serialised,
      siblings,
      m_rank);
  }

  static constexpr size_t max_node_size ()
  {
    return
      2 * VarInt64::max_codeword_size ()  +
      max_label_size                      +
      Serialise<T>::estimated_max_size () +
      1;
  }

private:

  std::string m_label;
  std::uint64_t m_rank;
  boost::optional<T> m_metadata;
  std::vector<std::uint8_t> m_subtree_serialised;
};

/***********************************************************/
/**
 * Trie implementation
 */
template<typename T>
class TrieImpl
{
public:

  using metadata_type = T;
  using node_type = Node<T>;

  TrieImpl (std::vector<std::uint8_t> data)
    : m_data (data)
    , m_root (data.c_data ())
  {}

  static constexpr const char* version_id ()
  {
    return "1.0.0";
  }

  const node_type &root () const
  {
    return m_root;
  }

private:
  std::vector<std::uint8_t> m_data;
  Node<T> m_root;
};

} // namespace v0 {

}} // namespace ordered_trie { namespace detail {
