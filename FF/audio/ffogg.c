/**
Copyright (c) 2015 Simon Zolin
*/

#include <FF/audio/ogg.h>
#include <FF/string.h>
#include <FFOS/error.h>
#include <FFOS/mem.h>

#include <ogg/ogg-crc.h>


enum OGG_F {
	OGG_FCONTINUED = 1,
	OGG_FFIRST = 2,
	OGG_FLAST = 4,
};

typedef struct ogg_hdr {
	char sync[4]; //"OggS"
	byte version; //0
	byte flags; //enum OGG_F
	byte granulepos[8];
	byte serial[4];
	byte number[4];
	byte crc[4];
	byte nsegments;
	byte segments[0];
} ogg_hdr;

enum VORBIS_HDR_T {
	T_INFO = 1,
	T_COMMENT = 3,
};

#define OGG_STR  "OggS"
#define VORB_STR  "vorbis"

struct vorbis_hdr {
	byte type; //enum VORBIS_HDR_T
	char vorbis[6]; //"vorbis"
};

struct vorbis_info {
	byte ver[4]; //0
	byte channels;
	byte rate[4];
	byte br_max[4];
	byte br_nominal[4];
	byte br_min[4];
	byte blocksize;
	byte framing_bit; //1
};

enum {
	OGG_MAXHDR = sizeof(ogg_hdr) + 255,
	OGG_MAXPAGE = OGG_MAXHDR + 255 * 255,
	MAX_NOSYNC = 256 * 1024,
};

static uint ogg_checksum(const char *d, size_t len);
static int ogg_parse(ogg_hdr *h, const char *data, size_t len);
static uint ogg_pagesize(const ogg_hdr *h);
static uint ogg_packets(const ogg_hdr *h);
static int ogg_findpage(const char *data, size_t len, ogg_hdr *h);
static int ogg_pkt_next(ogg_packet *pkt, const char *buf, uint *segoff, uint *bodyoff);
static int ogg_pkt_write(ffogg_page *p, char *buf, const char *pkt, uint len);
static int ogg_page_write(ffogg_page *p, char *buf, uint64 granulepos, uint flags, ffstr *page);
static int ogg_hdr_write(ffogg_page *p, char *buf, ffstr *data,
	const ogg_packet *info, const ogg_packet *tag, const ogg_packet *codbk);

static int vorb_info(const char *d, size_t len, uint *channels, uint *rate, uint *br_nominal);
static void* ogg_tag(const char *d, size_t len, size_t *vorbtag_len);
static uint ogg_tag_write(char *d, size_t vorbtag_len);

static int _ffogg_gethdr(ffogg *o, ogg_hdr **h);
static int _ffogg_getbody(ffogg *o);
static int _ffogg_open(ffogg *o);
static int _ffogg_seek(ffogg *o);

static int _ffogg_pkt_vorbtag(ffogg_enc *o, ogg_packet *pkt);
static int _ffogg_enc_hdr(ffogg_enc *o);


enum { I_HDR, I_BODY, I_HDRPKT, I_COMM_PKT, I_COMM, I_BOOK_PKT, I_FIRSTPAGE
	, I_SEEK_EOS, I_SEEK_EOS2, I_SEEK_EOS3
	, I_SEEK, I_SEEK2, I_ASIS_SEEKHDR
	, I_PAGE, I_PKT, I_DECODE, I_DATA };

enum OGG_E {
	OGG_EOK
	, OGG_ESEEK
	, OGG_EHDR,
	OGG_EVERSION,
	OGG_ESERIAL,
	OGG_EPAGENUM,
	OGG_EJUNKDATA,
	OGG_ECRC,
	OGG_ETAG,
	OGG_ENOSYNC,
	OGG_EPKT,
	OGG_EBIGPKT,

	OGG_ESYS,
};

static const char* const ogg_errstr[] = {
	""
	, "seek error"
	, "bad OGG header",
	"unsupported page version",
	"the serial number of the page did not match the serial number of the bitstream",
	"out of order OGG page",
	"unrecognized data before OGG page",
	"CRC mismatch",
	"invalid tags",
	"couldn't find OGG page",
	"bad packet",
	"too large packet",
};

