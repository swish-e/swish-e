#!/usr/bin/perl -w
use strict;
use File::Find;

my $dir = shift || die "failed to specify directory";

find( \&process_doc, $dir );

# swish-e -S program to index the HTML docs in sections.



my $pat = qr[<h\d><a name="([^"]+)">([^<]+)</a></h\d>]i;


sub process_doc {
    my $file = $_;
    my $path = $File::Find::name;
    my $dir  = $File::Find::dir;

    return if -d;

    return unless /\.html$/;  # how's that!



    return if /^\./;
    return if /robots\.txt/;
    return if /\.css$/;
    return if $dir =~ m!/search!;
    return if $dir =~ m!/graphics!;


    unless ( open( FH, "<$path" ) ) {
        warn "Failed to open file [$path]\n";
        return;
    }

    local $/;
    index_doc( $path, <FH> );
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
