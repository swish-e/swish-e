#!/usr/bin/perl -w
use strict;

# swish-e -S program to index the HTML docs in sections.



my $pat = qr[<h\d><a name="([^"]+)">([^<]+)</a></h\d>]i;

opendir( DIR, '.' ) || die "Failed to opendir current directory: $!";

while ( my $name = readdir DIR ) {
    next unless $name =~ /\.html$/;
    next if $name =~ /^index/;

    unless ( open( FH, "<$name" ) ) {
        warn "Failed to open file [$name]\n";
        next;
    }

    local $/;
    index_doc( $name, <FH> );
}

sub index_doc {
    my ($name, $doc) = @_;


    my @sections = split /$pat/, $doc;
    die unless @sections;

    # Get rid of the first part
    shift @sections;


    my ( $title ) = $doc =~ m[<title>([^<]+)]is;
    $title ||= "Swish-e Documentation";

    $title =~ s/^SWISH-Enhanced: //;


    while ( @sections ) {
        my ( $section, $sec_text, $text ) = splice( @sections, 0, 3 );
        output( $name, $section, $sec_text, $text, $title );
    }

}
sub output {
    my ( $name, $section, $sec_text, $text, $title ) = @_;

    my $date = (stat $name)[9];


    my $doc = <<EOF;
<html><head><title>$title : $sec_text</title></head>
<body>$text</body>
</html>
EOF

    my $len = length $doc;
    print <<EOF;
Path-Name: $name#$section
Content-Length: $len
Last-Mtime: $date
Document-Type: HTML*

EOF

    print $doc;
}
