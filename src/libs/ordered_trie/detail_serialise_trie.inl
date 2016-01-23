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
template<typename Parameters>
template<typename FwdRange>
inline void
SerialisedSiblings<Parameters>::append_children (
  FwdRange &children)
{
  constexpr auto max_labe_size =
    Parameters::encoding_type::max_label_size ();

  BOOST_ASSERT (m_subtree_serialised.empty ());
  BOOST_ASSERT_MSG (!boost::empty (children),
		    "Invalid empty range of children");

  if (boost::size (children) == 1)
  {
    /*
     * In case of unique children, evaluate possibility to
     * collapse label if their size can fit into single label
     */
    const auto &child_root = std::begin (children)->m_root;

    if ((child_root.label ().size () + m_label.size ()) <= max_label_size)
    {
      /* append label and copy metadata */

      m_label.insert (m_label.end (),
		      child_root.label ().begin (),
		      child_root.label ().end ());

      m_subtree_serialised = std::move (child_root.m_subtree_serialised);
      m_rank               = child_root.rank ();
      m_payload_data       = std::move (child_root.m_payload_data);
      return;
    }
  }

  m_rank = serialise_siblings (m_subtree_serialised, children);
}

/************************************************************/

template<typename Parameters>
template<typename Input>
inline std::uint64_t
SerialiseTree<Parameters>::serialise_siblings (
  std::vector<std::uint8_t> &output,
  Input                     &siblings)
{
  using MetaData = typename Parameters::metadata_type;
  using Encoding =
    typename Parameters::encoding_type<MetaData>;

  constexpr size_t max_node_size =
    Parameters::encoding_type::max_encoding_size () +
    Serialise<MetaData>::estimated_max_size ();

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
 
  const auto first_tree = std::begin (siblings);
  const auto min_rank   = first_tree->m_root.rank ();
  	auto prev_tree  = first_tree;
	auto base_rank  = first_tree->m_root.rank ();

  for (auto this_tree  = std::next (first_tree);
  	    this_tree != std::end (siblings); 
  	  ++this_tree)
  {
    auto &this_root = this_tree.m_root;

    const auto current_rank = this_root.rank;

    this_root.children_offset =
      prev_node->m_subtree_serialised.size ();

    this_root.rank -= base_rank;

    Encoding::serialise (output, this_root);

    prev_tree = this_tree;
    base_rank = current_rank;
  }

  /*
   * We can serialise the first children only after
   * all other siblings, as this requires to know
   * total size of the headers
   */
  const size_t total_headers_size = output.size () - initial_size;

  /*
   * Append first node, then perform a rotate to move its encoding
   * before all the other siblings
   */
  first_tree->m_root.rank = 0;
  first_tree->m_root.children_offset = total_headers_size;

  Encoding::serialise (output, first_tree->m_root);

  std::rotate (output.begin () + initial_size,
	       output.begin () + initial_size + total_headers_size,
	       output.end ());

  /*
   * Second part of the serialisation consists of
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

}} // namespace ordered_trie { namespace detail {
