export

CXX ?= c++
AR ?= ar
CPPFLAGS ?= -DNDEBUG
CXXFLAGS ?= -O3 -g -Wall -Wextra -pedantic
LDFLAGS ?=
BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE ?=

all: lib.a

.DELETE_ON_ERROR:

FORCE:

libsh-treis.hpp: libsh-treis.cpp
	grep '//@' $< | sed 's~ *//@\( \|\)~~' > $@

gnu-source.hpp: gnu-source.cpp
	grep '//@' $< | sed 's~ *//@\( \|\)~~' > $@

libsh-treis.o: libsh-treis.cpp FORCE
	if [ -n '$(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE)' ]; then \
		./compile $< $(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a -DBOOST_STACKTRACE_USE_BACKTRACE -DBOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE=\"'$(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE)'\" ; \
	else \
		./compile $< $(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a; \
	fi

gnu-source.o: gnu-source.cpp FORCE
	./compile $< $(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a

lib.a: libsh-treis.o gnu-source.o
	rm -f $@ && $(AR) rcsD $@ $^
