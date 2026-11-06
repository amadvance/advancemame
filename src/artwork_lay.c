// license:BSD-3-Clause
// copyright-holders:Andrea Mazzoleni
/*
 * Static MAME layout compiler and software artwork compositing.
 * The DOM, parameters and group references are discarded after compilation.
 * All coordinates are normalised to the selected screen rectangle.
 */

#include "artwork_lay.h"
#include "artwork_package.h"
#include "xmlfile.h"
#include "png.h"
#include "nanojpeg.h"
#include "mame.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <stdint.h>

#define LAY_BACKDROP 1
#define LAY_OVERLAY 2
#define LAY_BEZEL 3
#define LAY_SCREEN 4
#define LAY_IMAGE 1
#define LAY_RECT 2
#define LAY_DISK 3
#define LAY_ALPHA 0
#define LAY_NONE 1
#define LAY_MULTIPLY 2
#define LAY_ADD 3
#define LAY_PARAM_COUNT 128
#define LAY_PARAM_LENGTH 256
#define LAY_MAX_DEPTH 24
#define LAY_MAX_ITEMS 65536

typedef struct
{
	double x0, y0, x1, y1;
} lay_box;

typedef struct
{
	double a, b, c, d, e, f;
} lay_matrix;

typedef struct
{
	char name[64];
	char value[LAY_PARAM_LENGTH];
} lay_param;

typedef struct
{
	lay_param params[LAY_PARAM_COUNT];
	int size;
	int blend;
} lay_env;

typedef struct
{
	char name[256];
	xml_data_node *node;
} lay_def;

typedef struct
{
	char name[512];
	UINT32 *rgba;
	int width, height;
} lay_image;

typedef struct
{
	int type, category, blend, legacy, front, screen, visible;
	lay_matrix mat;
	lay_box bounds;
	float color[4];
	char file[256], alphafile[256];
	lay_image *img;
	int px0, py0, width, height;
	UINT32 *pixels;            /* pre-scaled canonical ARGB */
} lay_item;

typedef struct
{
	lay_item *items;
	size_t count, capacity;
	int screens, art, primary, valid;
	char name[256];
	lay_box screenbox;
	lay_box extents;
	lay_box explicit_bounds;
	int has_explicit_bounds;
} lay_view;

typedef struct
{
	artwork_package package;
	lay_view scene;
	lay_image **images;
	size_t image_count, image_cap;
	int active, external_overlay, backdrop, explicit_screen_only;
	lay_matrix screen_transform;
	int screen_swapped, screen_transformed;
	int display_width, display_height;
	rectangle game_rect;
} lay_state;
static lay_state L;

typedef struct
{
	xml_data_node *root;
	lay_def *elements;
	size_t elements_count;
	lay_def *groups;
	size_t groups_count;
	lay_env global;
	int game_width, game_height;
} lay_document;

static int lay_equal(const char *a, const char *b)
{
	return a && b && !strcmp(a, b);
}

static double lay_min(double x, double y)
{
	return x < y ? x : y;
}

static double lay_max(double x, double y)
{
	return x > y ? x : y;
}

static int lay_clamp(int x)
{
	return x < 0 ? 0 : x > 255 ? 255 : x;
}

static double lay_unit(double v)
{
	return v < 0 ? 0 : v > 1 ? 1 : v;
}

static char *lay_copy(char *d, size_t cap, const char *s)
{
	size_t n;

	if (!s)
		s = "";
	n = strlen(s);
	if (n >= cap)
		n = cap - 1;
	memcpy(d, s, n);
	d[n] = 0;
	return d;
}

static const char *lay_raw(xml_data_node *n, const char *name)
{
	return xml_get_attribute_string(n, name, "");
}

static const char *lay_param_get(const lay_env *env, const char *name)
{
	int i;

	for (i = env->size - 1; i >= 0; --i)
		if (!strcmp(name, env->params[i].name))
			return env->params[i].value;
	return NULL;
}

static void lay_param_set(lay_env *env, const char *name, const char *value)
{
	int i;

	if (!name || !*name || strlen(name) >= sizeof(env->params[0].name))
		return;
	for (i = env->size - 1; i >= 0; --i)
		if (!strcmp(name, env->params[i].name))
			break;
	if (i < 0)
	{
		if (env->size >= LAY_PARAM_COUNT)
			return;
		i = env->size++;
	}
	lay_copy(env->params[i].name, sizeof(env->params[i].name), name);
	lay_copy(env->params[i].value, sizeof(env->params[i].value), value);
}

/* Expand MAME ~parameter~ syntax, avoiding eval or allocation during parsing. */
static void lay_expand(const lay_env *env, const char *src, char *dst, size_t size)
{
	int passes;
	char a[2048], b[2048];

	lay_copy(a, sizeof(a), src);
	for (passes = 0; passes < 5; ++passes)
	{
		const char *p = a;
		size_t used = 0;
		int substitutions = 0;
		while (*p && used + 1 < sizeof(b))
		{
			if (*p == '~')
			{
				const char *end = strchr(p + 1, '~');
				if (end && end - p - 1 < 64)
				{
					char key[64];
					const char *value;
					size_t n = (size_t)(end - p - 1), z;
					memcpy(key, p + 1, n);
					key[n] = 0;
					value = lay_param_get(env, key);
					if (value)
					{
						z = strlen(value);
						if (z > sizeof(b) - 1 - used)
							z = sizeof(b) - 1 - used;
						memcpy(b + used, value, z);
						used += z;
						p = end + 1;
						substitutions = 1;
						continue;
					}
				}
			}
			b[used++] = *p++;
		}
		b[used] = 0;
		lay_copy(a, sizeof(a), b);
		if (!substitutions)
			break;
	}
	lay_copy(dst, size, a);
}

static void lay_attr(xml_data_node *node, const char *key,
	const lay_env *env, char *out, size_t cap)
{
	lay_expand(env, lay_raw(node, key), out, cap);
}

static double lay_number_string(const char *str, double fallback)
{
	char *end;
	double v;

	if (!str || !*str)
		return fallback;
	v = strtod(str, &end);
	return (*end || !isfinite(v)) ? fallback : v;
}

static double lay_num(xml_data_node *n, const char *key, const lay_env *env, double def)
{
	char tmp[2048];

	lay_attr(n, key, env, tmp, sizeof(tmp));
	return lay_number_string(tmp, def);
}

static long lay_int(xml_data_node *n, const char *key, const lay_env *env, long def)
{
	char tmp[2048], *end;
	long v;

	lay_attr(n, key, env, tmp, sizeof(tmp));
	if (!tmp[0])
		return def;
	if (tmp[0] == '$')
	{
		v = strtol(tmp + 1, &end, 16);
		return *end ? def : v;
	}
	v = strtol(tmp[0] == '#' ? tmp + 1 : tmp, &end, 0);
	return *end ? def : v;
}

static int lay_bool(xml_data_node *n, const char *key, const lay_env *env, int def)
{
	char tmp[128];

	lay_attr(n, key, env, tmp, sizeof(tmp));
	if (!*tmp)
		return def;
	return !strcmp(tmp, "yes") || !strcmp(tmp, "true") || !strcmp(tmp, "1");
}

static xml_data_node *lay_child(xml_data_node *node, const char *name)
{
	xml_data_node *ch;

	if (!node)
		return NULL;
	for (ch = node->child; ch; ch = ch->next)
		if (lay_equal(ch->name, name))
			return ch;
	return NULL;
}

static lay_box lay_box_make(double x, double y, double w, double h)
{
	lay_box b;

	b.x0 = x;
	b.y0 = y;
	b.x1 = x + w;
	b.y1 = y + h;
	return b;
}

static lay_box lay_bounds(xml_data_node *node, const lay_env *env, lay_box def)
{
	xml_data_node *b = lay_child(node, "bounds");

	if (b)
	{
		double x = lay_num(b, "x", env, 0), y = lay_num(b, "y", env, 0);
		double w = lay_num(b, "width", env, 1), h = lay_num(b, "height", env, 1);
		if (xml_get_attribute(b, "left"))
		{
			double l = lay_num(b, "left", env, 0), t = lay_num(b, "top", env, 0);
			double r = lay_num(b, "right", env, 1), bt = lay_num(b, "bottom", env, 1);
			return lay_box_make(l, t, r - l, bt - t);
		}
		return lay_box_make(x, y, w, h);
	}
	return def;
}

