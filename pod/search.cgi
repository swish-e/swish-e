#!/usr/local/bin/perl -w
=pod

    If this text is displayed on your browser then your web server
    is not configured to run .cgi programs.

=cut

    use strict;
    use CGI;

    my $swish_binary = './swish-e';

    my $q = CGI->new;


    print $q->header,
          header(),
          $q->start_form,
          '<p><center>',
          $q->textfield(
                -name       => 'query',
                -size       => 40,
                -maxlength  => 60,
          ),
          ' &nbsp; ',
          $q->submit,
          '</center><p>',
          $q->end_form;

    show_results( $q ) if $q->param('query');

    print footer();

    
#============================================
sub show_results {
    my $q = shift;

    my $query = $q->param('query') || '';

    for ( $query ) {
        s/^\s+//;
        s/\s+$//;
        tr/\0//d;
    }

    return unless $query;


    my $pid;

    if ( $pid = open( SWISH, '-|' ) ) {
        my @rows;

        while (<SWISH>) {

            next unless /^\d/;
            chomp;

            my ($rank,$url,$title,$size) = split /::/;

            my $link = $q->a({ href => $url}, ($title || 'no title') );

            push( @rows,
                    $q->td( {align=>'right'}, $rank ) . 
                    $q->td( {align=>'left'}, $link ) .
                    $q->td( {align=>'right'}, $size )
            );
        }
        if ( @rows ) {
            print '<h2 align=center>Found: ',
                  scalar @rows,
                  (scalar @rows > 1) ? ' hits' : ' hit',
                  '</h2>';
                  
            unshift @rows, $q->th(['Rank','Title','Length']);
            print $q->table({align=>'center'}, $q->Tr( \@rows ) );
        } else {
            print '<h2 align=CENTER>Failed to find any results</h2>';
        }
    } else {
        unless ( defined $pid ) {
            print "<h2 align=CENTER>Failed to Fork: $!</h2>";
            return;
        }
        

        exec $swish_binary,
            '-w', $query,
            '-d','::';

        die "Failed to exec '$swish_binary' Error:$!";
    }
}

sub header {
    return <<EOF;
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 3.2//EN">
 
<html>
<head>
    <title>SWISH-Enhanced</title>
    <link href="style.css" rel=stylesheet type="text/css" title="refstyle">
</head>

<body>

    <h1 class="banner">
        <img src="images/swish.gif" alt="Swish Logo"><br>
        <img src="images/swishbanner1.gif"><br>
        <img src="images/dotrule1.gif"><br>
        <img src="images/swish2.gif" alt="SwishE2 Logo">
    </h1>

    <center>
        <a href="index.html">Table of Contents</a>
    </center>

    <p>
EOF
}

sub footer {
    return <<EOF;
    
    <P ALIGN="CENTER">
    <IMG ALT="" WIDTH="470" HEIGHT="10" SRC="images/dotrule1.gif"></P>
    <P ALIGN="CENTER">

    <div class="footer">
        <BR>SWISH-E is distributed with <B>no warranty</B> under the terms of the
        <A HREF="http://www.fsf.org/copyleft/gpl.html">GNU Public License</A>,<BR>
        Free Software Foundation, Inc., 
        59 Temple Place - Suite 330, Boston, MA  02111-1307, USA<BR> 
        Public questions may be posted to 
        the <A HREF="http://swish-e.org/Discussion/">SWISH-E Discussion</A>.
    </div>

  </body>
</html>
EOF
}



