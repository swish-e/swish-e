#!/usr/local/bin/perl -w
use strict;

my %mem;
my %files;
my $cur_file = '';
my @cur_file;
my $last_line = '';
my $output = '';

my %diff_mods;



while (<>) {

    if ( /^Alloc: ([^ ]+) line (\d+): Addr: ([^ ]+) Size: (\d+)/ ) {

        my ( $file, $line, $addr, $size ) = ( $1, $2, $3, $4 );
        print "Allocate without free: $_" if $mem{ $addr };

        @{$mem{ $addr }}[0,1,2] = ( $file, $line, $size );
        $files{$file}{malloc}++;
        $files{$file}{malloc_size} += $size;
        next;
    }
        
    if ( /^Free: ([^ ]+) line (\d+): Addr: ([^ ]+) Size: (\d+)/ ) {

        my ( $file, $line, $addr, $size ) = ( $1, $2, $3, $4 );

        unless ( exists $mem{ $addr } ) {
            print "Free without Allocate: $_";
            next;
        }
        my ( $filex, $linex, $sizex ) = @{$mem{ $addr }}[0,1,2];

        $diff_mods{"$filex:$linex freed at $file:$line"}++ if $filex ne $file;


        print "Size mismatch: Allocated in $filex:$linex:$sizex freed in $file:$line:$size\n"
            if $sizex ne $size;

        $files{$file}{free}++;
        $files{$file}{free_size} += $size;

        delete $mem{ $addr };
        next;
    }

    $output .= $_;

}


print "Allocated $diff_mods{$_} times in $_\n" for sort keys %diff_mods;


print "\n\nHere's the unfreed blocks (file name: line number : size )\n\n";

my %failed_to_free;

#for ( sort { "@{$mem{$a}}[0,1]" cmp "@{$mem{$b}}[0,1]" } keys %mem ) {
for ( keys %mem ) {

    $failed_to_free{ join ':', @{$mem{$_}}[0,1] }{bytes} += $mem{$_}[2];
    $failed_to_free{ join ':', @{$mem{$_}}[0,1] }{count} ++;

#    print $i++, ': Failed to free: ', join(':',@{$mem{$_}}[0,1,2]),"\n";

#    print print_line( @{$mem{$_}}[0,1] );

}

for ( sort keys %failed_to_free ) {
    print "\nFailed to free $failed_to_free{ $_ }{count} malloc's: $_ ($failed_to_free{ $_ }{bytes} bytes)\n";
    print print_line( $_ );
}
    



print "\n\nCounts by file\n\n";

my @heads = ( 'File Name', '# malloc', '# free', 'diff', 'malloc', 'free', 'diff', 'Avg malloc', 'Avg free');
printf("%18s %8s %8s %8s %8s %8s %8s %10s %10s\n", @heads );

my %totals;

my @fields = qw/malloc free difcnt malloc_size free_size difsize/;

for my $file ( sort keys %files ) {

    $files{$file}{difcnt} = ( $files{$file}{malloc} || 0 ) - ( $files{$file}{free} || 0 );
    $files{$file}{difsize} = ( $files{$file}{malloc_size} || 0 ) - ( $files{$file}{free_size} || 0 );


    print_summary( $file, @{$files{$file}}{ @fields } );

    $files{$file}{$_} && ($totals{$_} += $files{$file}{$_}) for @fields;

}

print_summary('TOTAL', @totals{ @fields } );

sub print_summary {
    my ($file, $malloc, $free, $dif_cnt, $malloc_size, $free_size, $dif_size) = @_;

    printf("%18s %8s %8s %8s %8s %8s %8s %10s %10s\n",
        $file,
        
        $malloc || '-',
        $free || '-',
        $dif_cnt,
        $malloc_size || '-',
        $free_size || '-',
        $dif_size,
        $malloc ? int( $malloc_size / $malloc ) : '-',
        $free ? int( $free_size / $free ) : '-',
    );
}



print "\n\nSwish indexing ouptut\n\n$output\n";


sub print_line {
   
    my ( $file, $line ) = split /:/, shift;

    if ( $last_line eq "$file$line" ) {
        return '';
    }

    $last_line = "$file$line";


    unless ( $file eq $cur_file ) {

        open FH, $file or return "Failed to open $file $!\n";
        @cur_file = <FH>;
        $cur_file = $file;
    }

    return $cur_file[ $line - 1 ];
}
