package My::Dirlist;
use strict;
use base 'Template::Plugin';


sub new {
    my ( $class, $context, $download_dir ) = @_;

    my %dir;

    opendir DIR, $download_dir or die "Failed to open directory [$download_dir]: $!";

    while ( my $file = readdir DIR ) {

        next if $file =~ /^\./;

        my $path = "$download_dir/$file";

        next if -d $path;

        my $type = -d $path
                    ? 'dir'
                    : get_file_type( $file );

        next unless $type;

        push @{$dir{$type}}, {
            name    => $file,
            mtime   => (stat $path)[9],
            size    => (stat _)[7],
            size_formatted => format_num( (stat _)[7] ),
        };

    }
    for my $type ( keys %dir ) {
        $dir{$type} = [ sort { $b->{mtime} <=> $a->{mtime} } @{$dir{$type}} ];
    }

    return \%dir;
}

sub get_file_type {
    my $type = shift;

    return $1 if $type =~ /\.(gz|exe|sig)$/;
    return;
}

sub format_num {
    my $number = shift;
    my $format = '%.1f';


    my $suffix = "";
    if ($number > 0x40000000)
    {
        $number /= 0x40000000;
        $suffix = 'G';
    }
    elsif ($number > 0x100000)
    {
        $number /= 0x100000;
        $suffix = 'M';
    }
    elsif ($number > 0x400)
    {
        $number /= 0x400;
        $suffix = 'K';
    } else {
        $format = '%d';
    }

    $number = sprintf($format, $number);

    return $number.$suffix;
}




1;