const char* ffogg_errstr(int e)
{
	if (e == OGG_ESYS)
		return fferr_strp(fferr_last());

	if (e >= 0)
		return ogg_errstr[e];
	return vorbis_errstr(e);
}

#define ERR(o, n) \
	(o)->err = n, FFOGG_RERR


/** Get page checksum. */
static uint ogg_checksum(const char *d, size_t len)
{
	uint crc = 0, i;

	for (i = 0;  i != 22;  i++)
		crc = (crc << 8) ^ ogg_crc_table[((crc >> 24) & 0xff) ^ (byte)d[i]];
	for (;  i != 26;  i++)
		crc = (crc << 8) ^ ogg_crc_table[((crc >> 24) & 0xff) ^ 0x00];
	for (;  i != len;  i++)
		crc = (crc << 8) ^ ogg_crc_table[((crc >> 24) & 0xff) ^ (byte)d[i]];

	return crc;
}

/** Parse header.
Return header length;  0 if more data is needed;  -OGG_E* on error. */
static int ogg_parse(ogg_hdr *h, const char *data, size_t len)
{
	ogg_hdr *hdr = (void*)data;
	if (len < sizeof(ogg_hdr))
		return 0;
	if (!!ffs_cmp(data, OGG_STR, FFSLEN(OGG_STR)) || hdr->version != 0)
		return -OGG_EHDR;
	if (len < sizeof(ogg_hdr) + hdr->nsegments)
		return 0;
	return sizeof(ogg_hdr) + hdr->nsegments;
}

static uint ogg_pagesize(const ogg_hdr *h)
{
	uint i, r = 0;
	for (i = 0;  i != h->nsegments;  i++) {
		r += h->segments[i];
	}
	return sizeof(ogg_hdr) + h->nsegments + r;
}

/** Get number of packets in page. */
static FFINL uint ogg_packets(const ogg_hdr *h)
{
	uint i, n = 0;
	for (i = 0;  i != h->nsegments;  i++) {
		if (h->segments[i] < 255)
			n++;
	}
	return n;
}

/**
Return offset of the page;  -1 on error. */
static int ogg_findpage(const char *data, size_t len, ogg_hdr *h)
{
	const char *d = data, *end = data + len;

	while (d != end) {

		if (d[0] != 'O' && NULL == (d = ffs_findc(d, end - d, 'O')))
			break;

		if (ogg_parse(h, d, end - d) > 0)
			return d - data;

		d++;
	}

	return -1;
}

/** Get next packet from page.
@segoff: current offset within ogg_hdr.segments
@bodyoff: current offset within page body
Return packet body size;  -1 if no more packets;  -2 for an incomplete packet. */
static int ogg_pkt_next(ogg_packet *pkt, const char *buf, uint *segoff, uint *bodyoff)
{
	const ogg_hdr *h = (void*)buf;
	uint seglen, pktlen = 0, nsegs = h->nsegments, i = *segoff;

	if (i == nsegs)
		return -1;

	do {
		seglen = h->segments[i++];
		pktlen += seglen;
	} while (seglen == 255 && i != nsegs);

	pkt->packet = (byte*)buf + sizeof(ogg_hdr) + nsegs + *bodyoff;
	pkt->bytes = pktlen;
	*segoff = i;
	*bodyoff += pktlen;

	pkt->b_o_s = 0;
	pkt->e_o_s = 0;
	pkt->granulepos = -1;
	return (seglen == 255) ? -2 : (int)pktlen;
}

/** Add packet into page. */
static int ogg_pkt_write(ffogg_page *p, char *buf, const char *pkt, uint len)
{
	uint i, newsegs = p->nsegments + len / 255 + 1;

	if (newsegs > 255)
		return OGG_EBIGPKT; // partial packets aren't supported

	for (i = p->nsegments;  i != newsegs - 1;  i++) {
		p->segs[i] = 255;
	}
	p->segs[i] = len % 255;
	p->nsegments = newsegs;

	ffmemcpy(buf + p->size + OGG_MAXHDR, pkt, len);
	p->size += len;
	return 0;
}

