#!/usr/local/bin/tclsh
#
# return a `search result' consisting of <n0> lines, each of
# which has an href of <n1> characters and a text description
# of <n2> characters.

proc one_line {desclen sellen} {
  puts -nonewline 0
  for {set i 0} {$i<$desclen} {incr i} {puts -nonewline d}
  puts -nonewline "\t"
  for {set i 0} {$i<$sellen} {incr i} {puts -nonewline s}
  puts "\tmocha-int.bunyip.com\t70\r"
}

proc search_res str {
  switch [scan $str {%d.%d.%d} nlines hwidth twidth] {
    3 {
      set i 0
      while {$i < $nlines} {
        one_line $hwidth $twidth
        incr i
      }
    }

    default {
      puts "3Bad search pattern\txxx\tmocha-int.bunyip.com\t70\r"
    }
  }
}

  
gets stdin line
set line [string trimright $line "\r\n"]
set l [split $line "\t"]
switch [llength $l] {
  0 { puts "7Search term of form n0.n1.n2\tS\tmocha-int.bunyip.com\t70\r" }
  2 { search_res [lindex $line 1] }
  default { puts "3Bad input\txxx\tmocha-int.bunyip.com\t70\r" }
}
