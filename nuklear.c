#include <u.h>
#include <libc.h>
#include <draw.h>
#include <keyboard.h>
#include <mouse.h>

#define NK_IMPLEMENTATION
#include "nuklear.h"

enum {
	COLORGC	= 1000,
};

typedef struct Color Color;
struct Color {
	ulong gen;
	ulong color;
	Image *image;
	Color *next;
};

static Color *ctab[257];
static ulong cgen;

static void
colorgc(void)
{
	int i;
	Color *c, *tmp, **cc;

	if((cgen++ % COLORGC) != 0)
		return;

	for(i = 0; i < nelem(ctab); i++){
		cc = &ctab[i];
		while(*cc != nil){
			if(cgen - (*cc)->gen > COLORGC){
				c = *cc;
				*cc = c->next;
				freeimage(c->image);
				free(c);
			} else {
				cc = &(*cc)->next;
			}
		}
	}
}

static Image*
colorget(struct nk_color nkcolor)
{
	Color *c, **cc;
	ulong color;

	color = nkcolor.r << 24 | nkcolor.g << 16 | nkcolor.b << 8 | nkcolor.a;

	for(cc = &ctab[color % nelem(ctab)]; c = *cc, c != nil; cc = &c->next){
		if(c->color == color){
			c->gen = cgen;
			return c->image;
		}
	}

	c = malloc(sizeof(Color));
	if(c == nil)
		display->error(display, "cannot allocate color memory");
	c->gen = cgen;
	c->color = color;
	c->image = allocimage(display, Rect(0,0,1,1), screen->chan, 1, color);
	if(c->image == nil)
		display->error(display, "cannot allocate color image");
	c->next = nil;
	*cc = c;
	return c->image;
}

static float
nk_plan9_fontwidth(nk_handle handle, float height, const char *text, int len)
{
	Font *f;
	float width;

	USED(height);

	f = handle.ptr;
	assert(f != nil);
	width = stringnwidth(f, text, len);

	return width;
}

void
nk_plan9_makefont(struct nk_user_font *nkfont, Font *f)
{
	nkfont->userdata.ptr = f;
	nkfont->height = f->height;
	nkfont->width = nk_plan9_fontwidth;
}

NK_API void
nk_plan9_render(struct nk_context *ctx, Image *image)
{
	const struct nk_command *cmd;
	Point p;
	Image *color, *color2;

	p = image->r.min;

	nk_foreach(cmd, ctx){
		switch(cmd->type){
		case NK_COMMAND_NOP:
			break;

		case NK_COMMAND_SCISSOR:
			{
				const struct nk_command_scissor *s = (const struct nk_command_scissor*)cmd;
				replclipr(image, 0, rectaddpt(Rect(s->x, s->y, s->x+s->w, s->y+s->h), p));
        	}
			break;

		case NK_COMMAND_LINE:
			{
				const struct nk_command_line *l = (const struct nk_command_line*)cmd;
				color = colorget(l->color);
				line(image, addpt(p, Pt(l->begin.x, l->begin.y)), addpt(p, Pt(l->end.x, l->end.y)), 0, 0,
					l->line_thickness, color, ZP);
			}
			break;

		case NK_COMMAND_RECT:
			{
				const struct nk_command_rect *r = (const struct nk_command_rect*)cmd;
				color = colorget(r->color);
				border(image, rectaddpt(Rect(r->x, r->y, r->x+r->w, r->y+r->h), p),
					r->line_thickness, color, ZP);
			}
			break;

		case NK_COMMAND_RECT_FILLED:
			{
				const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
				color = colorget(r->color);
				draw(image, rectaddpt(Rect(r->x, r->y, r->x+r->w, r->y+r->h), p), color, nil, ZP);
			}
			break;

		case NK_COMMAND_CIRCLE:
			{
				const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
				Point mid;
				color = colorget(c->color);
				mid = Pt(c->w/2, c->h/2);
				ellipse(image, addpt(addpt(Pt(c->x, c->y), mid), p), mid.x, mid.y, 1, color, ZP);
			}
			break;

		case NK_COMMAND_CIRCLE_FILLED:
			{
				const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
				Point mid;
				color = colorget(c->color);
				mid = Pt(c->w/2, c->h/2);
				fillellipse(image, addpt(addpt(Pt(c->x, c->y), mid), p), mid.x, mid.y, color, ZP);
			}
			break;

		case NK_COMMAND_TRIANGLE:
			{
				const struct nk_command_triangle *t = (const struct nk_command_triangle*)cmd;
				Point pts[4] = {
					addpt(Pt(t->a.x, t->a.y), p),
					addpt(Pt(t->b.x, t->b.y), p),
					addpt(Pt(t->c.x, t->c.y), p),
					addpt(Pt(t->a.x, t->a.y), p),
				};
				color = colorget(t->color);
				poly(image, pts, nelem(pts), 0, 0, t->line_thickness, color, ZP);
			}
			break;

		case NK_COMMAND_TRIANGLE_FILLED:
			{
				const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
				Point pts[4] = {
					addpt(Pt(t->a.x, t->a.y), p),
					addpt(Pt(t->b.x, t->b.y), p),
					addpt(Pt(t->c.x, t->c.y), p),
					addpt(Pt(t->a.x, t->a.y), p),
				};
				color = colorget(t->color);
				fillpoly(image, pts, nelem(pts), 1, color, ZP);
			}
			break;

		case NK_COMMAND_TEXT:
			{
				const struct nk_command_text *t = (const struct nk_command_text*)cmd;
				color = colorget(t->foreground);
				color2 = colorget(t->background);
				stringnbg(image, addpt(Pt(t->x, t->y), p), color, ZP, t->font->userdata.ptr,
					t->string, t->length, color2, ZP);
			}
			break;

		case NK_COMMAND_CURVE:
			{
				const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
				color = colorget(q->color);
				bezier(image, addpt(Pt(q->begin.x, q->begin.y), p), addpt(Pt(q->ctrl[0].x, q->ctrl[0].y), p),
					addpt(Pt(q->ctrl[1].x, q->ctrl[1].y), p), addpt(Pt(q->end.x, q->end.y), p),
					0, 0, q->line_thickness, color, ZP);
			}
			break;

		case NK_COMMAND_ARC:
			{
				const struct nk_command_arc *a = (const struct nk_command_arc *)cmd;
				color = colorget(a->color);
				arc(image, addpt(Pt(a->cx, a->cy), p), a->r, a->r, a->line_thickness, color, ZP,
					(int)a->a[0], (int)a->a[1]);
			}
			break;

		case NK_COMMAND_ARC_FILLED:
			{
				const struct nk_command_arc_filled *a = (const struct nk_command_arc_filled*)cmd;
				color = colorget(a->color);
				fillarc(image, addpt(Pt(a->cx, a->cy), p), a->r, a->r, color, ZP, (int)a->a[0], (int)a->a[1]);
			}
			break;

		case NK_COMMAND_IMAGE:
			{
				const struct nk_command_image *i = (const struct nk_command_image*)cmd;
				Image *imag = i->img.handle.ptr;
				assert(imag != nil);
				draw(image, rectaddpt(Rect(i->x, i->y, i->x+i->w, i->y+i->h), p), imag, nil, ZP);
			}
			break;

		/* apparently unused by nuklear, so abort if called */
#ifdef NOTYET
		case NK_COMMAND_POLYGON:
			break;
		case NK_COMMAND_POLYGON_FILLED:
			break;
		case NK_COMMAND_POLYLINE:
			break;
		case NK_COMMAND_RECT_MULTI_COLOR:
			break;
#endif

		default:
			{
				static int whined[NK_COMMAND_CUSTOM];
				if(!whined[cmd->type]){
					fprint(2, "unhandled render command %d\n", cmd->type);
					whined[cmd->type] = 1;
				}
			}
			break;
		}
	}

	nk_clear(ctx);

	colorgc();
}