/** Write page header into the position in buffer before page body.
Buffer: [... OGG_HDR PKT1 PKT2 ...]
@page: is set to page data within buffer.
@flags: enum OGG_F. */
static int ogg_page_write(ffogg_page *p, char *buf, uint64 granulepos, uint flags, ffstr *page)
{
	ogg_hdr *h = (void*)(buf + OGG_MAXHDR - (sizeof(ogg_hdr) + p->nsegments));
	ffmemcpy(h->sync, OGG_STR, FFSLEN(OGG_STR));
	h->version = 0;
	h->flags = flags;
	ffint_htol64(h->granulepos, granulepos);
	ffint_htol32(h->serial, p->serial);
	ffint_htol32(h->number, p->number++);
	h->nsegments = p->nsegments;
	ffmemcpy(h->segments, p->segs, h->nsegments);

	p->size += sizeof(ogg_hdr) + p->nsegments;
	ffint_htol32(h->crc, ogg_checksum((void*)h, p->size));

	ffstr_set(page, (void*)h, p->size);

	p->nsegments = 0;
	p->size = 0;
	return 0;
}

/** Get 2 pages at once containting all 3 vorbis headers. */
static int ogg_hdr_write(ffogg_page *p, char *buf, ffstr *data,
	const ogg_packet *info, const ogg_packet *tag, const ogg_packet *codbk)
{
	ffstr s;
	char *d = buf + OGG_MAXHDR + info->bytes;
	if (0 != ogg_pkt_write(p, d, (void*)tag->packet, tag->bytes)
		|| 0 != ogg_pkt_write(p, d, (void*)codbk->packet, codbk->bytes))
		return OGG_EBIGPKT;
	p->number = 1;
	ogg_page_write(p, d, 0, 0, &s);

	if (info->bytes != sizeof(struct vorbis_hdr) + sizeof(struct vorbis_info))
		return OGG_EHDR;
	d = s.ptr - (OGG_MAXHDR + info->bytes);
	ogg_pkt_write(p, d, (void*)info->packet, info->bytes);
	p->number = 0;
	ogg_page_write(p, d, 0, OGG_FFIRST, data);
	data->len += s.len;
	p->number = 2;

	return 0;
}

/** Parse Vorbis-info packet. */
static int vorb_info(const char *d, size_t len, uint *channels, uint *rate, uint *br_nominal)
{
	const struct vorbis_hdr *h = (void*)d;
	if (len < sizeof(struct vorbis_hdr) + sizeof(struct vorbis_info)
		|| !(h->type == T_INFO && !ffs_cmp(h->vorbis, VORB_STR, FFSLEN(VORB_STR))))
		return -1;

	const struct vorbis_info *vi = (void*)(d + sizeof(struct vorbis_hdr));
	if (0 != ffint_ltoh32(vi->ver)
		|| 0 == (*channels = vi->channels)
		|| 0 == (*rate = ffint_ltoh32(vi->rate))
		|| vi->framing_bit != 1)
		return -1;

	*br_nominal = ffint_ltoh32(vi->br_nominal);
	return 0;
}

/**
Return pointer to the beginning of Vorbis comments data;  NULL if not Vorbis comments header. */
static void* ogg_tag(const char *d, size_t len, size_t *vorbtag_len)
{
	const struct vorbis_hdr *h = (void*)d;

	if (len < (uint)sizeof(struct vorbis_hdr)
		|| !(h->type == T_COMMENT && !ffs_cmp(h->vorbis, VORB_STR, FFSLEN(VORB_STR))))
		return NULL;

	*vorbtag_len = len - sizeof(struct vorbis_hdr);
	return (char*)d + sizeof(struct vorbis_hdr);
}

/** Prepare OGG packet for Vorbis comments.
@d: buffer for the whole packet, must have 1 byte of free space at the end
Return packet length. */
static uint ogg_tag_write(char *d, size_t vorbtag_len)
{
	struct vorbis_hdr *h = (void*)d;
	h->type = T_COMMENT;
	ffmemcpy(h->vorbis, VORB_STR, FFSLEN(VORB_STR));
	d[sizeof(struct vorbis_hdr) + vorbtag_len] = 1; //set framing bit
	return sizeof(struct vorbis_hdr) + vorbtag_len + 1;
}


void ffogg_init(ffogg *o)
{
	ffmem_tzero(o);
	o->nxstate = I_HDRPKT;
}

