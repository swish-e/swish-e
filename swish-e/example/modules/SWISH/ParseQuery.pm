package SWISH::ParseQuery;

#    Module to parse the "Parsed Words:" header returned by swish
#
#    Copyright (C) 2003  Bill Moseley
#
#    This program is free software; you can redistribute it and/or
#    modify it under the terms of the GNU General Public License
#    as published by the Free Software Foundation; either version
#    2 of the License, or (at your option) any later version.
#    The full text of the GNU General Public License is at URL
#    http://www.fsf.org/copyleft/gpl.html and this software is
#    licensed as specified there.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.

use strict;

require Exporter;

use vars qw/$VERSION @ISA @EXPORT/;


@ISA = qw(Exporter);
$VERSION = '0.01';

@EXPORT = 'parse_query';


sub parse_query {

    my ( $query, $phraseDelimiter ) = @_;

    return {} unless $query;

    s/^\s+//, s/\s+$// for $query;
    my @tokens = split /\s+/, $query;

    my %p = (
             query     => [ split /\s+/, $query ],
	     phrase    => $phraseDelimiter || '"',
             metas     => {},
    );

    process_query( \%p );

    # sort in reverse phrase length order
    $_ = [ sort { @$b <=> @$a } @$_ ] for values %{$p{metas}};

    return $p{metas};

}


sub process_query {

    my ( $p, $current_meta, $end_char, $single_token ) = @_;

    $current_meta ||= 'swishdefault';

    my $query  = $p->{query};
    my $phrase = $p->{phrase};
    my $metas  = $p->{metas};

    while ( my $next_token = shift @$query ) {

	last if $end_char && $next_token eq $end_char;

	# check for sub query
	if ( $next_token eq '(' ) {
	    process_query( $p, $current_meta, ')' );


	# check for start of a phrase
	} elsif ( @$query > 1 && $next_token eq $phrase ) {
	    push @{$metas->{$current_meta}}, fetch_words( $query, $phrase );


	# check for metaname
	} elsif ( @$query > 1 && $query->[0] eq '=' ) {
	    shift @$query;
            warn "nested metaname '$next_token' inside meta '$current_meta'" if $single_token;
	    process_query( $p, $next_token, undef, 1  );  # fetch one word, phrase, or sub-query
		

	# ignore operators outside of quotes
	} elsif ( $next_token =~ /^(?:and|or|not)$/ ) {
	    next;


	# just a regular word
	} else {
	    push @{$metas->{$current_meta}}, [$next_token];
	}


	last if $single_token;  # use for meta names
    }
}

sub fetch_words {
    my ( $tokens, $end_char ) = @_;

    my @words;

    while ( my $word = shift @$tokens ) {
	last if $word eq $end_char;
        push @words, $word;
    }
    return \@words;
}


1;
