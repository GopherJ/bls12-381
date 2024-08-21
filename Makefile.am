ACLOCAL_AMFLAGS = -I build-aux/m4

AM_CXXFLAGS = -std=c++20 @CFLAGS@
AM_CPPFLAGS = -std=c++20 @CFLAGS@

AM_CFLAGS = @CFLAGS@

lib_LTLIBRARIES = libantelope.la

include_HEADERS = include/bls12-381/arithmetic.hpp include/bls12-381/bls12-381.hpp include/bls12-381/fp.hpp include/bls12-381/g.hpp include/bls12-381/pairing.hpp include/bls12-381/scalar.hpp include/groth16/groth16.hpp include/groth16/proof.hpp include/groth16/serialize.hpp include/groth16/vkey.hpp


libantelope_la_SOURCES = src/arithmetic.cpp src/fp.cpp src/g.cpp src/groth16.cpp src/pairing.cpp src/scalar.cpp
libantelope_la_LDFLAGS = -version-info $(LIB_VERSION_CURRENT):$(LIB_VERSION_REVISION):$(LIB_VERSION_AGE) @LDFLAGS@

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libantelope.pc


