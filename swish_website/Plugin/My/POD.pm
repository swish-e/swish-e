package My::POD;
use strict;
use Pod::POM;
use Pod::POM::View::HTML;
use base 'Template::Plugin';
use vars qw( $stash );  # for debugging links

if ( my $ file = shift ) {
    warn "Testing links\n";
    My::pod::test();
}


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

    $stash = $context->stash;

    my $template_name = $stash->get( 'template.name' );

    # Enable output of debugging messages from Pod::POM
    my $warn = sub { warn "[$template_name]: @_\n" }
        if $stash->get( 'self.config.verbose' );

    my $parser = Pod::POM->new( warn => $warn );


    # Parse the pod into a Pod::POM tree
    my $pom = $parser->parse_text( $content );


    # Merge sequential verbatim (<pre>) sections into one
    combine_verbatim_sections_hack( $pom );


    my @sections = $pom->head1; # get pod into sections

    # Get title of document and shift off first NAME section

    my $doc_title;
    if ( @sections && $sections[0]->title =~ /NAME/ ) {
        $doc_title = ( shift @sections )->content;
        $doc_title =~ s/^\s*|\s*$//sg;  # strip;
    }



    # Cache this page and overview for creating a table of contents page
    # the bin/build script passes in the hash reference

    if ( my $cache = $stash->get( 'toc_cache' ) ) {
        $cache->{ $template_name } = {
            page    => $stash->get( 'page.id' ),
            title   => $doc_title,
            abstract => fetch_abstract( \@sections ),
        };
    };


    # Structure for returning info to template
    # the "toc" is the table of contents for the top of the page
    # the "podparts" is the document broken into sequential parts by
    # =head*  This allows the template to wrap each section in a <div> and
    # therefore makes indexing each section with swish-e easy -- allows targeted searches

    my %data = (
        pom         => $pom,
        view        => 'My::Pod::View::HTML',
        toc         => fetch_toc( @sections ),
        podparts    => [ slice_by_head(@sections) ], # Must be lasts because modifies tree
        title       => $doc_title,
    );



    return \%data;
}



#-----------------------------------------------------------------------------
# This function is modified from Stas' DocSet
# It flattens the recursive structure of the tree into a list of sections by =head
#
# Pass in:
#   @sections = list of head1 sections
#
# Returns:
#   flat list of sections for each headX section
#
# Note:
#   This MODIFIES the original tree!  Should probably clone and build a new tree.
#
# This is useful for printing and wrapping each headX section in, say, <div>

sub slice_by_head {
    my @sections = @_;  # these are the sections at the current level.
    my @body = ();

    for my $node (@sections) {
        my @next = ();
        # assumption, after the first 'headX' section, there can only
        # be other 'headX' sections

        # Look for first =headX seciton and chop of the content there
        # then process that new =headX section again

        my $id = -1;
        for ($node->content) {
            $id++;
            # Keep content until a =head section is found.  Then end current content
            # section at that point.

            next unless $_->type =~ /^head\d$/;

            # Modify original content to just include the nodes up to the first =head
            @next = splice @{$node->content}, $id;
            last;
        }

        # combine all =head nodes (flatten)
        push @body, $node, slice_by_head(@next);
    }
    return @body;
}

#---------------------------------------------------------------------------
# Merge verbatim sequential <pre> sections together

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



#----------------------------------------------------------------------------
# Grabs the first paragraph from either DESCRIPTION or OVERVIEW -- whichever comes first


sub fetch_abstract {
    my ( $sections ) = @_;
    for ( 0 .. 2 ) {
        next unless $sections->[$_] && $sections->[$_]->title =~ /DESCRIPTION|OVERVIEW/;

        my $abstract = $sections->[$_]->content->present($view_mode);
        $abstract =~ s|<p>(.*?)</p>.*|$1|s;
        return $abstract;
    }
}


#---------------------------------------------------------------------------------
# Walk tree building up a TOC

sub fetch_toc {
    my ( @sections ) = @_;

    my @toc = ();
    my $level = 1;
    for my $node (@sections) {
        push @toc, render_toc_level($node, $level);
    }
    return \@toc;
}

