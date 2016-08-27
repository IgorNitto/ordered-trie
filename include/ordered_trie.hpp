#ifndef ORDERED_TRIE_HPP
#define ORDERED_TRIE_HPP

/**
 * @file ordered_trie.h
 */

#include "detail/ordered_trie_node.hpp"
#include "detail/ordered_trie_store.hpp"
#include "ordered_trie_serialise.hpp"

#include <initializer_list>
#include <memory>
#include <string>
#include <tuple>

namespace ordered_trie {

/**
 * Completion is the type of values stored in an
 * oredered trie instance.
 * It is a pair of a string and a score extended
 * with two accessor functions string () and score ().
 */
template<typename Score>
class Completion : public std::pair<std::string, Score>
{
public:
  using pair_t = std::pair<std::string, Score>;

  template<typename T, typename U>
  Completion (T&& string, U&& score);
  Completion ()                             = default;
  Completion (const Completion&)            = default;
  Completion (Completion&&)                 = default;
  Completion& operator= (const Completion&) = default;
  Completion& operator= (Completion&&)      = default;
  
  const std::string &string () const;
  Score score () const;

  operator const pair_t& () const;
};  

/**
 * OrderedTrie
 */
template<typename Score>
class OrderedTrie
{
public:

  using score_type = Score;
  using value_type = Completion<Score>;

  /**
   * Ordered completions iterator. 
   */
  class iterator;

  /**
   * Default empty trie
   */
  OrderedTrie ();

  /**
   * Construct trie over input contained in half-open range
   * [@p first, @p last). The input elements are std::pair
   * where the first component is the suggestion string and
   * the second is the associated ranking value of type Score.
   * The input range is required to be in increasing lexicographic
   * order of suggestion strings.
   */
  template<typename FwdIt>
  explicit OrderedTrie (FwdIt first, FwdIt last);

  /**
   * Range based ctor allowing to specify a custom score
   * comparison functor.
   */
  template<typename FwdIt, typename Comparer>
  explicit OrderedTrie (FwdIt first,
			FwdIt last,
			const Comparer &score_comparer);

  /**
   * Ctor from initializer list
   */
  explicit OrderedTrie (
    const std::initializer_list<value_type>&);

  /**
   * Ctor from initializer list with user-defined score comparison
   */
  template<typename Comparer>
  explicit OrderedTrie (
    const std::initializer_list<value_type> &values,
    const Comparer &score_comparer);

  /**
   * Returns iterator to first suggestion in order of decreasing score.
   */
  iterator begin () const;

  /**
   * Returns end iterator.
   */
  iterator end () const;

  /**
   * Returns true if this is an empty instance.
   */ 
  bool empty () const;

  /**
   * Returns range of completions for given prefix 
   * ordered by decreasing score.
   */ 
  auto complete (const std::string &prefix) const
    -> boost::iterator_range<iterator>;  

  /**
   * @overload complete() taking input prefix in range form.
   */ 
  template<typename FwdIt>
  auto complete (FwdIt first, FwdIt last) const
    -> boost::iterator_range<iterator>;  

  /**
   * Match input string against trie content returning
   * length of the longest prefix of input string which
   * is also prefix of a suggestion contained in the trie.
   */
  size_t mismatch (const std::string &input) const;

  /**
   * @overload of mismatch() accepting input string in
   * range form.
   */
  template<typename FwdIt>
  FwdIt mismatch (FwdIt begin, FwdIt end) const;

  /**
   * Search a trie suggestion equal to @p input string.
   * If present, returns true and store the associated
   * score in @p output_score, otherwise returns false.
   */
  bool score (Score &output_score,
	      const std::string &input) const;
	      
  /**
   * @overload of score() taking input string in
   * range form [@p first, @p last)
   */
  template<typename FwdIt>
  bool score (Score &output_score,
	      FwdIt first,
	      FwdIt last) const;

  /**
   * @overload of score() returning score associated
   * to input suggestion. Raise an exception if the
   * suggestion string is not found.
   */
  Score score (const std::string &input) const;

  /**
   * Range-based form of score()
   */
  template<typename FwdIt>
  Score score (FwdIt first, FwdIt last) const;

  /**
   * Returns number of times a suggestion with text
   * equal to given @p input string appears in the trie.
   */
  size_t count (const std::string &input) const;

  /**
   * Range based overload of @p count()
   */
  template<typename FwdIt>
  size_t count (FwdIt first, FwdIt last) const;
  
  /**
   * Write serialised trie to file.
   */
  void write (const std::string &path) const;

  /**
   * Read instance from file where it was previously
   * stored using write().
   */
  static OrderedTrie read (const std::string &path);

private:
  class Parameters;
  using Node = detail::Node<Void>;
  using Store = detail::Store<Parameters>;

  explicit OrderedTrie (std::shared_ptr<const Store>);  
  Node m_root;
  const std::uint8_t *m_score_table;
  std::shared_ptr<const Store> m_store;
};

/**
 * Make an OrderedTrie instance from range containing
 * suggestion pairs where the first component is suggestion
 * string and the second component is the associated score.
 * The input suggestions text are expected to appear in
 * ascending lexicographic order, or an exception will be raised.
 */
template<typename FwdRange>
auto make_ordered_trie (const FwdRange &suggestions);

/**
 * @overload of make_ordered_trie() allowing the user to
 * provide a generic score comparison functor.
 */
template<typename FwdRange, typename Comparer>
auto make_ordered_trie (const FwdRange &suggestions,
			const Comparer &score_comparer);

} // namespace ordered_trie {

#include "detail/ordered_trie_impl.hpp"

#endif 
