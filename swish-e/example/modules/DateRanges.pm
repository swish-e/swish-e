# $Id$

package DateRanges;
use strict;

=head1 NAME

DateRanges

=head1 SYNOPSIS

    use DateRanges;
    use CGI;
    my $cgi = CGI->new();
     ...

    my %hash = (
        date_ranges     => {

            # Define what buttons to include
            time_periods    => [
                'All',
                'Today',
                'Yesterday',
                #'Yesterday onward',
                'This Week',
                'Last Week',
                'Last 90 Days',
                'This Month',
                'Last Month',
                #'Past',
                #'Future',
                #'Next 30 Days',
            ],

            # Default button
            default         => 'All',

            # Should buttons be in a row or a column?
            line_break      => 0,

            # Should a date input form be shown, too?
            date_range      => 1,
        },
    );

    


=head1 DESCRIPTION

This module provides I<basic> support for entering and using date ranges.  It
was written to use with swish-e (http://swish-e.org).

See swish.cgi in the swish-e distribution for an example.

Sorry about the interface -- if anyone really wants to use this please let me know and I'll
rewrite as OO interface!

=head1 FUNCTIONS

=cut

require Exporter;

use vars qw/$VERSION @ISA @EXPORT/;


@ISA = qw(Exporter);
$VERSION = '0.01';

@EXPORT = qw (
    DateRangeForm
    DateRangeParse
    GetDateRangeArgs
);

    # what to pick from
    my @TIME_PERIODS = (
        'Today',
        'Yesterday',
        'Yesterday onward',
        'This Week',
        'Last Week',
        'Last 90 Days',
        'This Month',
        'Last Month',
        'Past',
        'Future',
        'Next 30 Days',
        'All'
    );

    my %TIME_PERIODS = map { $_, 1} @TIME_PERIODS;


use Date::Calc qw /
    Day_of_Week_to_Text
    Day_of_Week
    Date_to_Text
    Monday_of_Week
    Week_of_Year
    Today
    Add_Delta_Days
    Days_in_Month
    check_date
/;

use Time::Local;


=item DateRangeForm( $cgi, $params, $fields );

This function simple creates a simple form for selecting date ranges based
on the fields passed in C<$params>.  Will call C<die()> on errors.

C<$params> must be a hash reference with a key named C<time_periods> as shown in
B<SYNOPSIS> above.  This is used to select which time periods to display.

C<$fields> is a reference to a hash where C<DateRanges> returns data.

These store the HTML for display on your form.

    buttons - the buttons to select the different time ranges
    date_range_button - the button to select a date range
    date_range_low - the low range select form fields
    date_range_hi  - the hight range select form fields

=cut

sub DateRangeForm {
    my ( $CGI, $params, $fields ) = @_;

    die "Must supply arrary ref for 'options'"
        unless $params->{time_periods} && ref $params->{time_periods} eq 'ARRAY';

    my @time_periods = grep { $TIME_PERIODS{$_} } @{ $params->{time_periods} };


    $fields->{buttons} = '';
    $fields->{date_range_button} = '';
    $fields->{date_range_low}  =  '';
    $fields->{date_range_high} =  '';

    $fields->{buttons} = 
          $CGI->radio_group( 
            -name       => 'DateRanges_date_option',
            -values     => \@time_periods,
            -default    => ($params->{default} || $time_periods[0]),
            -linebreak  => (exists $params->{line_break} ? $params->{line_break} : 1),
          ) if @time_periods;


    return unless $params->{date_range};

    $fields->{date_range_button} = 
        $CGI->radio_group(
            -name       => 'DateRanges_date_option',
            -values     => ['Select Date Range'],
            -default    => ($params->{default} || $time_periods[0]),
            -linebreak  => (exists $params->{line_break} ? $params->{line_break} : 1),
          );



    $fields->{date_range_low}  =  show_date_input($CGI, 'start');
    $fields->{date_range_high} =  show_date_input($CGI, 'end');

=pod
    print '<br>Limit to the hour of: ',
          popup_menu( -name       => 'Limit_hour',
                      -default    => ' ',
                      -values     => [' ',0..23], ),
          '<br>';
=cut          
          
}

=item my $args = GetDateRangeArgs( $cgi );

Returns a string to use in a HREF with all the parameters set.

=cut

sub GetDateRangeArgs {
    my $CGI = shift;

    my %args;

    
    $args{DateRanges_date_option} = $CGI->param('DateRanges_date_option')
        if defined $CGI->param('DateRanges_date_option');


    for ( qw/ mon day year / ) {
        my $start = "DateRanges_start_$_";
        my $end   =  "DateRanges_end_$_";
        $args{$start} = $CGI->param($start) if defined $CGI->param($start);
        $args{$end} = $CGI->param($end) if defined $CGI->param($end);
    }

    return '' unless %args;

    return join '&amp;', map { "$_=" . $CGI->escape($args{$_}) } keys %args;
    
}

=item DateRangeParse( $cgi, $form )

Parses the date range form and returns a low and high range unix timestamp.
Returns false on error with the folowing key set in C<$form>:

    DateRanges_error - error string explaining the problem

C<$form> is a hash reference where the following keys may be set:

    All - no date ranges were selected
    DateRanges_time_low - low range unix timestamp
    DateRanges_time_high - high range unix timestamp

=cut    
    



#------------------------ Get the report dates ---------------------
sub DateRangeParse {
    my ( $q, $form ) = @_;

    $form->{DateRanges_error} = '';
    


    # If requesting ALL (or not found in form) return true for all
    if ( !$q->param('DateRanges_date_option') || $q->param('DateRanges_date_option') eq 'All' ) {
        $form->{All}++;
        return 1;
    }

    my $time = time();

    my ( @start, @end );

    for ($q->param('DateRanges_date_option') ) {
    
        /^Today/      && do { @start = @end = Today(); last; };

        /^Yesterday onward/  && do { @start = Add_Delta_Days( Today(), -1 ); last };
        
        /^Yesterday/  && do { @start = @end = Add_Delta_Days( Today(), -1 ); last };


        /^This Week/ && do {
            @start = Monday_of_Week( Week_of_Year( Today() ) );
            @end   = Add_Delta_Days( @start, 6 );
            last;
        };

        /^Last Week/ && do {
            @start = Monday_of_Week( Week_of_Year( Add_Delta_Days( Today(), -7 ) ) );
            @end   = Add_Delta_Days( @start, 6 );
            last;
        };

        /^This Month/ && do {
            @start = @end = Today();
            $start[2] = 1;
            $end[2] = Days_in_Month($end[0],$end[1]);
            last;
        };
        
        /^Last Month/ && do {
            @start = Today();
            $start[2] = 1;
            $start[1] = 1 if --$start[1] == 0;
            @end = @start;
            $end[2] = Days_in_Month($end[0],$end[1]);
            last;
        };

        /^Last 90 Days/ && do {
            @end = Today();
            @start = Add_Delta_Days( Today(), -90 );
            last
        };

        /^Past/ && return 1;  # use defaults;

        /^Future/ && do {
            $form->{DateRanges_time_low} = time;
            delete $form->{DateRanges_time_high};
            return 1;
        };

        /^Next 30 Days/   && do {
            @start = Today();
            @end = Add_Delta_Days( Today(), +30 );
            last
        };
        


        /^Select/ && do {
            my ( $day, $mon, $year );

            $day    = $q->param('DateRanges_start_day') || 0;
            $mon    = $q->param('DateRanges_start_mon') || 0;
            $year   = $q->param('DateRanges_start_year') || 0;
            @start = ( $year, $mon, $day );

            $day    = $q->param('DateRanges_end_day') || 0;
            $mon    = $q->param('DateRanges_end_mon') || 0;
            $year   = $q->param('DateRanges_end_year') || 0;
            @end = ( $year, $mon, $day );
            last;
        };

        $form->{DateRanges_error} = 'Invalid Date Option ' . $q->param('DateRanges_date_option') . ' Selected';
        return;
    }


    
    $form->{DateRanges_error} = 'Invalid Start Date' && return if @start && !check_date( @start );
    $form->{DateRanges_error} = 'Invalid Ending Date' && return if @end && !check_date( @end );


    my $start_time = @start ? timelocal( 0, 0, 0, $start[2], $start[1]-1, $start[0]-1900 ) : 0;
    my $end_time   = @end   ? timelocal( 59, 59, 23, $end[2], $end[1]-1, $end[0]-1900 ) : 0;


    $form->{DateRanges_error} = "Starting time should be before now, don't you think?" && return
        if $start_time && $start_time > time();
        
    $form->{DateRanges_error} = 'Start date must be same day or before end date' && return
        if $start_time && $end_time && $start_time > $end_time;


    $form->{DateRanges_time_low} = $start_time;
    $form->{DateRanges_time_high} = $end_time;
    return 1;
}


#----------------------------------------------------------------------------
sub show_date_input {
    my ( $CGI, $name ) = @_;

    $name = "DateRanges_$name";

    my @months = qw/Jan Feb Mar Apr May Jun Jul Aug Sep Oct Nov Dec/;
    my $x = 1;
    my %months = map { $x++, $_ } @months;

    my ($mon, $day, $year) = (localtime)[4,3,5];

    $year = $year + 1900;
    $mon++;

    my $cur_year = $year;

    $cur_year += 5;

    ($year,$mon,$day) = Date::Calc::Add_Delta_Days($year,$mon,$day, -28 ) if $name eq 'start';


    my $str = join "\n", 
        $CGI->popup_menu(
            -name   => "${name}_mon",
            -values     => [1..12],
            -default    => $mon,
            -labels     => \%months
        ),
        '&nbsp',
        $CGI->popup_menu(
            -name       => "${name}_day",
            -default    => $day,
            -values     => [1..31],
        ),

        '&nbsp',
        $CGI->popup_menu(
            -name       => "${name}_year",
            -default    => $year,
            -values     => [$year-5..$cur_year],
        );


    return $str;
}

#----------------------- ymd_to_unix --------------------------
# Silly Date::Calc
sub ymd_to_unix {
    my ($y, $m, $d ) = @_;
    $m--;
    $y -= 1900;
    return timelocal( 0, 0, 0, $d, $m, $y );
}

1;

=back

=head1 COPYRIGHT

Copyright 2001 Bill Moseley

This library is free software; you can redistribute it and/or
modify it under the same terms as Perl itself.

=cut