uint ffogg_bitrate(ffogg *o)
{
	if (o->total_samples == 0 || o->total_size == 0)
		return o->vinfo.bitrate_nominal;
	return ffpcm_brate(o->total_size - o->off_data, o->total_samples, o->vinfo.rate);
}

/** Get the whole OGG page header.
Return 0 on success. o->data points to page body. */
static int _ffogg_gethdr(ffogg *o, ogg_hdr **h)
{
	int r, n = 0;
	const ogg_hdr *hdr;

	struct ffbuf_gather d = {0};
	ffstr_set(&d.data, o->data, o->datalen);
	d.ctglen = OGG_MAXHDR;

	while (FFBUF_DONE != (r = ffbuf_gather(&o->buf, &d))) {

		if (r == FFBUF_ERR) {
			o->err = OGG_ESYS;
			return FFOGG_RERR;

		} else if (r == FFBUF_MORE) {
			goto more;
		}

		if (-1 != (n = ogg_findpage(o->buf.ptr, o->buf.len, NULL))) {
			hdr = (void*)(o->buf.ptr + n);
			o->hdrsize = sizeof(ogg_hdr) + hdr->nsegments;
			o->pagesize = ogg_pagesize(hdr);
			d.ctglen = o->hdrsize;
			d.off = n + 1;
		}
	}
	o->off += d.data.ptr - (char*)o->data;
	o->data = d.data.ptr;
	o->datalen = d.data.len;

	hdr = (void*)o->buf.ptr;
	*h = (void*)o->buf.ptr;

	if (o->bytes_skipped != 0)
		o->bytes_skipped = 0;

	FFDBG_PRINTLN(10, "page #%u  end-pos:%xU  packets:%u  continued:%u  size:%u  offset:%xU"
		, ffint_ltoh32(hdr->number), ffint_ltoh64(hdr->granulepos)
		, ogg_packets(hdr), (hdr->flags & OGG_FCONTINUED) != 0
		, o->pagesize, o->off - o->hdrsize);

	if (n != 0) {
		o->err = OGG_EJUNKDATA;
		return FFOGG_RWARN;
	}
	return FFOGG_RDONE;

more:
	o->bytes_skipped += o->datalen;
	if (o->bytes_skipped > MAX_NOSYNC) {
		o->err = OGG_ENOSYNC;
		return FFOGG_RERR;
	}

	o->off += o->datalen;
	return FFOGG_RMORE;
}

static int _ffogg_getbody(ffogg *o)
{
	int r = ffarr_append_until(&o->buf, o->data, o->datalen, o->pagesize);
	if (r == 0) {
		o->off += o->datalen;
		return FFOGG_RMORE;
	} else if (r == -1) {
		o->err = OGG_ESYS;
		return FFOGG_RERR;
	}
	FFARR_SHIFT(o->data, o->datalen, r);
	o->off += r;
	o->buf.len = 0;
	const ogg_hdr *h = (void*)o->buf.ptr;

	uint crc = ogg_checksum(o->buf.ptr, o->pagesize);
	uint hcrc = ffint_ltoh32(h->crc);
	if (crc != hcrc) {
		FFDBG_PRINTLN(1, "Bad page CRC:%xu, CRC in header:%xu", crc, hcrc);
		o->err = OGG_ECRC;
		return FFOGG_RERR;
	}

	if (h->version != 0)
		return ERR(o, OGG_EVERSION);

	uint serial = ffint_ltoh32(h->serial);
	if (!o->init_done)
		o->serial = serial;
	else if (serial != o->serial)
		return ERR(o, OGG_ESERIAL);

	o->page_last = (h->flags & OGG_FLAST) != 0;

	uint pagenum = ffint_ltoh32(h->number);
	if (o->page_num != 0 && pagenum != o->page_num + 1)
		o->pagenum_err = 1;
	o->page_num = pagenum;

	o->page_gpos = ffint_ltoh64(h->granulepos);
	o->segoff = 0;
	o->bodyoff = 0;
	return FFOGG_RDONE;
}

