export

CXX ?= c++
CPPFLAGS ?= -DNDEBUG
CXXFLAGS ?= -O3 -g -Wall -Wextra -pedantic
LDFLAGS ?=
BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE ?=

all: lib.a

libsh-treis.hpp: libsh-treis.cpp
	grep '//@' $< | sed 's~ *//@\( \|\)~~' > $@ || { rm -f $@; exit 1; }

gnu-source.hpp: gnu-source.cpp
	grep '//@' $< | sed 's~ *//@\( \|\)~~' > $@ || { rm -f $@; exit 1; }

libsh-treis.o: libsh-treis.cpp libsh-treis.hpp gnu-source.hpp
	if [ -n '$(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE)' ]; then \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a -DBOOST_STACKTRACE_USE_BACKTRACE -DBOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE=\"'$(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE)'\" -c $<; \
	else \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a -c $<; \
	fi

gnu-source.o: gnu-source.cpp gnu-source.hpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a -c $<

lib.a: libsh-treis.o gnu-source.o
	rm -f $@; ar rsD $@ $^ || { rm -f $@; exit 1; }
