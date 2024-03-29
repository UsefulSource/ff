/**
Copyright (c) 2017 Simon Zolin
*/

/*
File structure:
SIG_HDR
STREAM_PACKED(FILE_DATA...)...
META_PACKED(META META_HDR) | META

Meta data structure:
STREAM { offset packed_size } []
FOLDER {
	CODER { id properties stream input_coders[] } []
} []
CODER { unpacked_size } []
FOLDER {
	datafile_unpacked_size[]
	datafile_crc[]
} []
empty_file_index[]
file_name[]
file_mtime[]
file_attr[]
*/


#pragma once

#include <FF/pack/7z-def.h>
#include <FF/array.h>
#include <FF/time.h>


struct z7_folder;
struct z7_filter;
struct z7_block;

typedef struct ff7z {
	uint st;
	uint nxst;
	int err;
	uint64 off;
	uint hdr_packed :1;

	uint gst;
	size_t gsize;
	ffarr gbuf;
	ffstr gdata;

	struct z7_block *blks;
	uint iblk;

	ffarr folders; //z7_folder[] = (folder)... (folder for empty files)
	struct z7_folder *cur_folder;

	struct z7_filter *filters;
	uint nfilters;
	uint ifilter;
	uint crc;

	ffarr buf;
	ffstr input;
	ffstr out;
} ff7z;

FF_EXTN const char* ff7z_errstr(ff7z *z);

enum FF7Z_R {
	FF7Z_ERR = 1,
	FF7Z_DATA,
	FF7Z_MORE,
	FF7Z_SEEK,
	FF7Z_FILEHDR,
	FF7Z_FILEDONE,
};

FF_EXTN void ff7z_open(ff7z *z);
FF_EXTN void ff7z_close(ff7z *z);
FF_EXTN int ff7z_read(ff7z *z);

/**
Return NULL if no next file. */
FF_EXTN const ff7zfile* ff7z_nextfile(ff7z *z);

#define ff7z_input(z, data, len) \
	ffstr_set(&(z)->input, data, len)

#define ff7z_offset(z)  ((z)->off)
