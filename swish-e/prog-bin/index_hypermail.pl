#!/usr/local/bin/perl -w
use strict;

=pod

    This is an example program for use with swish-e's -S prog indexing method.

    This will scan and index a hypermail (http://hypermail.org) mailing list archive.

    You might use a config file such as:

        IndexDir ./index_hypermail.pl
        SwishProgParameters /usr/local/hypermail/foo

        MetaNames swishtitle name email
        PropertyNames name email
        PropertyNamesDate sent
        IndexContents HTML2 .html
        StoreDescription HTML2 <body> 100000
        UndefinedMetaTags  ignore

    The above expects this file (index_hypermail.pl) to be in the current diretory,
    and expects the hypermail files to be in the directory /usr/local/hypermail/foo.

    Index with the command:

        ./swish-e -c swish.conf -S prog

    See perldoc examples/swish.cgi for how to search this index.  Here's a possible
    config file for use with swish.cgi:

    >cat .swishcgi.conf

    return {
        title           => "Search the Foo List Archive",
        swish_binary    => '../swish-e',
        display_props   => [qw/ name email sent /],
        sorts           => [qw/swishrank swishtitle email sent/],
        secondary_sort  => [qw/sent desc/],  
        metanames       => [qw/swishdefault swishtitle name email/],
        name_labels     => {
            swishrank   =>  'Rank',
            swishtitle  =>  'Subject Only',
            name        =>  "Poster's Name",
            email       =>  "Poster's Email",
            sent        =>  'Message Date',
            swishdefault  =>  'Subject & Body',
        },

        highlight       => {
            package         => 'PhraseHighlight',
            show_words      => 10,    # Number of swish words words to show around highlighted word
            max_words       => 100,   # If no words are found to highlighted then show this many words
            occurrences     => 6,     # Limit number of occurrences of highlighted words
            highlight_on    => '<font style="background:#FFFF99">',
            highlight_off   => '</font>',
            meta_to_prop_map => {   # this maps search metatags to display properties
                swishdefault    => [ qw/swishtitle swishdescription/ ],
                swishtitle      => [ qw/swishtitle/ ],
                email           => [ qw/email/ ],
                name            => [ qw/name/ ], 
                swishdocpath    => [ qw/swishdocpath/ ],
            },
        },
        date_ranges     => {
            property_name   => 'sent',      # property name to limit by
            time_periods    => [  
                'All',
                'Today',
                'Yesterday',
                'This Week',
                'Last Week',
                'Last 90 Days',
                'This Month',
                'Last Month',
            ],

            line_break      => 0,
            default         => 'All',
            date_range      => 1,
        },
    };


    
=cut



use File::Find;  # for recursing a directory tree
use Date::Parse;
    
# Recurse the directory(s) passed in on the command line

find( { wanted   => \&wanted, no_chdir => 1, }, @ARGV );


sub wanted {
    return if -d;
    return  unless m!(^|/)\d+\.html$!;

    my $mtime = (stat $File::Find::name )[9];

    my $html = format_message($File::Find::name );
    return unless $html;

    my $size = length $html;

    my $name = $File::Find::name;
    $name =~ s[^./][];

    print <<EOF;
Content-Length: $size
Last-Mtime: $mtime
Path-Name: $name

EOF

    print $html;
}
    
    

sub format_message {
    my $file = shift;
    local $_;

    unless ( open FH, "<$file" ) {
        warn "Failed to open '$file'. Error: $!";
        return;
    }

    my %fields;

    while (<FH>) {
        if ( my( $tag, $content) = /<!-- ([^=]+)="(.+)" -->$/ ) {
            last if $tag eq 'body';

            unless ( $content ) {
                warn "File '$file' tag '$tag' empty content\n";
                next;
            }

            if ( $tag eq 'sent' ) {
                my $date = str2time $content;
                unless ( defined $date ) {
                    warn "File '$file' tag '$tag' failed to parse '$content'\n";
                    next;
                }
                $content = $date;
            }

            if ( $tag eq 'received' ) {
                my $date = str2time $content;
                unless ( defined $date ) {
                    warn "File '$file' tag '$tag' failed to parse '$content'\n";
                    next;
                }
                $content = $date;
            }
                   
            
            if ( $tag eq 'subject' ) {
                $content =~ s/\Q[SWISH-E]\E//;
                if ( $content =~ s/\s*Re:\s*//i ) {
                   $content .= ' (Re)';
                }
            }

            $fields{$tag} = $content;
        }
    }

    # Let's now use received header, if possible
    $fields{sent} = $fields{received} || $fields{sent};

    my $body = '';

    while ( <FH> ) {
        last if /<!-- body="end" -->/ || /^-- $/ || /^--$/ || /^(_|-){40,}\s*$/;

        $body .= $_;
    }

    return  join "\n",
            '<html>',
            '<head>',
            '<title>',
            ($fields{subject} || '' ),
            '</title>',
            map( { qq[<meta name="$_" content="$fields{$_}">] } keys %fields ),
            '</head><body>',
            $body,
            '</body>',
            '</html>',
            '';
}            

