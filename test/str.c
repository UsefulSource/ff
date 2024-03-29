/**
Copyright (c) 2013 Simon Zolin
*/

#include <FFOS/test.h>
#include <FF/array.h>
#include <FF/data/xml.h>
#include <FF/data/utf8.h>

#include <test/all.h>

#define x FFTEST_BOOL

int test_strqcat()
{
	ffsyschar buf[255];
	ffsyschar *p = buf;
	const ffsyschar *end = buf + FFCNT(buf);

	p = ffq_copyc(p, end, 'a');
	p = ffq_copyz(p, end, TEXT("sd"));
	p = ffq_copy(p, end, TEXT("faz"), 2);
	p = ffq_copyc(p, end, '\0');
	x(0 == ffq_cmpz(buf, TEXT("asdfa")));

	p = ffq_copys(buf, end, FFSTR("ASDFA"));
	x(0 == ffq_cmpz(buf, TEXT("ASDFA")));

	x(FFSLEN(TEXT("asdf")) == ffq_lens("asdf", FFSLEN("asdf")));
	x(FFSLEN("asdf") == ffq_lenq(TEXT("asdf"), FFSLEN(TEXT("asdf"))));

	x(4 == ffutf8_len("фыва", FFSLEN("фыва")));

	{
		char sbuf[255];
		char *sp;
		size_t n;
		n = ffq_lens("фыва", FFSLEN("фыва"));
		p = ffq_copys(buf, end, "фыва", FFSLEN("фыва"));
		x(n == p - buf);

		x(FFSLEN("фыва") == ffq_lenq(buf, p - buf));
		sp = ffs_copyq(sbuf, sbuf + FFCNT(sbuf), buf, p - buf);
		x(sp - sbuf == FFSLEN("фыва") && !ffmemcmp(sbuf, "фыва", FFSLEN("фыва")));
	}

	return 0;
}

int test_strcat()
{
	char buf[255];
	char *p = buf;
	const char *end = buf + FFCNT(buf);

	p = ffs_copyc(p, end, 'a');
	p = ffs_copyz(p, end, "sd");
	p = ffs_copy(p, end, "faz", 2);
	p = ffs_copyc(p, end, '\0');
	x(0 == strcmp(buf, "asdfa"));

	p = ffs_copyq(buf, end, FFSTRQ("ASDFA"));
	x(0 == strcmp(buf, "ASDFA"));

	p = NULL;
	end = NULL;
	p = ffs_copyc(p, end, 'a');
	p = ffs_copyz(p, end, "sd");
	p = ffs_copy(p, end, "faz", 2);
	p = ffs_copyc(p, end, '\0');
	x(p == NULL);

	return 0;
}

static int test_arr()
{
	ffarr ar;
	ffstr str;
	char *ptr = "asdf";
	int n = 4;
	char *p;
	int i;

	ffarr_null(&ar);
	ffarr_set(&ar, ptr, n);
	x('f' == ffarr_back(&ar));
	x(ptr + n == ffarr_end(&ar));
	ar.cap = n;
	x(0 == ffarr_unused(&ar));
	ffarr_shift(&ar, 1);
	x(ar.len == n - 1 && ar.ptr == ptr + 1);

	ffarr_set(&ar, ptr, n);
	i = 0;
	FFARR_WALK(&ar, p) {
		i++;
	}
	x(i == n);
	i = 0;

	FFARR_RWALK(&ar, p) {
		i++;
	}
	x(i == n);

	ffstr_set(&str, ptr, n);
	x(ffstr_eqcz(&str, "asdf"));
	x(!ffstr_eqcz(&str, "asd"));
	x(!ffstr_eqcz(&str, "zxcv"));
	x(!ffstr_eqcz(&str, "asdfz"));
	x(ffstr_ieqcz(&str, "ASDF"));

	x(ffstr_eqcz(&str, "asdf"));
	x(!ffstr_eqcz(&str, "asdfz"));
	x(ffstr_icmp(&str, "ASDFZ", 5) < 0);
	x(ffstr_icmp(&str, "ASD", 3) > 0);
	x(ffstr_icmp(&str, "ASDF", 4) == 0);

	{
		ffstr s3 = { 0 };
		ffstr_setcz(&s3, "ASDF");
		x(ffstr_ieq2(&str, &s3));

		ffstr_setcz(&s3, "asdf");
		x(ffstr_eq2(&str, &s3));
	}

	x(ffstr_match(&str, FFSTR("asd")));
	x(!ffstr_match(&str, FFSTR("asdz")));
	x(ffstr_imatch(&str, FFSTR("ASD")));

	p = ffstr_alloc(&str, 4);
	x(p == str.ptr);
	ffstr_free(&str);

	return 0;
}

static int test_arrmem()
{
	static const int ints[] = { 1, 2, 3, 4, 1000 };
	struct { FFARR(int) } ar;

	x(NULL != ffarr_alloc(&ar, 4));
	x(ar.cap == 4 && ar.len == 0);
	ar.len = 3;
	x(NULL != ffarr_realloc(&ar, 8));
	x(ar.cap == 8 && ar.len == 3);
	x(NULL != ffarr_grow(&ar, 5, 10));
	x(ar.cap == 8 && ar.len == 3);
	ar.len = 8;
	x(NULL != ffarr_grow(&ar, 1, FFARR_GROWQUARTER));
	x(ar.cap == 8 + 8/4 && ar.len == 8);
	ar.len = 0;

	x(NULL != ffarr_append(&ar, ints, FFCNT(ints) - 1));
	x(!memcmp(ar.ptr, ints, FFCNT(ints) - 1));

	{
		int *i = ffarr_push(&ar, int);
		x(i != NULL);
		*i = 1000;
		x(!memcmp(ar.ptr, ints, FFCNT(ints)));
	}

	ffarr_rmswap(&ar, ar.ptr);
	x(ar.len == 4 && ar.ptr[0] == 1000 && !memcmp(ar.ptr + 1, ints + 1, 3 * sizeof(int)));
	ffarr_free(&ar);

	x(NULL != ffarr_copy(&ar, ints, FFCNT(ints)));
	_ffarr_rmleft((ffarr*)&ar, 2, sizeof(int));
	x(ar.len == 3 && !memcmp(ar.ptr, ints + 2, 3 * sizeof(int)));
	ffarr_free(&ar);

	ffarr_set(&ar, (int*)ints, FFCNT(ints));
	_ffarr_rmleft((ffarr*)&ar, 2, sizeof(int));
	x(ar.len == 3 && ar.ptr == ints + 2);

	return 0;
}

