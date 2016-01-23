# Define test module

# Assign name to current target
set (THIS_TARGET "test_node_encoding")

# Declate type of current module as one of:
# STATIC_LIBRARY, DYNAMIC_LIBRARY, EXECUTABLE, TEST

set (THIS_TARGET_TYPE "TEST")

# List headers and source files
set (THIS_SOURCES "test_node_encoding.cpp")

# State dependencies
set (USE_ORDERED_TRIE  "TRUE")
set (USE_BOOST         "TRUE")
set (USE_BOOST_TEST    "TRUE")

# always include these two files the end
include (${PROJECT_PATH}/common.cmake)
include (${PROJECT_PATH}/exported.cmake)
