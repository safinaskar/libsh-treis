export

CXX ?= c++
CPPFLAGS ?=
CXXFLAGS ?= -O3 -g
LDFLAGS ?=
BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE ?=

all: libsh-treis.o

libsh-treis.hpp: libsh-treis.cpp
	grep '//@' $< | sed 's~ *//@\( \|\)~~' > $@ || { rm -f $@; exit 1; }

libsh-treis.o: libsh-treis.cpp libsh-treis.hpp
	if [ -n '$(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE)' ]; then \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 -DBOOST_STACKTRACE_USE_BACKTRACE -DBOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE=\"'$(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE)'\" -c $<; \
	else \
		$(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++17 -c $<; \
	fi
	touch stamp