int test_inttostr()
{
	char s[FFINT_MAXCHARS];
	ffstr ss;
	ss.ptr = s;

	ss.len = ffs_fromint((uint64)-1, s, FFCNT(s), FFINT_SIGNED);
	x(ffstr_eqcz(&ss, "-1"));

	ss.len = ffs_fromint((uint64)-1, s, FFCNT(s), 0);
	x(ffstr_eqcz(&ss, "18446744073709551615"));

	ss.len = ffs_fromint(-1, s, FFCNT(s), FFINT_HEXUP);
	x(ffstr_eqcz(&ss, "FFFFFFFFFFFFFFFF"));

	ss.len = ffs_fromint(0xabc1, s, FFCNT(s), FFINT_HEXLOW | FFINT_ZEROWIDTH | FFINT_WIDTH(8));
	x(ffstr_eqcz(&ss, "0000abc1"));

	ss.len = ffs_fromint(0xabc1, s, FFCNT(s), FFINT_HEXLOW | FFINT_WIDTH(8));
	x(ffstr_eqcz(&ss, "    abc1"));

	ss.len = ffs_fromint(-1234, s, FFCNT(s), FFINT_SIGNED | FFINT_WIDTH(8));
	x(ffstr_eqcz(&ss, "   -1234"));

	ss.len = ffs_fromint(-1234, s, FFCNT(s), FFINT_SIGNED | FFINT_ZEROWIDTH | FFINT_WIDTH(8));
	x(ffstr_eqcz(&ss, "-0001234"));

	ss.len = ffs_fromint(1000, s, FFCNT(s), FFINT_SEP1000);
	x(ffstr_eqcz(&ss, "1,000"));

	ss.len = ffs_fromint(999, s, FFCNT(s), FFINT_SEP1000);
	x(ffstr_eqcz(&ss, "999"));

	return 0;
}

int test_strtoint()
{
	union {
		int64 i8;
		uint64 ui8;
		int i4;
		uint ui4;
		short i2;
		ushort ui2;
		byte ui1;
	} u;

	x(0 == ffs_toint(FFSTR(""), &u.i8, FFS_INT64));

	x(FFSLEN("3213213213213213123") == ffs_toint(FFSTR("3213213213213213123"), &u.i8, FFS_INT64)
		&& u.ui8 == 3213213213213213123ULL);
	x(FFSLEN("18446744073709551615") == ffs_toint(FFSTR("18446744073709551615;"), &u.i8, FFS_INT64)
		&& u.ui8 == 18446744073709551615ULL);
	x(FFSLEN("-123456789") == ffs_toint(FFSTR("-123456789"), &u.i8, FFS_INT64 | FFS_INTSIGN)
		&& u.i8 == -123456789);
	x(FFSLEN("-123456789") == ffs_toint(FFSTR("-123456789"), &u.i8, FFS_INT32 | FFS_INTSIGN)
		&& u.i4 == -123456789);

	x(FFSLEN("-1") == ffs_toint(FFSTR("-1"), &u.i8, FFS_INT64 | FFS_INTSIGN)
		&& u.i8 == -1);
	x(FFSLEN("+1") == ffs_toint(FFSTR("+1"), &u.i8, FFS_INT64 | FFS_INTSIGN)
		&& u.i8 == 1);

	x(FFSLEN("-3123456789") == ffs_toint(FFSTR("-3123456789"), &u.i8, FFS_INT64 | FFS_INTSIGN)
		&& u.i8 == -3123456789LL);
	x(0 == ffs_toint(FFSTR("-3123456789"), &u.i8, FFS_INT64));
	x(0 == ffs_toint(FFSTR("-3123456789"), &u.i8, FFS_INT32 | FFS_INTSIGN));

	x(FFSLEN("65535") == ffs_toint(FFSTR("65535"), &u.i8, FFS_INT16)
		&& u.ui2 == 65535);
	x(0 == ffs_toint(FFSTR("65536"), &u.i8, FFS_INT16));

	x(FFSLEN("255") == ffs_toint(FFSTR("255"), &u.i8, FFS_INT8)
		&& u.ui1 == 255);
	x(0 == ffs_toint(FFSTR("256"), &u.i8, FFS_INT8));

	x(FFSLEN("000abc1") == ffs_toint(FFSTR("000abc1"), &u.i8, FFS_INT32 | FFS_INTHEX)
		&& u.ui4 == 0xabc1);
	x(FFSLEN("abcdef") == ffs_toint(FFSTR("abcdef"), &u.i8, FFS_INT32 | FFS_INTHEX)
		&& u.ui4 == 0xabcdef);
	x(FFSLEN("abcdef") == ffs_toint(FFSTR("ABCDEF"), &u.i8, FFS_INT32 | FFS_INTHEX)
		&& u.ui4 == 0xabcdef);
	x(0 == ffs_toint(FFSTR("ffffffffffffffff"), &u.i8, FFS_INT32 | FFS_INTHEX));
	x(FFSLEN("ffffffffffffffff") == ffs_toint(FFSTR("ffffffffffffffff"), &u.i8, FFS_INT64 | FFS_INTHEX)
		&& u.ui8 == 0xffffffffffffffffLL);
	x(FFSLEN("ABCDE") == ffs_toint(FFSTR("ABCDEg"), &u.i4, FFS_INT32 | FFS_INTHEX)
		&& u.i4 == 0xabcde);

	x(0 == ffs_toint(FFSTR(":"), &u.i4, FFS_INT32 | FFS_INTHEX));
	x(0 == ffs_toint(FFSTR("-"), &u.i4, FFS_INT32 | FFS_INTSIGN));
	x(0 == ffs_toint(FFSTR("184467440737095516150"), &u.i8, FFS_INT64));

	return 0;
}

