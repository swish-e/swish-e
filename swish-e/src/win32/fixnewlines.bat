REM Let me count the ways I dislike Windows

REM Convert misc text files
unix2dos.pl COPYING
unix2dos.pl src/win32/vars.bat
unix2dos.pl src/swishspider

REM Convert filters
unix2dos.pl filters/SWISH/Filter.pm
unix2dos.pl filters/SWISH/Filters/Doc2txt.pm
unix2dos.pl filters/SWISH/Filters/ID3toHTML.pm
unix2dos.pl filters/SWISH/Filters/Pdf2HTML.pm
unix2dos.pl filters/SWISH/Filters/XLtoHTML.pm

REM Convert prog-bin
unix2dos.pl prog-bin/DirTree.pl
unix2dos.pl prog-bin/MySQL.pl
unix2dos.pl prog-bin/README
unix2dos.pl prog-bin/SwishSpiderConfig.pl
unix2dos.pl prog-bin/doc2txt.pm
unix2dos.pl prog-bin/file.pl
unix2dos.pl prog-bin/index_hypermail.pl
unix2dos.pl prog-bin/pdf2html.pm
unix2dos.pl prog-bin/pdf2xml.pm
unix2dos.pl prog-bin/spider.pl

REM Convert examples
unix2dos.pl example/README
unix2dos.pl example/search.tt
unix2dos.pl example/swish.cgi
unix2dos.pl example/swish.tmpl

REM Convert example modules
unix2dos.pl example/modules/DateRanges.pm
unix2dos.pl example/modules/DefaultHighlight.pm
unix2dos.pl example/modules/PhraseHighlight.pm
unix2dos.pl example/modules/SimpleHighlight.pm
unix2dos.pl example/modules/TemplateDefault.pm
unix2dos.pl example/modules/TemplateDumper.pm
unix2dos.pl example/modules/TemplateHTMLTemplate.pm
unix2dos.pl example/modules/TemplateToolkit.pm

REM Convert conf
unix2dos.pl conf/README
unix2dos.pl conf/example1.config
unix2dos.pl conf/example2.config
unix2dos.pl conf/example3.config
unix2dos.pl conf/example4.config
unix2dos.pl conf/example5.config
unix2dos.pl conf/example6.config
unix2dos.pl conf/example7.config
unix2dos.pl conf/example8.config
unix2dos.pl conf/example9.config
unix2dos.pl conf/example9.pl
unix2dos.pl conf/stopwords/dutch.txt
unix2dos.pl conf/stopwords/english.txt
unix2dos.pl conf/stopwords/german.txt
unix2dos.pl conf/stopwords/spanish.txt