# From DocSet POD.pm

sub render_toc_level {
    my( $node, $level) = @_;
    my $title = $node->title->present($view_mode);
    my $anchor = My::Pod::View::HTML->escape_name( $title );

    my %toc_entry = (
        title    => $title,
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



#---------------- Package to override Pod::POM::View::HTML ----------------

package My::Pod::View::HTML;
use strict;
use warnings;
use base 'Pod::POM::View::HTML';
use HTML::Entities;
use Template::Filters;
use File::Basename;

use vars qw( %targets @links );


#-------------- For checking that all pod links work ----------------
sub save_link {
    my ($self, $href) = @_;


    my $out_file = basename( $My::POD::stash->get( 'this.out_file' ) );  # for debugging links


    push @links, {
        href => $href,
        on_page => $out_file,
    };
}

sub save_target {
    my ($self, $target) = @_;
    my $out_file = basename( $My::POD::stash->get( 'this.out_file' ) );
    $targets{$out_file}{$target}++;
}

sub validate_links {
    my $all_files_processed = $My::POD::stash->get( 'self.config.all');

    for my $link ( @links ) {
        my ($name, $fragment) = split /#/, $link->{href}, 2;

        # If processing all files then can check for page links
        if ( $all_files_processed ) {
            if ( $name && !$targets{$name} ) {
                warn "Link to page [$name] from $link->{on_page} could not be resolved\n";
                next;
            }
        } else {
            next if $name && $name ne $link->{on_page};  # can't check (unless links are cached in the future
        }

        $name ||= $link->{on_page};  # default to current page;

        # Check page link
        unless ( $targets{ $name } ) {
            warn "Link on page $link->{on_page} to page $name is unresolved\n";
            next;
        }
        next unless $fragment;

        # And check frag if one
        unless ( $targets{ $name}{ $fragment } ) {
            warn "Link [$link->{href}] on page $link->{on_page} could not be resolved\n";
        } else {
            warn "Link $link->{href} on page $link->{on_page} has multiple targets\n"
                if $targets{$name}{$fragment} > 1;
        }


    }

    @links = ();
    %targets = ();
}


#---------------- Override Pod::POM ---------------------------------

sub view_head1 { shift->show_head( @_ ) };
sub view_head2 { shift->show_head( @_ ) };
sub view_head3 { shift->show_head( @_ ) };
sub view_head4 { shift->show_head( @_ ) };

sub show_head {  # all all child nodes
    my ($self, $head) = @_;
    my $level = substr( $head->type, -1 );
    return "\n<h$level>" . $self->anchor($head->title) . "</h$level>\n\n" .
        $head->content->present($self);
}


#----------------- <a name="foo"></a>foo ---------------------
sub anchor {
    my($self, $title) = @_;
    my $text = $self->escape_name( $title->present($self) );
    my $t = Template::Filters::html_filter( $title );
    $self->save_target( $text );
    return qq[<a name="$text"></a>$t];
}

#----------------- prepare text for name="" --------------------
# This first unescapes any entities, and then removes all non-word chars
# and finally returns all lower case;


sub escape_name {
    my ($self, $text) = @_;

    # Comes in already HTML escaped -- well, kind of, quotes are not escaped.
    my $plain = decode_entities( $text );

    # Not much is allowed in the name="" attribute, so just hack away:
    $plain =~ s/\W+/_/g;
    return lc $plain;
}


#----------------- Fixup L<> links in pod -------------------------------

# Thes first two fixup L<> links.
# Pod::POM doesn't provide a way to get at the fragment for escaping,
# so let Pod::POM deal with it and then then update

sub view_seq_link {
    my $self = shift;
    my $url = $self->SUPER::view_seq_link( @_ );
    return unless $url;

    return unless $url =~ /href="(.+)(?=">)/;
    my ( $href, $fragment ) = split /#/, $1, 2;

    $href = '' unless defined $href;
    $href .= '#'. $self->escape_name( $fragment ) if defined $fragment;

    $self->save_link( $href );  # for checking that they all go some place.

    # Now replace the href into the original string

    $url =~ s/href=".+(?=">)/href="$href/;

    return $url;
}


