package My::POD;
use strict;
use Pod::POM;
use Pod::POM::View::HTML;
use base 'Template::Plugin';

my %split_by = map {"head".$_ => 1} 1..4;

my $view_mode = 'Pod::POM::View::HTML';  # The view module

# *Much* of this is based on (or copied from) Stas' DocSet 0.17
# This takes a pod file and uses Pod::POM to split it into sections, builds a table
# of contents and generates the HTML.
#
# Will also cache the page's OVERVIEW
# 
# I'm not sure why spaces need to be removed from links.  Pod::POM doesn't remove them.
# Escaping of links and hrefs and name tags needs to be checked.  There was just a discussion
# on the TT list about escaping hrefs.  Then the xhtml validation rejected % and spaces,
# so no replace all non-word chars with an underscore.
#
# Todo:
#   might be nice to cache links to make sure there's no duplicates
#   and also need a way to cross validate links (but can use an external link checker)

sub new {
    my ( $class, $context, $content ) = @_;


    # Grab pod index variable
    my $stash = $context->stash;
    my $page = $stash->get( 'page.id' );
    my $template_name = $stash->get( 'template.name' );

    my $warn = sub { warn "[$template_name]: @_\n" }
        if $stash->get( 'self.config.verbose' );

    my $parser = Pod::POM->new( warn => $warn );    # Make this a coderef to report name
    my $pom = $parser->parse_text( $content );

    combine_verbatim_sections_hack( $pom );     # merge sequential <pre> sections into one


    my @sections = $pom->head1;                 # get pod into sections


    # Structure for returning info to template

    my %data = (
        pom         => $pom ,
        view        => 'My::Pod::View::HTML',
        sections    => \@sections,
        podparts    => [ slice_by_head(@sections) ],
        toc         => fetch_toc( \@sections ),
    );


    # Get title of document
    if ( @sections && $sections[0]->title =~ /NAME/ ) {
        $data{title} = ( shift @sections )->content;
        $data{title} =~ s/^\s*|\s*$//sg;  # strip;
    }


    # Cache this page and overview for creating a table of contents page
    # the bin/build script passes in the hash reference

    if ( my $cache = $stash->get( 'toc_cache' ) ) {
        $cache->{ $template_name } = {
            page    => $page,
            title   => $data{title},
            abstract => fetch_abstract( $data{sections} ),
        };
    };


    return \%data;
}



#-----------------------------------------------------------------------------

sub slice_by_head {
    my @sections = @_;
    my @body = ();
    for my $node (@sections) {
        my @next = ();
        # assumption, after the first 'headX' section, there can only
        # be other 'headX' sections
        my $count = scalar $node->content;
        my $id = -1;
        for ($node->content) {
            $id++;
            next unless exists $split_by{ $_->type };
            @next = splice @{$node->content}, $id;
            last;
        }
        push @body, $node, slice_by_head(@next);
    }
    return @body;
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

        my $abstract = $sections->[$_]->content->present($view_mode);
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
    my $anchor = My::Pod::View::HTML->escape_name( $title );

    my %toc_entry = (
        title    => $title->present($view_mode), # run the formatting if any
        link     => "#$anchor",
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
    $text = $self->escape_name( $text );
    return qq[<a name="$text"></a>$title];
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


# This needs work -- Pod::POM doesn't uri escape the href, for one thing.

sub view_seq_link_transform_path {
    my ( $self, $link ) = @_;
    return $self->escape_uri( lc $link ) . '.html';
}

# Returns $text as uri-escaped string
sub escape_uri {
    my ($self, $text) = @_;
    $text =~ s/\n/ /g;
    return Template::Filters::uri_filter( $text );
}

sub escape_name {
    my ($self, $text) = @_;
    $text =~ s/\W+/_/g;
    return $text;
}

sub view_seq_link {
    my $self = shift;
    my $url = $self->SUPER::view_seq_link( @_ );
    return unless $url;
    $url = $1 . $self->escape_name($2) . $3 if $url =~ /^([^#]+#)([^"]+)(.+)$/;
    return $url;
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
        $title =~ s/\*\s*//;
        if (length $title) {
            my $anchor = $self->escape_name( $title );
            $title = qq{<a name="$anchor"></a><b>$title</b>};
        }
    }

    return '<li>'
        . "$title\n"
        . $item->content->present($self)
        . "</li>\n";
}


1;