static int test_strtoflt()
{
	double d;
	x(1 == ffs_tofloat(FFSTR("1/"), &d, 0) && d == 1);
	x(1 == ffs_tofloat(FFSTR("1,0"), &d, 0) && d == 1);
	x(2 == ffs_tofloat(FFSTR("30#5"), &d, 0) && d == 30);
	x(2 == ffs_tofloat(FFSTR("1."), &d, 0) && d == 1.);
	x(2 == ffs_tofloat(FFSTR(".1"), &d, 0) && d == .1);
	x(3 == ffs_tofloat(FFSTR("1.1"), &d, 0) && d == 1.1);
	x(3 == ffs_tofloat(FFSTR("0.0"), &d, 0) && d == 0.0);

	x(0 != ffs_tofloat(FFSTR("1e1"), &d, 0) && d == 1e1);
	x(0 != ffs_tofloat(FFSTR("1e+1"), &d, 0) && d == 1e+1);
	x(0 != ffs_tofloat(FFSTR("1e-1"), &d, 0) && d == 1e-1);
	x(0 != ffs_tofloat(FFSTR("1.e-1"), &d, 0) && d == 1.e-1);
	x(0 != ffs_tofloat(FFSTR("1.1e-1"), &d, 0) && d == 1.1e-1);
	x(0 != ffs_tofloat(FFSTR(".1e-1"), &d, 0) && d == .1e-1);
	x(0 != ffs_tofloat(FFSTR("-.1e-1"), &d, 0) && d == -.1e-1);
	x(0 != ffs_tofloat(FFSTR("+.1e-1"), &d, 0) && d == +.1e-1);

	x(0 != ffs_tofloat(FFSTR("123.456e-052"), &d, 0) && d == 123.456e-052);
	x(0 != ffs_tofloat(FFSTR("1e-323"), &d, 0) && d == 1e-323);
	x(0 != ffs_tofloat(FFSTR("1e50/"), &d, 0) && d == 1e50);

	x(0 == ffs_tofloat(FFSTR("-"), &d, 0));
	x(0 == ffs_tofloat(FFSTR("+"), &d, 0));
	x(0 == ffs_tofloat(FFSTR("1e"), &d, 0));
	x(0 == ffs_tofloat(FFSTR("e-1"), &d, 0));
	x(0 == ffs_tofloat(FFSTR(".e-1"), &d, 0));
	return 0;
}

static int test_strf()
{
	ffstr3 s = { 0 };
	ffstr s1;
	ffqstr qs1;

	char ss[128];
	x(-((2+2)*2 + 5+5 + 1+5 + 1+1 + 1) == ffs_fmt2(NULL, 0, "%2b%*b" "%5s%*s" "%c%*c" "%%%Z"
		, "12", (size_t)2, "12"
		, "Strin", (size_t)5, "Strin"
		, 'C', (size_t)5, 'c'));
	x(((2+2)*2 + 5+5 + 1+5 + 1+1) == ffs_fmt2(ss, sizeof(ss), "%2b%*b" "%5s%*s" "%c%*c" "%%%Z"
		, "12", (size_t)2, "12"
		, "Strin", (size_t)5, "Strin"
		, 'C', (size_t)5, 'c')
			&& !ffsz_cmp(ss, "31323132StrinStrinCccccc%"));

	x(0 != ffstr_catfmt(&s, "%03D %03xI %3d 0x%p", (int64)-9, (size_t)-0x543fe, (int)-5, (void*)0xab1234));
	x(ffstr_eqcz(&s, "-09 -543fe  -5 0x00ab1234")),  s.len = 0;

	x(0 != ffstr_catfmt(&s, "%06.5F", (double)12345.6789) && ffstr_eqcz(&s, "012345.67890")),  s.len = 0;
	x(0 != ffstr_catfmt(&s, "%3.02F", (double)-0.123) && ffstr_eqcz(&s, " -0.12")),  s.len = 0;
	x(0 != ffstr_catfmt(&s, "%3.02F", (double)-12.12) && ffstr_eqcz(&s, "-12.12")),  s.len = 0;
	x(0 != ffstr_catfmt(&s, "%F", (double)-0.005) && ffstr_eqcz(&s, "-0.005000")),  s.len = 0;
	x(0 != ffstr_catfmt(&s, "%.0F", (double)-0.005) && ffstr_eqcz(&s, "-0")),  s.len = 0;

	s.len = 0;
	ffstr_setcz(&s1, "hello");
	ffqstr_set(&qs1, TEXT("hello"), 5);
	x(0 != ffstr_catfmt(&s, "%*s %S %*q %Q", (size_t)3, "hello", &s1, (size_t)3, TEXT("hello"), &qs1));
	x(ffstr_eqcz(&s, "hel hello hel hello"));

	s.len = 0;
	x(0 != ffstr_catfmt(&s, "%*c %Z%%", (size_t)5, (int)'-'));
	x(ffstr_eqcz(&s, "----- \0%"));

	s.len = 0;
	x(0 != ffstr_catfmt(&s, "%e: %E", (int)FFERR_FOPEN, (int)EINVAL));
#ifdef FF_UNIX
	x(ffstr_eqcz(&s, "file open: (22) Invalid argument"));
#else
	x(ffstr_eqcz(&s, "file open: (87) The parameter is incorrect"));
#endif

	ffarr_free(&s);
	return 0;
}

static int test_fmatch()
{
	char buf[8];
	uint u;
	uint64 u8;

	x(-2 == ffs_fmatch(FFSTR("1a"), "%2u:", &u));
	x(-3 == ffs_fmatch(FFSTR("12"), "%2u:", &u));

	x(8 + 1 == ffs_fmatch(FFSTR("asdfqwer:"), "%8s:", buf));
	x(!ffs_cmpz(buf, 8, "asdfqwer"));

	x(7 + 1 + 15 + 1 == ffs_fmatch(FFSTR("1234567:123456789012345%"), "%7u:%15U%%", &u, &u8));
	x(u == 1234567);
	x(u8 == 123456789012345);

	x(5 == ffs_fmatch(FFSTR(":123:"), ":%u:", &u) && u == 123);
	return 0;
}