static lay_box lay_box_union(lay_box a, lay_box b)
{
	a.x0 = lay_min(a.x0, b.x0);
	a.y0 = lay_min(a.y0, b.y0);
	a.x1 = lay_max(a.x1, b.x1);
	a.y1 = lay_max(a.y1, b.y1);
	return a;
}

static lay_matrix lay_identity(void)
{
	lay_matrix r =
	{
		1, 0, 0, 0, 1, 0
	};

	return r;
}

static lay_matrix lay_mul(lay_matrix a, lay_matrix b)
{
	lay_matrix r;

	r.a = a.a * b.a + a.b * b.d;
	r.b = a.a * b.b + a.b * b.e;
	r.c = a.a * b.c + a.b * b.f + a.c;
	r.d = a.d * b.a + a.e * b.d;
	r.e = a.d * b.b + a.e * b.e;
	r.f = a.d * b.c + a.e * b.f + a.f;
	return r;
}

static lay_matrix lay_rect_mat(lay_box b)
{
	lay_matrix r =
	{
		b.x1 - b.x0, 0, b.x0, 0, b.y1 - b.y0, b.y0
	};

	return r;
}

static lay_matrix lay_orientation(xml_data_node *node, const lay_env *env)
{
	xml_data_node *o = lay_child(node, "orientation");
	lay_matrix mat = lay_identity();
	int angle, swap, fx, fy;

	if (!o)
		return mat;
	angle = (int)lay_int(o, "rotate", env, 0);
	swap = lay_bool(o, "swapxy", env, 0);
	fx = lay_bool(o, "flipx", env, 0);
	fy = lay_bool(o, "flipy", env, 0);
	if (angle == 90)
	{
		lay_matrix m =
		{
			0, -1, 1, 1, 0, 0
		};
		mat = lay_mul(m, mat);
	}
	if (angle == 180)
	{
		lay_matrix m =
		{
			-1, 0, 1, 0, -1, 1
		};
		mat = lay_mul(m, mat);
	}
	if (angle == 270)
	{
		lay_matrix m =
		{
			0, 1, 0, -1, 0, 1
		};
		mat = lay_mul(m, mat);
	}
	if (swap)
	{
		lay_matrix m =
		{
			0, 1, 0, 1, 0, 0
		};
		mat = lay_mul(m, mat);
	}
	if (fx)
	{
		lay_matrix m =
		{
			-1, 0, 1, 0, 1, 0
		};
		mat = lay_mul(m, mat);
	}
	if (fy)
	{
		lay_matrix m =
		{
			1, 0, 0, 0, -1, 1
		};
		mat = lay_mul(m, mat);
	}
	return mat;
}

static void lay_point(lay_matrix m, double x, double y, double *ox, double *oy)
{
	*ox = m.a * x + m.b * y + m.c;
	*oy = m.d * x + m.e * y + m.f;
}

static lay_box lay_mat_box(lay_matrix m)
{
	double x[4], y[4];
	int i;
	lay_box b;

	lay_point(m, 0, 0, &x[0], &y[0]);
	lay_point(m, 1, 0, &x[1], &y[1]);
	lay_point(m, 0, 1, &x[2], &y[2]);
	lay_point(m, 1, 1, &x[3], &y[3]);
	b.x0 = b.x1 = x[0];
	b.y0 = b.y1 = y[0];
	for (i = 1; i < 4; i++)
	{
		b.x0 = lay_min(b.x0, x[i]);
		b.x1 = lay_max(b.x1, x[i]);
		b.y0 = lay_min(b.y0, y[i]);
		b.y1 = lay_max(b.y1, y[i]);
	}
	return b;
}

static void lay_color(xml_data_node *node, const lay_env *env, float out[4])
{
	xml_data_node *c = lay_child(node, "color");
	int i;
	static const char *names[] =
	{
		"red", "green", "blue", "alpha"
	};

	if (!c)
		return;
	for (i = 0; i < 4; i++)
		out[i] *= (float)lay_unit(lay_num(c, names[i], env, 1));
}

static int lay_enabled_component(xml_data_node *node, const lay_env *env, long state)
{
	int have_state = xml_get_attribute(node, "state") != NULL;
	int have_mask = xml_get_attribute(node, "statemask") != NULL;
	unsigned long s = (unsigned long)lay_int(node, "state", env, 0);
	unsigned long mask = (unsigned long)lay_int(node, "statemask", env, 0);

	if (have_mask && !have_state)
		return ((unsigned long)state & mask) == mask;
	if (have_mask)
		return ((unsigned long)state & mask) == (s & mask);
	return !have_state || (unsigned long)state == s;
}

static void lay_set_param(xml_data_node *node, lay_env *env)
{
	char key[64], value[LAY_PARAM_LENGTH];

	lay_attr(node, "name", env, key, sizeof(key));
	if (!*key)
		return;
	if (xml_get_attribute(node, "value"))
		lay_attr(node, "value", env, value, sizeof(value));
	else if (xml_get_attribute(node, "start"))
		lay_attr(node, "start", env, value, sizeof(value));
	else
		return;
	lay_param_set(env, key, value);
}

static xml_data_node *lay_find(lay_def *list, size_t count, const char *name)
{
	size_t i;

	for (i = 0; i < count; i++)
		if (!strcmp(list[i].name, name))
			return list[i].node;
	return NULL;
}

static int lay_register(lay_def **list, size_t *count, xml_data_node *node, const lay_env *env)
{
	char name[256];
	lay_def *items;

	lay_attr(node, "name", env, name, sizeof(name));
	if (!*name || lay_find(*list, *count, name))
		return 0;
	if (*count == SIZE_MAX / sizeof(**list))
		return 0;
	items = (lay_def*)realloc(*list, (*count + 1) * sizeof(**list));
	if (!items)
		return 0;
	*list = items;
	lay_copy(items[*count].name, sizeof(items[*count].name), name);
	items[*count].node = node;
	(*count)++;
	return 1;
}

static int lay_push(lay_view *v, const lay_item *it)
{
	lay_item *newitems;
	size_t next;

	if (v->count >= LAY_MAX_ITEMS)
		return 0;
	if (v->count == v->capacity)
	{
		next = v->capacity ? v->capacity * 2 : 64;
		if (next > LAY_MAX_ITEMS)
			next = LAY_MAX_ITEMS;
		newitems = (lay_item*)realloc(v->items, next * sizeof(*newitems));
		if (!newitems)
			return 0;
		v->items = newitems;
		v->capacity = next;
	}
	v->items[v->count++] = *it;
	return 1;
}

static void lay_discard(lay_view *v)
{
	size_t i;

	for (i = 0; i < v->count; i++)
		free(v->items[i].pixels);
	free(v->items);
	memset(v, 0, sizeof(*v));
}

static lay_box lay_element_box(xml_data_node *def, const lay_env *env)
{
	xml_data_node *ch;
	lay_box bounds = lay_box_make(0, 0, 1, 1);
	int found = 0;

	for (ch = def->child; ch; ch = ch->next)
	{
		lay_box b;
		if (!lay_equal(ch->name, "image") && !lay_equal(ch->name, "rect") &&
			!lay_equal(ch->name, "disk"))
			continue;
		b = lay_bounds(ch, env, lay_box_make(0, 0, 1, 1));
		if (!found)
		{
			bounds = b;
			found = 1;
		}
		else
			bounds = lay_box_union(bounds, b);
	}
	return bounds;
}

static int lay_blend(xml_data_node *node, int inherited)
{
	const char *blend = lay_raw(node, "blend");

	if (lay_equal(blend, "multiply"))
		return LAY_MULTIPLY;
	if (lay_equal(blend, "add"))
		return LAY_ADD;
	if (lay_equal(blend, "none"))
		return LAY_NONE;
	if (lay_equal(blend, "alpha"))
		return LAY_ALPHA;
	return inherited;
}

