# !/usr/bin/perl -w
use strict;

## See documentation below.  Script may require customization
## read documentation with "perldoc index_hypermail.pl"

use File::Find;
use Date::Parse;
use HTML::TreeBuilder;
use Data::Dumper;

## This is the string that is removed while indexing from email addresses
## as defined in the hypermailrc file.
#
#---------------------- config -----------------------------------------------------

my $dumb_spamblock = '(at)not-real.';

#------------------------------------------------------------------------------------


my $dir = shift || die "must specfy directory to search";

debug(@ARGV) if $dir eq 'debug';

# Do all the work

find( { wanted => \&wanted }, $dir );




sub wanted {
    return if -d;  # don't need to process directories
    return unless /^\d+\.html$/;

    # If you want it to parse using HTML::Parser use the first line
    # and comment out the second.  But it's a LOT slower

    #output_file( $File::Find::name,  parse_file($_) );
    output_file( $File::Find::name,  fast_parse($_) );
}


sub output_file {
    my ( $file, $data ) = @_;

    local $SIG{__WARN__} = sub { "$file: @_" };

    # Get last_mod date
    my $date = str2time( $data->{comments}{received} );
    unless ( $data ) {
        warn "Failed to parse received date in $file\n";
        $date = str2time( $data->{comments}{send} );
        unless ( $date ) {
            warn "Failed to parse any dates: skipping $file\n";
            return;
        }
    }

    $data->{received} = $date;

    my $comments = $data->{comments};

    $comments->{email} =~ s/\Q$dumb_spamblock/-blabla-/;

    my $metas = join "\n", map { qq[<meta name="$_" content="$comments->{$_}">] }
        sort keys %{$data->{comments}};

    my $title = $comments->{subject} || '';

    my $html = <<EOF;
<html>
<head>
<title>$title</title>
$metas
</head>
<body>
    $data->{body}
</body>
</html>
EOF

    my $bytecount = length pack 'C0a*', $html;

    print <<EOF;
Path-Name: $file
Content-Length: $bytecount
Last-Mtime: $date
Document-Type: HTML*

EOF

    print $html;

}


sub parse_file {
    my ( $file ) = @_;

    my %data;  # Return hash of data

    my $tree = HTML::TreeBuilder->new;
    $tree->store_comments(1);  # meta data is in the comments
    $tree->warn(1);

    $tree->parse_file( $file );

    my %comments;
    # Extract out metadata
    for ( $tree->look_down( '_tag', '~comment' )) {
        my $comment = $_->attr("text");
        $comments{$1} = $2 if $comment =~ /(\w+)="([^"]+)/;
    }

    $data{comments} = \%comments if %comments;  # should die here if not.

    # Extract out the searchable content
    my $body = $tree->look_down('_tag', 'div', 'class', 'mail');
    unless ( $body ) {
        warn "$file: failed to find <div class='mail'>\n";
        return;
    }

    # Remove some sub-nodes we don't care about
    $body->look_down('_tag', 'address', 'class', 'headers')->delete;
    $body->look_down('_tag', 'span', 'id', 'received')->delete;

    $data{body} = $body->as_HTML;


    $tree->delete;

    return \%data;

}

sub fast_parse {
    my $file = shift;
    local $_;

    unless ( open FH, "<$file" ) {
        warn "Failed to open '$file'. Error: $!";
        return;
    }

    my %data;
    my %comments;


    # First parse out the comments 
    while (<FH>) {

        if ( my( $tag, $content) = /<!-- ([^=]+)="(.+)" -->$/ ) {

            unless ( $content ) {
                warn "File '$file' tag '$tag' empty content\n";
                next;
            }
            last if $tag eq 'body';  # no more comments in this section
            $comments{$tag} = $content;
        }
    }

    $data{comments} = \%comments;


    # Now grab the content


    my $end_str;  # for skipping sections
    my $body = '';

    while ( <FH> ) {
        # loo for ending tag, or maybe even the signature

        last if /<!-- body="end" -->/ || /^-- $/ || /^--$/ || /^(_|-){40,}\s*$/;

        # Look for ending tag for a skipped tag set

        if ( $end_str ) {
            $end_str = '' if /\Q$end_str/;
            next;
        }


        # These are sections to skip 
        if ( /\Q<address class="headers"/ ) {
            $end_str = "</address>";
            next;
        }

        if (/\Q<span id="received"/ ) {
            $end_str = '</div>';
            next;
        }

        $body .= $_;
    }

    $data{body} = $body;
    return \%data;
}



