package Pod::HtmlPsPdf;

$VERSION = '0.03';

=head1 NAME

Pod::HtmlPsPdf - documentation projects builder in HTML, PS and PDF formats

=head1 SYNOPSIS

  pod2hpp [options] configuration_file_location

Options:

  -h    this help
  -v    verbose
  -i    podify pseudo-pod items (s/^* /=item */)
  -s    create the splitted html version
  -t    create tar.gz
  -p    generate PS file
  -d    generate PDF file
  -f    force a complete rebuild
  -a    print available hypertext anchors
  -l    do hypertext links validation
  -e    slides mode (for presentations)
  -m    executed from Makefile (forces rebuild,
				no PS/PDF file,
				no tgz archive!)

=head1 DESCRIPTION

This code knows to do following with a collection of your POD files.

=over

=item 1

Generate HTMLs

=item 2

Generate a split version HTML, creating html file for each pod
section, and having everything interlinked of course. This version is
used best for the search.

=item 3

Generate a single book-like version in PostScript format

=item 4

Generate a single book-like version in PDF format

=item 5

Complete the POD on the fly from the files in POD format. This is used
to ease the generating of the presentations slides, so one can use
C<*> instead of a long =over/=item/.../=item/=back strings. The rest
is done as before. Take a look at the special version of the html2ps
format to generate nice slides in I<conf/html2ps-slides.conf>.

=item 6 

If you turn the slides mode on, it automatically turns the C<-i> (C<*>
preprocessing) mode and does a page break before each =head tag.

=back

You can customise the look and feel of the HTML files, PS and
therefore the PDF by tweaking the template files in I<./tmpl>
directory.

You can change look and feel of the PS (PDF) versions by modifying
I<./conf/html2ps.conf>.  Be careful that if your documentation that
you want to put in one PS or PDF file is very big and you tell html2ps
to put the TOC at the beginning you will need lots of memory because
it won't write a single byte to the disk before it gets all the HTML
markup converted to PS.

To generate HTML this code use a slightly modified version of the
C<Pod::Html> code and than does a lot of massage on the resulting
HTML. I've tried to keep the pod2html code modified as little as
possible, so when a new versions of the original C<Pod::Html> module
will be released I'll be able to merge the changes with my version.

=head1 EXTENDED POD SYNTAX

I've extended the POD syntax to accomodate my own needs. Note that
this can be always converted back to the standard POD. To see the
extended syntax, refer to the I<docs/extended_pod.pod> in the package
distribution.


=head1 CONFIGURATION

All you have to prepare is a single config file that you then pass as
an argument to C<pod2hpp>:

  pod2hpp [options] /full/path/to/config/file

Use the file I<project.config> supplied in the directory
I<sample>. Modify it to be suit your documentation project layout.

Note that I<sample/bin/build> script automatically locates your
project's directory, so you can move your project around filesystem
without changing anything.

I<sample/README.sample> explains the layout of the directories. The
easiest way to learn to use this package is to look at the
C<Apache::mod_perl_guide> package available at CPAN. I've developed
the package C<Pod::HtmlPsPdf> especially for
C<Apache::mod_perl_guide>.

=head1 PREREQUISITES

All these are not required if all you want is to generate only the
html version.

=over 4

=item * ps2pdf

Needed to generate the PDF version

=item * Storable

Perl module available from CPAN (http://cpan.org/)

Allows source modification control, so if you modify only one file you
will not have to rebuild everything to get the updated HTML/PS/PDF
files.

=back


=head1 SUPPORT

Notice that this tool relies on two tools (ps2pdf and html2ps) which I
don't support. So if you have any problem first make sure that it's
not a problem of these tools.

Note that while C<html2ps> is included in this distribution, it's
written in the old style Perl, so if you have patches send them along,
but I won't try to fix/modify this code otherwise. I didn't write this
utility.

This package works for me on Linux RedHat and Mandrake systems. I
release it only to share. Unfortunately I don't have time to help with
each available platform.  If you have a problem, please don't contact
me. I'm not going to solve it. If you solve the problem, I'll gladly
accept the patch for others to benefit. Remember that this is a free
software.

=head1 BUGS

Huh? Probably many...

=head1 AUTHOR

Stas Bekman <stas@stason.org>

=head1 SEE ALSO

perl(1), Pod::HTML(3), html2ps(1), ps2pod(1), Storable(3)

=head1 COPYRIGHT

This program is distributed under the Artistic License, like the Perl
itself.

=cut
