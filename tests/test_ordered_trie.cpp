#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MODULE test_ordered_trie
#define BOOST_TEST_NO_MAIN

#ifndef BOOST_SYSTEM_NO_DEPRECATED
#define BOOST_SYSTEM_NO_DEPRECATED 1
#endif

#include "test_utils.h"

#include "ordered_trie.hpp"
#include "detail/ordered_trie_node.hpp"
#include "detail/ordered_trie_varint.hpp"

#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/range.hpp>
#include <boost/optional.hpp>

#include <cassert>
#include <iterator>
#include <random>
#include <tuple>
#include <bitset>
#include <unordered_set>

using namespace ordered_trie;
using namespace ordered_trie::test_utils;

namespace
{

template<typename S>
std::vector<std::pair<std::string, S>>
make_two_digits_suggestions (size_t length,
			     size_t sample_size,
			     size_t seed)
{
  BOOST_ASSERT (length <= 16);
  BOOST_ASSERT (sample_size <= (1u << (length + 1)));

  std::vector<std::uint64_t>
  suggestion_int ((1 << (length + 1)) - 1, 0);
  
  std::iota (suggestion_int.begin (),
	     suggestion_int.end (),
	     0u);

  std::shuffle (suggestion_int.begin (),
		suggestion_int.end (),
		std::mt19937_64 {seed});
  
  suggestion_int.resize (sample_size);

  std::vector<std::pair<std::string, S>> result;
  result.reserve (sample_size);
  S score = 0;

  for (const auto v : suggestion_int)
  {
    auto text = std::bitset<17> (v)
     .to_string ()
     .substr (17 - (length + 1));

    text.erase (text.find_last_of ('0'));
    result.push_back ({text, score / 10});
    ++score;
  }
  
  std::sort (result.begin (), result.end ());
  return result;
}  

} // namespace {

BOOST_AUTO_TEST_CASE (test_serialise_integer)
{
  std::vector<std::uint8_t> output;

  constexpr std::uint8_t a = 0;
  constexpr std::uint8_t b = 255;
  constexpr std::uint16_t c = 0xF00;
  constexpr std::uint16_t d = 0xFF00;
  constexpr std::uint32_t e = 1;
  constexpr std::uint64_t f = std::uint64_t{1} << 40;
  
  serialise (output, a);
  serialise (output, b);
  serialise (output, c);
  serialise (output, d);
  serialise (output, e);
  serialise (output, f);
  serialise (output, 0.5);
  
  const std::uint8_t* read_pointer = output.data ();

  BOOST_CHECK_EQUAL (a, deserialise<std::uint8_t> (read_pointer));
  read_pointer = Serialise<std::uint8_t>::skip (read_pointer);
  
  BOOST_CHECK_EQUAL (b, deserialise<std::uint8_t> (read_pointer));
  read_pointer = Serialise<std::uint8_t>::skip (read_pointer);

  BOOST_CHECK_EQUAL (c, deserialise<std::uint16_t> (read_pointer));
  read_pointer = Serialise<std::uint16_t>::skip (read_pointer);

  BOOST_CHECK_EQUAL (d, deserialise<std::uint16_t> (read_pointer));
  read_pointer = Serialise<std::uint16_t>::skip (read_pointer);

  BOOST_CHECK_EQUAL (e, deserialise<std::uint32_t> (read_pointer));
  read_pointer = Serialise<std::uint32_t>::skip (read_pointer);

  BOOST_CHECK_EQUAL (f, deserialise<std::uint64_t> (read_pointer));
  read_pointer = Serialise<std::uint64_t>::skip (read_pointer);

  BOOST_CHECK_EQUAL (0.5, deserialise<double> (read_pointer));
  read_pointer = Serialise<double>::skip (read_pointer);

  BOOST_CHECK_EQUAL (output.data () + output.size (), read_pointer);     
}

