package SWISHE;

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);

# Probably shouldn't export everything 
@EXPORT = qw(
    SwishOpen
    SwishSearch
    SwishClose
    SwishNext
    SwishSeek
    SwishError
    SwishHeaderParameter
    SwishStopWords
    SwishWords
    SwishStem
    SwishErrorString
    SwishHeaders
    SetLimitParameter
);

$VERSION = '0.01';

bootstrap SWISHE $VERSION;

# Preloaded methods go here.

# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__
# Below is the stub of documentation for your module. You better edit it!
