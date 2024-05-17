#
# Tcl front-end to Prospero.
#
#                           - Bill Heelan (wheelan@bunyip.com)
#
# To Do:
#
#  - make more robust


proc namedSublist {list name} {
  set idx [lsearch -regexp $list "^$name "]
  lrange [lindex $list $idx] 1 end
}


#
# Return a string of the form <host>::<hsoname>.
#
# bug: should quote the hsoname so Tcl doesn't put {} around it.
#
proc epath_of {item} {
  set host [namedSublist $item HOST]
  set hsoname [prsp_quote [lindex [namedSublist $item HSONAME] 0]]
  return $host::$hsoname
}


proc name_of {item} {
  namedSublist $item NAME
}


proc headline_of {item} {
  set attrs [namedSublist $item ATTRIBUTES]
  set hidx [lsearch $attrs BUNYIP-HEADLINE*]
  if {$hidx < 0} {
    set res [name_of $item]
  } else {
    set res ""
    set hdlns [lrange [lindex $attrs $hidx] 1 end]
    foreach h $hdlns {
      set cidx [lsearch -regexp $attrs "CONTENTS TAGGED [prsp_renocase $h]"]
      if {$cidx == -1} {
        global argv0
        puts stderr "$argv0: headline_of: can't find CONTENTS value for `$h'."
      } else {
        set res [concat $res [lrange [lindex $attrs $cidx] 3 end]]
      }
    }
    # If no CONTENTS were found return the name
    if {[string compare $res ""] == 0} {
      set res [name_of $item]
    }
  }
  return $res
}


# Replace with var. args & options?
proc http_reply {status type encoding size message} {
  #
  # Print the status line.
  #
  switch $status {
    bad_request { puts "HTTP/1.0 400 Bad Request\r" }
    created     { puts "HTTP/1.0 201 Created\r" }
    forbidden   { puts "HTTP/1.0 403 Forbidden\r" }
    found       { puts "HTTP/1.0 302 Found\r" }
    internal    { puts "HTTP/1.0 500 Internal Error\r" }
    method      { puts "HTTP/1.0 303 Method\r" }
    moved       { puts "HTTP/1.0 301 Moved\r" }
    not_found   { puts "HTTP/1.0 404 Not Found\r" }
    ok          { puts "HTTP/1.0 200 OK\r" }
    payment     { puts "HTTP/1.0 402 Payment Required\r" }
    unauth      { puts "HTTP/1.0 401 Unauthorized\r" }
    unimp       { puts "HTTP/1.0 501 Not Implemented\r" }

    default     {
      global argv0
      puts stderr "$argv0: http_reply: unknown status `$status'."
      puts "HTTP/1.0 500 Internal Error\r"
      return
    }
  }

  #
  # Print the MIME headers.
  #
  puts "Server: POSTprocessor/0.0\r"
  puts "MIME-Version: 1.0\r"
  puts "Content-Type: $type\r"
  puts "Content-Transfer-Encoding: $encoding\r"
  if {$size > -1} {
    puts "Content-Length: $size\r"
  }
  puts "\r"

  if {[string length $message] != 0} {
    puts -nonewline $message
  }
}


proc print_wais {res db str} {
  if {[llength $res] == 0} {
    http_reply ok text/html 7bit -1 "\tNothing was found that matched `$str'\r\n"
  } else {
    global srvHost srvPort

    http_reply ok text/html 7bit -1 ""
    puts "<head><title>Items matching `$str'</title></head>\r"
    set nitems [llength $res]
    if {$nitems == 1} {
      puts "<body><h2>1 item matched `$str'</h2>\r\n<p><menu>\r"
    } else {
      puts "<body><h2>$nitems items matched `$str'</h2>\r\n<p><menu>\r"
    }
    foreach item $res {
      set epath [epath_of $item]
      set hdln [headline_of $item]
      set main_hdln [lindex $hdln 0]

      puts "\t<li><a href=\"http://$srvHost:$srvPort/$epath\"><strong>$main_hdln</strong></a>\r"
      foreach h [lrange $hdln 1 end] {
        puts "\t\t<br>$h\r"
      }
    }
    puts "</menu></body>\r"
  }
}

  
proc skip_mail_headers {} {
  while {[gets stdin line] != -1} {
    if {[string compare $line "\r"] == 0 || [string compare $line ""] == 0} {
      return 1
    }
  }
  return 0
}


#
# A request is of the form
#
#  <var1>=<val1>&<var2>=<val2>&...
#
# where <varn> and <valn> have any "special" characters encoded
# in %xx notation.
#
# Split such a line into a list of pairs, in which the first element
# of the pair is <varn> and the second is <valn>.
#
proc split_request {line} {
  set pairList [split $line "&"]
  foreach item $pairList {
    set pair [split $item "="]
    lappend res [list [prsp_dequote [lindex $pair 0]] [prsp_dequote [lindex $pair 1]]]
  }
  return $res
}


proc wais_search {db maxhits str} {
  set code [catch "prsp_get_dir ARCHIE FIND($maxhits,$maxhits,0,$db) \{[list $str *]\}" res]
  if {$code == 0} {
    print_wais $res $db $str
  } else {
    http_reply internal text/html 7bit -1 "\tServer error: search failed: $res.\r\n"
  }
}


#
# These environment variables are set by the server. (Ugh.)
#
#set srvHost $env(BUNYIP_HOST)
#set srvPort $env(BUNYIP_PORT)
set srvHost localhost
set srvPort 1234

#
# Skip the MIME headers.
#
# (None currently passed on...)
#
#if { ! [skip_mail_headers]} {
#  http_reply bad_request text/html 7bit -1 "\tUnexpected EOF on input.\r\n"
#  exit
#}

##
## Extract the database name, maximum number of hits and the search string.
##
#gets stdin line
##
## bug: we should require the variable name to have a certain format, so a
## malicious user can't change an arbitrary variable.
##
#foreach pair [split_request [string trimright $line "\r\n"]] {
#  set var [lindex $pair 0]
#  set val [lindex $pair 1]
#  # bug: hack around forms value="" bug (use value=" ")
#  if {[string compare $val +] == 0} {
#    set val ""
#  }
#  if {[info exists $var] == 0} {
#    set $var $val
#  } else {
#    set $var "[set $var]$val"
#  }
#}
##
## For this to work the request must contain, at least,
## db=<val>&maxhits=<val>&string=<val>
##
#foreach v {db maxhits string} {
#  if {[info exists $v] == 0} {
#    http_reply internal text/html 7bit -1 "\tError in form: no value for `$v'.\r\n"
#    exit
#  }
#}
#
#prsp_init
#wais_search $db $maxhits $string
