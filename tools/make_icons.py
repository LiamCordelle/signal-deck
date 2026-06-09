#!/usr/bin/env python3
"""Generate Pebble Draw Command (.pdc) vector icons for Signal Deck.

No external dependencies. Emits PDCI image files directly.

PDC image file layout (little-endian):
  'PDCI'                         magic (4 bytes)
  uint32  size                   bytes of the GDrawCommandImage that follows
  -- GDrawCommandImage --
  uint8   version (=1)
  uint8   reserved (=0)
  int16   viewbox_w, int16 viewbox_h
  -- GDrawCommandList --
  uint16  num_commands
  -- each GDrawCommand --
  uint8   type        (1=path, 2=circle)
  uint8   flags        (bit0 = hidden)
  uint8   stroke_color (GColor8)
  uint8   stroke_width
  uint8   fill_color   (GColor8)
  2 bytes  path: [open(uint8), unused(uint8)]  | circle: radius(uint16)
  uint16  num_points
  points[]  int16 x, int16 y   (circle stores 1 point: the center)

GColor8 byte = AARRGGBB with 2 bits per channel; alpha 0b11 = opaque, 0 = clear.

Icons are authored in a nominal 40x40 (weather) / 24x20 (sun event) space and
emitted at SCALE so they fit their slot on the watchface while staying crisp.
"""
import math
import os
import struct

CLEAR = 0x00
SCALE = 1.0  # set per icon group before building


def gcolor(hex_rgb, alpha=3):
    r = (hex_rgb >> 16) & 0xFF
    g = (hex_rgb >> 8) & 0xFF
    b = hex_rgb & 0xFF
    return (alpha << 6) | ((r >> 6) << 4) | ((g >> 6) << 2) | (b >> 6)


# Palette (matches main.c color helpers, quantised to the 64-colour space)
SUN = gcolor(0xFFAA00)
CLOUD = gcolor(0x00A7FF)
RAIN = gcolor(0x009BFF)
SNOW = gcolor(0xFFFFFF)
BOLT = gcolor(0xFFCC00)
FOG = gcolor(0x9AA5AA)
ARROW = gcolor(0xFFFFFF)


def _s(v):
    return int(round(v * SCALE))


def _sw(w):
    return max(1, int(round(w * SCALE)))


def circle(cx, cy, r, fill=CLEAR, stroke=CLEAR, sw=0):
    body = struct.pack('<BBBBB', 2, 0, stroke, _sw(sw) if sw else 0, fill)
    body += struct.pack('<H', _s(r))
    body += struct.pack('<H', 1)
    body += struct.pack('<hh', _s(cx), _s(cy))
    return body


def path(points, fill=CLEAR, stroke=CLEAR, sw=0, open_=0):
    body = struct.pack('<BBBBB', 1, 0, stroke, _sw(sw) if sw else 0, fill)
    body += struct.pack('<BB', open_, 0)
    body += struct.pack('<H', len(points))
    for x, y in points:
        body += struct.pack('<hh', _s(x), _s(y))
    return body


def line(x1, y1, x2, y2, color, sw=2):
    return path([(x1, y1), (x2, y2)], stroke=color, sw=sw, open_=1)


def rect(x, y, w, h, fill):
    return path([(x, y), (x + w, y), (x + w, y + h), (x, y + h)], fill=fill)


def write_pdc(name, viewbox, commands):
    vb = (_s(viewbox[0]), _s(viewbox[1]))
    body = struct.pack('<BB', 1, 0)
    body += struct.pack('<hh', vb[0], vb[1])
    body += struct.pack('<H', len(commands))
    body += b''.join(commands)
    blob = b'PDCI' + struct.pack('<I', len(body)) + body
    out = os.path.join(OUT_DIR, name + '.pdc')
    with open(out, 'wb') as f:
        f.write(blob)
    print('wrote %s  %dx%d  (%d cmds, %d bytes)' % (out, vb[0], vb[1], len(commands), len(blob)))


# ---- icon builders ---------------------------------------------------------

def sun_rays(cx, cy, r_in, r_out, sw=2):
    cmds = []
    for i in range(8):
        a = math.radians(i * 45)
        cmds.append(line(cx + r_in * math.cos(a), cy + r_in * math.sin(a),
                         cx + r_out * math.cos(a), cy + r_out * math.sin(a),
                         SUN, sw=sw))
    return cmds


def sun_disc(cx, cy, r):
    return [circle(cx, cy, r, fill=SUN)]


