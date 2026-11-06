#!/usr/bin/env python3
# license:BSD-3-Clause
# copyright-holders:Andrea Mazzoleni
"""
Filter artwork packages (.zip) containing backdrop and/or overlay layers,
using the layout resolution and layer detection logic of AdvanceMAME.

This script inspects arcade artwork packages (.zip archives) containing
a 'default.lay' layout description. It parses layout views, expands groups,
repeats and parameters, selects the primary game view, and classifies
each graphic element (backdrop, overlay, or bezel) according to its position
relative to the screen, blending mode, and alpha channel characteristics.

Packages that only contain bezels, marquees, or control panels are filtered
out by default, printing only the paths of archives that contain backdrops
and/or overlays to standard output (one per line).

Usage:
    artwork_filter.py [options] <zipfile> [<zipfile> ...]

Positional Arguments:
    zipfiles             One or more artwork .zip archives to inspect.

Options:
    --backdrop           Output only archives containing at least one backdrop.
    --overlay            Output only archives containing at least one overlay.
                         (If neither --backdrop nor --overlay is specified,
                         archives with backdrop, overlay, or both are returned).
    --details            Display detailed information for matching archives:
                         the file path, detected layer types (backdrop, overlay),
                         and the name of the evaluated view.
    --view <name>        Force evaluation of a specific view name rather than
                         using automatic view selection ('auto').
    -h, --help           Show help message and exit.

Examples:
    # Print all artwork packages containing backdrops or overlays:
    ./support/artwork_filter.py /extra/advmame/artwork/*.zip

    # Print only packages containing overlays:
    ./support/artwork_filter.py --overlay /extra/advmame/artwork/*.zip

    # Print details for a set of archives:
    ./support/artwork_filter.py --details /extra/advmame/artwork/*.zip
"""

import argparse
import io
import math
import os
import sys
import xml.etree.ElementTree as ET
import zipfile
from PIL import Image

LAY_BACKDROP = 1
LAY_OVERLAY = 2
LAY_BEZEL = 3
LAY_SCREEN = 4


class Matrix:
	__slots__ = ("a", "b", "c", "d", "e", "f")

	def __init__(self, a=1.0, b=0.0, c=0.0, d=0.0, e=1.0, f=0.0):
		self.a = float(a)
		self.b = float(b)
		self.c = float(c)
		self.d = float(d)
		self.e = float(e)
		self.f = float(f)

	def mul(self, o):
		return Matrix(
			self.a * o.a + self.b * o.d,
			self.a * o.b + self.b * o.e,
			self.a * o.c + self.b * o.f + self.c,
			self.d * o.a + self.e * o.d,
			self.d * o.b + self.e * o.e,
			self.d * o.c + self.e * o.f + self.f,
		)

	def map_point(self, x, y):
		return (
			self.a * x + self.b * y + self.c,
			self.d * x + self.e * y + self.f,
		)


def rect_matrix(x0, y0, x1, y1):
	return Matrix(x1 - x0, 0.0, x0, 0.0, y1 - y0, y0)


def mat_box(m):
	pts = [
		m.map_point(0.0, 0.0),
		m.map_point(1.0, 0.0),
		m.map_point(0.0, 1.0),
		m.map_point(1.0, 1.0),
	]
	xs = [p[0] for p in pts]
	ys = [p[1] for p in pts]
	return (min(xs), min(ys), max(xs), max(ys))


def box_union(b1, b2):
	return (
		min(b1[0], b2[0]),
		min(b1[1], b2[1]),
		max(b1[2], b2[2]),
		max(b1[3], b2[3]),
	)


class LayItem:
	__slots__ = (
		"item_type",
		"category",
		"blend",
		"legacy",
		"front",
		"screen",
		"mat",
		"bounds",
		"color",
		"file",
		"alphafile",
	)

	def __init__(self):
		self.item_type = None
		self.category = None
		self.blend = "alpha"
		self.legacy = 0
		self.front = False
		self.screen = 0
		self.mat = Matrix()
		self.bounds = (0.0, 0.0, 1.0, 1.0)
		self.color = [1.0, 1.0, 1.0, 1.0]
		self.file = ""
		self.alphafile = ""