static int _ffogg_open(ffogg *o)
{
	int r;
	ogg_hdr *h;

	for (;;) {
	switch (o->state) {

	case I_HDRPKT:
		if (0 != vorb_info((void*)o->opkt.packet, o->opkt.bytes, &o->vinfo.channels, &o->vinfo.rate, &o->vinfo.bitrate_nominal))
			return ERR(o, OGG_EPKT);

		if (0 != (r = vorbis_decode_init(&o->vctx, &o->opkt)))
			return ERR(o, r);

		o->state = I_PKT, o->nxstate = I_COMM_PKT;
		return FFOGG_RHDR;

	case I_COMM_PKT:
		if (NULL == (o->vtag.data = ogg_tag((void*)o->opkt.packet, o->opkt.bytes, &o->vtag.datalen)))
			return ERR(o, OGG_ETAG);
		o->state = I_COMM;
		// break

	case I_COMM:
		r = ffvorbtag_parse(&o->vtag);
		if (r == FFVORBTAG_ERR) {
			o->err = OGG_ETAG;
			return r;
		} else if (r == FFVORBTAG_DONE) {
			if (!(o->vtag.datalen != 0 && o->vtag.data[0] == 1)) {
				o->err = OGG_ETAG;
				return FFOGG_RERR;
			}
			o->state = I_PKT, o->nxstate = I_BOOK_PKT;
			return FFOGG_RDATA;
		}
		return FFOGG_RTAG;

	case I_BOOK_PKT:
		if (0 != (r = vorbis_decode_init(&o->vctx, &o->opkt)))
			return ERR(o, r);

		o->off_data = o->off;
		o->state = I_FIRSTPAGE;
		break;

	case I_FIRSTPAGE:
		r = _ffogg_gethdr(o, &h);
		if (r != FFOGG_RDONE)
			return r;
		uint pagenum = ffint_ltoh32(h->number);
		if (pagenum != 2) {
			o->seektab[0].off = o->pagesize; //remove the first audio page from seek table, because we don't know the audio sample index
			o->first_sample = ffint_ltoh64(h->granulepos);
		}
		o->page_num = pagenum - 1;

		if (o->seekable && o->total_size != 0)
			o->state = I_SEEK_EOS;
		else
			o->state = I_PAGE;
		o->init_done = 1;
		o->nxstate = I_DECODE;
		return FFOGG_RHDRFIN;

	case I_SEEK_EOS:
		o->off = o->total_size - ffmin(OGG_MAXPAGE, o->total_size);
		o->state = I_SEEK_EOS2;
		return FFOGG_RSEEK;

	case I_SEEK_EOS2:
		for (;;) {
			r = _ffogg_gethdr(o, &h);
			if (r != FFOGG_RDONE && !(r == FFOGG_RWARN && o->err == OGG_EJUNKDATA)) {
				if (o->off == o->total_size)
					break; // no eos page
				return FFOGG_RMORE;
			}
			uint buflen = o->buf.len;
			o->buf.len = 0;

			o->total_samples = ffint_ltoh64(h->granulepos) - o->first_sample;
			if (h->flags & OGG_FLAST)
				break;

			o->off += o->pagesize - buflen;
			if (o->off == o->total_size)
				break; // no eos page
			return FFOGG_RSEEK;
		}

		if (o->total_samples != 0) {
			o->seektab[1].sample = o->total_samples;
			o->seektab[1].off = o->total_size - o->off_data;
		}

		o->state = I_SEEK_EOS3;
		return FFOGG_RINFO;

	case I_SEEK_EOS3:
		o->off = o->off_data;
		o->state = I_HDR;
		return FFOGG_RSEEK;
	}
	}
	//unreachable
}

void ffogg_close(ffogg *o)
{
	ffarr_free(&o->buf);
	FF_SAFECLOSE(o->vctx, NULL, vorbis_decode_free);
}

void ffogg_seek(ffogg *o, uint64 sample)
{
	if (sample >= o->total_samples || o->total_size == 0)
		return;
	o->seek_sample = sample;
	if (o->state == I_SEEK_EOS3)
		o->firstseek = 1;
	o->state = I_SEEK;
	ffmemcpy(o->seekpt, o->seektab, sizeof(o->seekpt));
	o->skoff = (uint64)-1;
	o->buf.len = 0;
}

