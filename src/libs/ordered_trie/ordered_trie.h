#ifndef ORDERED_TRIE_H
#define ORDERED_TRIE_H

/**
 * @file  ordered_trie.h
 */

#include "detail_encoding.h"

#include <string>

namespace ordered_trie {

/**
 * Group of template parameters of ordered trie
 */
template<typename MetaData,
         typename Suggestion = std::string,
         typename Encoding = encoders::v0::Impl>
class Parameters;

} // namespace ordered_trie {

#include "ordered_trie.inl"

#endif