sub debug {
    for ( @_ ) {
        print STDERR "Debugging [$_]\n";
        my $result = parse_file( $_, 1 );
        print STDERR Dumper $result;
        output_file( $_, $result );
    }
    exit;
}
__END__

=head1 NAME

index_hypermail.pl - Parse Hypermail archive for indexing with Swish-e

=head1 SYNOPSIS

Using an example data structure like this:

    hypermail/
        archive/
        search/

Create the hypermail archive:

    $ cd hypermail
    $ hypermail -i -d archive < messages.mbox

Create a swish-e config file:

    $ cd search
    $ cat swish.conf

    # config for indexing hypermail v2.1.8 archives

    IndexDir ./index_hypermail.pl
    SwishProgParameters ../archive

    MetaNames swishtitle name email
    PropertyNames name email
    IndexContents HTML* .html
    StoreDescription HTML* <body> 100000
    UndefinedMetaTags  ignore

Copy index_hypermail.pl to the current directory.  Swish-e installs
index_hypermail.pl in the $prefix/share/doc/swish-e/examples/prog-bin directory, where
$prefix is typically "/usr/local" or simply "/usr" on some distributions.

    $ cp /usr/local/share/doc/swish-e/example/prog-bin/index_hypermail .

Then 


Index the documents:

    $ swish-e -c swish.conf -S prog

Now create the search interface:

    $ cp /usr/local/lib/swish-e/swish.cgi .
    $ cat .swishcgi.conf

    $ENV{TZ} = 'UTC'; # display dates in UTC format

    return {
        title           => "Search the Foo List Archive",
        display_props   => [qw/ name email swishlastmodified /],
        sorts           => [qw/swishrank swishtitle email swishlastmodified/],
        metanames       => [qw/swishdefault swishtitle name email/],
        name_labels     => {
            swishrank           =>  'Rank',
            swishtitle          =>  'Subject Only',
            name                =>  "Poster's Name",
            email               =>  "Poster's Email",
            swishlastmodified   =>  'Message Date',
            swishdefault        =>  'Subject & Body',
        },

        highlight       => {
            package         => 'SWISH::PhraseHighlight',

            xhighlight_on    => '<font style="background:#FFFF99">',
            xhighlight_off   => '</font>',

            meta_to_prop_map => {   # this maps search metatags to display properties
                swishdefault    => [ qw/swishtitle swishdescription/ ],
                swishtitle      => [ qw/swishtitle/ ],
                email           => [ qw/email/ ],
                name            => [ qw/name/ ],
                swishdocpath    => [ qw/swishdocpath/ ],
            },
        },
    };

Setup web server (OS/web server dependent):

    /var/www # ln -s /path/to/hypermail/search
    /var/www # ln -s /path/to/hypermail/archive

and maybe tell apache to run the script:

    $ cat .htaccess 
    Deny from all
    <files swish.cgi>
        Allow from all
        SetHandler cgi-script
        Options +ExecCGI
    </files>


=head1 DESCRIPTION

This script is used to parse files produced by hypermail.
Last tested with hypermail pre-2.1.9.

It scans the directory passed as the first parameter for files matching \d+\.html
and then extracts out the content, email, name and subject.  This is then passed to
swish-e for indexing.

The swish.cgi script is used for searching the resulting index.  Configuration settings
are stored in the .swish.cgi file located in the current directory.  By default, swish.cgi
expects the current working directory to be the location of the cgi script.  On other web
servers this may not be the case and you will need to edit swish.cgi to use absolute path
names for .swishcgi.conf and the index files.


=head1 USAGE

See the SYNOPSIS above.

If you do not use the directory structure above you may need to use ReplaceRules in
the swish-e config file to adjust the paths stored in the swish-e index file.


=head1 COPYRIGHT

This library is free software; you can redistribute it
and/or modify it under the same terms as Perl itself.

=head1 SEE ALSO

Hypermail can be downloaded from:

   http://hypermail.org

=head1 AUTHOR

Bill Moseley moseley@hank.org. 2004

=head1 SUPPORT

Please contact the Swish-e discussion email list for support with this module
or with Swish-e.  Please do not contact the developers directly.

