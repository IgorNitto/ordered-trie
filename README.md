
Ordered Trie
====================================

A C++ header-only library providing a static persistent container for the *ordered prefix completion* task.

Given a set of string `S`, a prefix completion query asks to enumerate all strings in `S` having a given input string `p` as prefix. In other words, the output to a prefix completion should be an iterator-type object enumerating the strings in `S` prefixed by `p` in an unspecified order.

If the strings in `S` have an associated ranking score, the ordered prefix completion query additionally requests the output strings to be iterated in order of rank (descending rank, by convention).

Traditional algorithmic solutions to such problem build upon the trie data structure <https://en.wikipedia.org/wiki/Trie>.

This library aims at offering an efficient tool for ordered prefix completion with the `OrderedTrie` class. This has an interface similar to that of standard C++ containers, can be easily serialised to file and is particularly optimised for space consumption. The following examples illustrate its main features.

Examples
-----------------------------------

The `OrderedTrie` class supports the traditional constructor idioms of standard containers:

```cpp

  #include "ordered_trie.hpp"

  using namepsace ordered_trie;

  // Construct via initializer list of (string, score) pairs

  const OrderedTrie<int> trie
  {
    {"abba", 5},
    {"babba", 14},
    {"bar", 30}
  };

   // Range constructor

   const std::vector<OrderedTrie<int>::value_type> input =
   {
     {"abba", 5},
     {"babba", 14},
     {"bar", 30}
   }

   const OrderedTrie<int> trie_2 {input.begin (), input.end ()};

   // Using make_orered_trie factory which implicitly deduce the Score type

   const auto trie_3 = make_ordered_trie (input)

```

The `complete()` member function return a range enumerating all completions for a given string in descending order of rank. The output is of type conforming the Forward Range concept as defined in Boost.Range. 

```cpp
  for (const auto &completion : trie.complete ("b"))
  {
    std::cout << completion.string () << ","
              << completion.score () << std::endl;
  }

  // This will print: bar,30 babba,14

```

The type of values in the output range is `ordered_trie::Completion<Score>`, which is just a glorified `std::pair` containing the completion string and the associated score object.

The enumeration order of `complete()` can be changed by providing a custom score comparison functor in the `OrderedTrie` constructor.

Moreover, `OrderedTrie` can be read/writen from file directly (literally by memory-mapping the data structure). 

```cpp
   trie.write ("./trie_file");
   const auto trie_2 = OrderedTrie<int>::read ("./trie_file")
```

Member functions `count()` and `score()` are provided to check the presence of a string in the collection and retrieve its associated score object:

```cpp
  // Test if a completion is present, get associated score

  if (trie.count ("babba"))
  {
    std::cout << trie.score ("babba");
  }
```

Requirements
-------------------------------

A C++14 compliant compiler and Boost (version 1.60.0 or higher is recommended).

Building
-------------------------------

This is an header-only library, so no separate compilation is necessary.

Building and running unit tests requires CMake and can be done by just launching scripts `make_tests.sh`

Benchmarks
-------------------------------

This benchmark was conducted over a text dump of all titles of Wikipedia articles (downloadable from <https://dumps.wikimedia.org/enwiki/latest/enwiki-latest-all-titles.gz>). I selected a sample of 9.2M rows (starting with letter in A-E) and constructed OrderedTrie. 

|-----------------------|--------:|
|Original text size     | 195226KB|
|OrderedTrie size       | 116466KB|
|Peak memory usage      | 343336KB|

TODO: Extend