static int test_str_cmp()
{
	const char *ptr = "asdfa";
	int n = FFSLEN("asdfa");

	x(0 == ffs_cmp(ptr, "asdff", 0));
	x(0 == ffs_cmp(ptr, "asdfaz", n));
	x(ffs_cmp(ptr, "asdff", n) < 0);

	x(0 == ffs_icmp(ptr, "ASDFF", 4));
	x(ffs_icmp(ptr, FFSTR("ASDF1")) > 0);
	x(ffs_icmp(ptr, FFSTR("ASDFF")) < 0);

	x(ffs_cmpz(ptr, 0, "asdff") < 0);
	x(ffs_cmpz(ptr, n, "") > 0);
	x(ffs_cmpz(ptr, 0, "") == 0);
	x(ffs_cmpz(ptr, n, "asdfa") == 0);
	x(ffs_cmpz(ptr, n, "asdf") == FFSLEN("asdf") + 1);
	x(ffs_cmpz(ptr, n, "asdff") == -FFSLEN("asdf") - 1);
	x(ffs_cmpz(ptr, n, "asdfaz") == -FFSLEN("asdfa") - 1);

	x(ffs_icmpz(ptr, n, "ASdFA") == 0);
	x(ffs_icmpz(ptr, n, "ASDF1") > 0);
	x(ffs_icmpz(ptr, n, "ASDFF") < 0);

	x(ffsz_cmp(ptr, "asdfa") == 0);
	x(ffsz_icmp(ptr, "ASdFA") == 0);

	x(ffs_eqcz(ptr, n, "asdfa"));
	x(!ffs_eqcz(ptr, n, "asdfaq"));
	x(!ffs_eqcz(ptr, n, "asdfA"));
	x(ffs_ieqcz(ptr, n, "asdfA"));

	x(ffs_match("key=val", 7, "key", 3));
	x(ffs_match("key=val", 7, "key=val", 7));
	x(!ffs_match("key=val", 7, "key1", 4));
	x(!ffs_match("key=val", 7, "keykeykeykey", 12));

	x(ffsz_match("key=val", "key", 3));
	x(ffsz_match("key=val", "key=val", 7));
	x(!ffsz_match("key=val", "key1", 4));
	x(!ffsz_match("key=val", "keykeykeykey", 12));

	return 0;
}

static int test_str_find()
{
	const char *ptr = "asdfa";
	int n = FFSLEN("asdfa");
	const char *end = ptr + n;

	x(ptr == ffs_find(ptr, n, 'a'));
	x(ptr + 3 == ffs_find(ptr, n, 'f'));
	x(end == ffs_find(ptr, n, 'z'));
	x(2 == ffs_nfindc(ptr, n, 'a'));
	x(0 == ffs_nfindc(ptr, n, 'z'));

	x(ptr + 4 == ffs_rfind(ptr, n, 'a'));
	x(ptr + 3 == ffs_rfind(ptr, n, 'f'));
	x(end == ffs_rfind(ptr, n, 'z'));

	x(ptr == ffs_finds(ptr, 0, FFSTR("")));
	x(ptr == ffs_finds(ptr, n, FFSTR("")));
	x(end == ffs_finds(ptr, n, FFSTR("dfb")));
	x(ptr + 3 == ffs_finds(ptr, n, FFSTR("f")));
	x(ptr + 2 == ffs_finds(ptr, n, FFSTR("df")));
	{
		const char *p = "abac";
		x(p + 2 == ffs_finds(p, 4, FFSTR("ac")));
	}

	x(ptr + 3 == ffs_findof(ptr, n, "zxcvf", FFSLEN("zxcvf")));
	x(end == ffs_findof(ptr, n, "zxcv", FFSLEN("zxcv")));

	x(ptr + 4 == ffs_rfindof(ptr, n, "zxcva", FFSLEN("zxcva")));
	x(end == ffs_rfindof(ptr, n, "zxcv", FFSLEN("zxcv")));

	{
		static const ffstr ss[] = { FFSTR_INIT("qwer"), FFSTR_INIT("asdf") };
		x(0 == ffstr_findarr(ss, FFCNT(ss), FFSTR("qwer")));
		x(1 == ffstr_findarr(ss, FFCNT(ss), FFSTR("asdf")));
		x(-1 == ffstr_findarr(ss, FFCNT(ss), FFSTR("asdfa")));
	}

	{
		static const char *const ss[] = { "qwer", "asdf" };
		x(1 == ffs_findarrz(ss, FFCNT(ss), FFSTR("asdf")));
		x(-1 == ffs_findarrz(ss, FFCNT(ss), FFSTR("asdfa")));
	}

	return 0;
}

