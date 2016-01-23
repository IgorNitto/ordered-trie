#ifndef ORDERED_TRIE_DETAIL_TRIE_IMPL_H
#define ORDERED_TRIE_DETAIL_TRIE_IMPL_H

/**
 * @file  detail_trie_impl.h
 * @brief Some abstractions on lower level implementation
 *        details affected by data layout and encoding.
 */

#include <boost/range.hpp>
#include <boost/utility/string_ref.hpp>

#include <vector>

namespace ordered_trie { namespace detail {

/**
 * Expose view over information associated to a trie node.
 */
template<typename T>
class Node
{
public:

  class SiblingsRange;

  /* Ctor from base address */
  explicit Node (const std::uint8_t *);

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
   * Returns range of direct children nodes
   * ordered by increasing score
   */
  SiblingsRange children () const;
};

/**
 * Recursive trie builder
 */
template<typename T>
class MakeTrie
{
public:

  /**
   * Make empty trie
   */
  MakeTrie () {};

  /**
   * Make leaf node
   */
  MakeTrie (std::string label,
	    std::size_t rank,
	    T           metadata = {});

  /**
   * Make internal node
   */
  MakeTrie (std::string              label,
	    std::vector<MakeTrie<T>> children);

  /**
   * Append serialisation of a range of siblings trie
   * sorted by _increasing_ score to the output vector
   */
  static void
  serialise_trie (std::vector<std::uint8_t> &output,
		  std::vector<MakeTrie<T>>   siblings);
}; 

/**
 * Trie implementation
 */
template<typename T>
class TrieImpl
{
public:

  using metadata_type = T;
  using node_type = NodeImpl<T>;

  /**
   * Unique version tag associated to this encoding
   */
  static constexpr const char* version_id ();

  /**
   * Get reference to root node of trie
   */
  const node_type &root () const;
};

}} // namespace ordered_trie { namespace detail {

#include "detail_trie_impl.inl"

#endif
