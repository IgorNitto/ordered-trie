
Ordered Trie
====================================

A C++ header-only library providing a static persistent container for the ordered prefix completion task.

Given a set of string S, a prefix completion query asks to enumerate all strings in S having a given input string p as prefix. In more programming terms, the output to a prefix completion should offer an iterator object going through all strings in S starting with p in an unspecified order.

If the strings in S have an associated ranking score, the ordered prefix completion additionally asks the the output strings to be iterated in order of rank (descending rank, by convention).

Traditional algorithmic solution to such problem build upon the trie data structure (https://en.wikipedia.org/wiki/Trie).

This library provides a single container class ```OrderedTrie```, particularly tuned to have small memory occupancy. The examples in the following section illustrate some of its features.

Examples
-------------

Let us construct and query construct Example of in-memory construction:

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
```

The ```complete()``` member function return a range enumerating all completions for a given string in descending order of rank. The output is of type conforming the Forward Range concept as defined in Boost.Range. 

```cpp
  for (const auto &completion : trie.complete ("b"))
  {
    std::cout << completion.string () << ","
              << completion.score () << std::endl;
  }

  // This will print: bar,30 babba,14

```
The type of values in the range returned by ```complete()``` is ```ordered_trie::Completion<Score>```. This is essentially a glorified std::pair containing the completion string and the associated score object.

To change the enumeration order it is possible to provide a custom score comparison functor in the ```OrderedTrie``` constructor .

Other construction patterns:

```cpp

   // Range constructor

   const std::vector<OrderedTrie<int>::value_type> input =
   {
     {"abba", 5},
     {"babba", 14},
     {"bar", 30}
   }

   const OrderedTrie<int> trie {input.begin (), input.end ()};

   // Using function make_ordered_trie()

   const auto trie_2 = make_ordered_trie (input);
```

Moreover, ```OrderedTrie``` can be read/writen from file directly (by memory-mapping the data structure). 

```cpp
   trie.write ("./trie_file");
   const auto trie_2 = OrderedTrie<int>::read ("./trie_file")
```

Member functions ```count``` and ```score``` are provided to check the presence of a string in the collection and retrieve its associated score object:

```cpp
  // Test if a completion is present, get associated score

  if (trie.count ("babba"))
  {
    std::cout << trie.score ("babba");
  }
```


Requirements
-------------

This is an header-only library requiring a C++14 compliant compiler and Boost (version 1.60.0 or higher is recommended).

Unit tests can be built and executed by the script `make_tests.sh`, which currently relies on CMake.

Benchmarks
-------------------------------

(TODO)