static int lay_emit_element(lay_document *doc, lay_view *view, xml_data_node *node, lay_env *env, lay_matrix parent, int legacy, int depth)
{
	char ref[256];
	xml_data_node *def, *ch;
	lay_box instance, defbox;
	lay_matrix map, orient;
	float color[4] =
	{
		1, 1, 1, 1
	};
	int blend = lay_blend(node, env->blend);
	long state = 0;

	lay_attr(node, legacy ? "element" : "ref", env, ref, sizeof(ref));
	if (!*ref)
		return 1;
	def = lay_find(doc->elements, doc->elements_count, ref);
	if (!def)
	{
		printf("artwork: unknown element '%s'\n", ref);
		return 1;
	}
	instance = lay_bounds(node, env, lay_box_make(0, 0, 1, 1));
	if (instance.x1 <= instance.x0 || instance.y1 <= instance.y0)
		return 1;
	orient = lay_orientation(node, env);
	map = lay_mul(parent, lay_mul(lay_rect_mat(instance), orient));
	lay_color(node, env, color);
	state = lay_int(def, "defstate", env, 0);
	defbox = lay_element_box(def, env);
	if (defbox.x1 <= defbox.x0 || defbox.y1 <= defbox.y0)
		return 1;
	for (ch = def->child; ch; ch = ch->next)
	{
		lay_item item;
		lay_box cb, norm;
		int primitive;
		if (lay_equal(ch->name, "image"))
			primitive = LAY_IMAGE;
		else if (lay_equal(ch->name, "rect"))
			primitive = LAY_RECT;
		else if (lay_equal(ch->name, "disk"))
			primitive = LAY_DISK;
		else
			continue;
		if (!lay_enabled_component(ch, env, state))
			continue;
		cb = lay_bounds(ch, env, lay_box_make(0, 0, 1, 1));
		norm = lay_box_make((cb.x0 - defbox.x0) / (defbox.x1 - defbox.x0),
				(cb.y0 - defbox.y0) / (defbox.y1 - defbox.y0),
				(cb.x1 - cb.x0) / (defbox.x1 - defbox.x0),
				(cb.y1 - cb.y0) / (defbox.y1 - defbox.y0));
		memset(&item, 0, sizeof(item));
		item.type = primitive;
		item.blend = blend;
		item.legacy = legacy;
		item.category = legacy;
		item.mat = lay_mul(map, lay_rect_mat(norm));
		item.bounds = lay_mat_box(item.mat);
		item.visible = 1;
		memcpy(item.color, color, sizeof(color));
		lay_color(ch, env, item.color);
		if (primitive == LAY_IMAGE)
		{
			lay_attr(ch, "file", env, item.file, sizeof(item.file));
			lay_attr(ch, "alphafile", env, item.alphafile, sizeof(item.alphafile));
			if (!*item.file)
				continue;
		}
		if (!lay_push(view, &item))
			return 0;
		view->art = 1;
	}
	(void)depth;
	return 1;
}

static int lay_walk(lay_document *doc, lay_view *view, xml_data_node *parent, lay_env *env, lay_matrix matrix, int depth);

/* Groups can request a new bounding rectangle. Determine the natural extent by
 * expanding a temporary group with an identity transform, then mapping it. */
static int lay_group(lay_document *doc, lay_view *view, xml_data_node *node, lay_env *env, lay_matrix parent, int depth)
{
	char ref[256];
	xml_data_node *def;
	lay_env sub = *env;
	lay_matrix m = parent;
	lay_box bbox;
	lay_view temporary;
	int has_bounds = lay_child(node, "bounds") != NULL;

	lay_attr(node, "ref", env, ref, sizeof(ref));
	def = lay_find(doc->groups, doc->groups_count, ref);
	if (!def)
	{
		printf("artwork: unknown group '%s'\n", ref);
		return 1;
	}
	if (depth >= LAY_MAX_DEPTH)
	{
		printf("artwork: recursive group '%s'\n", ref);
		return 0;
	}
	if (has_bounds)
	{
		lay_box target = lay_bounds(node, env, lay_box_make(0, 0, 1, 1));
		size_t i;
		memset(&temporary, 0, sizeof(temporary));
		if (!lay_walk(doc, &temporary, def, &sub, lay_identity(), depth + 1))
		{
			lay_discard(&temporary);
			return 0;
		}
		bbox = lay_box_make(0, 0, 1, 1);
		if (temporary.count)
		{
			bbox = temporary.items[0].bounds;
			for (i = 1; i < temporary.count; i++)
				bbox = lay_box_union(bbox, temporary.items[i].bounds);
		}
		lay_discard(&temporary);
		if (bbox.x1 > bbox.x0 && bbox.y1 > bbox.y0)
		{
			lay_matrix norm =
			{
				1 / (bbox.x1 - bbox.x0), 0, -bbox.x0 / (bbox.x1 - bbox.x0),
				0, 1 / (bbox.y1 - bbox.y0), -bbox.y0 / (bbox.y1 - bbox.y0)
			};
			m = lay_mul(parent, lay_mul(lay_rect_mat(target), lay_mul(lay_orientation(node, env), norm)));
		}
	}
	else
		m = lay_mul(parent, lay_orientation(node, env));
	{
		size_t first = view->count, i;
		float tint[4] =
		{
			1, 1, 1, 1
		};
		sub = *env;
		/* Group blending applies to children unless an element overrides it. */
		sub.blend = lay_blend(node, lay_blend(def, env->blend));
		if (!lay_walk(doc, view, def, &sub, m, depth + 1))
			return 0;
		lay_color(def, env, tint);
		lay_color(node, env, tint);
		for (i = first; i < view->count; i++)
		{
			int k;
			for (k = 0; k < 4; k++)
				view->items[i].color[k] *= tint[k];
		}
		return 1;
	}
}

static int lay_repeat(lay_document *doc, lay_view *view, xml_data_node *node, lay_env *env, lay_matrix mat, int depth)
{
	long count = lay_int(node, "count", env, 0), i;
	lay_env generators = *env;
	xml_data_node *p;

	if (count <= 0 || count > LAY_MAX_ITEMS)
	{
		printf("artwork: invalid repeat count %ld\n", count);
		return 0;
	}
	for (p = node->child; p; p = p->next)
		if (lay_equal(p->name, "param") && xml_get_attribute(p, "start"))
			lay_set_param(p, &generators);
	for (i = 0; i < count; i++)
	{
		lay_env iteration = generators;
		for (p = node->child; p; p = p->next)
		{
			if (lay_equal(p->name, "param") && xml_get_attribute(p, "start"))
				continue;
			if (lay_equal(p->name, "param"))
			{
				lay_set_param(p, &iteration);
				continue;
			}
			if (!lay_walk(doc, view, p, &iteration, mat, depth + 1))
				return 0;
		}
		for (p = node->child; p; p = p->next)
		{
			char key[64], text[LAY_PARAM_LENGTH], *end;
			long long v, shift;
			double d;
			if (!lay_equal(p->name, "param") || !xml_get_attribute(p, "start"))
				continue;
			lay_attr(p, "name", &generators, key, sizeof(key));
			lay_copy(text, sizeof(text), lay_param_get(&generators, key));
			if (!*text)
				continue;
			d = strtod(text, &end);
			d += lay_num(p, "increment", &generators, 0);
			shift = lay_int(p, "lshift", &generators, 0) - lay_int(p, "rshift", &generators, 0);
			if (shift)
			{
				if (!isfinite(d) || fabs(d) > 9.0e15 || shift > 60 || shift < -60)
					return 0;
				v = (long long)d;
				if (shift > 0)
				{
					if (v > (LLONG_MAX >> shift) || v < (LLONG_MIN >> shift))
						return 0;
					v *= ((long long)1 << shift);
				}
				else
					v /= ((long long)1 << (-shift));
				snprintf(text, sizeof(text), "%lld", v);
			}
			else
				snprintf(text, sizeof(text), "%.15g", d);
			lay_param_set(&generators, key, text);
		}
	}
	return 1;
}

