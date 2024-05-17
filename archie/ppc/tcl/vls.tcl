proc namedSublist {name list} {
  set idx [lsearch -regexp $list "^$name "]
  lrange [lindex $list $idx] 1 end
}

# A simple vls
proc vls {dir} {
  foreach file [prsp_rd_vdir $dir] {
    set target [namedSublist TARGET $file]
    if {[string compare $target "DIRECTORY"] == 0} {
      set type D
    } else {
      set type " "
    }
    puts [format "%s  %-30s  %s  %s" \
             $type \
             [namedSublist NAME $file] \
             [namedSublist HOST $file] \
             [namedSublist HSONAME $file]]
  }
}
