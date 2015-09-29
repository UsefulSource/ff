/** Array, buffer range.
Copyright (c) 2013 Simon Zolin
*/

#pragma once

#include <FFOS/mem.h>
#include <FF/string.h>

/** Declare an array. */
#define FFARR(T) \
	size_t len; \
	T *ptr; \
	size_t cap;

/** Default char-array. */
typedef struct ffarr { FFARR(char) } ffarr;

/** Set a buffer. */
#define ffarr_set(ar, data, n) \
do { \
	(ar)->ptr = data; \
	(ar)->len = n; \
} while(0)

#define ffarr_set3(ar, d, _len, _cap) \
do { \
	(ar)->ptr = d; \
	(ar)->len = _len; \
	(ar)->cap = _cap; \
} while(0)

/** Shift buffer pointers. */
#define ffarr_shift(ar, by) \
do { \
	ssize_t __shiftBy = (by); \
	(ar)->ptr += __shiftBy; \
	(ar)->len -= __shiftBy; \
} while (0)

/** Set null array. */
#define ffarr_null(ar) \
do { \
	(ar)->ptr = NULL; \
	(ar)->len = (ar)->cap = 0; \
} while (0)

/** Acquire array data. */
#define ffarr_acq(dst, src) \
do { \
	*(dst) = *(src); \
	(src)->cap = 0; \
} while (0)

/** The last element in array. */
#define ffarr_back(ar)  ((ar)->ptr[(ar)->len - 1])

/** The tail of array. */
#define ffarr_end(ar)  ((ar)->ptr + (ar)->len)

/** Get the edge of allocated buffer. */
#define ffarr_edge(ar)  ((ar)->ptr + (ar)->cap)

/** The number of free elements. */
#define ffarr_unused(ar)  ((ar)->cap - (ar)->len)

/** Return TRUE if array is full. */
#define ffarr_isfull(ar)  ((ar)->len == (ar)->cap)

/** Forward walk. */
#define FFARR_WALK(ar, it) \
	for (it = (ar)->ptr;  it != (ar)->ptr + (ar)->len;  it++)

/** Reverse walk. */
#define FFARR_RWALK(ar, it) \
	if ((ar)->len != 0) \
		for (it = (ar)->ptr + (ar)->len - 1;  it >= (ar)->ptr;  it--)

FF_EXTN void * _ffarr_realloc(ffarr *ar, size_t newlen, size_t elsz);

/** Reallocate array memory.
Return NULL on error. */
#define ffarr_realloc(ar, newlen) \
	_ffarr_realloc((ffarr*)(ar), newlen, sizeof(*(ar)->ptr))

static FFINL void * _ffarr_alloc(ffarr *ar, size_t len, size_t elsz) {
	ffarr_null(ar);
	return _ffarr_realloc(ar, len, elsz);
}

/** Allocate memory for an array. */
#define ffarr_alloc(ar, len) \
	_ffarr_alloc((ffarr*)(ar), (len), sizeof(*(ar)->ptr))

enum { FFARR_GROWQUARTER = -1 };

/** Reserve more space for an array. */
FF_EXTN char *_ffarr_grow(ffarr *ar, size_t by, ssize_t lowat, size_t elsz);

#define ffarr_grow(ar, by, lowat) \
	_ffarr_grow((ffarr*)(ar), (by), (lowat), sizeof(*(ar)->ptr))

FF_EXTN void _ffarr_free(ffarr *ar);

/** Deallocate array memory. */
#define ffarr_free(ar)  _ffarr_free((ffarr*)ar)

/** Add 1 item into array.
Return the item pointer.
Return NULL on error. */
FF_EXTN void * _ffarr_push(ffarr *ar, size_t elsz);

#define ffarr_push(ar, T) \
	(T*)_ffarr_push((ffarr*)ar, sizeof(T))

/** Add items into array.  Reallocate memory, if needed.
Return the tail.
Return NULL on error. */
FF_EXTN void * _ffarr_append(ffarr *ar, const void *src, size_t num, size_t elsz);

#define ffarr_append(ar, src, num) \
	_ffarr_append((ffarr*)ar, src, num, sizeof(*(ar)->ptr))

static FFINL void * _ffarr_copy(ffarr *ar, const void *src, size_t num, size_t elsz) {
	ar->len = 0;
	return _ffarr_append(ar, src, num, elsz);
}

#define ffarr_copy(ar, src, num) \
	_ffarr_copy((ffarr*)ar, src, num, sizeof(*(ar)->ptr))

/** Remove element from array.  Move the last element into the hole. */
static FFINL void _ffarr_rmswap(ffarr *ar, void *el, size_t elsz) {
	const void *last;
	FF_ASSERT(ar->len != 0);
	ar->len--;
	last = ar->ptr + ar->len * elsz;
	if (el != last)
		memcpy(el, last, elsz);
}

#define ffarr_rmswap(ar, el) \
	_ffarr_rmswap((ffarr*)ar, (void*)el, sizeof(*(ar)->ptr))


typedef struct ffstr {
	size_t len;
	char *ptr;
} ffstr;

