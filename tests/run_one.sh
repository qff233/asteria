#!/bin/bash

if [[ "$#" -lt 1 ]]; then
	printf '\e[31;1mNo source file provided.  FAILED\e[0m\n' >&2
	exit 2;
fi

_file="$(readlink -e "$1")"
shift

_cxx="g++"
_cxxflags="-std=c++11 -O0 -g3 -fPIC -Wall -Wextra -Werror -pedantic -pedantic-errors -D_GLIBCXX_DEBUG -D_GLIBCXX_DEBUG_PEDANTIC"
_ldflags="-L../lib -L../lib/.libs"
_runpath="../lib/.libs"
_libs="-lasteria"
_output="test.out~"
_errlog="stderr.log~"

printf '\e[33;1mRUNNING\e[0m  %s\n' "${_file}"
if ! ("${_cxx}" ${_cxxflags} ${_ldflags} "$_file" ${_libs} -o "${_output}" $* && LD_LIBRARY_PATH="${_runpath}" "./${_output}") >&2; then
	printf '\e[31;1mFAILED\e[0m   %s\n' "${_file}"
	exit 1
fi
printf '\e[32;1mPASSED\e[0m   %s\n' "${_file}"
