#!/usr/local/bin/perl

use SWISHE;

#$properties='prop1 prop2 prop3';
#$sortspec='prop1 asc prop2 desc';
$searchstring='meta1=metatest1';

$indexfilename1='../tests/test.index';
$indexfilename2='../tests/test.index';
#$indexfilename2='another.index';
$indexfilename="$indexfilename1";
# To search for several index just put them together
$indexfilename="$indexfilename1 $indexfilename2";

#Stemming sample
$word="parking";
$stem_word=SwishStem($word);
print "$word is stemmed as $stem_word\n";


unless($handle=SWISHE::SwishOpen($indexfilename))
{
	print "Could not open index files\n";
	die;
}

# Need some info from header ? Here is how
# Since we have open two files, two values are returned
@wordchars=SWISHE::SwishHeaderParameter($handle,"WordCharacters");
print "WordCharacters 0 = @wordchars[0]\n";
print "WordCharacters 1 = @wordchars[1]\n";

@beginchars=SWISHE::SwishHeaderParameter($handle,"BeginCharacters");
print "BeginCharacters 0 = @beginchars[0]\n";
print "BeginCharacters 1 = @beginchars[1]\n";

@endchars=SWISHE::SwishHeaderParameter($handle,"EndCharacters");
print "EndCharacters 0 = @endchars[0]\n";
print "EndCharacters 1 = @endchars[1]\n";

@ignorefirstchar=SWISHE::SwishHeaderParameter($handle,"IgnoreFirstChar");
print "IgnoreFirstChar 0 = @ignorefirstchar[0]\n";
print "IgnoreFirstChar 1 = @ignorefirstchar[1]\n";

@ignorelastchar=SWISHE::SwishHeaderParameter($handle,"IgnoreLastChar");
print "IgnoreLastChar 0 = @ignorelastchar[0]\n";
print "IgnoreLastChar 1 = @ignorelastchar[1]\n";

@indexedon=SWISHE::SwishHeaderParameter($handle,"Indexed on");
print "Indexed on 0 = @indexedon[0]\n";
print "Indexed on 1 = @indexedon[1]\n";

@description=SWISHE::SwishHeaderParameter($handle,"Description");
print "Description 0 = @description[0]\n";
print "Description 1 = @description[1]\n";

@indexpointer=SWISHE::SwishHeaderParameter($handle,"IndexPointer");
print "IndexPointer 0 = @indexpointer[0]\n";
print "IndexPointer 1 = @indexpointer[1]\n";

@indexadmin=SWISHE::SwishHeaderParameter($handle,"IndexAdmin");
print "IndexAdmin 0 = @indexadmin[0]\n";
print "IndexAdmin 1 = @indexadmin[1]\n";

@stemming=SWISHE::SwishHeaderParameter($handle,"Stemming");
print "Stemming 0 = @stemming[0]\n";
print "Stemming 1 = @stemming[1]\n";

@soundex=SWISHE::SwishHeaderParameter($handle,"Soundex");
print "Soundex 0 = @soundex[0]\n";
print "Soundex 1 = @soundex[1]\n";

# Do you want know the stopwords? Here is how
@stopwords=SWISHE::SwishStopWords($handle,$indexfilename1);
print "StopWords =";
for($i=0;@stopwords[$i];$i++)
{
	print " @stopwords[$i]"
}
print "\n";

# Do you want know the indexeded words starting with 't'? Here is how
@keywords=SWISHE::SwishWords($handle,$indexfilename1,"t");
print "KeyWords =";
for($i=0;@keywords[$i];$i++)
{
	print " @keywords[$i]"
}
print "\n";

$structure=1;

# Uncomment for an endless loop
#while (<>)
#{
$num_results=SwishSearch($handle,$searchstring,$structure,$properties,$sortspec);

if ($num_results<0) 
{
	print "Search error: $num_results\n";
} else{ 
	print "Search Results: $num_results\n";
}

while(($rank,$indexfile,$filename,$docdate,$title,$summary,$start,$size,$prop1,$prop2,$prop3)=SWISHE::SwishNext($handle))
{
   print "$rank $indexfile $filename \"$docdate\" \"$title\" \"$summary\" $start $size \"$prop1\" \"$prop2\" \"$prop3\"\n";
}
# Uncomment for an endless loop
#}

SWISHE::SwishClose($handle);