static int lay_walk(lay_document *doc, lay_view *view, xml_data_node *parent, lay_env *env, lay_matrix matrix, int depth)
{
	xml_data_node *p, *first, *last;

	if (depth >= LAY_MAX_DEPTH)
		return 0;
	/* Accept either a single item node or a parent containing multiple items. */
	first = parent;
	last = parent;
	if (lay_equal(parent->name, "view") || lay_equal(parent->name, "group") ||
		lay_equal(parent->name, "collection") || lay_equal(parent->name, "repeat"))
	{
		first = parent->child;
		last = NULL;
	}
	for (p = first; p && (!last || p == last); p = p->next)
	{
		lay_env sub;
		lay_item item;
		lay_box b;
		if (lay_equal(p->name, "param"))
		{
			lay_set_param(p, env);
			continue;
		}
		if (lay_equal(p->name, "bounds") || lay_equal(p->name, "color") ||
			lay_equal(p->name, "orientation") || lay_equal(p->name, "script"))
			continue;
		if (lay_equal(p->name, "collection"))
		{
			if (lay_bool(p, "visible", env, 1))
			{
				sub = *env;
				if (!lay_walk(doc, view, p, &sub, matrix, depth + 1))
					return 0;
			}
			continue;
		}
		if (lay_equal(p->name, "repeat"))
		{
			if (!lay_repeat(doc, view, p, env, matrix, depth + 1))
				return 0;
			continue;
		}
		if (lay_equal(p->name, "group"))
		{
			if (!lay_group(doc, view, p, env, matrix, depth + 1))
				return 0;
			continue;
		}
		if (lay_equal(p->name, "screen"))
		{
			memset(&item, 0, sizeof(item));
			item.type = LAY_SCREEN;
			item.category = LAY_SCREEN;
			item.screen = (int)lay_int(p, "index", env, 0);
			item.visible = 1;
			b = lay_bounds(p, env, lay_box_make(0, 0, 1, 1));
			if (b.x1 <= b.x0 || b.y1 <= b.y0)
				continue;
			item.mat = lay_mul(matrix, lay_mul(lay_rect_mat(b), lay_orientation(p, env)));
			item.bounds = lay_mat_box(item.mat);
			if (!lay_push(view, &item))
				return 0;
			view->screens++;
			continue;
		}
		if (lay_equal(p->name, "element") || lay_equal(p->name, "backdrop") ||
			lay_equal(p->name, "overlay") || lay_equal(p->name, "bezel") ||
			lay_equal(p->name, "cpanel") || lay_equal(p->name, "marquee"))
		{
			int legacy = 0;
			if (lay_equal(p->name, "backdrop"))
				legacy = LAY_BACKDROP;
			if (lay_equal(p->name, "overlay"))
				legacy = LAY_OVERLAY;
			if (lay_equal(p->name, "bezel") || lay_equal(p->name, "cpanel") ||
				lay_equal(p->name, "marquee"))
				legacy = LAY_BEZEL;
			if (!lay_emit_element(doc, view, p, env, matrix, legacy, depth + 1))
				return 0;
			continue;
		}
	}
	return 1;
}

static UINT32 lay_rgba(unsigned a, unsigned r, unsigned g, unsigned b)
{
	return (a << 24) | (r << 16) | (g << 8) | b;
}

static UINT32 *lay_read_png(const char *file, int *w, int *h)
{
	mame_file *f = artwork_package_open(&L.package, file);
	png_info info;
	UINT32 *rgb = NULL;
	size_t pixels, i;
	int channels = 0;

	if (!f)
	{
		printf("artwork: %s: image '%s' not found\n", L.package.game, file);
		return NULL;
	}
	memset(&info, 0, sizeof(info));
	if (!png_read_file(f, &info))
	{
		printf("artwork: %s: corrupt PNG '%s'\n", L.package.game, file);
		mame_fclose(f);
		return NULL;
	}
	mame_fclose(f);
	if (info.bit_depth > 8 || info.interlace_method ||
		(info.color_type != 0 && info.color_type != 2 &&
		info.color_type != 3 && info.color_type != 6) ||
		info.width == 0 || info.height == 0 || info.width > INT_MAX || info.height > INT_MAX ||
		(size_t)info.width > SIZE_MAX / (size_t)info.height / sizeof(UINT32))
	{
		printf("artwork: %s: unsupported PNG '%s'\n", L.package.game, file);
		goto done;
	}
	if (!png_expand_buffer_8bit(&info))
		goto done;
	pixels = (size_t)info.width * info.height;
	rgb = (UINT32*)malloc(pixels * sizeof(UINT32));
	if (!rgb)
		goto done;
	channels = (info.color_type == 6) ? 4 : (info.color_type == 2) ? 3 : 1;
	for (i = 0; i < pixels; i++)
	{
		const UINT8 *p = info.image + i * channels;
		unsigned a = 255, r = 0, g = 0, b = 0;
		if (info.color_type == 3)
		{
			unsigned idx = p[0];
			if (idx >= info.num_palette || !info.palette)
			{
				free(rgb);
				rgb = NULL;
				break;
			}
			r = info.palette[idx * 3];
			g = info.palette[idx * 3 + 1];
			b = info.palette[idx * 3 + 2];
			if (idx < info.num_trans && info.trans)
				a = info.trans[idx];
		}
		else if (info.color_type == 0)
		{
			r = g = b = p[0];
		}
		else
		{
			r = p[0];
			g = p[1];
			b = p[2];
			if (channels == 4)
				a = p[3];
		}
		rgb[i] = lay_rgba(a, r, g, b);
	}
	if (rgb)
	{
		*w = (int)info.width;
		*h = (int)info.height;
	}
done:
	free(info.image);
	free(info.palette);
	free(info.trans);
	return rgb;
}

static UINT32 *lay_read_jpeg(const char *file, int *w, int *h)
{
	mame_file *f = artwork_package_open(&L.package, file);
	UINT32 *rgba = NULL;
	UINT8 *data = NULL;
	UINT8 *p;
	UINT64 length;
	int color, width, height;
	size_t pixels, i;

	if (!f)
	{
		printf("artwork: %s: image '%s' not found\n", L.package.game, file);
		return NULL;
	}
	length = mame_fsize(f);
	if (!length || length > INT_MAX)
	{
		mame_fclose(f);
		return NULL;
	}
	data = (UINT8*)malloc((size_t)length);
	if (!data)
	{
		mame_fclose(f);
		return NULL;
	}
	if (mame_fread(f, data, (UINT32)length) != (UINT32)length)
		goto done;
	njInit();
	if (njDecode(data, (int)length) != NJ_OK)
	{
		printf("artwork: %s: unsupported/corrupt JPEG '%s'\n", L.package.game, file);
		njDone();
		goto done;
	}
	width = njGetWidth();
	height = njGetHeight();
	color = njIsColor();
	if (width <= 0 || height <= 0 || (size_t)width > SIZE_MAX / (size_t)height / 4 ||
		(size_t)njGetImageSize() < (size_t)width * height * (color ? 3 : 1))
	{
		njDone();
		goto done;
	}
	pixels = (size_t)width * height;
	rgba = (UINT32*)malloc(pixels * sizeof(UINT32));
	if (rgba)
	{
		p = njGetImage();
		for (i = 0; i < pixels; i++)
		{
			if (color)
				rgba[i] = lay_rgba(255, p[3 * i], p[3 * i + 1], p[3 * i + 2]);
			else
				rgba[i] = lay_rgba(255, p[i], p[i], p[i]);
		}
		*w = width;
		*h = height;
	}
	njDone();
done:
	mame_fclose(f);
	free(data);
	return rgba;
}

static UINT32 *lay_decode(const char *name, int *w, int *h)
{
	mame_file *f = artwork_package_open(&L.package, name);
	UINT8 sig[8];
	UINT32 n = 0;

	if (f)
	{
		n = mame_fread(f, sig, sizeof(sig));
		mame_fclose(f);
	}
	if (n >= 2 && sig[0] == 0xff && sig[1] == 0xd8)
		return lay_read_jpeg(name, w, h);
	if (n == 8 && !memcmp(sig, "\x89PNG\r\n\x1a\n", 8))
		return lay_read_png(name, w, h);
	printf("artwork: %s: unsupported image format '%s'\n", L.package.game, name);
	return NULL;
}