static int test_str_nextval()
{
	ffstr s, v;

	ffstr_setcz(&s, " , qwer , asdf ,");
	x(FFSLEN(" ,") == ffstr_nextval3(&s, &v, ','));
	x(ffstr_eqcz(&v, ""));
	x(FFSLEN(" qwer ,") == ffstr_nextval3(&s, &v, ','));
	x(ffstr_eqcz(&v, "qwer"));
	x(FFSLEN(" asdf ") == ffstr_nextval3(&s, &v, ','));
	x(ffstr_eqcz(&v, "asdf"));
	x(FFSLEN(",") == ffstr_nextval3(&s, &v, ','));
	x(ffstr_eqcz(&v, ""));
	x(s.len == 0);

	// FFS_NV_KEEPWHITE
	ffstr_setcz(&s, " ,qwer , asdf ,");
	x(FFSLEN(" ,") == ffstr_nextval3(&s, &v, ',' | FFS_NV_KEEPWHITE));
	x(ffstr_eqcz(&v, " "));
	x(FFSLEN("qwer ,") == ffstr_nextval3(&s, &v, ',' | FFS_NV_KEEPWHITE));
	x(ffstr_eqcz(&v, "qwer "));
	x(FFSLEN(" asdf ") == ffstr_nextval3(&s, &v, ',' | FFS_NV_KEEPWHITE));
	x(ffstr_eqcz(&v, " asdf "));
	x(FFSLEN(",") == ffstr_nextval3(&s, &v, ',' | FFS_NV_KEEPWHITE));
	x(ffstr_eqcz(&v, ""));
	x(s.len == 0);

	// FFS_NV_REVERSE
	ffstr_setcz(&s, ",qwer, asdf ,");
	x(FFSLEN(",") == ffstr_nextval3(&s, &v, ',' | FFS_NV_REVERSE));
	x(ffstr_eqcz(&v, ""));
	x(FFSLEN(", asdf ") == ffstr_nextval3(&s, &v, ',' | FFS_NV_REVERSE));
	x(ffstr_eqcz(&v, "asdf"));
	x(FFSLEN("qwer") == ffstr_nextval3(&s, &v, ',' | FFS_NV_REVERSE));
	x(ffstr_eqcz(&v, "qwer"));
	x(FFSLEN(",") == ffstr_nextval3(&s, &v, ',' | FFS_NV_REVERSE));
	x(ffstr_eqcz(&v, ""));
	x(s.len == 0);

	// FFS_NV_WORDS
	ffstr_setcz(&s, " \t qwer  \t  asdf \t");
	x(FFSLEN(" \t qwer  \t  ") == ffstr_nextval3(&s, &v, FFS_NV_WORDS | FFS_NV_TABS));
	x(ffstr_eqcz(&v, "qwer"));
	x(FFSLEN("asdf \t") == ffstr_nextval3(&s, &v, FFS_NV_WORDS | FFS_NV_TABS));
	x(ffstr_eqcz(&v, "asdf"));
	x(s.len == 0);

	// FFS_NV_WORDS | FFSTR_NV_DBLQUOT
	ffstr_setcz(&s, " \t qwer  \t  \" as df \" \t");
	x(FFSLEN(" \t qwer  \t  ") == ffstr_nextval3(&s, &v, FFS_NV_WORDS | FFS_NV_TABS | FFSTR_NV_DBLQUOT));
	x(ffstr_eqcz(&v, "qwer"));
	x(FFSLEN("\" as df \" \t") == ffstr_nextval3(&s, &v, FFS_NV_WORDS | FFS_NV_TABS | FFSTR_NV_DBLQUOT));
	x(ffstr_eqcz(&v, " as df "));
	x(s.len == 0);

	return 0;
}

static int test_escape()
{
	char buf[64];
	ffstr s;
	s.ptr = buf;

	FFTEST_FUNC;

	x(FFSLEN("hello\\x00\\x01\xff\\x07\\x00\t\r\n\\x5chi") == ffs_escape(NULL, 0, FFSTR("hello\x00\x01\xff\f\b\t\r\n\\hi"), FFS_ESC_NONPRINT));
	s.len = ffs_escape(buf, FFCNT(buf), FFSTR("hello\x00\x01\xff\f\b\t\r\n\\hi"), FFS_ESC_NONPRINT);
	x(ffstr_eqcz(&s, "hello\\x00\\x01\xff\\x0C\\x08\t\r\n\\x5Chi"));

	return 0;
}

static int test_chcase()
{
	char buf[255];
	ffstr s;
	s.ptr = buf;

	FFTEST_FUNC;

	s.len = ffs_lower(buf, buf + FFCNT(buf), FFSTR("ASDFqwer"));
	x(ffstr_eqcz(&s, "asdfqwer"));

	s.len = ffs_upper(buf, buf + FFCNT(buf), FFSTR("ASDFqwer"));
	x(ffstr_eqcz(&s, "ASDFQWER"));

	s.len = ffs_titlecase(buf, buf + FFCNT(buf), FFSTR("#it's ASDF qwer-ty"));
	x(ffstr_eqcz(&s, "#it's Asdf Qwer-ty"));

	return 0;
}

static int test_bufadd()
{
	char sbuf[11];
	ffstr3 buf;
	ffstr dst;

	FFTEST_FUNC;

	ffarr_set3(&buf, sbuf, 0, FFCNT(sbuf));

	x(5 == ffbuf_add(&buf, FFSTR("12345"), &dst));
	x(5 == ffbuf_add(&buf, FFSTR("67890"), &dst));
	x(1 == ffbuf_add(&buf, FFSTR("12345"), &dst));
	x(ffstr_eqcz(&dst, "12345678901"));
	x(4 == ffbuf_add(&buf, FFSTR("2345"), &dst));
	x(ffstr_eqcz(&buf, "2345"));

	x(7 == ffbuf_add(&buf, FFSTR(" 12345678901234567890"), &dst));
	x(ffstr_eqcz(&dst, "2345 123456"));
	x(11 == ffbuf_add(&buf, FFSTR("78901234567890"), &dst));
	x(ffstr_eqcz(&dst, "78901234567"));
	x(3 == ffbuf_add(&buf, FFSTR("890"), &dst));
	x(ffstr_eqcz(&buf, "890"));

	x(0 == ffbuf_add(&buf, FFSTR(""), &dst));
	x(ffstr_eqcz(&dst, ""));

	return 0;
}

static int test_xml(void)
{
	char buf[64];
	ffstr s;
	FFTEST_FUNC;

	s.ptr = buf;
	x(FFSLEN("hello&lt;&gt;&amp;&quot;hi") == ffxml_escape(NULL, 0, FFSTR("hello<>&\"hi")));
	s.len = ffxml_escape(buf, FFCNT(buf), FFSTR("hello<>&\"hi"));
	x(ffstr_eqcz(&s, "hello&lt;&gt;&amp;&quot;hi"));

	return 0;
}

static int test_bstr(void)
{
	ffstr s = {0}, d = {0};
	ffbstr *bs;
	size_t off;
	FFTEST_FUNC;

	bs = ffbstr_push(&s, FFSTR("123"));
	x(bs != NULL);
	bs = ffbstr_push(&s, FFSTR("4567"));
	x(bs != NULL);

	off = 0;
	x(0 != ffbstr_next(s.ptr, s.len, &off, &d));
	x(ffstr_eqcz(&d, "123"));
	x(0 != ffbstr_next(s.ptr, s.len, &off, &d));
	x(ffstr_eqcz(&d, "4567"));
	x(0 == ffbstr_next(s.ptr, s.len, &off, &d));

	ffstr_free(&s);
	return 0;
}