typedef ffarr ffstr3;

#define FFSTR_INIT(s)  { FFSLEN(s), (char*)(s) }

#define FFSTR2(s)  (s).ptr, (s).len

#define ffstr_set(s, d, len)  ffarr_set(s, (char*)(d), len)

#define ffstr_set2(s, src)  ffarr_set(s, (src)->ptr, (src)->len)

/** Set constant NULL-terminated string. */
#define ffstr_setcz(s, csz)  ffarr_set(s, (char*)csz, FFSLEN(csz))

/** Set NULL-terminated string. */
#define ffstr_setz(s, sz)  ffarr_set(s, (char*)sz, ffsz_len(sz))

#define ffstr_setnz(s, sz, maxlen)  ffarr_set(s, (char*)sz, ffsz_nlen(sz, maxlen))

/** Set ffstr from ffiovec. */
#define ffstr_setiovec(s, iov)  ffarr_set(s, (iov)->iov_base, (iov)->iov_len)

#define ffstr_null(ar) \
do { \
	(ar)->ptr = NULL; \
	(ar)->len = 0; \
} while (0)

static FFINL void ffstr_acq(ffstr *dst, ffstr *src) {
	*dst = *src;
	ffstr_null(src);
}

static FFINL void ffstr_acqstr3(ffstr *dst, ffstr3 *src) {
	dst->ptr = src->ptr;
	dst->len = src->len;
	ffarr_null(src);
}

#define ffstr_shift  ffarr_shift

/** Copy the contents of ffstr* into char* buffer. */
#define ffs_copystr(dst, bufend, pstr)  ffs_copy(dst, bufend, (pstr)->ptr, (pstr)->len)

/** Split string by a character.
@at: pointer within the range [s..s+len] or NULL.
Return @at or NULL. */
FF_EXTN const char* ffs_split2(const char *s, size_t len, const char *at, ffstr *first, ffstr *second);

#define ffs_split2by(s, len, by, first, second) \
	ffs_split2(s, len, ffs_find(s, len, by), first, second)

#define ffs_rsplit2by(s, len, by, first, second) \
	ffs_split2(s, len, ffs_rfind(s, len, by), first, second)


#define ffstr_cmp2(s1, s2)  ffs_cmp4((s1)->ptr, (s1)->len, (s2)->ptr, (s2)->len)

/** Compare ANSI strings.  Case-insensitive. */
#define ffstr_icmp(str1, s2, len2)  ffs_icmp4((str1)->ptr, (str1)->len, s2, len2)

#define ffstr_eq(s, d, n) \
	((s)->len == (n) && 0 == ffmemcmp((s)->ptr, d, n))

/** Return TRUE if both strings are equal. */
#define ffstr_eq2(s1, s2)  ffstr_eq(s1, (s2)->ptr, (s2)->len)

static FFINL ffbool ffstr_ieq(const ffstr *s1, const char *s2, size_t n) {
	return s1->len == n
		&& 0 == ffs_icmp(s1->ptr, s2, n);
}

/** Return TRUE if both strings are equal. Case-insensitive */
#define ffstr_ieq2(s1, s2)  ffstr_ieq(s1, (s2)->ptr, (s2)->len)

/** Return TRUE if an array is equal to a NULL-terminated string. */
#define ffstr_eqz(str1, sz2)  (0 == ffs_cmpz((str1)->ptr, (str1)->len, sz2))
#define ffstr_ieqz(str1, sz2)  (0 == ffs_icmpz((str1)->ptr, (str1)->len, sz2))

/** Compare ffstr object and constant NULL-terminated string. */
#define ffstr_eqcz(s, constsz)  ffstr_eq(s, constsz, FFSLEN(constsz))

/** Compare ffstr object and constant NULL-terminated string.  Case-insensitive. */
#define ffstr_ieqcz(s, constsz)  ffstr_ieq(s, constsz, FFSLEN(constsz))

/** Return TRUE if n characters are equal in both strings. */
static FFINL ffbool ffstr_match(const ffstr *s1, const char *s2, size_t n) {
	return s1->len >= n
		&& 0 == ffmemcmp(s1->ptr, s2, n);
}

static FFINL ffbool ffstr_imatch(const ffstr *s1, const char *s2, size_t n) {
	return s1->len >= n
		&& 0 == ffs_icmp(s1->ptr, s2, n);
}

#define ffstr_matchcz(s, csz)  ffstr_match(s, csz, FFSLEN(csz))
#define ffstr_imatchcz(s, csz)  ffstr_imatch(s, csz, FFSLEN(csz))


static FFINL char * ffstr_alloc(ffstr *s, size_t cap) {
	s->len = 0;
	s->ptr = (char*)ffmem_alloc(cap * sizeof(char));
	return s->ptr;
}

static FFINL void ffstr_free(ffstr *s) {
	FF_SAFECLOSE(s->ptr, NULL, ffmem_free);
	s->len = 0;
}

static FFINL void ffstr_cat(ffstr *s, size_t cap, const char *d, size_t len) {
	char *p = ffs_copy(s->ptr + s->len, s->ptr + cap, d, len);
	s->len = p - s->ptr;
}

