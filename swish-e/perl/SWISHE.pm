package SWISHE;

require Exporter;
require DynaLoader;

@ISA = qw(Exporter DynaLoader);
# Items to export into callers namespace by default. Note: do not export
# names by default without a very good reason. Use EXPORT_OK instead.
# Do not simply export all your public functions/methods/constants.
@EXPORT = qw(
SwishOpen SwishSearch SwishClose SwishNext SwishSeek SwishError SwishHeaderParameter SwishStopWords SwishWords SwishStem SwishErrorString SwishHeaders
);
$VERSION = '0.01';

bootstrap SWISHE $VERSION;

# Preloaded methods go here.

# Autoload methods go after __END__, and are processed by the autosplit program.

1;
__END__
# Below is the stub of documentation for your module. You better edit it!
