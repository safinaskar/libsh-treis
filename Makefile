export

include common.mk

CXX = c++
AR = ar

ifeq ($(RELEASE),1)
CPPFLAGS = -DNDEBUG
CXXFLAGS = -O3 -g $(WARNS) -pedantic
LDFLAGS = -O3 -g
else
CPPFLAGS =
CXXFLAGS = -g $(WARNS) -pedantic -fsanitize=undefined,bounds,nullability,float-divide-by-zero,implicit-conversion,address -fno-sanitize-recover=all -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fno-optimize-sibling-calls
LDFLAGS = -g -fsanitize=undefined,bounds,nullability,float-divide-by-zero,implicit-conversion,address -fno-sanitize-recover=all -fno-omit-frame-pointer -fsanitize-address-use-after-scope -fno-optimize-sibling-calls
endif

all: lib.a

.DELETE_ON_ERROR:

FORCE:

libsh-treis.hpp: libsh-treis.cpp
	grep -E '//@' $< | sed -E 's~ *//@ ?~~' > $@

gnu-source.hpp: gnu-source.cpp
	grep -E '//@' $< | sed -E 's~ *//@ ?~~' > $@

libsh-treis.o: libsh-treis.cpp FORCE
	./compile '$(MAKE)' $< $(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a

gnu-source.o: gnu-source.cpp FORCE
	./compile '$(MAKE)' $< $(CXX) $(CPPFLAGS) $(CXXFLAGS) -std=c++2a

lib.a: libsh-treis.o gnu-source.o
	rm -f $@ && $(AR) rcsD $@ $^
