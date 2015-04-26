/**
 * @file  detail_encoding.h
 * @brief Define trie structural encoding policy
 */

#ifndef ORDERED_TRIE_DETAIL_ENCODING_H
#define ORDERED_TRIE_DETAIL_ENCODING_H

#include "detail_payload_types.h"

#include <boost/utility/string_ref.hpp>
#include <boost/optional.hpp>

#include <vector>

#include "detail_encoding_impl.inl"

namespace ordered_trie {

/**
 * Tag type indicating if internal structure offset
 * and node score are differentially encoded
 */
struct DeltaEncTag {};
struct AbsoluteEncTag {};

/**
 * Describe generic node encoding policy 
 * (concrete implementations in @file node_impl.inl)
 */
template<typename PayLoad> struct EncodingPolicy
{
  /**
   * Expose metadata type
   */
  using metadata_t = PayLoad;

  /**
   * Unique version tag associated to the encoding
   */
  static constexpr const char* format_str ();

  /**
   * Maximum label size supported by this encoding
   */
  static constexpr size_t max_label_size ();

  /**
   * Maximum size of node's encoding in char
   * (excluding payload encoding)
   */
  static constexpr size_t max_encoding_size ();

  /**
   * Serialise internal node from components
   */
  static void
  serialise_internal (std::vector<std::uint8_t> &output,
		      std::size_t                children_offset,
		      std::uint64_t              rank,
		      boost::string_ref          label);

  /**
   * Serialise leaf node from components
   */
  static void
  serialise_leaf (std::vector<std::uint8_t> &output,
		  std::size_t                children_offset,
		  std::uint64_t              rank,
		  boost::string_ref          label,
		  const PayLoad             &metadata);

  /**
   * Offer a non-modifiable view over encoded node
   */
  class View
  {
  public:

    /* Ctor from base address */
    explicit View (const std::uint8_t *address)
      : m_data (data)
    {};

    /* Ctor from base encoding address */
    View (const View&)           = default;
    View& operator=(const View&) = default;

    /**
     * String label associated to incoming edge
     */
    boost::string_ref label () const;

    /**
     * Pointer to first char of label, if label is not empty.
     * Undefined otherwise.
     */
    inline const char *label_data () const;

    /**
     * Ranking score
     */
    std::uint64_t rank () const;

    /**
     * Test for leaf node
     */
    bool is_leaf () const;

    /**
     * Full encoding length in byte of underlying node
     * (including payload data encoding)
     */
    inline std::size_t size () const;

    /**
     * Access offset of underlying node
     */
    inline std::size_t children_offset () const;

    /**
     * Base address
     */
    inline std::uint8_t *data () const;

    /**
     * Base address of payload data
     *
     * @throws std::logic_error on non-leaf nodes.
     */
    inline std::uint8_t *payload_data () const;

  private:
    const std::uint8_t *m_data;
  };
};

/**
 * Collection of different encoder implementations
 */
namespace encoders
{

/**
 * Alias to default encoder implementation
 */
template<typename T> using Default = v0::Impl<T>;

} // namespace encoders {

} // namespace ordered_trie {

#endif // ORDERED_TRIE_DETAIL_ENCODING_H