static Rune nkeymap[NK_KEY_MAX] = {
[NK_KEY_SHIFT]			Kshift,
[NK_KEY_CTRL]			Kctl,
[NK_KEY_DEL]			Kdel,
[NK_KEY_ENTER]			'\n',
[NK_KEY_TAB]			'\t',
[NK_KEY_BACKSPACE]		Kbs,
[NK_KEY_UP]				Kup,
[NK_KEY_DOWN]			Kdown,
[NK_KEY_LEFT]			Kleft,
[NK_KEY_RIGHT]			Kright,
[NK_KEY_SCROLL_UP]		Kpgup,
[NK_KEY_SCROLL_DOWN]	Kpgdown,
};

static Rune frogs[] = {
	Kbs, Kdel, Kesc, '\n', '\t', Khome, Kend, 0
};

NK_API int
nk_plan9_handle_kbd(struct nk_context *ctx, char *kbd, int len)
{
	int i, down;
	char cmd, *rp;
	Rune rune;

	USED(len);

	cmd = *kbd;
	rp = kbd + 1;

	if(cmd == 'c'){
		chartorune(&rune, rp);
		if(runestrchr(frogs, rune) != nil)
			return 0;

		nk_input_unicode(ctx, rune);
		return 1;
	}

	if(cmd != 'k' && cmd != 'K')
		return 0;

	for(i = 0; i < nelem(nkeymap); i++){
		down = utfrune(rp, nkeymap[i]) != nil;
		nk_input_key(ctx, i, down);
	}

	return 1;
}

/*
	enum nk_keys nkkey;

	nkkey = nkeymap[k];

	if(nkkey)
		nk_input_key(&plan9.ctx, nkkey, down);
	else if(k == Khome){
		nk_input_key(&plan9.ctx, NK_KEY_TEXT_START, down);
		nk_input_key(&plan9.ctx, NK_KEY_SCROLL_START, down);
	} else if(k == Kend){
		nk_input_key(&plan9.ctx, NK_KEY_TEXT_END, down);
		nk_input_key(&plan9.ctx, NK_KEY_SCROLL_END, down);
	}
*/

NK_API int
nk_plan9_handle_mouse(struct nk_context *ctx, Mouse m, Point p)
{
	p = subpt(m.xy, p);

	nk_input_motion(ctx, p.x, p.y);
	nk_input_button(ctx, NK_BUTTON_LEFT,	p.x, p.y, m.buttons & 1);
	nk_input_button(ctx, NK_BUTTON_MIDDLE,	p.x, p.y, m.buttons & 2);
	nk_input_button(ctx, NK_BUTTON_RIGHT,	p.x, p.y, m.buttons & 4);

	if(m.buttons & 8)
		nk_input_scroll(ctx, nk_vec2(0, 1.0f));

	if(m.buttons & 16)
		nk_input_scroll(ctx, nk_vec2(0, -1.0f));

	return 1;
}
