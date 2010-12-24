ERL_ROOT=/opt/erlang/R14A
BOOST_ROOT=/opt/boost
ERL_INTERFACE=$(ERL_ROOT)/lib/erlang/lib/erl_interface-3.7
CPPFLAGS = $(if $(tr1),-std=c++0x) -I. -I../include -I $(BOOST_ROOT)/include \
		   -I$(ERL_INTERFACE)/include -I$(ERL_INTERFACE)/src
LDFLAGS  = -L$(ERL_INTERFACE)/lib -lei

#TARGETS  = pri_test
TARGETS  = pri_test test_node

all: $(TARGETS)

test_node: test_node.cpp atom.cpp basic_otp_node_local.cpp \
		    $(wildcard ../include/eixx/connect/*.?pp)
	g++ -g -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0 -ggdb -D_GLIBCXX_DEBUG) \
	-o $@ $(wordlist 1,3,$^) $(CPPFLAGS) $(LDFLAGS) \
	-lboost_system -lboost_thread -L$(BOOST_ROOT)/lib 

pri_test: pattern_match_handler.cpp
	g++ -g -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0 -ggdb -D_GLIBCXX_DEBUG) \
	-o $@ $< $(CPPFLAGS) $(LDFLAGS) \
	-lboost_system -lboost_date_time -lboost_thread -L$(BOOST_ROOT)/lib 

clean:
	rm -f $(TARGETS)

tar:
	DIR=$$PWD && cd .. && tar jcf eixx.tbz eixx/include eixx/Makefile eixx/src/ && \
	mv eixx.tbz $$DIR