BOOST_AUTO_TEST_CASE (test_encoding_32)
{
  using detail::RankEncoder;

  const std::vector<size_t> input =
  {
    0,
    0,
    0x1,
    0xF,
    0x100,
    0x101,
    0x1FF,
    0xFFFF,
    0x100000,
    0x10FFFFF,
    0xFFFFFFFF,
    0x100000000,
    0xABCDEFABCDEFA,
    0x100000000000000,
    0xFFFFFFFFFFFFFFFF
  };

  const std::vector<RankEncoder::wordsize_t> codewords =
  {
    RankEncoder::EMPTY,
    RankEncoder::EMPTY,
    RankEncoder::UINT8,
    RankEncoder::UINT8,
    RankEncoder::UINT16,
    RankEncoder::UINT16,
    RankEncoder::UINT16,
    RankEncoder::UINT16,
    RankEncoder::UINT64,
    RankEncoder::UINT64,
    RankEncoder::UINT64,
    RankEncoder::UINT64,
    RankEncoder::UINT64,
    RankEncoder::UINT64,
    RankEncoder::UINT64,
  };

  std::vector<std::uint8_t> out;

  for (size_t j=0; j < input.size (); ++j)
  {
    BOOST_CHECK_EQUAL (RankEncoder::serialise (out, input[j]),
		       codewords [j]);
  }
  
  const std::uint8_t *read_offset = out.data ();
  for (size_t j=0; j < input.size (); ++j)
  {

    const auto c = RankEncoder::deserialise (read_offset, codewords [j]);
    read_offset = RankEncoder::skip (read_offset, codewords [j]);
    BOOST_CHECK_EQUAL (c, input [j]);
  }
  
  BOOST_CHECK_EQUAL (read_offset, out.data () + out.size ());
}

BOOST_AUTO_TEST_CASE (test_encoding_64)
{
  using detail::OffsetEncoder;

  const std::vector<size_t> input =
  {
    0,
    0,
    0x1,
    0xF,
    0x100,
    0x101,
    0x1FF,
    0xFFFF,
    0x100000,
    0x10FFFFF,
    0xFFFFFFFF,
    0x100000000,
    0xFFFFFFFFFFFFFFFF
  };

  const std::vector<OffsetEncoder::wordsize_t> codewords =
  {
    OffsetEncoder::EMPTY,
    OffsetEncoder::EMPTY,
    OffsetEncoder::UINT8,
    OffsetEncoder::UINT8,
    OffsetEncoder::UINT16,
    OffsetEncoder::UINT16,
    OffsetEncoder::UINT16,
    OffsetEncoder::UINT16,
    OffsetEncoder::UINT64,
    OffsetEncoder::UINT64,
    OffsetEncoder::UINT64,
    OffsetEncoder::UINT64,
    OffsetEncoder::UINT64
  };

  std::vector<std::uint8_t> out;
  
  for (size_t j=0; j < input.size (); ++j)
  {
    BOOST_CHECK_EQUAL (OffsetEncoder::serialise (out, input[j]),
		       codewords [j]);    
  }
  
  const auto* read_offset = out.data ();
  for (size_t j=0; j < codewords.size (); ++j)
  {
    const auto c = OffsetEncoder::deserialise (read_offset, codewords [j]);
    read_offset = OffsetEncoder::skip (read_offset, codewords [j]);

    BOOST_CHECK_EQUAL (c, input [j]);
  }

  BOOST_CHECK_EQUAL (read_offset, out.data () + out.size ());
}

BOOST_AUTO_TEST_CASE (test_node_serialise_internal)
{
  std::vector<std::uint8_t> data;
  detail::serialise_node<Void> (data, "label", 10u, 20u, {});

  const auto node = make_root (data.data ());
  const std::string label {
    node.label_begin (),
    node.label_begin () + node.label_size ()};
  
  BOOST_CHECK (!node.is_leaf ()); 
  BOOST_CHECK_EQUAL (label, "label");
  BOOST_CHECK_EQUAL (node.rank (), 10u);
}

BOOST_AUTO_TEST_CASE (test_node_serialise_leaf)
{
  std::vector<std::uint8_t> data;
  detail::serialise_node<Void> (data, "label", 10u, 20u, Void {});

  const auto node = make_root (data.data ());
  const std::string label {
    node.label_begin (),
    node.label_begin () + node.label_size ()};
  
  BOOST_CHECK (node.is_leaf ()); 
  BOOST_CHECK_EQUAL (label, "label");
  BOOST_CHECK_EQUAL (node.rank (), 10u);
}

BOOST_AUTO_TEST_CASE (test_ordered_trie_empty)
{
  TemporaryFile tmp_file;
  const auto tmp_path = tmp_file.get ();

  {
    const OrderedTrie<int> trie;
    BOOST_CHECK (trie.empty ());
    BOOST_CHECK (trie.begin () == trie.end ());
    BOOST_CHECK (!trie.count ("x"));
    BOOST_CHECK (!trie.count (""));
    trie.write (tmp_path);
  }

  {
    const auto trie_from_disk =
      OrderedTrie<int>::read (tmp_path);

    BOOST_CHECK (trie_from_disk.empty ());
    BOOST_CHECK (trie_from_disk.begin () == trie_from_disk.end ());
    BOOST_CHECK (!trie_from_disk.count ("x"));
    BOOST_CHECK (!trie_from_disk.count (""));
  }
}

