
#############################################################
# Configure 3rd party library

set (BOOST_ROOT "/home/nitto/software/boost/boost_1_60_0")

find_package (Boost 1.56 COMPONENTS unit_test_framework REQUIRED)
if (Boost_FOUND)
  message(STATUS "Boost library version ${Boost_LIB_VERSION} found, with headers at '${Boost_INCLUDE_DIR}' and libraries at '${Boost_LIBRARY_DIRS}' for libraries: \n${Boost_LIBRARIES}")
endif ()
 