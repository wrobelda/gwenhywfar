#!/bin/sh
# Runs one gwentest command as a test of "make installcheck".
#
# Usage: gwentest-run.sh GWENTEST DIR/COMMAND.gwt
#
# GWENTEST is the absolute path to gwentest. The command name is the base
# name of the .gwt file. All commands run in DIR/run, because some read the
# files that earlier ones write. Each runs with stdin from /dev/null and a
# time limit.
#
# Environment:
#   GWENTEST_TIMEOUT  time limit in seconds (default 300)
#   GWENTEST_SKIP     space-separated commands to skip
set -u

if [ $# -ne 2 ]; then
  echo "Usage: $0 GWENTEST DIR/COMMAND.gwt" >&2
  exit 99
fi

gwentest=$1
name=$(basename "$2" .gwt)
dir=$(dirname "$2")/run

case " ${GWENTEST_SKIP:-} " in
  *" $name "*)
    echo "$name is in GWENTEST_SKIP"
    exit 77
    ;;
esac

cd "$dir" || exit 99
# Perl's alarm gives a time limit on both Linux and macOS, which has no
# timeout(1) by default.
exec perl -e 'alarm shift; exec @ARGV or die "exec: $!\n"' \
  "${GWENTEST_TIMEOUT:-300}" "$gwentest" "$name" </dev/null