def cloud(cx, base_y, scale=1.0, fill=CLOUD):
    """A cloud silhouette centred horizontally at cx, sitting on base_y."""
    s = scale
    lr = int(7 * s)            # side puff radius
    tr = int(9 * s)            # top puff radius
    spread = int(9 * s)
    return [
        circle(cx - spread, base_y - lr, lr, fill=fill),
        circle(cx + spread, base_y - lr, lr, fill=fill),
        circle(cx, base_y - tr - 2, tr, fill=fill),
        rect(cx - spread, base_y - lr, spread * 2, lr, fill),
    ]


def build_sun():
    cmds = sun_rays(20, 20, 12, 17, sw=3) + sun_disc(20, 20, 9)
    write_pdc('wx_sun', (40, 40), cmds)


def build_partly():
    cmds = sun_rays(14, 13, 8, 11, sw=2) + sun_disc(14, 13, 6)
    cmds += cloud(23, 33, scale=0.9)
    write_pdc('wx_partly', (40, 40), cmds)


def build_cloudy():
    cmds = cloud(20, 30, scale=1.1)
    write_pdc('wx_cloudy', (40, 40), cmds)


def build_rain():
    cmds = cloud(20, 24, scale=1.0)
    for x in (13, 20, 27):
        cmds.append(line(x + 2, 28, x - 1, 36, RAIN, sw=2))
    write_pdc('wx_rain', (40, 40), cmds)


def build_snow():
    cmds = cloud(20, 24, scale=1.0)
    for x in (13, 20, 27):
        cmds.append(circle(x, 33, 2, fill=SNOW))
    write_pdc('wx_snow', (40, 40), cmds)


def build_storm():
    cmds = cloud(20, 23, scale=1.0)
    bolt = [(23, 25), (15, 34), (20, 34), (16, 40), (27, 31), (21, 31)]
    cmds.append(path(bolt, fill=BOLT))
    write_pdc('wx_storm', (40, 40), cmds)


def build_fog():
    cmds = sun_rays(20, 14, 7, 10, sw=2) + sun_disc(20, 14, 5)
    for y in (24, 29, 34):
        cmds.append(line(7, y, 33, y, FOG, sw=2))
    write_pdc('wx_fog', (40, 40), cmds)


def half_sun(cx, horizon_y, r):
    """Filled dome (top half disc) resting flat on the horizon."""
    pts = []
    for i in range(9):
        a = math.pi + (math.pi * i / 8.0)
        pts.append((cx + r * math.cos(a), horizon_y + r * math.sin(a)))
    pts.append((cx + r, horizon_y))
    pts.append((cx - r, horizon_y))
    return path(pts, fill=SUN)


def build_sun_event(name, direction):
    horizon = 15
    cmds = [half_sun(9, horizon, 6)]
    cmds += [line(9 + 6.5 * math.cos(math.radians(d)), horizon - 3 - 6.5 * math.sin(math.radians(d)),
                  9 + 9 * math.cos(math.radians(d)), horizon - 3 - 9 * math.sin(math.radians(d)),
                  SUN, sw=1) for d in (35, 90, 145)]
    cmds.append(line(2, horizon + 2, 16, horizon + 2, FOG, sw=2))
    ax = 20
    if direction == 'up':
        cmds.append(line(ax, 13, ax, 4, ARROW, sw=2))
        cmds.append(line(ax, 4, ax - 3, 7, ARROW, sw=2))
        cmds.append(line(ax, 4, ax + 3, 7, ARROW, sw=2))
    else:
        cmds.append(line(ax, 4, ax, 13, ARROW, sw=2))
        cmds.append(line(ax, 13, ax - 3, 10, ARROW, sw=2))
        cmds.append(line(ax, 13, ax + 3, 10, ARROW, sw=2))
    write_pdc(name, (24, 20), cmds)


if __name__ == '__main__':
    here = os.path.dirname(os.path.abspath(__file__))
    OUT_DIR = os.path.normpath(os.path.join(here, '..', 'resources', 'icons'))
    os.makedirs(OUT_DIR, exist_ok=True)

    SCALE = 0.75  # weather icons render at 30x30
    build_sun()
    build_partly()
    build_cloudy()
    build_rain()
    build_snow()
    build_storm()
    build_fog()

    SCALE = 1.0   # sun-event icons stay 24x20
    build_sun_event('sun_rise', 'up')
    build_sun_event('sun_set', 'down')
    print('done')
