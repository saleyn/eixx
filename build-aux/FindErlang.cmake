# vim:ts=2:sw=2:et
# - Find Erlang
# This module finds if Erlang is installed and determines where the
# include files and libraries are. This code sets the following
# variables:
#
#  Erlang_ERL              = the full path to the Erlang runtime
#  Erlang_COMPILE          = the full path to the Erlang compiler
#  Erlang_EI_DIR           = the full path to the Erlang erl_interface path
#  Erlang_ERTS_DIR         = the full path to the Erlang erts path
#  Erlang_EI_INCLUDE_DIR   = /include appended to Erlang_EI_DIR
#  Erlang_EI_LIBRARY_DIR   = /lib appended to Erlang_EI_DIR
#  Erlang_ERTS_INCLUDE_DIR = /include appended to Erlang_ERTS_DIR
#  Erlang_ERTS_LIBRARY_DIR = /lib appended to Erlang_ERTS_DIR
#

IF (NOT Erlang_DIR)
  SET(ERLANG_BIN_PATH
    $ENV{ERLANG_HOME}/bin
    /opt/bin
    /sw/bin
    /usr/bin
    /usr/local/bin
    /opt/local/bin
    )
  FIND_PROGRAM(Erlang_ERL
    NAMES erl
    PATHS ${ERLANG_BIN_PATH}
  )

  FIND_PROGRAM(Erlang_COMPILE
    NAMES erlc
    PATHS ${ERLANG_BIN_PATH}
  )

  EXECUTE_PROCESS(
    COMMAND erl -noshell -eval "io:format(\"~s\", [code:lib_dir()])" -s erlang halt
    OUTPUT_VARIABLE Erlang_OTP_LIB_DIR
  )

  EXECUTE_PROCESS(
    COMMAND erl -noshell -eval "io:format(\"~s\", [code:root_dir()])" -s erlang halt
    OUTPUT_VARIABLE Erlang_OTP_ROOT_DIR
  )

  EXECUTE_PROCESS(
    COMMAND
    erl -noshell -eval "io:format(\"~s\", [code:lib_dir(erl_interface)])" -s erlang halt
    OUTPUT_VARIABLE Erlang_EI_DIR
  )

  EXECUTE_PROCESS(
    COMMAND
    erl -noshell -eval "io:format(\"~s\", [erlang:system_info(otp_release)])" -s erlang halt
    OUTPUT_VARIABLE Erlang_OTP_VERSION
  )

  MESSAGE(STATUS "Using Erlang OTP: ${Erlang_OTP_ROOT_DIR} - found OTP version ${Erlang_OTP_VERSION}")

  EXECUTE_PROCESS(
    COMMAND ls ${Erlang_OTP_ROOT_DIR}
    COMMAND grep erts
    COMMAND sort -n
    COMMAND tail -1
    COMMAND tr -d \n
    OUTPUT_VARIABLE Erlang_ERTS_DIR
  )

  MESSAGE(STATUS "Using erl_interface version: ${Erlang_EI_DIR}")
  MESSAGE(STATUS "Using erts version: ${Erlang_ERTS_DIR}")

  SET(Erlang_EI_DIR           ${Erlang_EI_DIR}                       CACHE STRING "Erlang EI Dir")
  SET(Erlang_EI_INCLUDE_DIR   ${Erlang_OTP_ROOT_DIR}/usr/include     CACHE STRING "Erlang EI Include Dir")
  SET(Erlang_EI_LIBRARY_DIR   ${Erlang_OTP_ROOT_DIR}/usr/lib         CACHE STRING "Erlang EI Libary Dir")
  SET(Erlang_EI_LIBRARIES     ei                                     CACHE STRING "Erlang EI Libraries")

  SET(Erlang_DIR              ${Erlang_OTP_ROOT_DIR}                 CACHE STRING "Erlang Root Dir")
  SET(Erlang_ERTS_DIR         ${Erlang_OTP_ROOT_DIR}/${Erlang_ERTS_DIR})
  SET(Erlang_ERTS_INCLUDE_DIR ${Erlang_OTP_ROOT_DIR}/${Erlang_ERTS_DIR}/include)
  SET(Erlang_ERTS_LIBRARY_DIR ${Erlang_OTP_ROOT_DIR}/${Erlang_ERTS_DIR}/lib)

  FIND_PROGRAM(Erlang_ARCHIVE
    NAMES zip
    PATHS ${ERLANG_BIN_PATH}
  )
  MARK_AS_ADVANCED(
    Erlang_ERL
    Erlang_COMPILE
    Erlang_ARCHIVE
#	Erlang_EI_DIR
#	Erlang_EI_INCLUDE_DIR
#	Erlang_EI_LIBRARY_DIR
  )

ENDIF(NOT Erlang_DIR)