/* Seeking in OGG Vorbis:

... [P1 ^P2 P3] P4 ...
where:
 P{N} are OGG pages
 ^ is the target page
 [...] are the search boundaries

An OGG page is not enough to determine whether this page is our target,
 e.g. audio position of P2 becomes known only from P1.granule_pos.
Therefore, we need additional processing:

 0. Parse 2 consecutive pages and pass info from the 2nd page to ffpcm_seek(),
    e.g. parse P2 and P3, then pass P3 to ffpcm_seek().

 1. If the 2nd page is out of search boundaries (P4), we must not pass it to ffpcm_seek().
    We seek backward on a file (somewhere to [P1..P2]) and try again (cases 0 or 2).

 2. If the currently processed page is the first within search boundaries (P2 in [^P2 P3])
    and it's proven to be the target page (its granulepos > target sample),
    use audio position value from the lower search boundary, so we don't need the previous page.
*/
static int _ffogg_seek(ffogg *o)
{
	int r;
	struct ffpcm_seek sk;
	ogg_hdr *h;
	uint64 gpos, foff = o->off;

	if (o->firstseek) {
		// we don't have any page right now
		o->firstseek = 0;
		sk.fr_index = 0;
		sk.fr_samples = 0;
		sk.fr_size = sizeof(struct ogg_hdr);
		goto sk;
	}

	for (;;) {
		r = _ffogg_gethdr(o, &h);
		if (r != FFOGG_RDONE && !(r == FFOGG_RWARN && o->err == OGG_EJUNKDATA))
			return r;
		foff = o->off - o->hdrsize;

		switch (o->state) {
		case I_SEEK:
			gpos = ffint_ltoh64(h->granulepos) - o->first_sample;
			if (gpos > o->seek_sample && o->skoff == o->seekpt[0].off) {
				sk.fr_index = o->seekpt[0].sample;
				sk.fr_samples = gpos - o->seekpt[0].sample;
				sk.fr_size = o->pagesize;
				break;
			}
			o->buf.len = 0;
			o->cursample = gpos;
			o->state = I_SEEK2;
			continue;

		case I_SEEK2:
			o->state = I_SEEK;
			if (foff - o->off_data >= o->seekpt[1].off) {
				uint64 newoff = ffmax((int64)o->skoff - OGG_MAXPAGE, (int64)o->seekpt[0].off);
				if (newoff == o->skoff) {
					o->err = OGG_ESEEK;
					return FFOGG_RERR;
				}
				o->buf.len = 0;
				o->skoff = newoff;
				o->off = o->off_data + o->skoff;
				return FFOGG_RSEEK;
			}
			gpos = ffint_ltoh64(h->granulepos) - o->first_sample;
			sk.fr_index = o->cursample;
			sk.fr_samples = gpos - o->cursample;
			sk.fr_size = o->pagesize;
			break;
		}

		break;
	}

sk:
	sk.target = o->seek_sample;
	sk.off = foff - o->off_data;
	sk.lastoff = o->skoff;
	sk.pt = o->seekpt;
	sk.avg_fr_samples = 0;
	sk.flags = FFPCM_SEEK_ALLOW_BINSCH;

	r = ffpcm_seek(&sk);
	if (r == 1) {
		o->buf.len = 0;
		o->skoff = sk.off;
		o->off = o->off_data + sk.off;
		return FFOGG_RSEEK;

	} else if (r == -1) {
		o->err = OGG_ESEEK;
		return FFOGG_RERR;
	}

	o->page_num = 0;
	return FFOGG_RDONE;
}

