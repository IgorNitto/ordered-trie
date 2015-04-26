/**
 * @file  detail_make_ordered_trie.inl
 * @brief
 */

#include "serialise.h"

#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/sort.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/range/size.hpp>

namespace ordered_trie { namespace detail {

/************************************************************/
template <typename Encoder>
void
ConcreteNode<Encoder>::serialise_header (
  std::vector<std::uint8_t> &out,
  size_t                     children_offset)
{
  if (is_leaf ())
  {
    Encoder::serialise_leaf (out,
			     children_offset,
			     m_rank,
			     {m_label.data (), m_label.size ()},
			     *m_payload_data);
  }
  else
  {
    Encoder::serialise_internal (out,
				 children_offset,
				 m_rank,
				 {m_label.data (), m_label.size ()});
  }
};

/************************************************************/
template<typename Encoder>
template<typename FwdRange>
inline void
ConcreteNode<Encoder>::append_children (FwdRange &children)
{
  BOOST_ASSERT (m_subtree_serialised.empty ());
  BOOST_ASSERT_MSG (!boost::empty (children),
		    "Invalid empty range of children");

  if (boost::size (children) == 1)
  {
    /*
     * In case of unique children, evaluate possibility to
     * collapse label if their size can fit into single label
     */
    auto &child = *std::begin (children);

    if ((child.label ().size () + m_label.size ()) < max_label_size)
    {
      /* append label and copy metadata */

      m_label.insert (m_label.end (),
		      child.label ().begin (),
		      child.label ().end ());

      m_subtree_serialised = std::move (child.m_subtree_serialised);
      m_rank               = child.rank ();
      m_payload_data       = std::move (child.m_payload_data);
      return;
    }
  }

  m_rank = serialise_siblings (m_subtree_serialised, children);
}

/************************************************************/

template<typename Encoder>
template<typename Input>
inline std::uint64_t
ConcreteNode<Encoder>::serialise_siblings (
  std::vector<std::uint8_t> &output,
  Input                     &siblings)
{
  constexpr size_t max_node_size =
    max_encoding_size +
    Serialise<payload_t>::estimated_max_size ();

  /*
   * Make room for serialisation of children headers
   */
  const size_t initial_size = output.size ();
  output.reserve ((boost::size (siblings) * max_node_size)
		  + initial_size);

  /*
   * First part of serialisation consists of concatenation
   * of all node's header, ordered by _increasing_ rank value
   */
  boost::sort (siblings, 
	       [] (const self_t &lhs, const self_t &rhs)
	       {
		 return (lhs.rank () < rhs.rank ());
	       });
 
  const auto first_node = std::begin (siblings);
  const auto min_rank   = first_node->rank ();
  	auto prev_node  = first_node;
	auto base_rank  = first_node->rank ();

  for (auto this_node  = std::next (first_node);
  	    this_node != std::end (siblings); 
  	  ++this_node)
  {
    const auto children_offset =
      prev_node->m_subtree_serialised.size ();

    const auto current_rank = this_node->rank ();

    this_node->m_rank  -= base_rank;
    this_node->serialise_header (output, children_offset);

    prev_node = this_node;
    base_rank = current_rank;
  }

  /*
   * We can serialise the first children only after
   * the remaining siblings, as this requires to know
   * total size of the headers
   */
  const size_t total_headers_size = output.size () - initial_size;

  /*
   * Append first node, then perform a rotate to move its encoding
   * before all the other siblings
   */
  first_node->m_rank = 0;
  first_node->serialise_header (output, total_headers_size);

  std::rotate (output.begin () + initial_size,
	       output.begin () + initial_size + total_headers_size,
	       output.end ());

  /*
   * Second part of the serialisation consists of,
   * concatenation of all sub-tries serialisation
   */
  for (auto &&node: siblings)
  {
    output.insert (output.end (),
		   node.m_subtree_serialised.begin (),
		   node.m_subtree_serialised.end ());

    node.m_subtree_serialised.clear ();
  }

  return min_rank;
}

/************************************************************/
template<typename Encoder>
ConcreteNode<Encoder>
ConcreteNode<Encoder>::Leaf (char             leading_char,
			     std::uint64_t    rank,
			     const payload_t &payload)
{
  auto node = Leaf (rank, payload);
  node.m_label.insert (node.m_label.begin (), leading_char);
  return node;
}

/************************************************************/
template<typename Encoder>
ConcreteNode<Encoder>
ConcreteNode<Encoder>::Leaf (std::uint64_t    rank,
			     const payload_t &payload)
{
  return self_t {rank, payload};
}

/************************************************************/
template<typename Encoder>
ConcreteNode<Encoder>
ConcreteNode<Encoder>::Internal (std::string         label,
			         std::vector<self_t> children)
{
  BOOST_ASSERT (label.size () < max_label_size);

  self_t result;

  result.m_label.insert (
    result.m_label.end (),
    label.begin (),
    label.end ());
  
  result.append_children (children);
  return result;
}

}} // namespace ordered_trie { namespace detail {
