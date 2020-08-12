export

CXX ?= c++
AR ?= ar
BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE ?=

# -fsanitize=address не совместим с моими настройками overcommit. То же с -fsanitize=thread
ifeq ($(RELEASE),1)
CPPFLAGS ?= -DNDEBUG
CXXFLAGS ?= -O3 -g -Wall -Wextra -pedantic
LDFLAGS ?= -O3 -g
else
CPPFLAGS ?=
CXXFLAGS ?= -g -Wall -Wextra -pedantic -fsanitize=undefined,bounds,nullability,float-divide-by-zero,implicit-conversion -fno-sanitize-recover=all -fno-omit-frame-pointer
LDFLAGS ?= -g -fsanitize=undefined,bounds,nullability,float-divide-by-zero,implicit-conversion -fno-sanitize-recover=all -fno-omit-frame-pointer
endif

all: lib.a

.DELETE_ON_ERROR:

FORCE:

libsh-treis.hpp: libsh-treis.cpp
	grep -E '//@' $< | sed -E 's~ *//@ ?~~' > $@

gnu-source.hpp: gnu-source.cpp
	grep -E '//@' $< | sed -E 's~ *//@ ?~~' > $@

# Похоже, все остальные сорцы собираются без -DBOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE, и это плохо
libsh-treis.o: libsh-treis.cpp FORCE
	if [ -n '$(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE)' ]; then \
		./compile '$(MAKE)' $< $(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a -DBOOST_STACKTRACE_USE_BACKTRACE -DBOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE=\"'$(BOOST_STACKTRACE_BACKTRACE_INCLUDE_FILE)'\" ; \
	else \
		./compile '$(MAKE)' $< $(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a; \
	fi

gnu-source.o: gnu-source.cpp FORCE
	./compile '$(MAKE)' $< $(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a

lib.a: libsh-treis.o gnu-source.o
	rm -f $@ && $(AR) rcsD $@ $^
