# vim:ts=2:sw=2:et

list(APPEND TEST_SRCS
  test_eterm.cpp
  test_eterm_encode.cpp
  test_eterm_format.cpp
  test_eterm_match.cpp
  test_eterm_pool.cpp
  test_eterm_refc.cpp
  test_mailbox.cpp
  test_node.cpp
)

# This must be AFTER link_directories
add_executable(test-eterm ${TEST_SRCS})
target_compile_definitions(test-eterm PRIVATE -DBOOST_TEST_DYN_LINK)
target_include_directories(test-eterm PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  test-eterm
  eixx ei
  boost_unit_test_framework
  boost_system
  boost_thread
)

add_executable(test-connect test_async_queue.cpp)
target_link_libraries(
  test-connect
  eixx
  boost_unit_test_framework
  boost_system
  boost_thread
)

add_executable(test-perf test_perf.cpp)
target_link_libraries(
  test-perf
  eixx ei
  boost_system
)

install(TARGETS test-eterm test-connect test-perf RUNTIME DESTINATION test)
