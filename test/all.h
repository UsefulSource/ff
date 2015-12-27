/**
Copyright (c) 2013 Simon Zolin
*/

#include <FFOS/types.h>
#include <FFOS/string.h>


FF_EXTN size_t _test_readfile(const char *fn, char *buf, size_t n);

FF_EXTN int test_str();
FF_EXTN int test_rbtlist();
FF_EXTN int test_list();
FF_EXTN int test_htable();

FF_EXTN int test_url();
FF_EXTN int test_http();
FF_EXTN int test_json();
FF_EXTN int test_conf();
FF_EXTN int test_args();

FF_EXTN int test_time();
FF_EXTN int test_timerq();

#define TESTDIR "./test"

#ifdef FF_UNIX
#define TMPDIR "/tmp"
#else
#define TMPDIR "%TMP%"
#endif
