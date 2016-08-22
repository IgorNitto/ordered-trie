include_directories ("${ORDERED_TRIE_SOURCE_DIR}/include")

find_package (Boost 1.60 COMPONENTS unit_test_framework filesystem system REQUIRED)
if (Boost_FOUND)
  message(STATUS "Boost library version ${Boost_LIB_VERSION} found, with headers at '${Boost_INCLUDE_DIR}' and libraries at '${Boost_LIBRARY_DIRS}' for libraries: \n${Boost_LIBRARIES}")
endif ()

include_directories (${Boost_INCLUDE_DIRS})
link_directories (${Boost_LIBRARY_DIRS})

add_executable (test_ordered_trie test_ordered_trie.cpp)
target_link_libraries (test_ordered_trie ${Boost_SYSTEM_LIBRARY} ${Boost_FILESYSTEM_LIBRARY} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