static lay_image *lay_image_get(const char *file, const char *alpha)
{
	size_t k, pixels;
	lay_image *img, **newimages;
	int width = 0, height = 0, aw = 0, ah = 0;
	UINT32 *rgba, *mask = NULL;
	char key[512];

	if (snprintf(key, sizeof(key), "%s|%s", file, alpha) >= (int)sizeof(key))
		return NULL;
	for (k = 0; k < L.image_count; k++)
		if (!strcmp(L.images[k]->name, key))
			return L.images[k];
	rgba = lay_decode(file, &width, &height);
	if (!rgba)
		return NULL;
	if (*alpha)
	{
		mask = lay_read_png(alpha, &aw, &ah);
		if (!mask || aw != width || ah != height)
		{
			printf("artwork: %s: alphafile '%s' does not match '%s'\n", L.package.game, alpha, file);
			free(mask);
			free(rgba);
			return NULL;
		}
		pixels = (size_t)width * height;
		for (k = 0; k < pixels; k++)
		{
			UINT32 v = mask[k];
			unsigned r = (v >> 16) & 255, g = (v >> 8) & 255, b = v & 255;
			unsigned brightness = (299 * r + 587 * g + 114 * b + 500) / 1000;
			rgba[k] = (rgba[k] & 0xffffff) | (brightness << 24);
		}
		free(mask);
	}
	img = (lay_image*)calloc(1, sizeof(*img));
	if (!img)
	{
		free(rgba);
		return NULL;
	}
	lay_copy(img->name, sizeof(img->name), key);
	img->rgba = rgba;
	img->width = width;
	img->height = height;
	if (L.image_count == L.image_cap)
	{
		size_t cap = L.image_cap ? L.image_cap * 2 : 16;
		newimages = (lay_image**)realloc(L.images, cap * sizeof(*newimages));
		if (!newimages)
		{
			free(img->rgba);
			free(img);
			return NULL;
		}
		L.images = newimages;
		L.image_cap = cap;
	}
	L.images[L.image_count++] = img;
	return img;
}

static int lay_items_for_view(lay_document *doc, xml_data_node *view,
	lay_view *out)
{
	size_t i;
	lay_env env = doc->global;

	memset(out, 0, sizeof(*out));
	lay_attr(view, "name", &env, out->name, sizeof(out->name));
	lay_param_set(&env, "viewname", out->name);
	out->has_explicit_bounds = lay_child(view, "bounds") != NULL;
	if (out->has_explicit_bounds)
		out->explicit_bounds = lay_bounds(view, &env, lay_box_make(0, 0, 1, 1));
	if (!lay_walk(doc, out, view, &env, lay_identity(), 0))
	{
		lay_discard(out);
		return 0;
	}
	out->primary = -1;
	for (i = 0; i < out->count; i++)
	{
		if (out->items[i].type == LAY_SCREEN && out->items[i].screen == 0)
		{
			out->primary = (int)i;
			break;
		}
	}
	if (out->primary < 0)
		for (i = 0; i < out->count; i++)
			if (out->items[i].type == LAY_SCREEN)
			{
				out->primary = (int)i;
				break;
			}
	if (out->primary < 0)
		return 1;
	out->screenbox = out->items[out->primary].bounds;
	if (out->screenbox.x1 <= out->screenbox.x0 || out->screenbox.y1 <= out->screenbox.y0)
		out->primary = -1;
	out->valid = out->primary >= 0;
	return 1;
}

static int lay_rank(const lay_view *v)
{
	if (!v->valid)
		return 99;
	if (v->screens == 1)
		return v->art ? 0 : 1;
	return v->art ? 2 : 3;
}

static int lay_select(lay_document *doc, const char *request)
{
	xml_data_node *root = lay_child(doc->root, "mamelayout"), *v;
	lay_view chosen;
	int best = 99, exact = 0;
	int named = request && *request && strcmp(request, "auto") && strcmp(request, "none");

	memset(&chosen, 0, sizeof(chosen));
	for (v = root->child; v; v = v->next)
	{
		lay_view candidate;
		int rank;
		if (!lay_equal(v->name, "view"))
			continue;
		if (!lay_items_for_view(doc, v, &candidate))
		{
			lay_discard(&chosen);
			return 0;
		}
		rank = lay_rank(&candidate);
		if (named && !strcmp(request, candidate.name) && rank < 99)
		{
			lay_discard(&chosen);
			chosen = candidate;
			exact = 1;
			break;
		}
		if (!named && rank < best)
		{
			lay_discard(&chosen);
			chosen = candidate;
			best = rank;
		}
		else
			lay_discard(&candidate);
	}
	if (named && !exact)
	{
		printf("artwork: %s: view '%s' unavailable, using auto\n", L.package.game, request);
		for (v = root->child; v; v = v->next)
		{
			lay_view candidate;
			int rank;
			if (!lay_equal(v->name, "view"))
				continue;
			if (!lay_items_for_view(doc, v, &candidate))
			{
				lay_discard(&chosen);
				return 0;
			}
			rank = lay_rank(&candidate);
			if (rank < best)
			{
				lay_discard(&chosen);
				chosen = candidate;
				best = rank;
			}
			else
				lay_discard(&candidate);
		}
	}
	if (!chosen.valid)
	{
		lay_discard(&chosen);
		printf("artwork: %s: no usable view\n", L.package.game);
		return 0;
	}
	L.explicit_screen_only = (exact && named && !strcmp(chosen.name, "Screen Only"));
	L.scene = chosen;
	if (chosen.screens > 1)
		printf("artwork: %s: view '%s' has %d screens; using the first\n",
			L.package.game, chosen.name, chosen.screens);
	return 1;
}

static void lay_normalise(void)
{
	lay_view *v = &L.scene;
	lay_box s = v->screenbox;
	double w = s.x1 - s.x0, h = s.y1 - s.y0;
	size_t i;

	if (v->has_explicit_bounds)
	{
		lay_box b = v->explicit_bounds;
		v->extents = lay_box_make((b.x0 - s.x0) / w, (b.y0 - s.y0) / h,
				(b.x1 - b.x0) / w, (b.y1 - b.y0) / h);
	}
	else
		v->extents = lay_box_make(0, 0, 1, 1);
	for (i = 0; i < v->count; i++)
	{
		lay_item *it = &v->items[i];
		it->front = ((int)i > v->primary);
		it->mat.c = (it->mat.c - s.x0) / w;
		it->mat.a /= w;
		it->mat.b /= w;
		it->mat.f = (it->mat.f - s.y0) / h;
		it->mat.d /= h;
		it->mat.e /= h;
		it->bounds = lay_mat_box(it->mat);
		if (!v->has_explicit_bounds)
			v->extents = lay_box_union(v->extents, it->bounds);
	}
}

/* Layout artwork uses the driver's upright coordinates, while the OSD
 * rotates the complete composed bitmap. Undo that orientation for the scene
 * and preserve the native screen pixels, so artwork is oriented only once. */
static void lay_native_orientation(int orientation)
{
	lay_view *v = &L.scene;
	lay_matrix upright = lay_identity(), native;
	double det;
	size_t i;

	if (!orientation)
		return;
	if (orientation & ORIENTATION_SWAP_XY)
	{
		upright.a = upright.e = 0;
		upright.b = upright.d = 1;
	}
	if (orientation & ORIENTATION_FLIP_X)
	{
		upright.a = -upright.a;
		upright.b = -upright.b;
		upright.c = 1;
	}
	if (orientation & ORIENTATION_FLIP_Y)
	{
		upright.d = -upright.d;
		upright.e = -upright.e;
		upright.f = 1;
	}
	det = upright.a * upright.e - upright.b * upright.d;
	native.a = upright.e / det;
	native.b = -upright.b / det;
	native.d = -upright.d / det;
	native.e = upright.a / det;
	native.c = -native.a * upright.c - native.b * upright.f;
	native.f = -native.d * upright.c - native.e * upright.f;
	if (v->has_explicit_bounds)
		v->extents = lay_mat_box(lay_mul(native, lay_rect_mat(v->extents)));
	else
		v->extents = lay_box_make(0, 0, 1, 1);
	for (i = 0; i < v->count; ++i)
	{
		lay_item *it = &v->items[i];
		it->mat = lay_mul(native, it->mat);
		if (it->type == LAY_SCREEN)
			it->mat = lay_mul(it->mat, upright);
		it->bounds = lay_mat_box(it->mat);
		if (!v->has_explicit_bounds)
			v->extents = lay_box_union(v->extents, it->bounds);
	}
}

