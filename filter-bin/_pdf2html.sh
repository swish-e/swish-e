#! /usr/local/bin/perl
# -- Filter PDF to simple HTML for swish 
# --
# -- 2000-05  rasc


$file=$ARGV[0];

$title = "" ;
$subject = "";
$keywords = "";

#
# -- read pdf meta information
#
open (F, "pdfinfo \'$file\' |") || die "$0: Die on $file\n";
  while (<F>) {
    chomp;
    if (m/subject:\s*/i) { $subject = $'; }
    if (m/title:\s*/i) { $title= $'; }
    if (m/keywords:\s*/i) { $keywords = $'; }
  }
close (F);

if ($subject ne "") {
  if ($title ne "")  {
     $title .= " // $subject";
  } else {
     $title = $subject;
  }
}


$|=1;

print "<head>";
print "<title>$title</title>\n";
if ("$keywords" ne "") {
  print "<meta name=\"keywords\" content=\"$keywords\">\n";
}
print  "</head>\n";

# -- To do:  '<' '>' translation to avoid confusion...
print "<body><pre>\n";
system ("pdftotext \'$file\' -");
print "</pre></body>\n";
