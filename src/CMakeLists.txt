# vim:ts=2:sw=2:et

list(APPEND EIXX_SRCS
  am.cpp
  basic_otp_node_local.cpp
  test_node.cpp
)

add_library(${PROJECT_NAME} SHARED ${EIXX_SRCS})
add_library(${PROJECT_NAME}_static STATIC ${EIXX_SRCS})
set_target_properties(${PROJECT_NAME}_static PROPERTIES OUTPUT_NAME ${PROJECT_NAME})
target_link_libraries(${PROJECT_NAME} ${EIXX_LIBS})

if (${CMAKE_BUILD_TYPE} STREQUAL "debug")
  set_target_properties(${PROJECT_NAME} PROPERTIES
    DEBUG_POSTFIX   "${LIB_SUFFIX}"
    RELEASE_POSTFIX "")
  set_target_properties(${PROJECT_NAME}_static PROPERTIES
    DEBUG_POSTFIX   "${LIB_SUFFIX}"
    RELEASE_POSTFIX "")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION})

add_executable(test-node test_node.cpp)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
target_link_libraries(test-node eixx pthread)

install(
  TARGETS
    ${PROJECT_NAME}
    ${PROJECT_NAME}_static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)
install(
  TARGETS
    test-node
  RUNTIME DESTINATION test
)