class LayView:
	__slots__ = ("name", "items", "screens", "art", "primary", "valid", "screenbox")

	def __init__(self, name):
		self.name = name
		self.items = []
		self.screens = 0
		self.art = False
		self.primary = -1
		self.valid = False
		self.screenbox = (0.0, 0.0, 1.0, 1.0)


def expand_params(text, env):
	if not text or "~" not in text:
		return text or ""
	result = text
	for _ in range(5):
		substituted = False
		p = 0
		out = []
		while p < len(result):
			if result[p] == "~":
				end = result.find("~", p + 1)
				if end != -1 and (end - p - 1) < 64:
					key = result[p + 1:end]
					val = env.get(key)
					if val is not None:
						out.append(str(val))
						p = end + 1
						substituted = True
						continue
			out.append(result[p])
			p += 1
		result = "".join(out)
		if not substituted:
			break
	return result


def get_attr(node, key, env, default=""):
	raw = node.get(key)
	if raw is None:
		return default
	return expand_params(raw, env)


def get_num(node, key, env, default=0.0):
	val = get_attr(node, key, env, "")
	if not val:
		return default
	try:
		v = float(val)
		return v if math.isfinite(v) else default
	except ValueError:
		return default


def get_int(node, key, env, default=0):
	val = get_attr(node, key, env, "")
	if not val:
		return default
	try:
		if val.startswith("$"):
			return int(val[1:], 16)
		if val.startswith("#"):
			return int(val[1:])
		return int(val, 0)
	except ValueError:
		return default


def get_bool(node, key, env, default=False):
	val = get_attr(node, key, env, "")
	if not val:
		return default
	val = val.lower()
	return val in ("yes", "true", "1")


def get_bounds(node, env, default=(0.0, 0.0, 1.0, 1.0)):
	b = node.find("bounds")
	if b is not None:
		if "left" in b.attrib:
			left = get_num(b, "left", env, 0.0)
			top = get_num(b, "top", env, 0.0)
			right = get_num(b, "right", env, 1.0)
			bottom = get_num(b, "bottom", env, 1.0)
			return (left, top, right, bottom)
		x = get_num(b, "x", env, 0.0)
		y = get_num(b, "y", env, 0.0)
		w = get_num(b, "width", env, 1.0)
		h = get_num(b, "height", env, 1.0)
		return (x, y, x + w, y + h)
	return default


def get_orientation(node, env):
	o = node.find("orientation")
	mat = Matrix()
	if o is None:
		return mat
	angle = get_int(o, "rotate", env, 0)
	swap = get_bool(o, "swapxy", env, False)
	fx = get_bool(o, "flipx", env, False)
	fy = get_bool(o, "flipy", env, False)
	if angle == 90:
		mat = Matrix(0.0, -1.0, 1.0, 1.0, 0.0, 0.0).mul(mat)
	elif angle == 180:
		mat = Matrix(-1.0, 0.0, 1.0, 0.0, -1.0, 1.0).mul(mat)
	elif angle == 270:
		mat = Matrix(0.0, 1.0, 0.0, -1.0, 0.0, 1.0).mul(mat)
	if swap:
		mat = Matrix(0.0, 1.0, 0.0, 1.0, 0.0, 0.0).mul(mat)
	if fx:
		mat = Matrix(-1.0, 0.0, 1.0, 0.0, 1.0, 0.0).mul(mat)
	if fy:
		mat = Matrix(1.0, 0.0, 0.0, 0.0, -1.0, 1.0).mul(mat)
	return mat


def apply_color(node, env, color):
	c = node.find("color")
	if c is None:
		return
	for idx, name in enumerate(("red", "green", "blue", "alpha")):
		val = get_num(c, name, env, 1.0)
		val = max(0.0, min(1.0, val))
		color[idx] *= val