/* For a generic front element, find out whether it should be classified as a
 * bezel (opaque areas, or transparent cutout surrounded by a bezel frame) or
 * as an overlay (uniformly translucent color filter across the screen). */
static int lay_front_opaque(const lay_item *it)
{
	lay_box b = it->bounds;
	int i, j;
	int has_opaque = 0;
	int has_trans = 0;
	int has_translucent = 0;

	if (b.x1 <= 0 || b.y1 <= 0 || b.x0 >= 1 || b.y0 >= 1)
		return 1;
	if (it->type == LAY_RECT || it->type == LAY_DISK)
	{
		return it->color[3] >= 0.999f;
	}
	if (!it->img)
		return 0;
	for (j = 0; j < 32; j++)
		for (i = 0; i < 32; i++)
		{
			double x = lay_max(0, b.x0) + (lay_min(1, b.x1) - lay_max(0, b.x0)) * ((i + 0.5) / 32.0);
			double y = lay_max(0, b.y0) + (lay_min(1, b.y1) - lay_max(0, b.y0)) * ((j + 0.5) / 32.0);
			double det = it->mat.a * it->mat.e - it->mat.b * it->mat.d;
			double u, v, alpha;
			int ix, iy;
			if (fabs(det) < 1e-15)
				return 1;
			x -= it->mat.c;
			y -= it->mat.f;
			u = (it->mat.e * x - it->mat.b * y) / det;
			v = (-it->mat.d * x + it->mat.a * y) / det;
			if (u < 0 || v < 0 || u >= 1 || v >= 1)
				continue;
			ix = (int)(u * it->img->width);
			iy = (int)(v * it->img->height);
			alpha = ((it->img->rgba[(size_t)iy * it->img->width + ix] >> 24) & 255) * it->color[3];
			if (alpha >= 254.5)
				has_opaque = 1;
			else if (alpha <= 10.0)
				has_trans = 1;
			else
				has_translucent = 1;
		}
	if (has_opaque)
		return 1;
	if (has_trans)
	{
		if (b.x0 < 0 || b.x1 > 1 || b.y0 < 0 || b.y1 > 1)
			return 1;
		else
		{
			static const int sample_coords[8][2] =
			{
				{
					0, 0
				}, {
					1, 0
				}, {
					0, 1
				}, {
					1, 1
				},
				{
					2, 0
				}, {
					2, 1
				}, {
					0, 2
				}, {
					1, 2
				}
			};
			int k;
			int w = it->img->width, h = it->img->height;
			for (k = 0; k < 8; k++)
			{
				int px = sample_coords[k][0] == 0 ? 0 : sample_coords[k][0] == 1 ? w - 1 : w / 2;
				int py = sample_coords[k][1] == 0 ? 0 : sample_coords[k][1] == 1 ? h - 1 : h / 2;
				if (((it->img->rgba[(size_t)py * w + px] >> 24) & 255) * it->color[3] >= 250.0)
					return 1;
			}
		}
	}
	if (!has_translucent)
		return 1;
	return 0;
}

/*
 * Check if a nominally background element contributes visible, non-black pixels
 * over the game screen area [0, 1] x [0, 1].
 *
 * In classic arcade machines, backdrops (e.g. Space Invaders, Boot Hill) physically
 * sit behind transparent or half-silvered mirrors, providing artwork visible within
 * the game screen area.
 *
 * However, many modern MAME layout files define "pseudo-backdrops":
 * 1. Side pillarboxes and widescreen borders (e.g. PSP/GameCube/PC compilations like
 *    flstory, chaknpop, kikikai, retofinv, ncv2) that use <backdrop> tags with an
 *    alphafile mask or transparent cutout (alpha <= 10) over the entire screen area.
 * 2. Elements with additive blend (blend="add") or dark bezels placed before <screen>
 *    (e.g. mk, mk2) where the screen area is pure black (0, 0, 0), which has zero effect
 *    under additive blending (pixel + (0,0,0) = pixel).
 * 3. Ancillary artwork (such as marquees or instruction cards placed before <screen>)
 *    whose bounding box lies completely outside the screen bounds [0, 1] x [0, 1].
 *
 * If this function returns 0, the element does not alter the game screen background
 * and is ignored (dropped from display) in lay_finalize.
 */
static int lay_backdrop_affects_screen(const lay_item *it)
{
	lay_box b = it->bounds;
	int i, j;

	/* If bounding box has no intersection with the screen [0, 1] x [0, 1],
	 * it cannot contribute to the screen background. */
	if (b.x1 <= 0 || b.y1 <= 0 || b.x0 >= 1 || b.y0 >= 1)
		return 0;
	if (it->type == LAY_RECT || it->type == LAY_DISK)
		return it->color[3] > 0.01f && (it->color[0] > 0.01f || it->color[1] > 0.01f || it->color[2] > 0.01f);
	if (!it->img)
		return 0;

	for (j = 0; j < 32; ++j)
	{
		for (i = 0; i < 32; ++i)
		{
			double x = lay_max(0, b.x0) + (lay_min(1, b.x1) - lay_max(0, b.x0)) * ((i + 0.5) / 32.0);
			double y = lay_max(0, b.y0) + (lay_min(1, b.y1) - lay_max(0, b.y0)) * ((j + 0.5) / 32.0);
			double det = it->mat.a * it->mat.e - it->mat.b * it->mat.d;
			double u, v;
			int ix, iy;
			UINT32 px;
			float alpha;

			if (fabs(det) < 1e-15)
				continue;
			x -= it->mat.c;
			y -= it->mat.f;
			u = (it->mat.e * x - it->mat.b * y) / det;
			v = (-it->mat.d * x + it->mat.a * y) / det;
			if (u < 0 || v < 0 || u >= 1 || v >= 1)
				continue;
			ix = (int)(u * it->img->width);
			iy = (int)(v * it->img->height);
			px = it->img->rgba[(size_t)iy * it->img->width + ix];
			alpha = ((px >> 24) & 255) * it->color[3];
			if (alpha > 10.0f)
			{
				float r = ((px >> 16) & 255) * it->color[0];
				float g = ((px >> 8) & 255) * it->color[1];
				float b_comp = (px & 255) * it->color[2];
				if (r > 5.0f || g > 5.0f || b_comp > 5.0f)
					return 1;
			}
		}
	}
	return 0;
}

static int lay_finalize(int mask)
{
	lay_view *v = &L.scene;
	size_t read, write = 0;

	for (read = 0; read < v->count; read++)
	{
		lay_item item = v->items[read];
		if (item.type == LAY_SCREEN)
			continue;
		if (item.type == LAY_IMAGE)
		{
			item.img = lay_image_get(item.file, item.alphafile);
			if (!item.img)
				continue;
		}
		if (!item.legacy)
		{
			if (!item.front || item.blend == LAY_ADD)
				item.category = LAY_BACKDROP;
			else if (item.blend == LAY_MULTIPLY)
				item.category = LAY_OVERLAY;
			else
				item.category = lay_front_opaque(&item) ? LAY_BEZEL : LAY_OVERLAY;
		}

		/*
		 * Ignore pseudo-backdrops when bezel display is not enabled: elements
		 * nominally categorized as backdrops that do not contribute any visible
		 * pixels over the game screen area.
		 */
		if (item.category == LAY_BACKDROP && !(mask & ARTWORK_USE_BEZELS) && !lay_backdrop_affects_screen(&item))
			continue;
		if (item.category == LAY_BACKDROP && !(mask & ARTWORK_USE_BACKDROPS))
			continue;
		if (item.category == LAY_OVERLAY && !(mask & ARTWORK_USE_OVERLAYS))
			continue;
		if (item.category == LAY_BEZEL && !(mask & ARTWORK_USE_BEZELS))
			continue;
		if (item.category == LAY_BACKDROP)
			L.backdrop = 1;
		if (item.category == LAY_OVERLAY)
			L.external_overlay = 1;
		/* Keep legacy layer routing explicit, irrespective of item order. */
		if (item.legacy)
			item.front = (item.category != LAY_BACKDROP);
		v->items[write++] = item;
	}
	v->count = write;
	v->art = (write != 0);
	return 1;
}

