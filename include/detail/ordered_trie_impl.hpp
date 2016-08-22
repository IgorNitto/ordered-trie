#ifndef DETAIL_ORDERED_TRIE_IMPL_HPP
#define DETAIL_ORDERED_TRIE_IMPL_HPP

/**
 * @file  detail_ordered_trie_impl.hpp
 */

#include "ordered_trie_iterator.hpp"
#include "ordered_trie_builder.hpp"

#include <boost/assert.hpp>

#include <limits>

namespace ordered_trie {
namespace detail {

/***********************************************************/

template<typename T = Void>
auto make_trie_root (const std::uint8_t *data) -> Node<T>
{
  return Node<T> {data, 0u, Node<T>::skip (data)};
}  

/***********************************************************/
  
template<typename Score>
auto make_empty_trie ()
  -> const std::vector<std::uint8_t>&
{
  static const auto result = [&]
  {
    return make_serialised_ordered_trie (
      std::vector<std::pair<std::string, std::uint64_t>>{});
  } ();

  return result;
}

/***********************************************************/

template<typename Node, typename FwdIt>
Node prefix_match (Node  locus,
		   FwdIt &first,
		   const FwdIt last)
{
  while (first != last)
  {
    auto children_it = find_sibling (
      visit_children (locus), [&first] (const Node &node)
      {
  	return node.label_size () &&
     	       *node.label_begin () == *first;
      });

    if (children_it)
    {
      locus = *children_it;
      const auto *label_ptr = children_it->label_begin() + 1;
      const auto *label_end = children_it->label_begin() +
      	                      children_it->label_size();
      ++first;

      while ((label_ptr != label_end) && (first != last))
      {
	if (*first != *label_ptr)
	{
	  return locus; 
	}

	++label_ptr;
	++first;
      }
    }
    else
    {
      break;
    }
  }

  return locus;
}

/***********************************************************/

template<typename Node, typename FwdIt>
bool find_leaf (Node &locus,
		FwdIt first,
		const FwdIt last)
{
  while (first != last)
  {
    auto children_it = find_sibling (
      visit_children (locus), [&first] (const Node &node)
      {
  	return node.label_size () &&
     	       *node.label_begin () == *first;
      });
    
    if (children_it)
    {
      locus = *children_it;
      const auto *label_ptr = children_it->label_begin() + 1;
      const auto *label_end = children_it->label_begin() +
	                      children_it->label_size();
      ++first;

      while ((label_ptr != label_end) && (first != last))
      {
	if (*first != *label_ptr)
	{
	  return false; 
	}

	++label_ptr;
	++first;
      }

      if ((label_ptr != label_end) && (first == last))
      {
	return false;
      }
    }
    else
    {
      return false;
    }
  }

  if (locus.is_leaf ())
  {
    return true;
  }
      
  auto leaf_it = find_sibling (
    visit_children (locus),
    [] (const Node &node)
    {
  	return (node.is_leaf () && !node.label_size());
    });

  if (leaf_it)
  {
    locus = *leaf_it;
    return true;
  }
  
  return false;
}

} // namespace detail {

/**
 * Store template parameters
 */
template<typename S>
class OrderedTrie<S>::Parameters
{
public:
  using Score = S;
  using ScoreSerialiser = Serialise<S>;
};

/**
 * Iterate over set of leaves contained in subtrie
 * rooted at given node in order of increasing score
 * and return a completion object for each of them
 */
template<typename Score>
class OrderedTrie<Score>::iterator
  : public boost::iterator_facade< 
     /* CRTP       */ typename OrderedTrie<Score>::iterator,
     /* value_type */ typename OrderedTrie<Score>::value_type,
     /* category   */ boost::forward_traversal_tag,
     /* reference  */ typename OrderedTrie<Score>::value_type>
{
public:

  using Node = typename OrderedTrie<Score>::Node;

  explicit iterator (
    const OrderedTrie<Score> &owner)
      : m_visitor {}
      , m_trie {&owner}
  {
  }
  
  explicit iterator (
    detail::SiblingsIterator<Node> siblings_range,
    const OrderedTrie<Score> &owner)
      : m_visitor {siblings_range}
      , m_trie {&owner}
  {
  }
  
private:

  friend class boost::iterator_core_access;
  
  OrderedTrie<Score>::value_type dereference () const
  {
    return leaf_to_suggestion (*m_visitor);
  }

  bool equal (const iterator &other) const
  {
    return (m_trie == other.m_trie) &&
           (m_visitor == other.m_visitor);
  }

  void increment ()
  {
    ++m_visitor;
  }

private:

  auto leaf_to_suggestion (const Node &leaf) const
    -> typename OrderedTrie<Score>::value_type
  {
    typename OrderedTrie<Score>::value_type result;

    detail::traverse_descending_path (
      m_trie->m_root, leaf,
      [&result] (const Node &n)
      {
	result.first.append (
          n.label_begin (),
	  n.label_begin () + n.label_size ());
      });

    const auto score_address =
      m_trie->m_score_table + leaf.rank ();
    
    result.second = deserialise<Score> (score_address);	  
    return result;
  }
  
private:
  detail::OrderedLeavesIterator<Node> m_visitor;
  const OrderedTrie<Score> *m_trie;
};
  
/***************************************************/
/**
 * Internal template parameters passed to store layer
 */
template<typename Score>
template<typename FwdIt, typename Comparer>
OrderedTrie<Score>::
OrderedTrie (FwdIt begin_suggestions,
	     FwdIt end_suggestions,
	     const Comparer &score_comparer)
{
  using namespace ordered_trie::detail;

  const auto suggestions =
    boost::make_iterator_range (
      begin_suggestions,
      end_suggestions);

  std::vector<std::uint8_t> serialised_scores;

  const auto score_map = serialise_scores (
    serialised_scores,
    suggestions,
    score_comparer);

  auto serialised_trie =
    make_serialised_ordered_trie (
      suggestions,
      [&] (const Score &score) -> std::uint64_t
      {
	return score_map.at (score);
      });

  m_store = Store::from_memory (
    std::move (serialised_trie),
    std::move (serialised_scores));
  
  m_score_table = m_store->score_table_data ().first;
  m_root = detail::make_trie_root (m_store->trie_data ().first);
}

/***************************************************/

template<typename Score>
template<typename FwdIt>
OrderedTrie<Score>::
OrderedTrie (FwdIt begin_suggestions,
	     FwdIt end_suggestions)
  : OrderedTrie<Score> (begin_suggestions,
                        end_suggestions,
			std::greater<Score> {})
{
}

/***************************************************/

template<typename Score>
OrderedTrie<Score>::
OrderedTrie (const std::initializer_list<value_type> &values)
  : OrderedTrie<Score> (values.begin (),
			values.end ())
{
}

/***************************************************/

template<typename Score>
template<typename Cmp>
OrderedTrie<Score>::
OrderedTrie (const std::initializer_list<value_type> &values,
	     const Cmp &comparer)
  : OrderedTrie<Score> (values.begin (),
			values.end (),
			comparer)
{
}


/***************************************************/

template<typename Score>
OrderedTrie<Score>::OrderedTrie ()
{
  m_store = Store::from_memory (
    detail::make_empty_trie<Score> ());

  m_score_table = nullptr;
  m_root = detail::make_trie_root (m_store->trie_data ().first);
}

/***************************************************/

template<typename Score>
OrderedTrie<Score>::OrderedTrie (
  std::shared_ptr<const Store> store)
{
  const auto trie_data = store->trie_data ().first;
  BOOST_ASSERT (trie_data);

  if (trie_data)
  {
    m_root = detail::make_trie_root (trie_data);
    m_score_table = store->score_table_data ().first;
    m_store = store;
  }
}

/***************************************************/

template<typename Score>
bool OrderedTrie<Score>::empty () const
{
  return m_root.is_leaf ();
}

/***************************************************/

template<typename Score>
auto OrderedTrie<Score>::begin () const
  -> iterator
{
  return iterator
  {
    detail::visit_children (m_root),
    *this
  };
}

/***************************************************/

template<typename Score>
auto OrderedTrie<Score>::end () const
  -> iterator
{
  return iterator {*this};
}

/***************************************************/
template<typename Score>
auto OrderedTrie<Score>::complete (
  const std::string &prefix) const
  -> boost::iterator_range<iterator>
{
  return complete (prefix.begin (), prefix.end ());
}

/***************************************************/

template<typename Score>
template<typename FwdIt>
auto OrderedTrie<Score>::complete (FwdIt first, FwdIt last) const
  -> boost::iterator_range<iterator>
{
  if (!empty ())
  {
    auto match_node =
      detail::prefix_match (m_root, first, last);

    if (first == last)
    {
      detail::SiblingsIterator<Node> search_loc
      {
	match_node,
	Node::skip (match_node.data ())
      };

      return boost::make_iterator_range (
	iterator {search_loc, *this},
	iterator {*this});
    }
  }

  return boost::make_iterator_range (
    iterator {*this},
    iterator {*this});
}

/***************************************************/
template<typename Score>
size_t OrderedTrie<Score>::mismatch (
  const std::string &prefix) const
{
  auto f = prefix.begin ();
  detail::prefix_match (m_root, f, prefix.end ());
  return std::distance (prefix.begin (), f);
}
  
/***************************************************/

template<typename Score>
template<typename FwdIt>
FwdIt
OrderedTrie<Score>::mismatch (FwdIt first,
			      const FwdIt last) const
{
  detail::prefix_match (m_root, first, last);
  return first;
}

/***************************************************/

template<typename Score>
size_t OrderedTrie<Score>::count (const std::string &prefix) const
{
  return count (prefix.begin (), prefix.end ());
}

/***************************************************/

template<typename Score>
template<typename FwdIt>
size_t OrderedTrie<Score>::count (FwdIt first,
				  const FwdIt last) const
{
  auto locus = m_root;
  
  if (detail::find_leaf (locus, first, last))
  {
    return (locus == m_root) ? 0u : 1u;
  }
  
  return 0u;
}
  
/***************************************************/

template<typename Score>
template<typename FwdIt>
Score OrderedTrie<Score>::score (FwdIt first,
				 const FwdIt last) const
{
  auto locus = m_root;
  const auto found =
    detail::find_leaf (locus, first, last);
  
  if (!found || (locus == m_root))
  {
    throw std::logic_error (
      "No leaf node associated to input suggestion");
  }
  
  return deserialise<Score> (m_score_table + locus.rank());
}

/***************************************************/

template<typename Score>
template<typename FwdIt>
bool OrderedTrie<Score>::score (Score &output,
				FwdIt first,
				const FwdIt last) const
{
  auto locus = m_root;

  if (detail::find_leaf (locus, first, last) &&
      !(locus == m_root))
  {
    output = deserialise<Score> (m_score_table + locus.rank());
    return true;
  }

  return false;
}

/***************************************************/

template<typename Score>
bool OrderedTrie<Score>::score (
  Score &output,
  const std::string &suggestion) const
{
  return score (output, suggestion.begin (), suggestion.end ());
}

/***************************************************/

template<typename Score>
Score OrderedTrie<Score>::score (
  const std::string &suggestion) const
{
  return score (suggestion.begin (), suggestion.end ());
}

/***************************************************/

template<typename Score>
void OrderedTrie<Score>::write (const std::string &path) const
{
  m_store->write (path);
}

/***************************************************/

template<typename Score>
auto OrderedTrie<Score>::read (const std::string &path)
  -> OrderedTrie<Score>
{
  return OrderedTrie<Score> {Store::from_file (path)};
}

/***************************************************/

template<typename FwdRange, typename Comparer>
auto make_ordered_trie (const FwdRange &suggestions,
			const Comparer &score_comparer)
{
  using Suggestion =
    typename boost::range_value<FwdRange>::type;

  using Score =
    typename std::tuple_element<1, Suggestion>::type;

  return OrderedTrie<Score>
  {
    std::begin (suggestions),
    std::end (suggestions),
    score_comparer
  };
}

/***************************************************/

template<typename FwdRange>
auto make_ordered_trie (const FwdRange &suggestions)
{
  return make_ordered_trie (suggestions,
			    std::greater<> {});
}
  
} // namespace ordered_trie {

#endif
