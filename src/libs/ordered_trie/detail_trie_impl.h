#ifndef ORDERED_TRIE_DETAIL_TRIE_IMPL_H
#define ORDERED_TRIE_DETAIL_TRIE_IMPL_H

/**
 * @file  detail_trie_impl.h
 * @brief Some abstractions on lower level implementation
 *        details affected by data layout and encoding.
 */

#include "detail_iterator.h"

#include <boost/optional.hpp>
#include <boost/range.hpp>
#include <boost/utility/string_ref.hpp>

#include <vector>

namespace ordered_trie { namespace detail {

inline namespace v0 {
    
/**
 * Expose view over information associated to a trie node.
 */
template<typename T>
class Node
{
public:

  using metadata_type = T;
  using SiblingsRange =
    boost::iterator_range<SiblingsIterator<Node<T>>>;

  /**
   * Ctor from base address
   */
  explicit Node (const std::uint8_t *) noexcept;

  /**
   * String label associated to this node
   */
  boost::string_ref label () const;

  /**
   * Get ranking score
   */
  std::uint64_t rank () const;

  /**
   * Returns true iff this is a leaf node
   */
  bool is_leaf () const;

  /**
   * If this node is a leaf get the associated
   * metadata, otherwise undefined behaviour
   */
  T metadata () const;

  /**
   * Range of children nodes ordered by increasing score
   */
  SiblingsRange children () const;

  bool operator== (const Node<T> &other) const;

  /* Implementation detail */
  void advance_to_sibling ();

private:
  Node () = default;
  Node (const std::uint8_t *data,
	const std::uint64_t base_score,
	const std::uint8_t *base_children) noexcept;

  std::uint64_t children_offset () const;
  const std::uint8_t* children_ptr () const;

  template<typename U>
  friend auto make_siblings_range (const Node<U>)
    -> typename Node<U>::SiblingsRange;

private:
  const std::uint8_t *m_data;
  std::uint64_t       m_score;
  const std::uint8_t *m_children;
};

/**
 * Trie implementation
 */
template<typename T>
class TrieImpl
{
public:

  using metadata_type = T;

  /**
   * Construct from serialisation
   */
  explicit TrieImpl (std::vector<std::uint8_t>);

  /**
   * Get reference to root node of trie
   */
  const Node<T> &root () const;

  /**
   * Unique version tag associated to this encoding
   */
  static constexpr const char* version_id ();

  const std::vector<std::uint8_t> &data () const
  {
    return m_data;
  };

private:
  std::vector<std::uint8_t> m_data;
  Node<T> m_root;
};

/**
 * Recursive trie builder
 */
template<typename T>
class MakeTrie
{
public:
  using metadata_type = T;
  static constexpr size_t max_label_size = 8;

  /**
   * Make empty trie
   */
  MakeTrie () = default;

  /**
   * Make leaf node
   */
  MakeTrie (std::string label,
	    std::size_t rank,
	    T           metadata);

  /**
   * Make internal node
   */
  MakeTrie (std::string              label,
	    std::vector<MakeTrie<T>> children);

  /**
   * Make root from childrennodes
   */
  explicit MakeTrie (std::vector<MakeTrie<T>> children);

  /**
   * Minimum scores associated to any leave in the subtrie
   */
  std::uint64_t min_score () const;
  
  /**
   * Label attached to root of contained subtrie
   */
  const std::string &label () const;

  /**
   * Add children nodes
   */
  void add_children (
    std::vector<MakeTrie<T>> siblings,
    const bool called_from_root = false);

  /**
   * Extract subtrie from this instance
   */
  TrieImpl<T> move_to_trie ();

  /**
   * Max encoding size of a single node 
   */
  static constexpr size_t max_node_size ();

private:
  void serialise_header (
    std::vector<std::uint8_t> &output,
    const size_t               children_offset) const;

  static void serialise_siblings (
    std::vector<std::uint8_t> &output,
    std::vector<MakeTrie<T>>  siblings,
    const std::uint64_t       base_rank = 0);

private:
  std::string m_label;
  std::uint64_t m_rank = 0;
  boost::optional<T> m_metadata;
  std::vector<std::uint8_t> m_subtree_serialised;
}; 

} // namespace v0 {

}} // namespace ordered_trie { namespace detail {

#include "detail_trie_impl.inl"

#endif