void artwork_lay_close(void)
{
	size_t i;

	lay_discard(&L.scene);
	for (i = 0; i < L.image_count; i++)
	{
		free(L.images[i]->rgba);
		free(L.images[i]);
	}
	free(L.images);
	memset(&L, 0, sizeof(L));
}

void artwork_lay_open(const game_driver *driver, const char *view,
	int game_width, int game_height, int mask)
{
	lay_document doc;
	xml_parse_error error;
	xml_parse_options opts;
	mame_file *f;
	xml_data_node *top, *p;
	int status;
	char tmp[LAY_PARAM_LENGTH];

	artwork_lay_close();
	if (view && !strcmp(view, "none"))
		return;
	status = artwork_package_find(driver, &L.package);
	if (status <= 0)
	{
		if (status < 0)
			printf("artwork: unable to locate artwork package\n");
		return;
	}
	f = artwork_package_open(&L.package, "default.lay");
	if (!f)
	{
		printf("artwork: %s: missing/unreadable default.lay\n", L.package.game);
		return;
	}
	memset(&doc, 0, sizeof(doc));
	memset(&opts, 0, sizeof(opts));
	memset(&error, 0, sizeof(error));
	opts.error = &error;
	doc.root = xml_file_read(f, &opts);
	mame_fclose(f);
	if (!doc.root)
	{
		printf("artwork: %s: default.lay: %s at %d:%d\n", L.package.game,
			error.error_message ? error.error_message : "invalid XML", error.error_line, error.error_column);
		return;
	}
	top = lay_child(doc.root, "mamelayout");
	if (!top)
	{
		printf("artwork: %s: missing mamelayout element\n", L.package.game);
		goto cleanup;
	}
	if (xml_get_attribute_int(top, "version", 0) != 2)
	{
		printf("artwork: %s: warning: layout version %d != 2\n", L.package.game, xml_get_attribute_int(top, "version", 0));
	}
	doc.game_width = game_width;
	doc.game_height = game_height;
	snprintf(tmp, sizeof(tmp), "%d", game_width);
	lay_param_set(&doc.global, "scr0width", tmp);
	snprintf(tmp, sizeof(tmp), "%d", game_height);
	lay_param_set(&doc.global, "scr0height", tmp);
	/* Aspect values are populated from the core's screen geometry. */
	snprintf(tmp, sizeof(tmp), "%d", game_width);
	lay_param_set(&doc.global, "scr0physicalxaspect", tmp);
	lay_param_set(&doc.global, "scr0nativexaspect", tmp);
	snprintf(tmp, sizeof(tmp), "%d", game_height);
	lay_param_set(&doc.global, "scr0physicalyaspect", tmp);
	lay_param_set(&doc.global, "scr0nativeyaspect", tmp);
	for (p = top->child; p; p = p->next)
	{
		if (lay_equal(p->name, "param"))
			lay_set_param(p, &doc.global);
		if (lay_equal(p->name, "element") && xml_get_attribute(p, "name"))
			if (!lay_register(&doc.elements, &doc.elements_count, p, &doc.global))
				goto cleanup;
		if (lay_equal(p->name, "group") && xml_get_attribute(p, "name"))
			if (!lay_register(&doc.groups, &doc.groups_count, p, &doc.global))
				goto cleanup;
	}
	if (!lay_select(&doc, view))
		goto cleanup;
	lay_normalise();
	lay_native_orientation(driver->flags & ORIENTATION_MASK);
	L.screen_transform = L.scene.items[L.scene.primary].mat;
	L.screen_swapped = fabs(L.screen_transform.b) > fabs(L.screen_transform.a);
	L.screen_transformed = fabs(L.screen_transform.a - 1) > 1e-8 ||
		fabs(L.screen_transform.b) > 1e-8 ||
		fabs(L.screen_transform.c) > 1e-8 ||
		fabs(L.screen_transform.d) > 1e-8 ||
		fabs(L.screen_transform.e - 1) > 1e-8 ||
		fabs(L.screen_transform.f) > 1e-8;
	if (!lay_finalize(mask))
		goto cleanup;
	L.active = L.scene.art || L.explicit_screen_only || L.screen_transformed;
cleanup:
	free(doc.elements);
	free(doc.groups);
	xml_file_free(doc.root);
}

int artwork_lay_active(void)
{
	return L.active;
}

int artwork_lay_has_backdrop(void)
{
	return L.backdrop;
}

int artwork_lay_has_overlay(void)
{
	return L.external_overlay;
}

int artwork_lay_screen_only(void)
{
	return L.explicit_screen_only;
}

int artwork_lay_has_art(void)
{
	return L.scene.art;
}

int artwork_lay_screen_swapped(void)
{
	return L.active && L.screen_swapped;
}

int artwork_lay_screen_transformed(void)
{
	return L.active && L.screen_transformed;
}

void artwork_lay_screen_matrix(double m[6])
{
	m[0] = L.screen_transform.a;
	m[1] = L.screen_transform.b;
	m[2] = L.screen_transform.c;
	m[3] = L.screen_transform.d;
	m[4] = L.screen_transform.e;
	m[5] = L.screen_transform.f;
}

void artwork_lay_extents(artwork_lay_range *out)
{
	out->left = out->top = 0;
	out->right = out->bottom = 1;
	if (!L.active)
		return;
	out->left = L.scene.extents.x0;
	out->right = L.scene.extents.x1;
	out->top = L.scene.extents.y0;
	out->bottom = L.scene.extents.y1;
}

/*
 * Filtering in premultiplied alpha prevents colour fringing when resizing
 * transparent artwork.  Area average is used for downscaling and bilinear for
 * upscaling, matching the legacy artwork scaler's choice of filter.
 */
static UINT32 lay_filter_image(const lay_image *img, double u, double v, double footprint_u, double footprint_v)
{
	double x = u * img->width - 0.5, y = v * img->height - 0.5;

	if (footprint_u > 1.001 / img->width || footprint_v > 1.001 / img->height)
	{
		double x0 = u * img->width - footprint_u * img->width / 2;
		double y0 = v * img->height - footprint_v * img->height / 2;
		double x1 = u * img->width + footprint_u * img->width / 2;
		double y1 = v * img->height + footprint_v * img->height / 2;
		int ix0 = (int)floor(x0), ix1 = (int)ceil(x1), iy0 = (int)floor(y0), iy1 = (int)ceil(y1);
		int ix, iy;
		double s = 0, rd = 0, gd = 0, bd = 0, ad = 0;
		for (iy = iy0; iy < iy1; iy++)
		{
			double wy = lay_min(y1, iy + 1) - lay_max(y0, iy);
			if (wy <= 0)
				continue;
			for (ix = ix0; ix < ix1; ix++)
			{
				double wx = lay_min(x1, ix + 1) - lay_max(x0, ix), wgt, alpha;
				UINT32 c;
				int px, py;
				if (wx <= 0)
					continue;
				wgt = wx * wy;
				px = ix < 0 ? 0 : ix >= img->width ? img->width - 1 : ix;
				py = iy < 0 ? 0 : iy >= img->height ? img->height - 1 : iy;
				c = img->rgba[(size_t)py * img->width + px];
				alpha = ((c >> 24) & 255) / 255.0;
				rd += wgt * alpha * ((c >> 16) & 255);
				gd += wgt * alpha * ((c >> 8) & 255);
				bd += wgt * alpha * (c & 255);
				ad += wgt * alpha;
				s += wgt;
			}
		}
		if (s <= 0 || ad <= 0)
			return 0;
		return lay_rgba(lay_clamp((int)(255 * ad / s + 0.5)),
			       lay_clamp((int)(rd / ad + 0.5)),
			       lay_clamp((int)(gd / ad + 0.5)),
			       lay_clamp((int)(bd / ad + 0.5)));
	}
	else
	{
		int x0 = (int)floor(x), y0 = (int)floor(y), i, j;
		double ax = x - x0, ay = y - y0, rd = 0, gd = 0, bd = 0, ad = 0;
		for (j = 0; j < 2; j++)
			for (i = 0; i < 2; i++)
			{
				int px = x0 + i, py = y0 + j;
				double wgt = (i ? ax : 1 - ax) * (j ? ay : 1 - ay);
				UINT32 c;
				double alpha;
				px = px < 0 ? 0 : px >= img->width ? img->width - 1 : px;
				py = py < 0 ? 0 : py >= img->height ? img->height - 1 : py;
				c = img->rgba[(size_t)py * img->width + px];
				alpha = ((c >> 24) & 255) / 255.0;
				rd += wgt * alpha * ((c >> 16) & 255);
				gd += wgt * alpha * ((c >> 8) & 255);
				bd += wgt * alpha * (c & 255);
				ad += wgt * alpha;
			}
		if (ad <= 0)
			return 0;
		return lay_rgba(lay_clamp((int)(255 * ad + 0.5)),
			       lay_clamp((int)(rd / ad + 0.5)),
			       lay_clamp((int)(gd / ad + 0.5)),
			       lay_clamp((int)(bd / ad + 0.5)));
	}
}

