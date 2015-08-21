/** GUI loader.
Copyright (c) 2014 Simon Zolin
*/

#pragma once

#include <FF/gui/winapi.h>
#include <FF/data/parse.h>


typedef struct ffui_loader ffui_loader;

typedef struct {
	HICON h;
	ffstr fn;
	int idx;
	ffui_loader *ldr;
	uint small :1;
} _ffui_ldr_icon_t;

typedef void* (*ffui_ldr_getctl_t)(void *udata, const ffstr *name);
typedef int (*ffui_ldr_getcmd_t)(void *udata, const ffstr *name);

struct ffui_loader {
	ffui_ldr_getctl_t getctl;
	ffui_ldr_getcmd_t getcmd;
	void *udata;
	ffpars_ctx ctx;

	struct { FFARR(ACCEL) } accels;
	ffstr path;
	_ffui_ldr_icon_t ico;
	ffui_wnd *wnd;
	int ir;
	union {
		ffui_pos r;
		struct {
			ffui_menuitem mi;
			uint iaccel :1;
		} menuitem;
		struct {
			_ffui_ldr_icon_t ico;
			uint show :1;
		} tr;
		ffui_font fnt;
		ffui_viewcol vicol;
	};
	union {
		ffui_ctl *ctl;
		ffui_btn *btn;
		ffui_edit *ed;
		ffui_paned *paned;
		ffui_trkbar *trkbar;
		ffui_view *vi;
		ffui_dialog *dlg;
		ffui_menu *menu;
		ffui_trayicon *tray;
	};
	char *errstr;
	uint vis :1;
};

FF_EXTN void ffui_ldr_init(ffui_loader *g);

FF_EXTN void ffui_ldr_fin(ffui_loader *g);

#define ffui_ldr_errstr(g)  ((g)->errstr)

/** Load GUI from file. */
FF_EXTN int ffui_ldr_loadfile(ffui_loader *g, const char *fn);