int ffogg_decode(ffogg *o)
{
	int r;
	ogg_hdr *h;

	for (;;) {

	switch (o->state) {
	default:
		if (FFOGG_RDATA != (r = _ffogg_open(o)))
			return r;
		continue;

	case I_SEEK:
	case I_SEEK2:
		if (FFOGG_RDONE != (r = _ffogg_seek(o)))
			return r;
		o->state = I_BODY;
		continue;

	case I_PAGE:
		if (o->page_gpos != (uint64)-1)
			o->cursample = o->page_gpos - o->first_sample;
		if (o->page_last)
			return FFOGG_RDONE;
		o->state = I_HDR;
		// break

	case I_HDR:
		r = _ffogg_gethdr(o, &h);
		if (r == FFOGG_RWARN)
			return (o->init_done) ? FFOGG_RWARN : FFOGG_RERR;
		else if (r != FFOGG_RDONE)
			return r;
		o->state = I_BODY;
		// break

	case I_BODY:
		r = _ffogg_getbody(o);
		if (r == FFOGG_RERR) {
			if (!o->init_done)
				return FFOGG_RERR;

			o->state = I_HDR;
			return FFOGG_RWARN;

		} else if (r != FFOGG_RDONE)
			return r;

		o->state = I_PKT;

		if (o->pagenum_err) {
			o->pagenum_err = 0;
			o->err = OGG_EPAGENUM;
			return FFOGG_RWARN;
		}
		// break;

	case I_PKT:
		r = ogg_pkt_next(&o->opkt, o->buf.ptr, &o->segoff, &o->bodyoff);
		if (r == -1) {
			o->state = I_PAGE;
			continue;
		} else if (r == -2)
			return ERR(o, OGG_EBIGPKT);

		FFDBG_PRINTLN(10, "packet #%u, size: %u", o->pktno, (int)o->opkt.bytes);

		o->opkt.packetno = o->pktno++;
		o->state = o->nxstate;
		continue;

	case I_DECODE:
		r = vorbis_decode(o->vctx, &o->opkt, &o->pcm);
		if (r < 0) {
			o->state = I_PKT;
			o->err = r;
			return FFOGG_RWARN;
		}
		o->nsamples = r;
		o->pcmlen = o->nsamples * sizeof(float) * o->vinfo.channels;
		o->state = I_DATA;
		return FFOGG_RDATA;

	case I_DATA:
		o->cursample += o->nsamples;
		o->state = I_PKT;
		break;
	}
	}
	//unreachable
}

void ffogg_set_asis(ffogg *o, uint64 from_sample)
{
	o->seek_sample = (uint64)-1;
	if (from_sample != (uint64)-1)
		ffogg_seek(o, from_sample);
	o->state = I_ASIS_SEEKHDR;
}

int ffogg_readasis(ffogg *o)
{
	int r;

	for (;;) {
	switch (o->state) {

	case I_ASIS_SEEKHDR:
		o->state = I_HDR;
		o->off = 0;
		return FFOGG_RSEEK;

	case I_PAGE:
		if (o->page_gpos != (uint64)-1)
			o->cursample = o->page_gpos - o->first_sample;
		if (o->page_last)
			return FFOGG_RDONE;
		o->state = I_HDR;
		// break

	case I_HDR: {
		ogg_hdr *h;
		r = _ffogg_gethdr(o, &h);
		if (r != FFOGG_RDONE)
			return r;
		o->state = I_BODY;
		// break
	}

	case I_BODY:
		r = _ffogg_getbody(o);
		if (r == FFOGG_RERR) {
			o->state = I_HDR;
			return FFOGG_RWARN;

		} else if (r != FFOGG_RDONE)
			return r;

		o->state = I_PAGE;
		if (o->off == o->off_data && o->seek_sample != (uint64)-1) {
			o->state = I_SEEK;
		}
		return FFOGG_RPAGE;

	case I_SEEK:
	case I_SEEK2:
		if (FFOGG_RDONE != (r = _ffogg_seek(o)))
			return r;
		o->state = I_BODY;
		continue;
	}
	}
}


void ffogg_enc_init(ffogg_enc *o)
{
	ffmem_tzero(o);
	o->min_tagsize = 1000;
	o->max_pagesize = 8 * 1024;

	if (NULL == ffarr_alloc(&o->vtag.out, 4096))
		return;
	o->vtag.out.len = sizeof(struct vorbis_hdr);
	const char *vendor = vorbis_vendor();
	ffvorbtag_add(&o->vtag, NULL, vendor, ffsz_len(vendor));
}

void ffogg_enc_close(ffogg_enc *o)
{
	ffvorbtag_destroy(&o->vtag);
	ffarr_free(&o->buf);
	FF_SAFECLOSE(o->vctx, NULL, vorbis_encode_free);
}

int ffogg_create(ffogg_enc *o, ffpcm *pcm, int quality, uint serialno)
{
	o->vinfo.channels = pcm->channels;
	o->vinfo.rate = pcm->sample_rate;
	o->vinfo.quality = quality;
	o->page.serial = serialno;
	return 0;
}

