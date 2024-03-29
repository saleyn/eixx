# vim:ts=2:sw=2:et

list(APPEND TEST_SRCS
  test_eterm.cpp
  test_eterm_encode.cpp
  test_eterm_format.cpp
  test_eterm_match.cpp
  test_eterm_pool.cpp
  test_eterm_refc.cpp
)

if (NOT EIXX_MARSHAL_ONLY)
  list(APPEND TEST_SRCS
    test_mailbox.cpp
    test_node.cpp
  )
endif()

# This must be AFTER link_directories
add_executable(test-eterm ${TEST_SRCS})
target_compile_definitions(test-eterm PRIVATE -DBOOST_TEST_DYN_LINK)
target_include_directories(test-eterm PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(
  test-eterm
  boost_unit_test_framework
  eixx
  ${Erlang_EI_LIBRARIES}
  ${Boost_LIBRARIES}
)

if(CMAKE_SYSTEM_NAME STREQUAL Linux OR
   CMAKE_SYSTEM_NAME STREQUAL Android)
  add_executable(test-perf test_perf.cpp)
  target_link_libraries(
    test-perf
    eixx
    ${Erlang_EI_LIBRARIES}
    ${Boost_LIBRARIES}
  )
endif()

if (NOT EIXX_MARSHAL_ONLY)
  add_executable(test-connect test_async_queue.cpp)
  target_link_libraries(
    test-connect
    boost_unit_test_framework
    eixx
    ${Erlang_EI_LIBRARIES}
    ${Boost_LIBRARIES}
  )

  install(
    TARGETS
      test-eterm
    RUNTIME DESTINATION test
  )

  if(CMAKE_SYSTEM_NAME STREQUAL Linux OR
     CMAKE_SYSTEM_NAME STREQUAL Android)
    install(
      TARGETS
        test-perf
      RUNTIME DESTINATION test
    )
  endif()
endif()

install(
  TARGETS
    test-eterm
  RUNTIME DESTINATION test
)

if(CMAKE_SYSTEM_NAME STREQUAL Linux OR
   CMAKE_SYSTEM_NAME STREQUAL Android)
  install(
    TARGETS
      test-perf
    RUNTIME DESTINATION test
  )
endif()
