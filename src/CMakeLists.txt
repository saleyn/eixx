# vim:ts=2:sw=2:et

list(APPEND EIXX_SRCS
  am.cpp
)

if (NOT EIXX_MARSHAL_ONLY)
  list(APPEND EIXX_SRCS
    basic_otp_node_local.cpp
    test_node.cpp
  )
endif()

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

if (NOT EIXX_MARSHAL_ONLY)
  add_executable(test-node test_node.cpp)
  target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
  target_link_libraries(test-node eixx pthread)
endif()

# In the install below we split library installation in a separate library clause
# so that it's possible to build/install both Release and Debug versions of the
# library and then include that into a package

install(
  TARGETS
    ${PROJECT_NAME}
    ${PROJECT_NAME}_static
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
)

if (${CMAKE_BUILD_TYPE} STREQUAL "release")
  install(
    FILES ${CMAKE_BINARY_DIR}/src/lib${PROJECT_NAME}_d.so.${PROJECT_VERSION}
    DESTINATION lib
  )
  install(
    FILES ${CMAKE_BINARY_DIR}/src/lib${PROJECT_NAME}_d.so
    DESTINATION lib
  )
  install(
    FILES ${CMAKE_BINARY_DIR}/src/lib${PROJECT_NAME}_d.a
    DESTINATION lib
  )
endif()

if (NOT EIXX_MARSHAL_ONLY)
  install(
    TARGETS
      test-node
    RUNTIME DESTINATION test
  )
endif()