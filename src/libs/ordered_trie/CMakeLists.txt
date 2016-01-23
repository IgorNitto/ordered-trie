# Define library module

# Assign name to current target
set (THIS_TARGET "ordered_trie")

# Declate type of current module as one of:
# STATIC_LIBRARY, DYNAMIC_LIBRARY, EXECUTABLE, TEST

set (THIS_TARGET_TYPE "STATIC_LIBRARY")

# List headers and source files
set (THIS_HEADERS
     "completion.h"
     "completion.inl"
     "detail_encoding.h"
     "detail_encoding_impl.inl"
     "detail_iterator.h"
     "detail_iterator.inl"
     "detail_payload_types.h"
     "detail_serialise_trie.h"
     "detail_serialise_trie.inl"
     "detail_iterator.h"
     "detail_iterator.inl"
     "detail_varint.h"
     "ordered_trie.h"
     "ordered_trie.inl"
     "serialise.h")

# Define include path in installation folder.
# Most likely this is always equal to target name.
set (THIS_INCLUDE_PATH "ordered_trie")

# Dependencies
set (USE_BOOST "TRUE")

# always include these two files the end
include (${PROJECT_PATH}/common.cmake)
include (${PROJECT_PATH}/exported.cmake)
