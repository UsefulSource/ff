/** GUI based on GTK+.
Copyright (c) 2019 Simon Zolin
*/

#pragma once

#include <FF/array.h>
#include <FF/data/parse.h>
#include <gtk/gtk.h>


static inline void ffui_init()
{
	int argc = 0;
	char **argv = NULL;
	gtk_init(&argc, &argv);
}

#define ffui_uninit()


// ICON
// CONTROL
// MENU
// BUTTON
// LABEL
// EDITBOX
// TRACKBAR
// TAB
// LISTVIEW COLUMN
// LISTVIEW ITEM
// LISTVIEW
// STATUSBAR
// TRAYICON
// DIALOG
// WINDOW
// MESSAGE LOOP


typedef struct ffui_pos {
	int x, y
		, cx, cy;
} ffui_pos;


// ICON
typedef struct ffui_icon {
	GdkPixbuf *ico;
} ffui_icon;

static inline int ffui_icon_load(ffui_icon *ico, const char *filename)
{
	ico->ico = gdk_pixbuf_new_from_file(filename, NULL);
	return (ico->ico == NULL);
}

static inline int ffui_icon_loadimg(ffui_icon *ico, const char *filename, uint cx, uint cy, uint flags)
{
	ico->ico = gdk_pixbuf_new_from_file_at_scale(filename, cx, cy, 0, NULL);
	return (ico->ico == NULL);
}



// CONTROL

enum FFUI_UID {
	FFUI_UID_WINDOW = 1,
};

typedef struct ffui_wnd ffui_wnd;

#define _FFUI_CTL_MEMBERS \
	GtkWidget *h; \
	enum FFUI_UID uid; \
	ffui_wnd *wnd;

typedef struct ffui_ctl {
	_FFUI_CTL_MEMBERS
} ffui_ctl;

static inline void ffui_show(void *c, uint show)
{
	if (show)
		gtk_widget_show_all(((ffui_ctl*)c)->h);
	else
		gtk_widget_hide(((ffui_ctl*)c)->h);
}

#define ffui_ctl_destroy(c)  gtk_widget_destroy(((ffui_ctl*)c)->h)

#define ffui_setposrect(ctl, r) \
	gtk_widget_set_size_request((ctl)->h, (r)->cx, (r)->cy)


// MENU
typedef struct ffui_menu {
	_FFUI_CTL_MEMBERS
} ffui_menu;

static inline int ffui_menu_createmain(ffui_menu *m)
{
	m->h = gtk_menu_bar_new();
	return (m->h == NULL);
}

static inline int ffui_menu_create(ffui_menu *m)
{
	m->h = gtk_menu_new();
	return (m->h == NULL);
}

#define ffui_menu_new(text)  gtk_menu_item_new_with_mnemonic(text)
#define ffui_menu_newsep()  gtk_separator_menu_item_new()

FF_EXTN void ffui_menu_setsubmenu(void *mi, ffui_menu *sub, ffui_wnd *wnd);

FF_EXTN void ffui_menu_setcmd(void *mi, uint id);

static inline void ffui_menu_ins(ffui_menu *m, void *mi, int pos)
{
	gtk_menu_shell_insert((void*)m->h, mi, pos);
}


// BUTTON
typedef struct ffui_btn {
	_FFUI_CTL_MEMBERS
	uint action_id;
} ffui_btn;

FF_EXTN int ffui_btn_create(ffui_btn *b, ffui_wnd *parent);
static inline void ffui_btn_settextz(ffui_btn *b, const char *textz)
{
	gtk_button_set_label(GTK_BUTTON(b->h), textz);
}
static inline void ffui_btn_settextstr(ffui_btn *b, const ffstr *text)
{
	char *sz = ffsz_alcopystr(text);
	gtk_button_set_label(GTK_BUTTON(b->h), sz);
	ffmem_free(sz);
}

static inline void ffui_btn_seticon(ffui_btn *b, ffui_icon *ico)
{
	GtkWidget *img = gtk_image_new();
	gtk_image_set_from_pixbuf(GTK_IMAGE(img), ico->ico);
	gtk_button_set_image(GTK_BUTTON(b->h), img);
}


// LABEL
typedef struct ffui_label {
	_FFUI_CTL_MEMBERS
} ffui_label;

FF_EXTN int ffui_lbl_create(ffui_label *l, ffui_wnd *parent);

