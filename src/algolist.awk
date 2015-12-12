BEGIN {
  print "<!DOCTYPE html>"
  print "<html dir=\"ltr\" lang=\"en\"><head>"
  print "<meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\">"
  print "<meta charset=\"UTF-8\"><title>Implemented Algorithms</title></head>"
  print "<body><h1>Algorithms list</h1>"
  print "All base algorithm blocks are written in C and are ",
        "called from SIOD scripts (test*.scm) which produce ",
        "some output images in out/* directory. To see input ",
        "images, click on links of <b>Input images</b> section ",
        "after each algorithm, the same is for <b>Output images</b>"
}

{
  print "<div style=\"padding-left:5px;background:brown;color:white;font-family:Courier\">"
  print "<h2>Algorithm ", NR, "</h2>"
  print "</div>"
  print "<div style=\"font:10pt monospace\">"
  while ((getline line < ($1 ".desc")) > 0) {
    print line, "<br/>"
  }
  print "<br/></div>"
  print "<div style=\"white-space:pre;font:10pt monospace;",
        ";background:wheat;margin:0;padding:0;",
        ";border:dotted 1px darkcyan;color:brown\"><span ",
        "style=\"background:white;color:darkcyan\">", $1, "</span>"
  delete ins; delete outs
  while ((getline line < $1) > 0) {
    idx = match(line, "in/[^\"]+")
    if (idx) {
        f = substr(line, idx, RLENGTH)
        ins[f] = f
    }
    idx = match(line, "out/[^\"]+")
    if (idx) {
        f = substr(line, idx, RLENGTH)
        outs[f] = f
    }
    print line
  }
  print "</div><br/>"
  print "<span style=\"color:brown;font:10pt Arial\">Input images: </span>"
  for (i in ins)
    printf "<a href=\"%s/%s\">%s</a>, ", rooturl, i, i
  print "<br/><span style=\"color:brown;font:10pt Arial\">Output images: </span>"
  for (i in outs)
    printf "<a href=\"%s/%s\">%s</a>, ", rooturl, i, i
  print "<br/>"
  close($2)
}

END {
      print "</body></html>"
}
