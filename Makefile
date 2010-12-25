ERL_ROOT=/opt/erlang/R14A
BOOST_ROOT=/opt/boost
ERL_INTERFACE=$(ERL_ROOT)/lib/erlang/lib/erl_interface-3.7
CPPFLAGS = $(if $(tr1),-std=c++0x) -I./include -isystem $(BOOST_ROOT)/include \
			-isystem $(ERL_INTERFACE)/include -isystem $(ERL_INTERFACE)/src \
			$(if $(debug),-ggdb -D_GLIBCXX_DEBUG)
LDFLAGS  = -L$(ERL_INTERFACE)/lib -L$(BOOST_ROOT)/lib -L./lib -lboost_system -lei

TARGETS  = lib/libeixx.so test_perf test_eterm src/test_node

all: $(TARGETS)

test_eterm_SOURCES = test/test_eterm.cpp test/test_eterm_encode.cpp \
					 test/test_eterm_format.cpp test/test_eterm_match.cpp \
					 test/test_eterm_pool.cpp test/test_eterm_refc.cpp \
					 test/test_mailbox.cpp

obj:
	mkdir $@

#obj/%.o: src/%.cpp
#	g++ -g -c -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0) \
#		-o $@ $< $(CPPFLAGS) $(LDFLAGS)
#
#lib/libeixx.a: obj obj/atom.o obj/basic_otp_node_local.o \
#		$(wildcard include/eixx/*.?pp) $(wildcard include/eixx/connect/*.?pp) \
#		$(wildcard include/eixx/marshal/*.?pp)
#	ar -vru $@ $(filter obj/%.o,$^)

lib/libeixx.so: src/atom.cpp src/basic_otp_node_local.cpp
	g++ -g3 -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0) \
		-o $@ $(filter src/%.cpp,$^) $(CPPFLAGS) -shared -fPIC $(LDFLAGS)

test_eterm: $(test_eterm_SOURCES) \
		    $(wildcard include/eixx/*.?pp) $(wildcard include/eixx/connect/*.?pp) \
			$(wildcard include/eixx/marshal/*.?pp) lib/libeixx.so
	g++ -g -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0) \
		$(if $(debug),-DEIXX_DEBUG) \
    	-o $@ $(test_eterm_SOURCES) $(CPPFLAGS) $(LDFLAGS) \
	-DBOOST_TEST_DYN_LINK -lboost_unit_test_framework -leixx -L.

test_perf: test/test_perf.cpp $(wildcard include/eixx/*.?pp) $(wildcard include/eixx/impl/*.?pp)
	g++ -g -O$(if $(optimize),3 -DBOOST_DISABLE_ASSERTS,0) -o $@ $< src/atom.cpp $(CPPFLAGS) $(LDFLAGS)

src/test_node:
	@$(MAKE) --directory=src

clean:
	rm -f $(TARGETS) obj/*.o
	rm -fr obj
tar:
	DIR=$$PWD && cd .. && tar jcf eixx.tbz eixx/include eixx/Makefile eixx/src/ && \
	mv eixx.tbz $$DIR