static FFINL char * ffstr_copy(ffstr *s, const char *d, size_t len) {
	if (NULL == ffstr_alloc(s, len))
		return NULL;
	ffstr_cat(s, len, d, len);
	return s->ptr;
}

#define ffstr_alcopystr(dst, src)  ffstr_copy(dst, (src)->ptr, (src)->len)


#if defined FF_UNIX
typedef ffstr ffqstr;
typedef ffstr3 ffqstr3;
#define ffqstr_set ffstr_set
#define ffqstr_alloc ffstr_alloc
#define ffqstr_free ffstr_free

#elif defined FF_WIN
typedef struct {
	size_t len;
	ffsyschar *ptr;
} ffqstr;

typedef struct { FFARR(ffsyschar) } ffqstr3;

static FFINL void ffqstr_set(ffqstr *s, const ffsyschar *d, size_t len) {
	ffarr_set(s, (ffsyschar*)d, len);
}

#define ffqstr_alloc(s, cap) \
	((ffsyschar*)ffstr_alloc((ffstr*)(s), (cap) * sizeof(ffsyschar)))

#define ffqstr_free(s)  ffstr_free((ffstr*)(s))

#endif

/** Find substring.
Return -1 if not found. */
static FFINL ssize_t ffstr_find(const ffstr *s, const char *search, size_t search_len) {
	const char *r = ffs_finds(s->ptr, s->len, search, search_len);
	if (r == s->ptr + s->len)
		return -1;
	return r - s->ptr;
}

static FFINL ssize_t ffstr_ifind(const ffstr *s, const char *search, size_t search_len) {
	const char *r = ffs_ifinds(s->ptr, s->len, search, search_len);
	if (r == s->ptr + s->len)
		return -1;
	return r - s->ptr;
}

/** Find string in an array of strings.
Return array index.
Return -1 if not found. */
FF_EXTN ssize_t ffstr_findarr(const ffstr *ar, size_t n, const char *search, size_t search_len);

FF_EXTN ssize_t ffstr_ifindarr(const ffstr *ar, size_t n, const char *search, size_t search_len);

enum FFSTR_NEXTVAL {
	FFSTR_NV_DBLQUOT = 0x100, // val1 "val2 with space" val3
};

/** Get the next value from input string like "val1, val2, ...".
Spaces on the edges are trimmed.
@spl: split-character OR-ed with enum FFSTR_NEXTVAL.
Return the number of processed bytes. */
FF_EXTN size_t ffstr_nextval(const char *buf, size_t len, ffstr *dst, int spl);

static FFINL void ffstr3_cat(ffstr3 *s, const char *d, size_t len) {
	ffstr_cat((ffstr*)s, s->cap, d, len);
}

FF_EXTN size_t ffstr_catfmtv(ffstr3 *s, const char *fmt, va_list args);

static FFINL size_t ffstr_catfmt(ffstr3 *s, const char *fmt, ...) {
	size_t r;
	va_list args;
	va_start(args, fmt);
	r = ffstr_catfmtv(s, fmt, args);
	va_end(args);
	return r;
}

/** Formatted output into a file.
'buf': optional buffer. */
FF_EXTN size_t fffile_fmt(fffd fd, ffstr3 *buf, const char *fmt, ...);

/** Buffered data output.
@dst is set if an output data block is ready.
Return the number of processed bytes. */
size_t ffbuf_add(ffstr3 *buf, const char *src, size_t len, ffstr *dst);


typedef struct ffbstr {
	ushort len;
	char data[0];
} ffbstr;

/** Add one more ffbstr into array.  Reallocate memory, if needed.
If @data is set, copy it into a new ffbstr. */
FF_EXTN ffbstr * ffbstr_push(ffstr *buf, const char *data, size_t len);

/** Copy data into ffbstr. */
static FFINL void ffbstr_copy(ffbstr *bs, const char *data, size_t len)
{
	bs->len = (ushort)len;
	ffmemcpy(bs->data, data, len);
}

/** Get the next string from array.
@off: set value to 0 before the first call.
Return 0 if there is no more data. */
static FFINL ffbstr* ffbstr_next(const char *buf, size_t len, size_t *off, ffstr *dst)
{
	ffbstr *bs = (ffbstr*)(buf + *off);
	if (*off == len)
		return NULL;

	if (dst != NULL)
		ffstr_set(dst, bs->data, bs->len);
	*off += sizeof(ffbstr) + bs->len;
	return bs;
}


typedef struct ffrange {
	ushort len
		, off;
} ffrange;

static FFINL void ffrang_set(ffrange *r, const char *base, const char *s, size_t len) {
	r->off = (short)(s - base);
	r->len = (ushort)len;
}

static FFINL ffstr ffrang_get(const ffrange *r, const char *base) {
	ffstr s;
	ffstr_set(&s, base + r->off, r->len);
	return s;
}

static FFINL void ffrang_clear(ffrange *r) {
	r->off = r->len = 0;
}
