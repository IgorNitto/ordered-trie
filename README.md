
Ordered Trie
====================================

C++ header-only library for ordered prefix search with persistency support.

Requirements
-------------

This is an header-only library requiring to be compiled with a C++14 compliant compiler with Boost in the include path (Boost 1.60.0 or higher is recommended).

Unit tests are built and run by launching the script `make_tests.sh`, which relies on CMake.

Examples
-------------

Example of in-memory construction and querying:

```cpp

  // Demo construction and querying

  #include "ordered_trie.hpp"

  using namepsace ordered_trie;

  // Construct via initializer list

  const OrderedTrie<int> trie
  {
    {"abba", 5},
    {"babba", 14},
    {"bar", 30}
  };

  // Retrieve completions for string "b" by decreasing
  // order of score. This prints:
  //
  //  bar,30   
  //  babba,14
  //

  for (const auto &completion : trie.complete ("b"))
  {
    std::cout << completion.string () << ","
              << completion.score () << std::endl;
  }

  // Test if a completion is present, get associated score

  if (trie.count ("babba"))
  {
    std::cerr << trie.score ("babba");
  }
```

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

   // Via free functions make_ordered_trie

   const auto trie_2 = make_ordered_trie (input);

   // Reading from file

   trie.write ("./trie_file");
   const auto trie_3 = OrderedTrie<int>::read ("./trie_file")
```

Benchmark
-------------------------------

(TODO)


