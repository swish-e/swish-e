TESTS = check_index check_search check_metasearch check_fuzzy

TESTS_ENVIRONMENT = top_builddir=$(top_builddir)


EXTRA_DIST = \
    test.config \
    test.fuzzy.config \
    test.html \
    test.txt \
    test.xml \
    test_meta.html \
    test_meta2.html \
    test_phrase.html \
    test_xml.html \
    $(TESTS) common.sh

DISTCLEANFILES = \
	index.swish-e.prop \
	index.swish-e

.PHONEY: test

test: check