BOOST_AUTO_TEST_CASE (test_ordered_trie_2)
{
  using Suggestion =
    typename OrderedTrie<std::uint64_t>::value_type;
  
  const OrderedTrie<std::uint64_t> trie
  {
    {"a", 1u}
  };

  const std::vector<Suggestion> suggestions
  {
    {"a", 1u}
  };

  BOOST_ASSERT (trie.count ("a") == 1);
  BOOST_ASSERT (!trie.count (""));
  BOOST_ASSERT (!trie.count ("aa"));
  BOOST_ASSERT (!trie.count ("b"));
  BOOST_ASSERT (trie.score ("a") == 1);
  BOOST_ASSERT (trie.mismatch ("a") == 1);
  BOOST_ASSERT (make_vector(trie) == suggestions);
  BOOST_ASSERT (make_vector(trie.complete("")) == 
		suggestions);
  BOOST_ASSERT (make_vector(trie.complete ("a")) == 
		suggestions);
}

BOOST_AUTO_TEST_CASE (test_ordered_trie_3)
{
  using Suggestion =
    typename OrderedTrie<std::uint64_t>::value_type;
  
  const std::vector<Suggestion> suggestions =
  {
    {"aaaaaaaaaaa", 30u},	
    {"abbb", 1u},
    {"b",    20u},
    {"bcc",  20u},
  };

  const OrderedTrie<std::uint64_t> trie
  {
    suggestions.begin (),
    suggestions.end ()
  };

  BOOST_ASSERT (!trie.count("aa"));
  BOOST_ASSERT (!trie.count(""));
  BOOST_CHECK_EQUAL (trie.count ("b"), 1u);
  BOOST_CHECK_EQUAL (trie.mismatch ("aaaaa"), 5u);
  BOOST_CHECK_EQUAL (trie.mismatch ("aaaaaaaaaaaaa"), 11u);
  BOOST_CHECK_EQUAL (trie.mismatch ("b"), 1u);

  BOOST_CHECK_EQUAL (trie.score ("b"), 20u);
  BOOST_CHECK_EQUAL (trie.score ("bcc"), 20u);
  BOOST_CHECK_EQUAL (trie.score ("b"), 20u);
  BOOST_CHECK_EQUAL (trie.score ("aaaaaaaaaaa"), 30u);

  std::uint64_t s;
  BOOST_CHECK (!trie.score (s, "bccc"));
  BOOST_CHECK (trie.score (s, "bcc"));
  BOOST_CHECK_EQUAL (s, 20u);

  BOOST_ASSERT (
    make_vector(trie) ==
    (std::vector<Suggestion>
    {
      suggestions[0], suggestions[2],
      suggestions[3], suggestions[1]
    }));

  BOOST_ASSERT (
    make_vector(trie.complete("")) ==
    (std::vector<Suggestion>
    {
      suggestions[0], suggestions[2],
      suggestions[3], suggestions[1]
    }));

  BOOST_ASSERT (
    make_vector(trie.complete("a")) ==
    (std::vector<Suggestion> {suggestions[0],
                             suggestions[1]}));

  BOOST_ASSERT (
    make_vector(trie.complete("aaa")) ==
    (std::vector<Suggestion> {suggestions[0]}));

  BOOST_ASSERT (
    make_vector(trie.complete("aaaaaaaaaaa")) ==
    (std::vector<Suggestion> {suggestions[0]}));

  BOOST_ASSERT (
    make_vector(trie.complete("b")) ==
    (std::vector<Suggestion> {suggestions[2],
 	                      suggestions[3]}));

  BOOST_ASSERT (make_vector(trie.complete("d")).empty ());
}

BOOST_AUTO_TEST_CASE (test_ordered_trie_4)
{
  using Suggestion =
    typename OrderedTrie<std::uint64_t>::value_type;
  
  const std::vector<Suggestion> suggestions =
  {
    {"", 7},
    {"a", 6},
    {"aa", 5},
    {"aaa", 4},
    {"aaaa", 3},
    {"aaaaa", 2},
    {"aaaaaa", 1},
  };

  const auto trie = [&]
  {
    TemporaryFile tmp_file;
    const auto tmp_path = tmp_file.get ();

    OrderedTrie<std::uint64_t>
    {
      suggestions.begin (),
      suggestions.end ()
    }.write (tmp_path);  

    return OrderedTrie<std::uint64_t>::read (tmp_path);
  } ();

  BOOST_CHECK_EQUAL (trie.count ("a"), 1u);
  BOOST_CHECK_EQUAL (trie.count ("aaa"), 1u);
  BOOST_CHECK_EQUAL (trie.mismatch ("aa"), 2u);

  BOOST_CHECK (make_vector(trie) == suggestions);
  BOOST_CHECK (
    make_vector (trie.complete ("aaaaa")) ==
    (std::vector<Suggestion>
     {
       suggestions[5], suggestions[6]
     }));
}