#define ffui_lbl_settextz(l, text)  gtk_label_set_text(GTK_LABEL((l)->h), text)
static inline void ffui_lbl_settext(ffui_label *l, const char *text, size_t len)
{
	char *sz = ffsz_alcopy(text, len);
	ffui_lbl_settextz(l, sz);
	ffmem_free(sz);
}
#define ffui_lbl_settextstr(l, str)  ffui_lbl_settext(l, str->ptr, str->len)


// EDITBOX
typedef struct ffui_edit {
	_FFUI_CTL_MEMBERS
} ffui_edit;

FF_EXTN int ffui_edit_create(ffui_edit *e, ffui_wnd *parent);

#define ffui_edit_settextz(e, text)  gtk_entry_set_text(GTK_ENTRY((e)->h), text)
static inline void ffui_edit_settext(ffui_edit *e, const char *text, size_t len)
{
	char *sz = ffsz_alcopy(text, len);
	ffui_edit_settextz(e, sz);
	ffmem_free(sz);
}

static inline void ffui_edit_textstr(ffui_edit *e, ffstr *s)
{
	const gchar *sz = gtk_entry_get_text(GTK_ENTRY(e->h));
	ffstr_alcopyz(s, sz);
}

#define ffui_edit_sel(e, start, end) \
	gtk_editable_select_region(GTK_EDITABLE((e)->h), start, end);


// TRACKBAR
typedef struct ffui_trkbar {
	_FFUI_CTL_MEMBERS
	uint scroll_id;
} ffui_trkbar;

FF_EXTN int ffui_trk_create(ffui_trkbar *t, ffui_wnd *parent);

FF_EXTN void ffui_trk_setrange(ffui_trkbar *t, uint max);

FF_EXTN void ffui_trk_set(ffui_trkbar *t, uint val);

#define ffui_trk_val(t)  gtk_range_get_value(GTK_RANGE((t)->h))


// TAB
typedef struct ffui_tab {
	_FFUI_CTL_MEMBERS
	uint change_id;
	uint changed_index;
} ffui_tab;

FF_EXTN int ffui_tab_create(ffui_tab *t, ffui_wnd *parent);

FF_EXTN void ffui_tab_ins(ffui_tab *t, int idx, const char *textz);

#define ffui_tab_append(t, textz)  ffui_tab_ins(t, -1, textz)

#define ffui_tab_del(t, idx)  gtk_notebook_remove_page(GTK_NOTEBOOK((t)->h), idx)

#define ffui_tab_count(t)  gtk_notebook_get_n_pages(GTK_NOTEBOOK((t)->h))

#define ffui_tab_active(t)  gtk_notebook_get_current_page(GTK_NOTEBOOK((t)->h))
#define ffui_tab_setactive(t, idx)  gtk_notebook_set_current_page(GTK_NOTEBOOK((t)->h), idx)


// LISTVIEW COLUMN
typedef struct ffui_view ffui_view;
typedef struct ffui_viewcol {
	char *text;
	uint width;
} ffui_viewcol;

#define ffui_viewcol_reset(vc)  ffmem_free0((vc)->text)

static inline void ffui_viewcol_settext(ffui_viewcol *vc, const char *text, size_t len)
{
	vc->text = ffsz_alcopy(text, len);
}

#define ffui_viewcol_setwidth(vc, w)  (vc)->width = (w)
#define ffui_viewcol_width(vc)  ((vc)->width)

FF_EXTN void ffui_view_inscol(ffui_view *v, int pos, ffui_viewcol *vc);
FF_EXTN void ffui_view_setcol(ffui_view *v, int pos, ffui_viewcol *vc);
FF_EXTN void ffui_view_col(ffui_view *v, int pos, ffui_viewcol *vc);

/** Get the number of columns. */
#define ffui_view_ncols(v) \
	gtk_tree_model_get_n_columns(GTK_TREE_MODEL((v)->store))


// LISTVIEW ITEM
typedef struct ffui_viewitem {
	char *text;
	int idx;
	uint text_alloc :1;
} ffui_viewitem;

static inline void ffui_view_iteminit(ffui_viewitem *it)
{
	ffmem_tzero(it);
}

static inline void ffui_view_itemreset(ffui_viewitem *it)
{
	if (it->text_alloc) {
		ffmem_free0(it->text);
		it->text_alloc = 0;
	}
}

#define ffui_view_setindex(it, i)  (it)->idx = (i)

#define ffui_view_settextz(it, sz)  (it)->text = (sz)
static inline void ffui_view_settext(ffui_viewitem *it, const char *text, size_t len)
{
	it->text = ffsz_alcopy(text, len);
	it->text_alloc = 1;
}
#define ffui_view_settextstr(it, str)  ffui_view_settext(it, (str)->ptr, (str)->len)