/** Get complete packet with Vorbis comments and padding. */
static int _ffogg_pkt_vorbtag(ffogg_enc *o, ogg_packet *pkt)
{
	ffarr *v = &o->vtag.out;
	uint taglen = v->len - sizeof(struct vorbis_hdr);
	uint npadding = (taglen < o->min_tagsize) ? o->min_tagsize - taglen : 0;
	if (NULL == ffarr_grow(v, 1 + npadding, 0)) { //allocate space for "framing bit" and padding
		o->err = OGG_ESYS;
		return OGG_ESYS;
	}

	if (npadding != 0)
		ffmem_zero(v->ptr + v->len + 1, npadding);

	pkt->packet = (void*)v->ptr;
	pkt->bytes = ogg_tag_write(v->ptr, taglen) + npadding;
	return 0;
}

static int _ffogg_enc_hdr(ffogg_enc *o)
{
	int r;
	ogg_packet pkt[3];

	vorbis_encode_params params = {0};
	params.channels = o->vinfo.channels;
	params.rate = o->vinfo.rate;
	params.quality = (float)o->vinfo.quality / 100;
	if (0 != (r = vorbis_encode_create(&o->vctx, &params, &pkt[0], &pkt[2])))
		return r;

	ffvorbtag_fin(&o->vtag);
	if (0 != (r = _ffogg_pkt_vorbtag(o, &pkt[1])))
		return r;

	o->max_pagesize = ffmin(o->max_pagesize, OGG_MAXPAGE - OGG_MAXHDR);
	uint sz = ffmax(OGG_MAXHDR + o->max_pagesize, OGG_MAXHDR + (uint)pkt[0].bytes + OGG_MAXHDR + (uint)pkt[1].bytes + (uint)pkt[2].bytes);
	if (NULL == ffarr_alloc(&o->buf, sz)) {
		o->err = OGG_ESYS;
		goto err;
	}

	ffstr s;
	if (0 != (r = ogg_hdr_write(&o->page, o->buf.ptr, &s, &pkt[0], &pkt[1], &pkt[2]))) {
		o->err = r;
		goto err;
	}
	o->data = s.ptr;
	o->datalen = s.len;

	o->err = 0;

err:
	ffvorbtag_destroy(&o->vtag);
	return o->err;
}

int ffogg_encode(ffogg_enc *o)
{
	enum { I_HDRFLUSH, I_INPUT, I_ENCODE, ENC_PKT, ENC_DONE };
	int r, n = 0;

	for (;;) {

	switch (o->state) {
	case I_HDRFLUSH:
		if (0 != (r = _ffogg_enc_hdr(o)))
			return r;
		o->state = I_INPUT;
		return FFOGG_RDATA;

	case I_INPUT:
		n = (uint)(o->pcmlen / (sizeof(float) * o->vinfo.channels));
		o->pcmlen = 0;
		o->state = I_ENCODE;
		// break

	case I_ENCODE:
		r = vorbis_encode(o->vctx, o->pcm, n, &o->opkt);
		if (r < 0) {
			o->err = r;
			return FFOGG_RERR;
		} else if (r == 0) {
			if (!o->fin) {
				o->state = I_INPUT;
				return FFOGG_RMORE;
			}
			n = -1;
			continue;
		}

		if (o->page.size + o->opkt.bytes > o->max_pagesize) {
			ffstr s;
			ogg_page_write(&o->page, o->buf.ptr, o->granpos, 0, &s);
			o->data = s.ptr;
			o->datalen = s.len;
			o->state = ENC_PKT;
			return FFOGG_RDATA;
		}
		o->state = ENC_PKT;
		// break

	case ENC_PKT:
		if (0 != (r = ogg_pkt_write(&o->page, o->buf.ptr, (void*)o->opkt.packet, o->opkt.bytes))) {
			o->err = r;
			return FFOGG_RERR;
		}
		o->granpos = o->opkt.granulepos;

		if (o->opkt.e_o_s) {
			ffstr s;
			ogg_page_write(&o->page, o->buf.ptr, o->granpos, OGG_FLAST, &s);
			o->data = s.ptr;
			o->datalen = s.len;
			o->state = ENC_DONE;
			return FFOGG_RDATA;
		}
		o->state = I_ENCODE;
		n = 0;
		continue;

	case ENC_DONE:
		return FFOGG_RDONE;
	}
	}

	//unreachable
}