def is_component_enabled(node, env, state):
	have_state = "state" in node.attrib
	have_mask = "statemask" in node.attrib
	s = get_int(node, "state", env, 0)
	mask = get_int(node, "statemask", env, 0)
	if have_mask and not have_state:
		return (state & mask) == mask
	if have_mask:
		return (state & mask) == (s & mask)
	return (not have_state) or (state == s)


def get_element_defbox(element_node, env):
	found = False
	box = (0.0, 0.0, 1.0, 1.0)
	for ch in element_node:
		if ch.tag in ("image", "rect", "disk"):
			b = get_bounds(ch, env, (0.0, 0.0, 1.0, 1.0))
			if not found:
				box = b
				found = True
			else:
				box = box_union(box, b)
	return box


def emit_element(elements, view, node, env, parent_mat, legacy, depth):
	ref_attr = "element" if legacy else "ref"
	ref = get_attr(node, ref_attr, env, "")
	if not ref or ref not in elements:
		return
	def_node = elements[ref]
	inst_box = get_bounds(node, env, (0.0, 0.0, 1.0, 1.0))
	if inst_box[2] <= inst_box[0] or inst_box[3] <= inst_box[1]:
		return
	orient = get_orientation(node, env)
	map_mat = parent_mat.mul(rect_matrix(*inst_box).mul(orient))

	color = [1.0, 1.0, 1.0, 1.0]
	apply_color(node, env, color)

	state = get_int(def_node, "defstate", env, 0)
	default_blend = env.get("__blend__", "alpha")
	blend = get_attr(node, "blend", env, default_blend).lower()
	if blend not in ("multiply", "add", "none"):
		blend = "alpha"

	defbox = get_element_defbox(def_node, env)
	dw = defbox[2] - defbox[0]
	dh = defbox[3] - defbox[1]
	if dw <= 0 or dh <= 0:
		return

	for ch in def_node:
		primitive = ch.tag
		if primitive not in ("image", "rect", "disk"):
			continue
		if not is_component_enabled(ch, env, state):
			continue
		cb = get_bounds(ch, env, (0.0, 0.0, 1.0, 1.0))
		norm = (
			(cb[0] - defbox[0]) / dw,
			(cb[1] - defbox[1]) / dh,
			(cb[2] - defbox[0]) / dw,
			(cb[3] - defbox[1]) / dh,
		)
		item = LayItem()
		item.item_type = primitive
		item.blend = blend
		item.legacy = legacy
		item.category = legacy
		item.mat = map_mat.mul(rect_matrix(*norm))
		item.bounds = mat_box(item.mat)
		item.color = list(color)
		apply_color(ch, env, item.color)
		if primitive == "image":
			item.file = get_attr(ch, "file", env, "")
			item.alphafile = get_attr(ch, "alphafile", env, "")
			if not item.file:
				continue
		view.items.append(item)
		view.art = True