int artwork_lay_prepare(int width, int height, const rectangle *game)
{
	size_t i;

	if (!L.active)
		return 1;
	L.display_width = width;
	L.display_height = height;
	L.game_rect = *game;
	for (i = 0; i < L.scene.count; i++)
	{
		lay_item *it = &L.scene.items[i];
		double gw = (double)(game->max_x - game->min_x + 1);
		double gh = (double)(game->max_y - game->min_y + 1);
		lay_matrix m = it->mat;
		lay_box b;
		double det, inv_a, inv_b, inv_d, inv_e, fu, fv;
		int x, y;
		free(it->pixels);
		it->pixels = NULL;
		it->width = it->height = 0;
		m.a *= gw;
		m.b *= gw;
		m.c = game->min_x + m.c * gw;
		m.d *= gh;
		m.e *= gh;
		m.f = game->min_y + m.f * gh;
		b = lay_mat_box(m);
		if (!isfinite(b.x0) || !isfinite(b.x1) || !isfinite(b.y0) || !isfinite(b.y1) ||
			b.x1 <= 0 || b.y1 <= 0 || b.x0 >= width || b.y0 >= height)
			continue;
		it->px0 = (int)lay_max(0, floor(b.x0));
		it->py0 = (int)lay_max(0, floor(b.y0));
		if (b.x1 <= it->px0 || b.y1 <= it->py0)
			continue;
		it->width = (int)(lay_min(width, ceil(b.x1)) - it->px0);
		it->height = (int)(lay_min(height, ceil(b.y1)) - it->py0);
		if (it->width <= 0 || it->height <= 0)
		{
			it->width = it->height = 0;
			continue;
		}
		if ((size_t)it->width > SIZE_MAX / (size_t)it->height / 4)
		{
			printf("artwork: image dimensions overflow\n");
			return 0;
		}
		it->pixels = (UINT32*)malloc((size_t)it->width * it->height * sizeof(UINT32));
		if (!it->pixels)
		{
			printf("artwork: out of memory preparing image\n");
			return 0;
		}
		det = m.a * m.e - m.b * m.d;
		if (fabs(det) < 1e-15)
		{
			memset(it->pixels, 0, (size_t)it->width * it->height * 4);
			continue;
		}
		inv_a = m.e / det;
		inv_b = -m.b / det;
		inv_d = -m.d / det;
		inv_e = m.a / det;
		fu = fabs(inv_a) + fabs(inv_b);
		fv = fabs(inv_d) + fabs(inv_e);
		for (y = 0; y < it->height; y++)
			for (x = 0; x < it->width; x++)
			{
				double xx = it->px0 + x + 0.5 - m.c, yy = it->py0 + y + 0.5 - m.f;
				double u = inv_a * xx + inv_b * yy, v = inv_d * xx + inv_e * yy;
				UINT32 c = 0;
				int r, g, bb, a;
				if (u >= 0 && v >= 0 && u < 1 && v < 1)
				{
					if (it->type == LAY_IMAGE)
						c = lay_filter_image(it->img, u, v, fu, fv);
					else if (it->type == LAY_RECT)
						c = 0xffffffff;
					else if (it->type == LAY_DISK)
					{
						double dx = 2 * u - 1, dy = 2 * v - 1;
						double edge = 1 - (dx * dx + dy * dy);
						int coverage = lay_clamp((int)(255 * lay_unit(edge * lay_max(it->width, it->height) / 4 + 0.5)));
						c = lay_rgba(coverage, 255, 255, 255);
					}
				}
				a = lay_clamp((int)(((c >> 24) & 255) * it->color[3] + 0.5));
				r = lay_clamp((int)(((c >> 16) & 255) * it->color[0] + 0.5));
				g = lay_clamp((int)(((c >> 8) & 255) * it->color[1] + 0.5));
				bb = lay_clamp((int)((c & 255) * it->color[2] + 0.5));
				it->pixels[(size_t)y * it->width + x] = lay_rgba(a, r, g, bb);
			}
	}
	return 1;
}

static void lay_blend_item(mame_bitmap *dest, const lay_item *it, unsigned rsh, unsigned gsh, unsigned bsh, unsigned ash)
{
	int x, y;

	if (!it->pixels || it->width <= 0 || it->height <= 0)
		return;
	for (y = 0; y < it->height; y++)
	{
		UINT32 *row = (UINT32*)dest->line[it->py0 + y];
		const UINT32 *src = it->pixels + (size_t)y * it->width;
		for (x = 0; x < it->width; x++)
		{
			unsigned a = (src[x] >> 24) & 255;
			unsigned sr = (src[x] >> 16) & 255, sg = (src[x] >> 8) & 255, sb = src[x] & 255;
			unsigned dr, dg, db, da, or_, og, ob, oa;
			UINT32 d;
			if (!a)
				continue;
			d = row[it->px0 + x];
			dr = (d >> rsh) & 255;
			dg = (d >> gsh) & 255;
			db = (d >> bsh) & 255;
			/* Existing renderer stores alpha in the remaining channel; the
			 * packed RGB layout is runtime-specific and is passed by caller. */
			da = (d >> ash) & 255;
			oa = a + (da * (255 - a) + 127) / 255;
			if (it->blend == LAY_NONE)
			{
				or_ = sr;
				og = sg;
				ob = sb;
			}
			else if (it->blend == LAY_MULTIPLY)
			{
				or_ = dr * (255 - a) / 255 + (dr * sr / 255) * a / 255;
				og = dg * (255 - a) / 255 + (dg * sg / 255) * a / 255;
				ob = db * (255 - a) / 255 + (db * sb / 255) * a / 255;
			}
			else if (it->blend == LAY_ADD)
			{
				or_ = dr + sr * a / 255;
				og = dg + sg * a / 255;
				ob = db + sb * a / 255;
			}
			else
			{
				or_ = (sr * a + dr * (255 - a) + 127) / 255;
				og = (sg * a + dg * (255 - a) + 127) / 255;
				ob = (sb * a + db * (255 - a) + 127) / 255;
			}
			(void)da;
			(void)oa;
			row[it->px0 + x] = (d & ~(0xffUL << rsh | 0xffUL << gsh | 0xffUL << bsh | 0xffUL << ash)) |
				((UINT32)lay_clamp((int)oa) << ash) |
				((UINT32)lay_clamp((int)or_) << rsh) |
				((UINT32)lay_clamp((int)og) << gsh) |
				((UINT32)lay_clamp((int)ob) << bsh);
		}
	}
}

void artwork_lay_draw(mame_bitmap *dest, int pass, unsigned rsh, unsigned gsh, unsigned bsh, unsigned ash)
{
	size_t i;

	if (!L.active)
		return;
	/* Layout items are composed in document order, from back to front. */
	for (i = 0; i < L.scene.count; ++i)
	{
		lay_item *it = &L.scene.items[i];
		if (it->front == pass)
			lay_blend_item(dest, it, rsh, gsh, bsh, ash);
	}
}