// LISTVIEW
struct ffui_view_disp {
	uint idx;
	uint sub;
	ffstr text;
};

struct ffui_view {
	_FFUI_CTL_MEMBERS
	GtkTreeModel *store;
	GtkCellRenderer *rend;
	uint dblclick_id;
	uint dropfile_id;
	uint dispinfo_id;

	union {
	GtkTreePath *path;
	ffstr drop_data;
	struct ffui_view_disp disp;
	};
};

FF_EXTN int ffui_view_create(ffui_view *v, ffui_wnd *parent);

enum FFUI_VIEW_STYLE {
	FFUI_VIEW_GRIDLINES = 1,
	FFUI_VIEW_MULTI_SELECT = 2,
};
FF_EXTN void ffui_view_style(ffui_view *v, uint flags, uint set);

#define ffui_view_nitems(v)  gtk_tree_model_iter_n_children((void*)(v)->store, NULL)

FF_EXTN void ffui_view_setdata(ffui_view *v, uint first, int delta);

static inline void ffui_view_clear(ffui_view *v)
{
	if (v->store != NULL)
		gtk_list_store_clear(GTK_LIST_STORE(v->store));
}

#define ffui_view_selall(v)  gtk_tree_selection_select_all(gtk_tree_view_get_selection(GTK_TREE_VIEW((v)->h)))
#define ffui_view_unselall(v)  gtk_tree_selection_unselect_all(gtk_tree_view_get_selection(GTK_TREE_VIEW((v)->h)))

/** Get array of selected items.
Return ffarr4* (uint[]).  Free with ffui_view_sel_free(). */
FF_EXTN ffarr4* ffui_view_getsel(ffui_view *v);

static inline void ffui_view_sel_free(ffarr4 *sel)
{
	ffarr_free(sel);
	ffmem_free(sel);
}

/** Get next selected item. */
static inline int ffui_view_selnext(ffui_view *v, ffarr4 *sel)
{
	if (sel->off == sel->len)
		return -1;
	return *ffarr_itemT(sel, sel->off++, uint);
}

FF_EXTN void ffui_view_dragdrop(ffui_view *v, uint action_id);

/**
Note: must be called only from wnd.on_action(). */
static inline int ffui_view_focused(ffui_view *v)
{
	int *i = gtk_tree_path_get_indices(v->path);
	return i[0];
}

FF_EXTN void ffui_view_ins(ffui_view *v, int pos, ffui_viewitem *it);

#define ffui_view_append(v, it)  ffui_view_ins(v, -1, it)

FF_EXTN void ffui_view_set(ffui_view *v, int sub, ffui_viewitem *it);

FF_EXTN void ffui_view_rm(ffui_view *v, ffui_viewitem *it);


// STATUSBAR

FF_EXTN int ffui_stbar_create(ffui_ctl *sb, ffui_wnd *parent);

static inline void ffui_stbar_settextz(ffui_ctl *sb, const char *text)
{
	gtk_statusbar_push(GTK_STATUSBAR(sb->h), gtk_statusbar_get_context_id(GTK_STATUSBAR(sb->h), "a"), text);
}


// TRAYICON
typedef struct ffui_trayicon {
	_FFUI_CTL_MEMBERS
	uint lclick_id;
} ffui_trayicon;

FF_EXTN int ffui_tray_create(ffui_trayicon *t, ffui_wnd *wnd);

#define ffui_tray_hasicon(t) \
	(0 != gtk_status_icon_get_size(GTK_STATUS_ICON((t)->h)))

static inline void ffui_tray_seticon(ffui_trayicon *t, ffui_icon *ico)
{
	gtk_status_icon_set_from_pixbuf(GTK_STATUS_ICON(t->h), ico->ico);
}

#define ffui_tray_show(t, show)  gtk_status_icon_set_visible(GTK_STATUS_ICON((t)->h), show)


// DIALOG

typedef struct ffui_dialog {
	char *title;
	char *name;
	GSList *names;
	GSList *curname;
	uint multisel :1;
} ffui_dialog;

static inline void ffui_dlg_init(ffui_dialog *d)
{
}

static inline void ffui_dlg_destroy(ffui_dialog *d)
{
	g_slist_free_full(d->names, g_free);  d->names = NULL;
	ffmem_free0(d->title);
	g_free(d->name); d->name = NULL;
}