def walk_view(elements, groups, view, node, env, mat, depth=0):
	if depth >= 24:
		return
	for p in node:
		if p.tag == "param":
			name = get_attr(p, "name", env, "")
			val = get_attr(p, "value", env, get_attr(p, "start", env, ""))
			if name:
				env[name] = val
			continue
		if p.tag in ("bounds", "color", "orientation", "script"):
			continue
		if p.tag == "collection":
			if get_bool(p, "visible", env, True):
				sub_env = dict(env)
				walk_view(elements, groups, view, p, sub_env, mat, depth + 1)
			continue
		if p.tag == "repeat":
			count = get_int(p, "count", env, 0)
			if count <= 0 or count > 65536:
				continue
			gen_env = dict(env)
			for ch in p:
				if ch.tag == "param" and "start" in ch.attrib:
					k = get_attr(ch, "name", gen_env, "")
					v = get_attr(ch, "start", gen_env, "")
					if k:
						gen_env[k] = v
			for _ in range(count):
				iter_env = dict(gen_env)
				for ch in p:
					if ch.tag == "param":
						if "start" not in ch.attrib:
							k = get_attr(ch, "name", iter_env, "")
							v = get_attr(ch, "value", iter_env, "")
							if k:
								iter_env[k] = v
					else:
						walk_view(elements, groups, view, [ch], iter_env, mat, depth + 1)
				for ch in p:
					if ch.tag == "param" and "start" in ch.attrib:
						k = get_attr(ch, "name", gen_env, "")
						val = gen_env.get(k, "")
						if not val:
							continue
						try:
							d = float(val) + get_num(ch, "increment", gen_env, 0.0)
							shift = get_int(ch, "lshift", gen_env, 0) - get_int(ch, "rshift", gen_env, 0)
							if shift:
								v = int(d)
								if shift > 0:
									v <<= shift
								else:
									v >>= -shift
								gen_env[k] = str(v)
							else:
								gen_env[k] = f"{d:.15g}"
						except (ValueError, OverflowError):
							pass
			continue
		if p.tag == "group":
			ref = get_attr(p, "ref", env, "")
			if ref not in groups:
				continue
			def_group = groups[ref]
			m = mat
			if p.find("bounds") is not None:
				target = get_bounds(p, env, (0.0, 0.0, 1.0, 1.0))
				temp_view = LayView("temp")
				walk_view(elements, groups, temp_view, def_group, dict(env), Matrix(), depth + 1)
				if temp_view.items:
					bbox = temp_view.items[0].bounds
					for it in temp_view.items[1:]:
						bbox = box_union(bbox, it.bounds)
					bw = bbox[2] - bbox[0]
					bh = bbox[3] - bbox[1]
					if bw > 0 and bh > 0:
						norm = Matrix(1.0 / bw, 0.0, -bbox[0] / bw, 0.0, 1.0 / bh, -bbox[1] / bh)
						m = mat.mul(rect_matrix(*target).mul(get_orientation(p, env).mul(norm)))
			else:
				m = mat.mul(get_orientation(p, env))

			start_idx = len(view.items)
			sub_env = dict(env)
			grp_blend = get_attr(p, "blend", env, get_attr(def_group, "blend", env, env.get("__blend__", "")))
			if grp_blend:
				sub_env["__blend__"] = grp_blend
			walk_view(elements, groups, view, def_group, sub_env, m, depth + 1)
			tint = [1.0, 1.0, 1.0, 1.0]
			apply_color(def_group, env, tint)
			apply_color(p, env, tint)
			for i in range(start_idx, len(view.items)):
				for c in range(4):
					view.items[i].color[c] *= tint[c]
			continue
		if p.tag == "screen":
			item = LayItem()
			item.item_type = "screen"
			item.category = LAY_SCREEN
			item.screen = get_int(p, "index", env, 0)
			b = get_bounds(p, env, (0.0, 0.0, 1.0, 1.0))
			if b[2] <= b[0] or b[3] <= b[1]:
				continue
			item.mat = mat.mul(rect_matrix(*b).mul(get_orientation(p, env)))
			item.bounds = mat_box(item.mat)
			view.items.append(item)
			view.screens += 1
			continue
		if p.tag in ("element", "backdrop", "overlay", "bezel", "cpanel", "marquee"):
			legacy = 0
			if p.tag == "backdrop":
				legacy = LAY_BACKDROP
			elif p.tag == "overlay":
				legacy = LAY_OVERLAY
			elif p.tag in ("bezel", "cpanel", "marquee"):
				legacy = LAY_BEZEL
			emit_element(elements, view, p, env, mat, legacy, depth + 1)


def read_zip_member(z, filename):
	filename_clean = filename.replace("\\", "/")
	try:
		return z.read(filename_clean)
	except KeyError:
		pass
	# Case-insensitive search
	fl = filename_clean.lower()
	for name in z.namelist():
		if name.lower() == fl:
			try:
				return z.read(name)
			except Exception:
				break
	return None


