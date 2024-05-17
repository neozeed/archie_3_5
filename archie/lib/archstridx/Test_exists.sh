#!/bin/sh

try() {
  cmd=$1 ; exp=$2

  res=`$cmd`
  if expr match "$res" "$exp" >/dev/null ; then
    echo OK 1>&2
  else
    echo "FAILED: actual result, $res, not equal to expected result, $exp." 1>&2
  fi
}

tdir=/tmp/`basename $0`.$$
str=$tdir/Stridx.Strings
spl=$tdir/Stridx.Split
typ=$tdir/Stridx.Type

trap "rm -f $tdir/Stridx.* 2>/dev/null ; rmdir $tdir" 0


mkdir $tdir
chmod 000 $tdir

#
# Test: can't access directory.
#
try "Test_exists $tdir" "0 .*"

#
# Test: no files exist.
#
chmod 755 $tdir
try "Test_exists $tdir" "1 0"

#
# Test: one file exists
#
touch $str
try "Test_exists $tdir" "1 -1" ; rm $str

touch $spl
try "Test_exists $tdir" "1 -1" ; rm $spl

touch $typ
try "Test_exists $tdir" "1 -1" ; rm $typ

#
# Test: all files exist
#
touch $str $spl $typ
try "Test_exists $tdir" "1 1"
