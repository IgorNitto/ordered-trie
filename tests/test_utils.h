#ifndef TEST_UTILS_H
#define TEST_UTILS_H

#include "detail/ordered_trie_builtin_serialise.hpp"
#include "detail/ordered_trie_node.hpp"

#include <boost/filesystem.hpp>
#include <boost/range.hpp>

#include <iterator>
#include <tuple>

namespace ordered_trie {
namespace test_utils {
  
template<typename T = Void>
auto make_root (const std::uint8_t *data) -> detail::Node<T>
{
  return detail::Node<T> {data, 0u, detail::Node<T>::skip (data)};
} 

template<typename InputRange>
auto make_vector (const InputRange input)
{
  using value_type =
    typename boost::range_value<InputRange>::type;

  return std::vector<value_type> {
    std::begin (input),
    std::end (input)};
}    

class TemporaryFile
{
public:

  TemporaryFile ()
    : m_path (boost::filesystem::unique_path ())
  {
  }

  std::string get () const
  {
    return m_path.string ();
  }

  ~TemporaryFile ()
  {
    boost::filesystem::remove (m_path);
  }

private:
  boost::filesystem::path m_path;
};

template<typename Suggestion,
         typename ScoreComparer>
std::vector<Suggestion>
complete (
  const std::vector<Suggestion> suggestions,
  const std::string &prefix,
  const ScoreComparer &comparer)
{
  const auto first_it =
    std::lower_bound (
      suggestions.begin (), suggestions.end (),
      prefix,
      [] (const Suggestion &s,
	  const std::string &p)
      {
	return std::get<0> (s) < p;
      });

  const auto last_it =
    std::lower_bound (
      suggestions.begin (), suggestions.end (),
      prefix,
      [] (const Suggestion &s, const std::string &p)
      {
	const auto &rhs = std::get<0> (s);
	const auto mismatch =
	  std::mismatch (p.begin (),   p.end (),
			 rhs.begin (), rhs.end ());

	if (mismatch.first  != p.end () &&
	    mismatch.second != rhs.end ())
	{
	  return *mismatch.second < *mismatch.first;
	}

	return true;
      });

  BOOST_ASSERT (first_it <= last_it);
  std::vector<Suggestion> result {first_it, last_it};
  std::sort (result.begin (), result.end (), comparer);
  return result;
}

template<typename Suggestion>
size_t mismatch (const std::vector<Suggestion> &suggestions,
		 const std::string &prefix)
{
  const auto it = 
    std::lower_bound (
      suggestions.begin (), suggestions.end (),
      prefix,
      [] (const Suggestion &s,
	  const std::string &p)
      {
	return std::get<0> (s) < p;
      });

  const auto lcp = [] (const std::string &lhs,
		       const std::string &rhs)
  {
    const auto mismatch_it =
      std::mismatch (lhs.begin (), lhs.end (),
		     rhs.begin (), rhs.end ()).second;

    return static_cast<size_t> (
      std::distance (rhs.begin (), mismatch_it));
  };

  return std::max (
     ((it == suggestions.begin ()) ? 0u : lcp ((it-1)->first, prefix)),
     ((it == suggestions.end ())   ? 0u : lcp (it->first, prefix)));

}
 
}}

#endif