def get_image_rgba(z, filename, alphafile, cache):
	key = (filename, alphafile)
	if key in cache:
		return cache[key]
	data = read_zip_member(z, filename)
	if not data:
		cache[key] = None
		return None
	try:
		img = Image.open(io.BytesIO(data)).convert("RGBA")
	except Exception:
		cache[key] = None
		return None

	if alphafile:
		adata = read_zip_member(z, alphafile)
		if adata:
			try:
				aimg = Image.open(io.BytesIO(adata)).convert("RGB")
				if aimg.size == img.size:
					ar, ag, ab = aimg.split()
					# ITU luminance: (299*r + 587*g + 114*b + 500) // 1000
					mask = aimg.convert("L")
					img.putalpha(mask)
			except Exception:
				pass
	cache[key] = img
	return img


def check_front_opaque(item, z, img_cache):
	bx0, by0, bx1, by1 = item.bounds
	if bx1 <= 0.0 or by1 <= 0.0 or bx0 >= 1.0 or by0 >= 1.0:
		return True
	if item.item_type in ("rect", "disk"):
		return item.color[3] >= 0.999
	if item.item_type != "image" or not item.file:
		return False

	img = get_image_rgba(z, item.file, item.alphafile, img_cache)
	if img is None:
		return False

	width, height = img.size
	det = item.mat.a * item.mat.e - item.mat.b * item.mat.d
	if abs(det) < 1e-15:
		return True

	ix0 = max(0.0, bx0)
	ix1 = min(1.0, bx1)
	iy0 = max(0.0, by0)
	iy1 = min(1.0, by1)

	has_opaque_in_screen = False
	has_trans_in_screen = False
	has_translucent_in_screen = False

	for j in range(32):
		y = iy0 + (iy1 - iy0) * ((j + 0.5) / 32.0)
		for i in range(32):
			x = ix0 + (ix1 - ix0) * ((i + 0.5) / 32.0)
			xx = x - item.mat.c
			yy = y - item.mat.f
			u = (item.mat.e * xx - item.mat.b * yy) / det
			v = (-item.mat.d * xx + item.mat.a * yy) / det
			if 0.0 <= u < 1.0 and 0.0 <= v < 1.0:
				px = min(max(0, int(u * width)), width - 1)
				py = min(max(0, int(v * height)), height - 1)
				alpha = img.getpixel((px, py))[3] * item.color[3]
				if alpha >= 254.5:
					has_opaque_in_screen = True
				elif alpha <= 10.0:
					has_trans_in_screen = True
				else:
					has_translucent_in_screen = True

	if has_opaque_in_screen:
		return True

	if has_trans_in_screen:
		if bx0 < 0.0 or bx1 > 1.0 or by0 < 0.0 or by1 > 1.0:
			return True
		corners = [
			(0, 0),
			(width - 1, 0),
			(0, height - 1),
			(width - 1, height - 1),
			(width // 2, 0),
			(width // 2, height - 1),
			(0, height // 2),
			(width - 1, height // 2),
		]
		for cx, cy in corners:
			if img.getpixel((cx, cy))[3] * item.color[3] >= 250.0:
				return True

	if not has_translucent_in_screen:
		return True

	return False


def check_item_affects_screen(item, z, img_cache):
	bx0, by0, bx1, by1 = item.bounds
	if bx1 <= 0.0 or by1 <= 0.0 or bx0 >= 1.0 or by0 >= 1.0:
		return False
	if item.item_type in ("rect", "disk"):
		r, g, b, a = item.color
		if a > 0.01 and (r > 0.01 or g > 0.01 or b > 0.01):
			return True
		return False
	if item.item_type != "image" or not item.file:
		return False

	img = get_image_rgba(z, item.file, item.alphafile, img_cache)
	if img is None:
		return False

	width, height = img.size
	det = item.mat.a * item.mat.e - item.mat.b * item.mat.d
	if abs(det) < 1e-15:
		return False

	ix0 = max(0.0, bx0)
	ix1 = min(1.0, bx1)
	iy0 = max(0.0, by0)
	iy1 = min(1.0, by1)

	for j in range(32):
		y = iy0 + (iy1 - iy0) * ((j + 0.5) / 32.0)
		for i in range(32):
			x = ix0 + (ix1 - ix0) * ((i + 0.5) / 32.0)
			xx = x - item.mat.c
			yy = y - item.mat.f
			u = (item.mat.e * xx - item.mat.b * yy) / det
			v = (-item.mat.d * xx + item.mat.a * yy) / det
			if 0.0 <= u < 1.0 and 0.0 <= v < 1.0:
				px = min(max(0, int(u * width)), width - 1)
				py = min(max(0, int(v * height)), height - 1)
				pixel = img.getpixel((px, py))
				alpha = pixel[3] * item.color[3]
				if alpha > 10.0:
					r = pixel[0] * item.color[0]
					g = pixel[1] * item.color[1]
					b = pixel[2] * item.color[2]
					if r > 5.0 or g > 5.0 or b > 5.0:
						return True
	return False


def analyze_layout_zip(zip_path, requested_view="auto"):
	if not os.path.isfile(zip_path):
		return None
	try:
		with zipfile.ZipFile(zip_path, "r") as z:
			lay_data = read_zip_member(z, "default.lay")
			if not lay_data:
				return None
			try:
				root = ET.fromstring(lay_data)
			except ET.ParseError:
				return None

			if root.tag != "mamelayout":
				return None

			global_env = {
				"scr0width": "640",
				"scr0height": "480",
				"scr0physicalxaspect": "4",
				"scr0physicalyaspect": "3",
				"scr0nativexaspect": "4",
				"scr0nativeyaspect": "3",
			}
			elements = {}
			groups = {}

			for child in root:
				if child.tag == "param":
					k = child.get("name")
					v = child.get("value") or child.get("start")
					if k and v:
						global_env[k] = v
				elif child.tag == "element" and "name" in child.attrib:
					elements[child.attrib["name"]] = child
				elif child.tag == "group" and "name" in child.attrib:
					groups[child.attrib["name"]] = child

			candidates = []
			for child in root:
				if child.tag != "view":
					continue
				view_name = get_attr(child, "name", global_env, "")
				v_env = dict(global_env)
				v_env["viewname"] = view_name
				view = LayView(view_name)
				walk_view(elements, groups, view, child, v_env, Matrix(), 0)

				view.primary = -1
				for idx, it in enumerate(view.items):
					if it.item_type == "screen" and it.screen == 0:
						view.primary = idx
						break
				if view.primary < 0:
					for idx, it in enumerate(view.items):
						if it.item_type == "screen":
							view.primary = idx
							break
				if view.primary >= 0:
					view.screenbox = view.items[view.primary].bounds
					sw = view.screenbox[2] - view.screenbox[0]
					sh = view.screenbox[3] - view.screenbox[1]
					if sw > 0 and sh > 0:
						view.valid = True

				# rank
				if not view.valid:
					rank = 99
				elif view.screens == 1:
					rank = 0 if view.art else 1
				else:
					rank = 2 if view.art else 3

				candidates.append((rank, view))

			if not candidates:
				return None

			chosen_view = None
			is_named = requested_view and requested_view not in ("auto", "none")
			if is_named:
				for rank, v in candidates:
					if v.name == requested_view and rank < 99:
						chosen_view = v
						break

			if chosen_view is None:
				best_rank = 99
				for rank, v in candidates:
					if rank < best_rank:
						best_rank = rank
						chosen_view = v

			if chosen_view is None or not chosen_view.valid:
				return None

			# Normalise items relative to screen
			s = chosen_view.screenbox
			sw = s[2] - s[0]
			sh = s[3] - s[1]
			for idx, it in enumerate(chosen_view.items):
				it.front = (idx > chosen_view.primary)
				it.mat.c = (it.mat.c - s[0]) / sw
				it.mat.a /= sw
				it.mat.b /= sw
				it.mat.f = (it.mat.f - s[1]) / sh
				it.mat.d /= sh
				it.mat.e /= sh
				it.bounds = mat_box(it.mat)

			has_backdrop = False
			has_backdrop_overlap = False
			has_backdrop_screen_pixels = False
			has_overlay = False
			has_overlay_overlap = False
			img_cache = {}
			warnings = []

			for it in chosen_view.items:
				if it.item_type == "screen":
					continue
				if it.color[3] <= 0.001:
					continue
				if not it.legacy:
					if not it.front or it.blend == "add":
						it.category = LAY_BACKDROP
					elif it.blend == "multiply":
						it.category = LAY_OVERLAY
					else:
						it.category = LAY_BEZEL if check_front_opaque(it, z, img_cache) else LAY_OVERLAY

				if it.category == LAY_BACKDROP:
					if check_item_affects_screen(it, z, img_cache):
						has_backdrop = True
					else:
						it.category = LAY_BEZEL
						it.front = True
						bx0, by0, bx1, by1 = it.bounds
						outside = (bx1 <= 0.0 or by1 <= 0.0 or bx0 >= 1.0 or by0 >= 1.0)
						if outside:
							warnings.append(
								f"backdrop in view '{chosen_view.name}' does not overlap screen (treated as bezel)"
							)
						else:
							warnings.append(
								f"backdrop in view '{chosen_view.name}' does not affect screen area (transparent or black cutout, treated as bezel)"
							)
				elif it.category == LAY_OVERLAY:
					has_overlay = True
					bx0, by0, bx1, by1 = it.bounds
					outside_screen = (bx1 <= 0.0 or by1 <= 0.0 or bx0 >= 1.0 or by0 >= 1.0)
					if not outside_screen:
						has_overlay_overlap = True

			if has_overlay and not has_overlay_overlap:
				warnings.append(
					f"overlay in view '{chosen_view.name}' does not overlap screen (possible layout error)"
				)

			return {
				"view": chosen_view.name,
				"has_backdrop": has_backdrop,
				"has_overlay": has_overlay,
				"warnings": warnings,
			}
	except zipfile.BadZipFile:
		return None
	except Exception:
		return None


def main():
	parser = argparse.ArgumentParser(
		description="Filter artwork zip packages containing backdrop and/or overlay layers."
	)
	parser.add_argument("zipfiles", nargs="+", help="Artwork .zip files to analyze")
	parser.add_argument(
		"--backdrop",
		action="store_true",
		help="Filter only packages with backdrop",
	)
	parser.add_argument(
		"--overlay",
		action="store_true",
		help="Filter only packages with overlay",
	)
	parser.add_argument(
		"--details",
		action="store_true",
		help="Show details (view name, backdrop, overlay status)",
	)
	parser.add_argument(
		"--view",
		default="auto",
		help="Select view name instead of 'auto'",
	)
	args = parser.parse_args()

	filter_backdrop = args.backdrop
	filter_overlay = args.overlay
	if not filter_backdrop and not filter_overlay:
		filter_backdrop = True
		filter_overlay = True

	for path in args.zipfiles:
		info = analyze_layout_zip(path, args.view)
		if not info:
			continue

		for w in info.get("warnings", []):
			sys.stderr.write(f"warning: {path}: {w}\n")

		match = False
		if filter_backdrop and filter_overlay:
			match = info["has_backdrop"] or info["has_overlay"]
		elif filter_backdrop:
			match = info["has_backdrop"]
		elif filter_overlay:
			match = info["has_overlay"]

		if match:
			if args.details:
				tags = []
				if info["has_backdrop"]:
					tags.append("backdrop")
				if info["has_overlay"]:
					tags.append("overlay")
				print(f"{path}: {', '.join(tags)} (view: {info['view']!r})")
			else:
				print(path)


if __name__ == "__main__":
	main()
