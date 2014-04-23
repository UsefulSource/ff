/**
Copyright (c) 2013 Simon Zolin
*/

#include <FFOS/types.h>
#include <FFOS/string.h>


FF_EXTN int test_str();
FF_EXTN int test_rbtlist();
FF_EXTN int test_list();
FF_EXTN int test_htable();

FF_EXTN int test_url();
FF_EXTN int test_http();
FF_EXTN int test_json();
FF_EXTN int test_conf();
FF_EXTN int test_args();

FF_EXTN int test_all();

#define TESTDIR TEXT("./test")

#ifdef FF_UNIX
#define TMPDIR TEXT("/tmp")
#else
#define TMPDIR TEXT("%TMP%")
#endif
