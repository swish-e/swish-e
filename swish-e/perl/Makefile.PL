use ExtUtils::MakeMaker;
$Verbose = 1;
WriteMakefile(
	NAME      => 'SWISHE',
	LIBS      => ['-lswish-e -lm'],
	VERSION   => '0.01',
);

