#!/usr/local/bin/perl -w
use strict;

=pod

    This is an example program to index data stored in a MySQL database.

    In this example, a table is read that contains "minutes" from some
    organization's meetings.  The text of the minutes was stored in a
    blob, compressed with zlib.

    This script reads the record from MySQL, uncompresses the text, and
    formats for swish-e.  The example below uses HTML, but you could
    also format at XML, or even plain text.

=cut    


use DBI;
use Compress::Zlib;
use Time::Local;

my %database = {
        dsn      => 'dbi:mysql:test',
        user     => 'user',
        password => 'pass',
};

my $dbh = DBI->connect( @database{qw/ dsn user password /}, { RaiseError => 1 } );

my $sth = $dbh->prepare("select id, date, minutes from meetings");

$sth->execute();

while ( my( $id, $date, $minutes) = $sth->fetchrow_array ) {

    my $uncompressed = uncompress( $minutes );

    my $unix_date = unixtime( $date );
    

    my $content = <<EOF;
<html>
<head>
<title>
    Minutes for meeting on date $date
</title>
</head>
<body>
$uncompressed
</body>
</html>
EOF

    my $length = length $content;

    

    print <<EOF;
Content-Length: $length
Last-Mtime: $unix_date
Path-Name: /minutes/index.cgi?id=$id
Document-Type: HTML

EOF
    print $content;

}

sub unixtime {
        my ( $y, $m, $d ) = split /-/, shift;
        return timelocal(0,0,0,$d,$m-1,$y-1900);
    };

