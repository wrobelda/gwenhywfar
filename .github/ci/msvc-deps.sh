#!/bin/bash
# Prepare a dependency prefix for building with MSVC against the
# MinGW-built libraries of MSYS2.
#
# usage: msvc-deps.sh PREFIX MODULE...
#
# For each pkg-config MODULE and everything it requires, the script
# copies the .pc file into PREFIX/lib/pkgconfig with prefix= set to
# PREFIX, and the include files of the MSYS2 package that owns it into
# PREFIX/include. For each -lNAME that resolves to a MinGW import
# library libNAME.dll.a, it writes an MSVC import library PREFIX/lib/NAME.lib,
# generated from the exports of the DLL with gendef and lib.exe.
#
# PREFIX is not a system directory, so pkg-config keeps its -I and -L
# flags, and MSVC sees neither the MinGW C library headers nor the MinGW
# import and static libraries in the MSYS2 prefix.

set -euo pipefail

deps=$1
shift
mingw=${MINGW_PREFIX:?run this in an MSYS2 MinGW environment}
case ${MSYSTEM_CARCH:-x86_64} in
  x86_64) machine=x64 ;;
  aarch64) machine=arm64 ;;
  i686) machine=x86 ;;
  *) echo "unknown architecture: $MSYSTEM_CARCH" >&2; exit 1 ;;
esac

mkdir -p "$deps/include" "$deps/lib/pkgconfig"
deps_m=$(cygpath -m "$deps")
mingw_m=$(cygpath -m "$mingw")

# .pc files and include files of the modules and of everything they need
todo="$*"
seen=" "
while [ -n "${todo// /}" ]; do
  read -r m todo <<<"$todo"
  case $seen in *" $m "*) continue ;; esac
  seen="$seen$m "
  pc=$mingw/lib/pkgconfig/$m.pc
  sed -e "s|^prefix=.*|prefix=$deps_m|" \
      -e "s|$mingw_m/|$deps_m/|g" -e "s|$mingw/|$deps_m/|g" \
      "$pc" >"$deps/lib/pkgconfig/$m.pc"
  pkg=$(pacman -Qoq "$pc")
  pacman -Qlq "$pkg" | { grep "^$mingw/include/.*[^/]$" || true; } |
    while read -r f; do
      d=$deps/include/${f#"$mingw"/include/}
      mkdir -p "${d%/*}"
      cp "$f" "$d"
    done
  todo="$todo $(pkg-config --print-requires --print-requires-private "$m" |
                awk '{print $1}' | tr '\n' ' ')"
done

# MSVC import libraries for the MinGW DLLs
for flag in $(pkg-config --libs-only-l "$@"); do
  name=${flag#-l}
  implib=$mingw/lib/lib$name.dll.a
  [ -f "$implib" ] || continue
  dll=$(dlltool --identify "$implib")
  def=$deps/lib/$name.def
  gendef - "$mingw/bin/$dll" >"$def"
  lib -nologo -machine:"$machine" -def:"$(cygpath -m "$def")" \
      -name:"$dll" -out:"$deps_m/lib/$name.lib"
  rm -f "$def" "$deps/lib/$name.exp"
done

PKG_CONFIG_PATH=$deps/lib/pkgconfig pkg-config --cflags --libs "$@"
