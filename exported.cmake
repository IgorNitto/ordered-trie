
#############################################################
# Declare dependencies on this project's libraries

if (USE_ORDERED_TRIE STREQUAL "TRUE")
  if (NOT USED_ORDERED_TRIE)
    set (USED_ORDERED_TRIE "TRUE")
    include_directories ("${LIB_SOURCE_PATH}/ordered_trie")
  endif ()
endif ()

# Add 3rd-party libraries
if (USE_BOOST STREQUAL "TRUE")
  if (NOT USED_BOOST)
    set (USED_BOOST "TRUE")
    include_directories (${Boost_INCLUDE_DIRS})
    link_directories (${Boost_LIBRARY_DIRS})
  endif ()
endif ()

if (USE_BOOST_TEST STREQUAL "TRUE")
  if (NOT USED_BOOST_TEST)
    set (USED_BOOST_TEST "TRUE")
    message ("Linking ${THIS_TARGET} against ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}")
    include_directories (${Boost_INCLUDE_DIRS})
    link_directories (${Boost_LIBRARY_DIRS})
    target_link_libraries (${THIS_TARGET} ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY})
  endif ()
endif ()


#############################################################

