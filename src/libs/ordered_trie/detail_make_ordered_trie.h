/**
 * @file  detail_make_trie.h
 * @brief Auxiliary tools for trie serialisation algorithm
 */

#ifndef ORDERED_TRIE_DETAIL_MAKE_TRIE_H
#define ORDERED_TRIE_DETAIL_MAKE_TRIE_H

#include <boost/container/static_vector.hpp>

#include <boost/assert.hpp>
#include <boost/optional.hpp>

namespace ordered_trie { namespace detail {

/*
 * Helper class for stack allocated small string
 */
template<size_t N>
using SmallString = boost::container::static_vector<char, N>;

/*
 * Plain representation of node components,
 * used by trie construction process
 */ 
template<typename Encoder> struct ConcreteNode
{
  using serialiser_t = Encoder;
  using self_t       = ConcreteNode<Encoder>;
  using payload_t    = typename Encoder::metadata_t;

  constexpr static auto max_label_size =
    Encoder::max_label_size ();

  constexpr static auto max_encoding_size =
    Encoder::max_encoding_size ();

  /* Construct internal node given label  */

  static self_t Internal (
    std::string         label,
    std::vector<self_t> children);

  /* Construct leaf with given single char label, rank and metadata */

  static self_t Leaf (char, std::uint64_t, const payload_t&);

  /* Construct leaf with emply label given rank and metadata */

  static self_t Leaf (std::uint64_t, const payload_t&);

  /* Accessors */  

  inline char leading_char () const
  {
    return *(std::begin (m_label));
  };

  inline bool is_leaf () const
  {
    return static_cast<bool> (m_payload_data);
  };

  inline std::uint64_t rank () const
  {
    return m_rank;
  };

  inline auto label () const
    -> const SmallString<max_label_size>&
  {
    return m_label;
  };

  inline const std::uint8_t *subtree_data () const
  {
    return m_subtree_serialised.data ();
  };

  /*
   * Move out serialised data hold by this object
   */
  inline std::vector<std::uint8_t> move_to_vector ()
  {
    m_subtree_serialised.shrink_to_fit ();
    return m_subtree_serialised;
  }

  /*
   * Serialise range of children nodes.
   * The input siblings are given as pair of iterator to the
   * first and last siblings node.
   */
  template<typename FwdRange>
  inline static std::uint64_t
  serialise_siblings (std::vector<std::uint8_t> &output,
		      FwdRange                  &siblings);

  /*
   * Append encoding of range of children nodes to
   * currently serialised subtree
   */
  template<typename FwdRange>
  inline void append_children (FwdRange &childrens);

private:

  ConcreteNode () = default; //< Only used internally

  ConcreteNode (std::uint64_t rank, const payload_t &data)
    : m_rank (rank), m_payload_data (data)
  {};

  void serialise_header (std::vector<std::uint8_t> &out,
			 size_t         children_offset);

private:

  SmallString<max_label_size>  m_label;
  std::uint64_t                m_rank;
  std::vector<std::uint8_t>    m_subtree_serialised;
  boost::optional<payload_t>   m_payload_data;
};

}} // namespace detail { namespace serialise {

#include "detail_make_ordered_trie.inl"

#endif // DETAIL_ORDERED_TRIE_MAKE_TRIE_H
