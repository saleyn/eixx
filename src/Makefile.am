eixx_LTLIBRARIES = libeixx.la
bin_PROGRAMS = test_node

eixx_hdrs1 =  alloc_pool.hpp \
              alloc_pool_st.hpp \
              alloc_std_debug.hpp \
              alloc_std.hpp \
			  config.h \
              connect.hpp \
              eixx.hpp \
              eterm.hpp \
			  eterm_exception.hpp
eixx_hdrs2 =  connect/basic_otp_connection.hpp \
              connect/basic_otp_mailbox.hpp \
              connect/basic_otp_mailbox_registry.hpp \
              connect/basic_otp_node.hpp \
              connect/basic_otp_node.ipp \
              connect/basic_otp_node_local.hpp \
              connect/test_helper.hpp \
              connect/transport_msg.hpp \
              connect/transport_otp_connection.hpp \
              connect/transport_otp_connection.ipp \
              connect/transport_otp_connection_tcp.hpp \
              connect/transport_otp_connection_tcp.ipp \
              connect/transport_otp_connection_uds.hpp \
              connect/verbose.hpp
eixx_hdrs3 =  marshal/alloc_base.hpp \
              marshal/atom.hpp \
              marshal/binary.hpp \
              marshal/binary.ipp \
              marshal/defaults.hpp \
              marshal/endian.hpp \
              marshal/eterm_format.hpp \
              marshal/eterm_format.ipp \
              marshal/eterm.hpp \
              marshal/eterm.ipp \
              marshal/eterm_match.hpp \
              marshal/list.hpp \
              marshal/list.ipp \
              marshal/pid.hpp \
              marshal/pid.ipp \
              marshal/port.hpp \
              marshal/port.ipp \
              marshal/ref.hpp \
              marshal/ref.ipp \
              marshal/string.hpp \
              marshal/trace.hpp \
              marshal/tuple.hpp \
              marshal/tuple.ipp \
              marshal/varbind.hpp \
              marshal/var.hpp \
              marshal/visit_encoder.hpp \
              marshal/visit_encode_size.hpp \
              marshal/visit.hpp \
              marshal/visit_match.hpp \
              marshal/visit_subst.hpp \
              marshal/visit_to_string.hpp
eixx_hdrs4 =  util/async_wait_timeout.hpp \
              util/common.hpp \
              util/hashtable.hpp \
              util/string_util.hpp \
              util/sync.hpp

eixx_headers= $(eixx_hdrs1) \
			  $(eixx_hdrs2) \
			  $(eixx_hdrs3) \
			  $(eixx_hdrs4)

libeixx_la_hdr1dir		 = $(includedir)/@PACKAGE@
libeixx_la_hdr1_HEADERS  = $(eixx_hdrs1:%=../include/@PACKAGE@/%)
libeixx_la_hdr2dir		 = $(includedir)/@PACKAGE@/connect
libeixx_la_hdr2_HEADERS  = $(eixx_hdrs2:%=../include/@PACKAGE@/%)
libeixx_la_hdr3dir		 = $(includedir)/@PACKAGE@/marshal
libeixx_la_hdr3_HEADERS  = $(eixx_hdrs3:%=../include/@PACKAGE@/%)
libeixx_la_hdr4dir		 = $(includedir)/@PACKAGE@/util
libeixx_la_hdr4_HEADERS  = $(eixx_hdrs4:%=../include/@PACKAGE@/%)

libeixx_la_SOURCES  = atom.cpp defaults.cpp basic_otp_node_local.cpp \
					  $(eixx_headers:%=../include/@PACKAGE@/%)
libeixx_la_LTADD    = -version-info 0:1 -shared
libeixx_la_LIBS     = $(BOOST_THREAD_LIB) $(CRYPTO_LIBS)
libeixx_la_CXXFLAGS = $(if $(tr1),-std=c++0x) -g -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0) \
					  -I../include $(BOOST_CPPFLAGS) $(EI_CPPFLAGS)

#EXTRA_DIST = ../README overview.inl overview_usage.inl overview_config.inl

test_node_SOURCES   = test_node.cpp
test_node_CPPFLAGS  = -g -O0 -I$(srcdir)/../include $(BOOST_CPPFLAGS) $(EI_CPPFLAGS)
test_node_LDFLAGS  	= $(BOOST_LDFLAGS) $(EI_LDFLAGS)
test_node_LDADD		= libeixx.la $(EI_LIB) \
					  $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB) $(CRYPTO_LIBS)