BOOST_AUTO_TEST_CASE (test_ordered_trie_5)
{
  using Suggestion =
    typename OrderedTrie<std::uint64_t>::value_type;
  
  const std::vector<Suggestion> suggestions =
  {
    {"ac", 4u},
    {"ab", 3u},
    {"a",  2u},
    {"ba", 2u},
    {"bd", 1u},
  };

  const auto trie = [&]
  {
    TemporaryFile tmp_file;
    const auto tmp_path = tmp_file.get ();

    make_ordered_trie (suggestions).write (tmp_path);  

    return OrderedTrie<std::uint64_t>::read (tmp_path);
  } ();

  BOOST_CHECK (make_vector(trie) == suggestions);

  BOOST_CHECK_EQUAL (trie.count ("a"), 1u);
  BOOST_CHECK_EQUAL (trie.count ("aaa"), 0u);
  BOOST_CHECK_EQUAL (trie.mismatch ("bbb"), 1u);
  BOOST_CHECK (
    make_vector (trie.complete ("a")) ==
    (std::vector<Suggestion>
     {
       suggestions[0],
       suggestions[1],
       suggestions[2]
     }));
}

BOOST_AUTO_TEST_CASE (test_ordered_trie_random_data)
{
  const auto suggestions =
    make_two_digits_suggestions<std::uint64_t> (10, 1000, 63);

  const auto prefixes = [&]
  {
    std::unordered_set<std::string> result;

    for (const auto p : suggestions)
    {
      const auto text = p.first;
      for (size_t j = 0; j <= text.size(); ++j)
      {
	result.insert (text.substr (0, j));
	result.insert (text + "222");
      }
    }

    result.insert ("");
    return result;
  } ();

  const auto check = [&] (OrderedTrie<std::uint64_t> trie)
  {
    for (const auto prefix : prefixes)
    {
      auto result = make_vector (trie.complete (prefix));

      BOOST_CHECK (std::is_sorted (
	result.begin (), result.end (),
	[] (const auto &x, const auto &y)
	{
	  return y.second < x.second;
	}));

      std::sort (result.begin (), result.end ());

      auto expected =
	complete (suggestions, prefix, std::greater<> {});

      std::sort (expected.begin (), expected.end ());

      BOOST_CHECK (result == expected);

      BOOST_CHECK_EQUAL (
	trie.mismatch (prefix),
	mismatch (suggestions, prefix));

      const auto suggestion_it =
	std::lower_bound (
	  suggestions.begin (), suggestions.end (),
	  prefix,
	  [] (const auto &s, const std::string &p)
	  {
	    return std::get<0> (s) < p;
	  });

      if (suggestion_it == suggestions.end () ||
	  suggestion_it->first != prefix)
      {
	BOOST_CHECK_EQUAL (trie.count (prefix), 0u);
	size_t score = -1;
	BOOST_CHECK (!trie.score (score, prefix));
	BOOST_CHECK_EQUAL (score, static_cast<size_t> (-1));
      }
      else
      {
	BOOST_CHECK_EQUAL (trie.count (prefix), 1u);
	size_t score = -1;
	BOOST_CHECK (trie.score (score, prefix));
	BOOST_CHECK_EQUAL (score, suggestion_it->second);
	BOOST_CHECK_EQUAL (
	  trie.score (prefix), suggestion_it->second);
      }
    }
  };

  // Tests different construction patterns

  check (make_ordered_trie (suggestions));

  check (OrderedTrie<std::uint64_t>
  {
    suggestions.begin (),
    suggestions.end ()
  });

  check ([&]
  {
    TemporaryFile tmp_file;
    const auto tmp_path = tmp_file.get ();

    OrderedTrie<std::uint64_t>
    {
      suggestions.begin (),
      suggestions.end ()
    }.write (tmp_path);  

    return OrderedTrie<std::uint64_t>::read (tmp_path);
  } ());  
}

int main (int argc, char **argv)
{
  return boost::unit_test::unit_test_main(
            &init_unit_test, argc, argv);
}
