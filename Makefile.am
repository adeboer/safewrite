bin_PROGRAMS = safewrite take

safewrite_SOURCES = safewrite.c system.h

take_SOURCES = take.c system.h

man_MANS = safewrite.1 take.1

TESTS = test_newfile test_newfail test_replace \
	test_dash_m1 test_dash_m2 \
	test_dash_s1 test_dash_s2 \
	test_refail test_cleanup

# gnu seems to want to lose my manpages
EXTRA_DIST = safewrite.1 take.1 $(TESTS)
