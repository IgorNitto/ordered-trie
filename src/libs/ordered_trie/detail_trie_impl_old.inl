/**
 * @file  node_encoding_impl.inl
 * @brief
 */

#include "detail_varint.h"
#include "serialise.h"

#include <stdexcept>

namespace ordered_trie { namespace encoders {

namespace v0 {

using ordered_trie::detail::VarInt32;
using ordered_trie::detail::VarInt64;

/*
 * First byte of node's encoding is an header storing
 * the type of encoding used to represent all node component:
 * rank, subtrie size and label size.
 *
 * @TODO: Sub-optimal in case of leaves encoding
 *        (offset redundant in that case)
 * 
 * Possible implementation:
 * Note: avoid bit fields for portability
 *
 * @code
 * struct NodeHeader
 * {
 *    VarInt32::wordsize_t rank_encoding   : 2;
 *    VarInt64::wordsize_t offset_encoding : 2;
 *    bool                 is_leaf         : 1;
 *    std::uint8_t         label_size      : 3;
 * };
 * @endcode
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

/**
 * Build node encoding as concatenation of the following parts:
 *
 * - A single byte node description header
 * - Offset to first children (incrementally encoded) 
 * - Node ranking score (incrementally encoded)
 * - String associated to inbound edge, aka label
 */
struct Impl
{
  static constexpr const char* format_str ()
  {
    return "ENCODER_VER=0";
  }

  static constexpr size_t max_label_size ()
  {
    return 7;
  }

  static constexpr size_t max_encoding_size ()
  {
    return (2 * VarInt64::max_codeword_size ()) +
           max_label_size ()                    +
           1;
  }

  /***********************************************************/

  template<typename MetaData>
  static void
  serialise (std::vector<std::uint8_t>    &output,
	     const ConcreteNode<MetaData> &node)
  {
    using namespace detail;

    if (node.label.size () >= max_label_size ())
    {
      throw std::runtime_error ("Exceeded maximum label size");
    }

    output.reserve (output.size () + max_encoding_size ());
    output.push_back (0);
    const auto header_off = output.size () - 1;

    /* Stream out header fields: offset, rank, label */
    const auto offset_encoding =
      VarInt64::serialise (output, node.children_offset);

    const auto rank_encoding =
      VarInt32::serialise (output, node.rank);

    const auto label_size =
      static_cast<std::uint8_t> (node.label.size ());

    /* Fill in header byte fields */
    output [header_off] =
        label_size
      | (is_leaf << BIT_IS_LEAF)
      | (static_cast<std::uint8_t> (offset_encoding) << BIT_OFFSET)
      | (static_cast<std::uint8_t> (rank_encoding) << BIT_RANK);

    /* Append label bytes  */
    std::copy (node.label.begin (), node.label.end (),
	       back_inserter (output));

    /* On leaf nodes only: append metadata encoding */
    if (node.is_leaf ())
    {
      Serialise<MetaData>::serialise (output,
				      node.metadata.get ());
    }
  }

  /*************************************************************/

  class View
  {
  public:

    using Encoding = Impl;

    /* ctors  */
    explicit View (const std::uint8_t *address)
      : m_data (address)
    {};

    View (const View&)           = default;
    View& operator=(const View&) = default;

    /*********************************************************/

    inline const char *label_data () const
    {
      return
	reinterpret_cast<const char*> (m_data) + 
	VarInt32::codeword_size (detail::rank_encoding (*m_data)) +
	VarInt64::codeword_size (detail::offset_encoding (*m_data)) +
	1;
    }

    /*********************************************************/

    inline std::size_t label_size () const
    {
      return (*m_data) & LABEL_MASK;
    }

    /*********************************************************/

    std::uint64_t rank () const
    {
      const auto rank_offset =
	VarInt64::codeword_size (detail::offset_encoding (*m_data));

      return VarInt32::deserialise (m_data + 1 + rank_offset,
				    detail::rank_encoding (*m_data));
    }

    /*********************************************************/

    bool is_leaf () const
    {
      return (*m_data) & IS_LEAF_MASK;
    }

    /*********************************************************/

    inline std::size_t size () const
    {
      return
        VarInt32::codeword_size (rank_encoding ()) +
        VarInt64::codeword_size (offset_encoding ()) +
       	label_size () +
	1;
    }

    /*********************************************************/

    inline std::size_t children_offset () const
    {
      return VarInt64::deserialise (
        m_data + 1,
	detail::offset_encoding (*m_data));
    }

    /*********************************************************/

    inline const std::uint8_t *data () const
    {
      return m_data;
    }

    /*********************************************************/

    inline std::uint8_t *encoding_end () const
    { 
      return label_data () + label_size ();
    }

  private:

    /*****************************************************************/
    /* 
     * Helper functionalities for node's header decoding 
     */
    inline VarInt32::wordsize_t rank_encoding ()
    {
      return static_cast<VarInt32::wordsize_t> (
	       ((*m_data) & RANK_MASK) >> BIT_RANK);
    } 

    inline VarInt64::wordsize_t offset_encoding (std::uint8_t n)
    {
      return static_cast<VarInt64::wordsize_t> (
	      ((*m_data) & OFFSET_MASK) >> BIT_OFFSET);
    }

  private:
     const std::uint8_t *m_data;
  };
};

} // namespace v0 {

}} // namespace ordered_trie { namespace encoders {
