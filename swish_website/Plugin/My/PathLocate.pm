package My::PathLocate;
use strict;
use base 'Template::Plugin';
use Config;
use File::Spec;


sub new {
    my ( $class, $context ) = @_;

    return bless { _CONTEXT => $context }, $class;

}

sub in_path {
    my ( $self, $program ) = @_;

    for my $dir  (split /$Config{path_sep}/, $ENV{PATH}) {
        my $path = File::Spec->rel2abs( $program, $dir );
        return $path if -x $path;
    }
    return;
}

sub swish_perl_lib {
    my ( $self ) = @_;

    my $lib = `swish-config --prefix`;

    die "Failed to locate swish-config program in \$PATH"
        unless $lib;

    chomp $lib;

    return File::Spec->catfile( $lib, qw[ lib swish-e perl ] );
}

1;

