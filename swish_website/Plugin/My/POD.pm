package My::POD;
use strict;
use Pod::POM;
use Pod::POM::View::HTML;
use base 'Template::Plugin';

use vars '@pod_toc';


my $mode = 'Pod::POM::View::HTML';  # The view module

# Much of this is based on (or copied from) Stas' DocSet 0.17
# I'm not sure why spaces need to be removed from links.  Pod::POM doesn't remove them.

sub new {
    my ( $class, $context, $content ) = @_;

    # How's this for a hack?
    return \@pod_toc if $content eq 'toc';


    # Grab pod index variable
    my $stash = $context->stash;
    my $page = $stash->get( ['page', 0, 'id', 0 ] );

    my $parser = Pod::POM->new( warn => 0 );  # Need to fetch template.name for warnings

    my $pom = $parser->parse_text( $content );

    combine_verbatim_sections_hack( $pom );

    my @sections = $pom->head1;  # get pod into sections

    my %data = ( pom => $pom );


    # Get title of document
    if ( @sections && $sections[0]->title =~ /NAME/ ) {
        $data{title} = ( shift @sections )->content;
        $data{title} =~ s/^\s*|\s*$//sg;  # strip;
    }

    $data{sections} = \@sections;

    $data{toc} = fetch_toc( \@sections );


    # Look for an overview

    push @pod_toc, {
        page    => $page,
        title   => $data{title},
        abstract => fetch_abstract( $data{sections} ),
    };


    $data{view} = 'My::Pod::View::HTML';


    return \%data;
}

sub combine_verbatim_sections_hack {
    my $tree = shift;
    my @content = $tree->content;
    my $size = @content - 1;

    my $x = 0;
    while ( $x <= $size ) {

        my $item = $content[$x];

        my $type = $item->type;
        while ( $type eq 'verbatim' && $x < $size && $type eq $content[$x+1]->type ) {
            $item->{text} .= "\n\n" . $content[$x+1]->text;
            $content[$x+1]->{text} = '';
            $x++;
        }
        combine_verbatim_sections_hack($item);
        $x++;
    }
}


sub fetch_abstract {
    my ( $sections ) = @_;
    for ( 0 .. 2 ) {
        next unless $sections->[$_] && $sections->[$_]->title =~ /DESCRIPTION|OVERVIEW/;

        my $abstract = $sections->[$_]->content->present($mode);
        $abstract =~ s|<p>(.*?)</p>.*|$1|s;
        return $abstract;
    }
}


sub fetch_toc {
    my ( $sections ) = @_;

    my @toc = ();
    my $level = 1;
    for my $node (@$sections) {
        push @toc, render_toc_level($node, $level);
    }
    return \@toc;
}

# From DocSet POD.pm

sub render_toc_level {
    my( $node, $level) = @_;
    my $title = $node->title;
=pod
    my $link = "$title";     # must stringify to get the raw string
    $link =~ s/^\s*|\s*$//g; # strip leading and closing spaces
    $link =~ s/\W/_/g;       # META: put into a sub? see Doc::Common::pod_pom_html_anchor
    # prepand '#' for internal links
    my $toc_link = "toc_$link"; # self referring toc entry
    $link = "#$link";
=cut


    my %toc_entry = (
        title    => $title->present($mode), # run the formatting if any
        link     => "#$title",
    );

    my @sub = ();
    $level++;
    if ($level <= 4) {
        # if there are deeper than =head4 levels we don't go down (spec is 1-4)
        my $method = "head$level";
        for my $sub_node ($node->$method()) {
            push @sub, render_toc_level($sub_node, $level);
        }
    }
    $toc_entry{subs} = \@sub if @sub;

    return \%toc_entry;
}

package My::Pod::View::HTML;
use strict;
use warnings;
use base 'Pod::POM::View::HTML';

sub view_head1 {
    my ($self, $head1) = @_;
    return "<h1>" . $self->anchor($head1->title) . "</h1>\n\n" .
        $head1->content->present($self);
}

sub view_head2 {
    my ($self, $head2) = @_;
    return "<h2>" . $self->anchor($head2->title) . "</h2>\n\n" .
        $head2->content->present($self);
}

sub view_head3 {
    my ($self, $head3) = @_;
    return "<h3>" . $self->anchor($head3->title) . "</h3>\n\n" .
        $head3->content->present($self);
}

sub view_head4 {
    my ($self, $head4) = @_;
    return "<h4>" . $self->anchor($head4->title) . "</h4>\n\n" .
        $head4->content->present($self);
}

sub anchor {
    my($self, $title) = @_;
    my $text = $title->present($self);
    return qq[<a name="$title"></a>$title];


    my $anchor = "$title";
    $anchor =~ s/^\s*|\s*$//g; # strip leading and closing spaces
    my $anchor_with_spaces = $anchor;
    $anchor =~ s/\W/_/g;
    my $link = $title->present($self);

    # Stas/DocSet checks for duplicate anchors.  Probably a good idea.

    # die on duplicated anchors
    #return qq{<a name="$anchor"></a><a href="#toc_$anchor">$link</a>};
    return qq{<a name="$anchor_with_spaces"></a><a name="$anchor"></a>$link};
}

# This just adds a class attribute
sub view_verbatim {
    my ($self, $text) = @_;
    return '' unless $text;
    for ($text) {
        s/&/&amp;/g;
        s/</&lt;/g;
        s/>/&gt;/g;
    }

    return qq{<pre class="pre-section">$text</pre>\n};
}


# This needs work

sub view_seq_link_transform_path {
    my ( $self, $link ) = @_;
    return "$link.html";
}



# Pod::POM::View::HTML:
# view_seq_link is broken in many ways
# L<Foo Bar> ends up as a section due to spaces
# L</foo> or L<|/foo> don't work, but L<foo|foo> does work
#
#
#

# Modified version of item display that removes the item_ prefix and only takes the first
# word


sub view_item {
    my ($self, $item) = @_;

    my $title = $item->title();

    if (defined $title) {
        $title = $title->present($self) if ref $title;
        $title =~ s/\Q*//;
        if (length $title) {
            my $anchor = $title;
            $anchor =~ s/^\s*|\s*$//g; # strip leading and closing spaces
            $anchor =~ s/^(\S+).*$/$1/;
            #$anchor =~ s/\W/_/g;
            $title = qq{<a name="$anchor"></a><b>$title</b>};
        }
    }

    return '<li>'
        . "$title\n"
        . $item->content->present($self)
        . "</li>\n";
}


1;