static void test_wildcard(void)
{
	FFTEST_FUNC;

	x(0 == ffs_wildcard(NULL, 0, NULL, 0, 0));
	x(0 == ffs_wildcard(FFSTR("*"), NULL, 0, 0));
	x(0 < ffs_wildcard(FFSTR("?"), NULL, 0, 0));
	x(0 < ffs_wildcard(NULL, 0, FFSTR("a"), 0));
	x(0 == ffs_wildcard(FFSTR("aa"), FFSTR("aa"), 0));
	x(0 < ffs_wildcard(FFSTR("aa"), FFSTR("ba"), 0));
	x(0 == ffs_wildcard(FFSTR("*"), FFSTR("aa"), 0));
	x(0 == ffs_wildcard(FFSTR("?b?"), FFSTR("abc"), 0));

	x(0 == ffs_wildcard(FFSTR("*c"), FFSTR("abc"), 0));
	x(0 < ffs_wildcard(FFSTR("*c"), FFSTR("ab!"), 0));
	x(0 == ffs_wildcard(FFSTR("a*"), FFSTR("abc"), 0));
	x(0 == ffs_wildcard(FFSTR("a*c"), FFSTR("abbc"), 0));

	x(0 == ffs_wildcard(FFSTR("*aB*"), FFSTR("ac.Abc"), FFS_WC_ICASE));
	x(0 == ffs_wildcard(FFSTR("*ab*"), FFSTR("ac.abc"), 0));
	x(0 == ffs_wildcard(FFSTR("a*a*bb*c"), FFSTR("aabcabbc"), 0));
	x(0 < ffs_wildcard(FFSTR("a*a*bbc*c"), FFSTR("aabcabbc"), 0));
	x(0 < ffs_wildcard(FFSTR("*ab*"), FFSTR("ac.ac"), 0));
}

int test_regex(void)
{
	FFTEST_FUNC;

	x(0 == ffs_regex(FFSTR("ab"), FFSTR("ab"), 0));
	x(0 != ffs_regex(FFSTR("ab"), FFSTR("abc"), 0));
	x(0 != ffs_regex(FFSTR("abc"), FFSTR("ab"), 0));

	x(0 == ffs_regex(FFSTR("a|b"), FFSTR("a"), 0));
	x(0 != ffs_regex(FFSTR("a|b"), FFSTR("aa"), 0));
	x(0 == ffs_regex(FFSTR("a|b"), FFSTR("b"), 0));
	x(0 != ffs_regex(FFSTR("a|bc"), FFSTR("b"), 0));
	x(0 == ffs_regex(FFSTR("a|bc"), FFSTR("bc"), 0));
	x(0 == ffs_regex(FFSTR("a|"), FFSTR(""), 0));
	x(0 == ffs_regex(FFSTR("ab|abc|abcd"), FFSTR("abcd"), 0));
	x(0 != ffs_regex(FFSTR("ab|abc\\|abcd"), FFSTR("abcd"), 0));
	x(0 == ffs_regex(FFSTR("ab|abc\\|abcd"), FFSTR("abc|abcd"), 0));

	x(0 == ffs_regex(FFSTR("a."), FFSTR("ab"), 0));
	x(0 == ffs_regex(FFSTR("a\\..c"), FFSTR("a.bc"), 0));
	x(0 != ffs_regex(FFSTR("a\\..c"), FFSTR("a!bc"), 0));
	x(0 != ffs_regex(FFSTR("a\\!.c"), FFSTR("a!bc"), 0));

	x(0 != ffs_regex(FFSTR("a?bc"), FFSTR("bcc"), 0));
	//x(0 > ffs_regex(FFSTR("a??bc"), FFSTR("abc"), 0));
	//x(0 > ffs_regex(FFSTR("?bc"), FFSTR("abc"), 0));
	x(0 == ffs_regex(FFSTR("a?bc?"), FFSTR("b"), 0));
	x(0 == ffs_regex(FFSTR("a?bc?"), FFSTR("ab"), 0));
	x(0 == ffs_regex(FFSTR("a?bc?"), FFSTR("bc"), 0));
	x(0 == ffs_regex(FFSTR("a?bc?"), FFSTR("abc"), 0));
	x(0 == ffs_regex(FFSTR("a\\??"), FFSTR("a?"), 0));
	x(0 == ffs_regex(FFSTR("a\\??"), FFSTR("a"), 0));
	x(0 == ffs_regex(FFSTR("\\|?bc"), FFSTR("|bc"), 0));

	x(0 == ffs_regex(FFSTR("[123]"), FFSTR("1"), 0));
	x(0 == ffs_regex(FFSTR("[123]"), FFSTR("3"), 0));
	x(1 == ffs_regex(FFSTR("[123]"), FFSTR("4"), 0));
	x(1 == ffs_regex(FFSTR("[123]"), FFSTR("123"), 0));
	x(0 == ffs_regex(FFSTR("[12\\?3]"), FFSTR("?"), 0));
	x(0 == ffs_regex(FFSTR("[12\\]3]"), FFSTR("]"), 0));
	x(0 == ffs_regex(FFSTR("[\\[-\\]]"), FFSTR("\\"), 0));
	x(0 == ffs_regex(FFSTR("[1-2-3]"), FFSTR("3"), 0));
	x(0 == ffs_regex(FFSTR("[1-3]"), FFSTR("2"), 0));
	x(1 == ffs_regex(FFSTR("[1-3]"), FFSTR("4"), 0));
	x(0 == ffs_regex(FFSTR("[a-z0-9]"), FFSTR("1"), 0));
	x(0 == ffs_regex(FFSTR("[a-z0-9]"), FFSTR("3"), 0));
	x(0 == ffs_regex(FFSTR("[a-z0-9]"), FFSTR("w"), 0));
	x(0 == ffs_regex(FFSTR("[a-z0-9]"), FFSTR("z"), 0));
	x(1 == ffs_regex(FFSTR("[a-z0-9]"), FFSTR("Z"), 0));
	x(0 > ffs_regex(FFSTR("[]"), FFSTR(""), 0));
	x(0 > ffs_regex(FFSTR("[["), FFSTR(""), 0));
	x(0 > ffs_regex(FFSTR("["), FFSTR(""), 0));
	x(0 > ffs_regex(FFSTR("]"), FFSTR(""), 0));
	x(0 > ffs_regex(FFSTR("[1--"), FFSTR("1"), 0));
	x(1 == ffs_regex(FFSTR("[3-1]"), FFSTR("3"), 0));
	x(0 > ffs_regex(FFSTR("[-1]"), FFSTR("1"), 0));
	x(0 > ffs_regex(FFSTR("[1-]"), FFSTR("1"), 0));

	return 0;
}

