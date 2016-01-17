# vim:ts=2:sw=2:et

if(CMAKE_HOST_UNIX)
  get_filename_component(Dir ${CMAKE_INSTALL_PREFIX} DIRECTORY)
  message(STATUS "Symlink: ${Dir}/current -> ${CMAKE_INSTALL_PREFIX}")
  
  execute_process(
    COMMAND rm -f ${Dir}/current
    COMMAND ln -s ${CMAKE_INSTALL_PREFIX} ${Dir}/current
  )
endif()
