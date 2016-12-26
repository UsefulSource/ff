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

	ffarr accels; //ffui_wnd_hotkey[]
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
		ffarr sb_parts;
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

		union ffui_anyctl actl;
	};
	char *errstr;
	char *wndname;
	uint showcmd;
	uint vis :1;
};

FF_EXTN void ffui_ldr_init(ffui_loader *g);

FF_EXTN void ffui_ldr_fin(ffui_loader *g);

#define ffui_ldr_errstr(g)  ((g)->errstr)

/** Load GUI from file. */
FF_EXTN int ffui_ldr_loadfile(ffui_loader *g, const char *fn);

FF_EXTN void ffui_ldr_loadconf(ffui_loader *g, const char *fn);


typedef struct ffui_ldr_ctl ffui_ldr_ctl;
struct ffui_ldr_ctl {
	const char *name;
	uint flags; //=offset
	const ffui_ldr_ctl *children;
};

#define FFUI_LDR_CTL(struct_name, ctl) \
	{ #ctl, FFOFF(struct_name, ctl), NULL }

#define FFUI_LDR_CTL3(struct_name, ctl, children) \
	{ #ctl, FFOFF(struct_name, ctl), children }

/** Find control by its name in structured hierarchy.
@name: e.g. "window.control" */
FF_EXTN void* ffui_ldr_findctl(const ffui_ldr_ctl *ctx, void *ctl, const ffstr *name);


typedef struct ffui_loaderw {
	ffui_ldr_getctl_t getctl;
	void *udata;

	ffstr3 buf;
} ffui_loaderw;

FF_EXTN void ffui_ldrw_fin(ffui_loaderw *ldr);

FF_EXTN void ffui_ldr_setv(ffui_loaderw *ldr, const char *const *names, size_t n, uint flags);

enum FFUI_LDR_F {
	FFUI_LDR_FSTR = 1,
};

FF_EXTN void ffui_ldr_set(ffui_loaderw *ldr, const char *name, const char *val, size_t len, uint flags);

FF_EXTN int ffui_ldr_write(ffui_loaderw *ldr, const char *fn);