static inline void ffui_dlg_titlez(ffui_dialog *d, const char *sz)
{
	d->title = ffsz_alcopyz(sz);
}

#define ffui_dlg_multisel(d, val)  ((d)->multisel = (val))

/** Get the next file name (for a dialog with multiselect). */
FF_EXTN char* ffui_dlg_nextname(ffui_dialog *d);

FF_EXTN char* ffui_dlg_open(ffui_dialog *d, ffui_wnd *parent);

FF_EXTN char* ffui_dlg_save(ffui_dialog *d, ffui_wnd *parent, const char *fn, size_t fnlen);


// WINDOW
struct ffui_wnd {
	GtkWindow *h;
	enum FFUI_UID uid;
	GtkWidget *vbox;

	void (*on_create)(ffui_wnd *wnd);
	void (*on_destroy)(ffui_wnd *wnd);
	void (*on_action)(ffui_wnd *wnd, int id);

	uint onclose_id;
	uint hide_on_close :1;
};

static inline int ffui_wnd_initstyle()
{
	return 0;
}

FF_EXTN int ffui_wnd_create(ffui_wnd *w);

#define ffui_wnd_close(w)  gtk_window_close((w)->h)

#define ffui_wnd_destroy(w)  gtk_widget_destroy(GTK_WIDGET((w)->h))

static inline void ffui_wnd_setmenu(ffui_wnd *w, ffui_menu *m)
{
	m->wnd = w;
	gtk_box_pack_start(GTK_BOX(w->vbox), m->h, /*expand=*/0, /*fill=*/0, /*padding=*/0);
}

typedef uint ffui_hotkey;

#define ffui_hotkey_mod(hk)  ((hk) >> 16)
#define ffui_hotkey_key(hk)  ((hk) & 0xffff)

/** Parse hotkey string, e.g. "Ctrl+Alt+Shift+Q".
Return: low-word: char key or vkey, hi-word: control flags;  0 on error. */
FF_EXTN ffui_hotkey ffui_hotkey_parse(const char *s, size_t len);

typedef struct ffui_wnd_hotkey {
	ffui_hotkey hk;
	GtkWidget *h;
} ffui_wnd_hotkey;

/** Set hotkey table. */
FF_EXTN int ffui_wnd_hotkeys(ffui_wnd *w, const ffui_wnd_hotkey *hotkeys, size_t n);

#define ffui_wnd_settextz(w, text)  gtk_window_set_title((w)->h, text)
static inline void ffui_wnd_settextstr(ffui_wnd *w, const ffstr *str)
{
	char *sz = ffsz_alcopystr(str);
	ffui_wnd_settextz(w, sz);
	ffmem_free(sz);
}

#define ffui_wnd_seticon(w, icon)  gtk_window_set_icon((w)->h, (icon)->ico)

static inline void ffui_wnd_pos(ffui_wnd *w, ffui_pos *pos)
{
	int x, y, ww, h;
	gtk_window_get_position(w->h, &x, &y);
	gtk_window_get_size(w->h, &ww, &h);
	pos->x = x;
	pos->y = y;
	pos->cx = ww;
	pos->cy = h;
}

static inline void ffui_wnd_setplacement(ffui_wnd *w, uint showcmd, const ffui_pos *pos)
{
	gtk_window_move(w->h, pos->x, pos->y);
	gtk_window_set_default_size(w->h, pos->cx, pos->cy);
}


// MESSAGE LOOP
#define ffui_run()  gtk_main()

#define ffui_quitloop()  gtk_main_quit()

typedef void (*ffui_handler)(void *param);

enum {
	FFUI_POST_WAIT = 1 << 31,
};

/**
flags: FFUI_POST_WAIT */
FF_EXTN void ffui_thd_post(ffui_handler func, void *udata, uint flags);

enum FFUI_MSG {
	FFUI_QUITLOOP,
	FFUI_LBL_SETTEXT,
	FFUI_WND_SETTEXT,
	FFUI_VIEW_RM,
	FFUI_VIEW_CLEAR,
	FFUI_VIEW_GETSEL,
	FFUI_VIEW_SETDATA,
	FFUI_TRK_SETRANGE,
	FFUI_TRK_SET,
	FFUI_TAB_INS,
	FFUI_STBAR_SETTEXT,
};

/**
id: enum FFUI_MSG */
FF_EXTN void ffui_post(void *ctl, uint id, void *udata);
FF_EXTN size_t ffui_send(void *ctl, uint id, void *udata);