# This allows fixing up links to *other* pages.  In our case, they are mostly to
# other pod pages, which are now lower case and end in .html.


sub view_seq_link_transform_path {
    my ( $self, $link ) = @_;
    return lc $link . '.html';
}


# Modified version of item display that removes the item_ prefix and only takes the first
# word
#
# Oh this won't work everywhere.  The problem is we have things like:
#
# =item * UndefinedMetaTags [error|ignore|INDEX|auto]
#
# so we take only the first word.  But that breaks if linking to multi-word item.
#

sub view_item {
    my ($self, $item) = @_;

    my $title = $item->title();

    if (defined $title) {
        $title = $title->present($self) if ref $title;

        $title =~ s/^\*\s*//;  # Remove leading bullet

        if (length $title) {
            my $anchor = $title;
            $anchor =~ s/\s+.*$//;  # strip all trailing stuff from first space on
            $anchor = $self->escape_name( $anchor );

            $self->save_target( $anchor );
            $title = qq{<a name="$anchor"></a><b>$title</b>};
        }
    }

    return '<li>'
        . "$title\n"
        . $item->content->present($self)
        . "</li>\n";
}


package My::pod;
use strict;
use warnings;

sub test {

    # From perldoc perlpod:
    #
    my @l_tests = (
        { 
            in  => ['L<name>'],
            out => '<a href="name.html">name</a>',
        },

        {
            in  => [ 'L<name/"section here">', 'L<name/section here>', ],
            out => '<a href="name.html#section_here">section here</a>',
        },

        {
            in  => ['L</"section here">', 'L</section here>', 'L<"section here">', ],
            out => '<a href="#section_here">section here</a>',
        },



        {   in  => ['L<text here|name>'],
            out => '<a href="name.html">text here</a>',
        },

        {
            in  => ['L<text|name/"sec here">', 'L<text|name/sec here>'],
            out => '<a href="name.html#sec_here">text</a>',
        },

        {
            in  => ['L<text|/"sec">',  'L<text|/sec>', 'L<text|"sec">'],
            out => '<a href="#sec">text</a>',
        },

        {
            in  => ['L<http://host.name/some/path.html#fragment>'],
            out => '<a href="http://host.name/some/path.html#fragment">http://host.name/some/path.html#fragment</a>',
        },

        {
            in => ['L<Link to $foo-E<gt> with & "quotes" and %funny !!? chars>'],
            out => '<a href="#link_to_foo_with_quotes_and_funny_chars">Link to $foo-&gt; with &amp; "quotes" and %funny !!? chars</a>',
        },
        {
            in => ['L<Some & text|/Link to $foo-E<gt> with & "quotes" and %funny !!? chars>'],
            out => '<a href="#link_to_foo_with_quotes_and_funny_chars">Some &amp; text</a>',
        },
        {
            in => ['L<SWISH-RUN|SWISH-RUN>'],
            out => '<a href="swish-run.html">SWISH-RUN</a>',
        }
    );


    for my $test ( @l_tests ) {
        for my $input ( @{$test->{in}} ) {

            my $pod = Pod::POM->new( warn => 1 )->parse_text( "=head1 Title\n\n$input" );
            my $out = My::Pod::Test::HTML->print( $pod );

            warn "input  [$input]\n",
                 "output [$out]\n",
                 "test   [$test->{out}]\n\n"
                        if $out ne $test->{out} || $ENV{VERBOSE};
        }
    }
}

package My::Pod::Test::HTML;
use strict;
use warnings;
use base 'My::Pod::View::HTML';

sub view_textblock {
    my ($self, $text) = @_;
    return $text;
}

sub view_pod {
    my ($self, $pod) = @_;
    return $pod->content->present($self);
}

sub view_head1 {
    my ($self, $pod) = @_;
    return $pod->content->present($self);
}


1;

