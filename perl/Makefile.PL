use ExtUtils::MakeMaker;
$Verbose = 1;

WriteMakefile(
	NAME      => 'SWISHE',
	LIBS      => ['-L../src -lswish-e -lm'],
	VERSION_FROM    => 'SWISHE.pm',  # finds $VERSION
);