#define ffui_post_quitloop()  ffui_post(NULL, FFUI_QUITLOOP, NULL)
#define ffui_send_lbl_settext(ctl, sz)  ffui_send(ctl, FFUI_LBL_SETTEXT, sz)
#define ffui_send_wnd_settext(ctl, sz)  ffui_send(ctl, FFUI_WND_SETTEXT, sz)
#define ffui_send_view_rm(ctl, it)  ffui_send(ctl, FFUI_VIEW_RM, it)
#define ffui_post_view_clear(ctl)  ffui_send(ctl, FFUI_VIEW_CLEAR, NULL)

/** See ffui_view_getsel().
Return ffarr4* (uint[]) */
#define ffui_send_view_getsel(v)  ffui_send(v, FFUI_VIEW_GETSEL, NULL)
static inline void ffui_send_view_setdata(ffui_view *v, uint first, int delta)
{
	size_t p = ((first & 0xffff) << 16) | (delta & 0xffff);
	ffui_send(v, FFUI_VIEW_SETDATA, (void*)p);
}
#define ffui_post_trk_setrange(ctl, range)  ffui_post(ctl, FFUI_TRK_SETRANGE, (void*)(size_t)range)
#define ffui_post_trk_set(ctl, val)  ffui_post(ctl, FFUI_TRK_SET, (void*)(size_t)val)
#define ffui_send_tab_ins(ctl, textz)  ffui_send(ctl, FFUI_TAB_INS, textz)

#define ffui_send_stbar_settextz(sb, sz)  ffui_send(sb, FFUI_STBAR_SETTEXT, sz)


typedef void* (*ffui_ldr_getctl_t)(void *udata, const ffstr *name);

/** Get command ID by its name.
Return 0 if not found. */
typedef int (*ffui_ldr_getcmd_t)(void *udata, const ffstr *name);

typedef struct {
	char *fn;
} _ffui_ldr_icon;

typedef struct ffui_loader {
	ffui_ldr_getctl_t getctl;
	ffui_ldr_getcmd_t getcmd;
	void *udata;
	ffpars_ctx ctx;
	ffstr path;
	ffarr accels; //ffui_wnd_hotkey[]

	_ffui_ldr_icon ico;
	_ffui_ldr_icon ico_ctl;
	ffui_pos r;
	ffui_wnd *wnd;
	ffui_viewcol vicol;
	ffui_menu *menu;
	void *mi;
	GtkWidget *hbox;
	union {
		ffui_ctl *ctl;
		ffui_label *lbl;
		ffui_btn *btn;
		ffui_edit *edit;
		ffui_trkbar *trkbar;
		ffui_tab *tab;
		ffui_view *vi;
		ffui_trayicon *trayicon;
		ffui_dialog *dlg;
	};

	char *errstr;
	char *wndname;

	union {
	uint flags;
	struct {
		uint f_horiz :1;
	};
	};
} ffui_loader;

/** Initialize GUI loader.
getctl: get a pointer to a UI element by its name.
 Most of the time you just need to call ffui_ldr_findctl() from it.
getcmd: get command ID by its name
udata: user data */
FF_EXTN void ffui_ldr_init2(ffui_loader *g, ffui_ldr_getctl_t getctl, ffui_ldr_getcmd_t getcmd, void *udata);

static inline void ffui_ldr_fin(ffui_loader *g)
{
	ffmem_free(g->ico_ctl.fn);
	ffmem_free(g->ico.fn);
	ffarr_free(&g->accels);
	ffmem_safefree(g->errstr);
	ffmem_safefree0(g->wndname);
}

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

#define FFUI_LDR_CTL_END  {NULL, 0, NULL}

/** Find control by its name in structured hierarchy.
@name: e.g. "window.control" */
FF_EXTN void* ffui_ldr_findctl(const ffui_ldr_ctl *ctx, void *ctl, const ffstr *name);


#include <FF/data/conf.h>

typedef struct ffui_loaderw {
	ffui_ldr_getctl_t getctl;
	void *udata;

	ffconfw confw;
	uint fin :1;
} ffui_loaderw;

FF_EXTN void ffui_ldrw_fin(ffui_loaderw *ldr);

FF_EXTN void ffui_ldr_setv(ffui_loaderw *ldr, const char *const *names, size_t n, uint flags);

enum FFUI_LDR_F {
	FFUI_LDR_FSTR = 1,
};

FF_EXTN void ffui_ldr_set(ffui_loaderw *ldr, const char *name, const char *val, size_t len, uint flags);

FF_EXTN int ffui_ldr_write(ffui_loaderw *ldr, const char *fn);