static void test_utf(void)
{
	char utf8[FFUTF8_MAXCHARLEN];
	char utf16[8];
	size_t sl, r;
	uint n;

	FFTEST_FUNC;

	x(2 == ffutf8_size(0x044f));
	x(2 == ffutf8_encode1(utf8, 2, 0x044f));
	x(!memcmp(utf8, "\xd1\x8f", 2));

	x(2 == ffutf8_decode1("\xd1\x8f", 2, &n) && n == 0x044f);
	x(3 == ffutf8_decode1("\xef\xbf\xbd\x00", 4, &n) && n == 0xfffd);
	x(-3 == ffutf8_decode1("\xef\xbf", 2, &n));
	x(0 == ffutf8_decode1("\xff", 1, &n));

	sl = 2;
	x(2 == (r = ffutf8_encode(NULL, 0, "\xfc\x00", &sl, FFU_UTF16LE)) && sl == 2);

	sl = 2;
	x(2 == (r = ffutf8_encode(utf8, 2, "\xfc\x00", &sl, FFU_UTF16LE)) && sl == 2);

	sl = 2;
	x(2 == (r = ffutf8_encode(utf8, 2, "\x00\xfc", &sl, FFU_UTF16BE)) && sl == 2);

	//not enough output space
	sl = 2;
	x(0 == (r = ffutf8_encode(utf8, 1, "\xfc\x00", &sl, FFU_UTF16LE)) && sl == 0);

	//incomplete input
	sl = 1;
	x(0 == (r = ffutf8_encode(utf8, 2, "\xfc", &sl, FFU_UTF16LE)) && sl == 0);

	x(3 == (r = ffutf8_encodewhole(utf8, 3, "\xfc", 1, FFU_UTF16LE)));


	sl = 4;
	x(FFU_UTF8 == ffutf_bom("\xef\xbb\xbf\x00", &sl) && sl == 3);

	r = 4;
	x(4 == ffutf8_to_utf16(NULL, 0, "\xd1\x8f\xd1\x8f", &r, FFU_FWHOLE | FFU_UTF16BE)
		&& r == 4);
	x(4 == ffutf8_to_utf16(utf16, sizeof(utf16), "\xd1\x8f\xd1\x8f", &r, FFU_FWHOLE | FFU_UTF16BE)
		&& r == 4);
	x(!memcmp(utf16, "\x04\x4f\x04\x4f", 4));
}

static void test_str_fromsize(void)
{
	char buf[32];
	int r;
	x(0 != (r = ffs_fromsize(buf, sizeof(buf), 0, 0))
		&& ffs_eqcz(buf, r, "0"));
	x(0 != (r = ffs_fromsize(buf, sizeof(buf), 1023, 0))
		&& ffs_eqcz(buf, r, "1023"));
	x(0 != (r = ffs_fromsize(buf, sizeof(buf), 1024, 0))
		&& ffs_eqcz(buf, r, "1k"));
	x(0 != (r = ffs_fromsize(buf, sizeof(buf), 2048 + 1023, 0))
		&& ffs_eqcz(buf, r, "2k"));
	x(0 != (r = ffs_fromsize(buf, sizeof(buf), 1024 + 599, FFS_FROMSIZE_FRAC))
		&& ffs_eqcz(buf, r, "1.5k"));
	x(0 != (r = ffs_fromsize(buf, sizeof(buf), 64 * 1024 + 999, FFS_FROMSIZE_FRAC | FFS_FROMSIZE_Z))
		&& ffsz_eq(buf, "64.9k"));
	x(0 != (r = ffs_fromsize(buf, sizeof(buf), 64 * 1024 + 999, 0))
		&& ffs_eqcz(buf, r, "64k"));
	x(0 != (r = ffs_fromsize(buf, sizeof(buf), (uint64)-1, 0))
		&& ffs_eqcz(buf, r, "16777215t"));
}

static void test_str_crop(void)
{
	ffstr d, d2 = { 10, NULL };

	d = d2;
	x(4 == ffstr_crop_abs(&d, 1, 2, 3));
	x(d.ptr == (void*)1 && d.len == 3);

	d = d2;
	x(2 == ffstr_crop_abs(&d, 3, 2, 3));
	x(d.ptr == NULL && d.len == 2);

	d = d2;
	x(0 == ffstr_crop_abs(&d, 5, 2, 3));
	x(d.ptr == NULL && d.len == 0);
}

static void test_str_contig(void)
{
	ffarr a = {0};
	ffstr in, s;

	ffstr_setz(&in, "a");
	x(0 == ffbuf_contig(&a, &in, 4, &s));
	x(1 == ffbuf_contig_store(&a, &in, 4)
		&& ffstr_eqz(&a, "a"));
	a.len = 0;

	ffstr_setz(&in, "abcd12");
	x(0 == ffbuf_contig(&a, &in, 4, &s)
		&& ffstr_eq2(&s, &in) && a.len == 0);
	x(FFSLEN("abcd12") == ffbuf_contig_store(&a, &in, 4)
		&& ffstr_eqz(&a, "d12"));
	in.len = 0;
	x(0 == ffbuf_contig(&a, &in, 4, &s));
	x(0 == ffbuf_contig_store(&a, &in, 4)
		&& ffstr_eqz(&a, "d12"));

	ffstr_setz(&in, "3");
	x(FFSLEN("3") == ffbuf_contig(&a, &in, 4, &s)
		&& ffstr_eqz(&a, "d123"));
	x(0 == ffbuf_contig_store(&a, &in, 4)
		&& ffstr_eqz(&a, "123"));

	ffstr_setz(&in, "4efgh");
	x(FFSLEN("4ef") == ffbuf_contig(&a, &in, 4, &s)
		&& ffstr_eqz(&a, "1234ef"));
	x(0 == ffbuf_contig_store(&a, &in, 4)
		&& ffstr_eqz(&a, ""));
	x(0 == ffbuf_contig(&a, &in, 4, &s)
		&& ffstr_eq2(&s, &in) && a.len == 0);

	ffarr_copy(&a, "123456789", 9);
	x(0 == ffbuf_contig(&a, &in, 4, &s)
		&& ffstr_eqz(&a, "123456789"));
	x(0 == ffbuf_contig_store(&a, &in, 4)
		&& ffstr_eqz(&a, "789"));

	ffarr_free(&a);
}

