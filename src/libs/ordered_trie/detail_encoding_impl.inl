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

namespace detail {

/*
 * First byte of node's encoding is an header storing
 * the type of encoding used to represent all node component:
 * rank, subtrie size and label size.
 *
 * @TODO: Sub-optimal in case of leaves encoding
 *        (offset redundant in that case)
 * 
 * Possible implementation:
 * Notice: avoid bit fields for portability
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

/*****************************************************************/
/* 
 * Helper functionalities for node's header decoding 
 */
inline VarInt32::wordsize_t rank_encoding (std::uint8_t n)
{
  return static_cast<VarInt32::wordsize_t> (
          (n & RANK_MASK) >> BIT_RANK);
} 

inline VarInt64::wordsize_t offset_encoding (std::uint8_t n)
{
  return static_cast<VarInt64::wordsize_t> (
          (n & OFFSET_MASK) >> BIT_OFFSET);
}

inline std::uint8_t label_size (std::uint8_t n)
{
  return n & LABEL_MASK;
}

inline bool is_leaf (std::uint8_t n)
{
  return n & IS_LEAF_MASK;
}

} // namespace detail {

/**
 * Build node encoding as concatenation of the following parts:
 *
 * - A single byte node description header
 * - Offset to first children (incrementally encoded) 
 * - Node ranking score (incrementally encoded)
 * - String associated to inbound edge, aka label
 */
template<typename PayLoad> struct Impl
{
  using metadata_t = PayLoad;

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

  static void
  serialise_base_ (std::vector<std::uint8_t> &output,
		   const std::size_t          children_offset,
		   const std::uint64_t        rank,
		   const boost::string_ref    label,
		   const bool                 is_leaf)
  {
    using namespace detail;

    if (label.size () >= max_label_size ())
    {
      throw std::runtime_error ("Exceeded maximum label size");
    }

    output.reserve (output.size () + max_encoding_size ());
    output.push_back (0);
    const auto header_off = output.size () - 1;

    /* Stream out header fields: offset, rank, label */
    const auto offset_encoding = VarInt64::serialise (output, children_offset);
    const auto rank_encoding   = VarInt32::serialise (output, rank);
    const auto label_size      = static_cast<std::uint8_t> (label.size ());

    /* Fill in header byte fields */
    output [header_off] =
        label_size
      | (is_leaf << BIT_IS_LEAF)
      | (static_cast<std::uint8_t> (offset_encoding) << BIT_OFFSET)
      | (static_cast<std::uint8_t> (rank_encoding) << BIT_RANK);

    /* Append label bytes  */
    std::copy (label.begin (), label.end (), back_inserter (output));
  }

  /***********************************************************/
  static void
  serialise_internal (std::vector<std::uint8_t> &output,
		      std::size_t                children_offset,
		      std::uint64_t              rank,
		      boost::string_ref          label)
  {
    serialise_base_ (output, children_offset, rank, label,
		     false /* internal node */);
  }

  static void
  serialise_leaf (std::vector<std::uint8_t> &output,
		  std::size_t                children_offset,
		  std::uint64_t              rank,
		  boost::string_ref          label,
		  const PayLoad             &metadata)
  {
    serialise_base_ (output, children_offset, rank, label,
		     true /* leaf node */);

    /* Append metadata encoding */

    Serialise<PayLoad>::serialise (output, metadata);
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

    /************************************************************/

    boost::string_ref label () const
    {
      return {label_data (), detail::label_size (*m_data)};
    }

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
      return detail::is_leaf (*m_data);
    }

    /*********************************************************/

    inline std::size_t size () const
    {
      auto result = header_size ();
  
      if (is_leaf ())
      {
	auto payload_end = m_data + header_size ();
	result += Serialise<PayLoad>::next (payload_end)
		  - payload_end;
	            
      }

      return result;
    }

    /*********************************************************/

    inline std::size_t children_offset () const
    {
      return VarInt64::deserialise (m_data + 1,
				    detail::offset_encoding (*m_data));

    }

    /*********************************************************/

    inline const std::uint8_t *data () const
    {
      return m_data;
    }

    /*********************************************************/

    inline std::uint8_t *payload_data () const
    {
      if (!is_leaf ())
      {
	throw std::logic_error ("Attempt accessing payload data"
				" over non-leaf node");
      }

      return m_data + header_size ();
    }

  private:

    inline std::size_t header_size () const
    {
      return
        VarInt32::codeword_size (detail::rank_encoding (*m_data)) +
        VarInt64::codeword_size (detail::offset_encoding (*m_data)) +
       	detail::label_size (*m_data) +
	1;
    }

  private:
    const std::uint8_t *m_data;
  };
};

} // namespace v0 {

}} // namespace ordered_trie { namespace encoders {