static void test_str_gather(void)
{
	ffarr a = {0};
	ffstr s, in;
	x(4 == ffarr_gather2(&a, "abcd", 4, 7, &s)
		&& ffstr_eqz(&s, "abcd") && s.ptr == a.ptr);
	x(3 == ffarr_gather2(&a, "1234", 4, 7, &s)
		&& ffstr_eqz(&s, "abcd123") && s.ptr == a.ptr);
	x(0 == ffarr_gather2(&a, "1234", 4, 3, &s)
		&& ffstr_eqz(&s, "abc") && s.ptr == a.ptr);
	a.len = 0;
	ffstr_setz(&in, "1234");
	x(3 == ffarr_gather2(&a, in.ptr, in.len, 3, &s)
		&& a.len == 0 && a.cap != 0
		&& ffstr_eqz(&s, "123") && s.ptr == in.ptr);
	ffarr_free(&a);
}

static void test_str_vercmp(void)
{
	FFTEST_FUNC;
	ffstr s1, s2;
	ffstr_setz(&s1, "1.0.1");
	ffstr_setz(&s2, "1.0.2");
	x(ffstr_vercmp(&s1, &s2) == -1);

	ffstr_setz(&s1, "1.1.0");
	ffstr_setz(&s2, "1.0.2");
	x(ffstr_vercmp(&s1, &s2) > 0);

	ffstr_setz(&s1, "1.1");
	ffstr_setz(&s2, "1.1.1");
	x(ffstr_vercmp(&s1, &s2) < 0);

	ffstr_setz(&s1, "1.1");
	ffstr_setz(&s2, "1.1.0");
	x(ffstr_vercmp(&s1, &s2) == 0);

	ffstr_setz(&s1, "1.0.3");
	ffstr_setz(&s2, "1.0.2");
	x(ffstr_vercmp(&s1, &s2) > 0);

	ffstr_setz(&s1, "1.002.3");
	ffstr_setz(&s2, "1.1.2");
	x(ffstr_vercmp(&s1, &s2) > 0);

	ffstr_setz(&s1, "1.2..3");
	ffstr_setz(&s2, "1.1.2");
	x(ffstr_vercmp(&s1, &s2) == FFSTR_VERCMP_ERRV1);

	ffstr_setz(&s1, "1.2.3");
	ffstr_setz(&s2, "1.1.2.");
	x(ffstr_vercmp(&s1, &s2) == FFSTR_VERCMP_ERRV2);

	ffstr_setz(&s1, "1.2.3");
	ffstr_setz(&s2, ".1.1.2");
	x(ffstr_vercmp(&s1, &s2) == FFSTR_VERCMP_ERRV2);
}

static const char str_sql[] = FFS_QUOTE(
	select  *
	from table
	where id = "?"
	or id = ?
);

static void test_replace()
{
	char buf[8];
	size_t n;
	x(FFSLEN("asdfasdf") == ffs_replacechar(FFSTR("asdfasdf"), buf, FFCNT(buf), 'a', 'z', &n));
	x(n == 2);
	x(0 == ffmemcmp(buf, "zsdfzsdf", n));

	x(buf + 8 == ffmem_copycz(buf, "asdfasdf"));

	ffstr d, s, f, r;
	d.ptr = buf;
	ffstr_setz(&s, "1234567");
	ffstr_setz(&f, "234");
	ffstr_setz(&r, "xx");
	x(FFSLEN("1xx") == ffstr_replace(&d, &s, &f, &r, 0));
	x(ffstr_eqz(&d, "1xx567"));

	ffstr_setz(&s, "qwerty");
	ffstr_setz(&f, "Rt");
	ffstr_setz(&r, "XXX");
	x(FFSLEN("qweXXX") == ffstr_replace(&d, &s, &f, &r, FFSTR_REPL_ICASE));
	x(ffstr_eqz(&d, "qweXXXy"));

	ffstr_setz(&s, "1234567");
	ffstr_setz(&f, "234");
	ffstr_setz(&r, "xx");
	x(FFSLEN("1xx") == ffstr_replace(&d, &s, &f, &r, FFSTR_REPL_NOTAIL));
	x(ffstr_eqz(&d, "1xx"));

	ffstr_setz(&s, "1234567");
	ffstr_setz(&f, "78");
	x(0 == ffstr_replace(&d, &s, &f, &r, 0));
}

int test_str()
{
	FFTEST_FUNC;

	x(ffsz_eq(str_sql, "select * from table where id = \"?\" or id = ?"));

#define STR "  asdf  "
	x(STR + 2 == ffs_skip(FFSTR(STR), ' '));
	x(STR + FFSLEN(STR) - 2 == ffs_rskip(FFSTR(STR), ' '));
#undef STR
#define STR "  asdf\n\r\n"
	x(STR + FFSLEN(STR) - 3 == ffs_rskipof(FFSTR(STR), FFSTR("\r\n")));
#undef STR

	test_str_cmp();
	test_str_find();
	test_strcat();
	test_strqcat();
	test_inttostr();
	test_strtoint();
	test_strtoflt();
	test_strf();
	test_fmatch();
	test_arr();
	test_arrmem();
	test_str_nextval();
	test_escape();
	test_chcase();
	test_bufadd();
	test_replace();

	x(ffchar_islow('a') && !ffchar_islow('A') && !ffchar_islow('\xff') && !ffchar_islow('-'));
	x(ffchar_isname('A') && !ffchar_isname('\xff') && !ffchar_isname('-'));
	x(ffchar_isansiwhite(' ') && ffchar_isansiwhite('\xff') && !ffchar_isansiwhite('-'));
	x(10 == ffchar_sizesfx('k') && 10 * 4 == ffchar_sizesfx('t'));

	test_xml();
	test_bstr();
	test_wildcard();
	test_utf();
	test_str_fromsize();
	test_str_crop();
	test_str_contig();
	test_str_gather();
	test_str_vercmp();
	return 0;
}
