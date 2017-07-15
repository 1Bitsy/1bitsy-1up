#!/usr/bin/env python3

from itertools import groupby
import sys

from PIL import Image
from xml.etree import ElementTree

white_pixel = 255

########################################################################
# General helper functions
########################################################################


def by_n(n, seq):
    return ((x for (i, x) in g)
            for (k, g) in groupby(enumerate(seq), lambda x: x[0] // n))


def pack_rgb565(tup):
    return (tup[0] >> 3 << 11 |
            tup[1] >> 2 << 5 |
            tup[2] >> 3 << 0)


def rgb888_to_rgb565(val):
    return (((val & 0xFF0000) >> 19) << 11 |
            ((val & 0x00FF00) >> 10) << 5 |
            ((val & 0x0000FF) >> 3) << 0)

########################################################################
# Tilesheet
########################################################################

ts_path = 'tilesheet.png'
ts_img = Image.open(ts_path)
ts_img = ts_img.convert("RGBA")
ts_pix = ts_img.load()
ts_w, ts_h = ts_img.size

ts_packed = [[pack_rgb565(ts_pix[x, y])
              for x in range(ts_w)]
             for y in range(ts_h)]

ts_bytes = ',\n'.join('    {{ {} }}'
                      .format(',\n      '.join(', '.join('{:3d}'.format(b)
                                                         for b in line)
                                               for line in by_n(8, row)))
                      for row in ts_packed)

ts_template = '''
#define TS_PIXMAP_HEIGHT {h}
#define TS_PIXMAP_WIDTH {w}

static const uint16_t ts_pixmap[TS_PIXMAP_HEIGHT][TS_PIXMAP_WIDTH] = {{
{bytes}
}};'''.lstrip()

ts_def = ts_template.format(h=ts_h, w=ts_w, bytes=ts_bytes)

########################################################################
# Tilesheet
########################################################################

ss_path = 'spritesheet.png'
ss_img = Image.open(ss_path)
ss_img = ss_img.convert("RGBA")
ss_pix = ss_img.load()
ss_w, ss_h = ss_img.size

ss_packed = [[pack_rgb565(ss_pix[x, y])
              for x in range(ss_w)]
             for y in range(ss_h)]

ss_bytes = ',\n'.join('    {{ {} }}'
                      .format(',\n      '.join(', '.join('{:3d}'.format(b)
                                                         for b in line)
                                               for line in by_n(8, row)))
                      for row in ss_packed)

ss_template = '''
#define SS_PIXMAP_HEIGHT {h}
#define SS_PIXMAP_WIDTH {w}

static const uint16_t ss_pixmap[SS_PIXMAP_HEIGHT][SS_PIXMAP_WIDTH] = {{
{bytes}
}};'''.lstrip()

ss_def = ss_template.format(h=ss_h, w=ss_w, bytes=ss_bytes)

########################################################################
# Level Tilemap
########################################################################

tml1_path = '1bitsy-1up-game1.tmx'
tml1e = ElementTree.parse(tml1_path).getroot()
tml1_width = int(tml1e.attrib['width'])
tml1_height = int(tml1e.attrib['height'])
tml1_background = rgb888_to_rgb565(int(tml1e.attrib['backgroundcolor'].replace("#", ""), 16))


def filter_empty(x):
    return list(filter(lambda d: len(d) > 0, x))

# Split the data apart and remove empty stuff while doing it and convert the numbers to ints.
tml1_layers = list(map(lambda l: list(map(lambda d: list(map(lambda i: int(i), filter_empty(d.split(",")))),
                                          filter_empty(l.find('data').text.split("\n")))), tml1e.findall('layer')))

# Convert the gathered layer data into a usable C array.
tml1_struct = ',\n'.join('{{ {} }}'
                         .format(',\n  '
                                 .join('{{ {} }}'
                                       .format(',\n    '.join(','.join('{:3d}'.format(b)
                                                                   for b in line)
                                                          for line in by_n(16, row)))
                                       for row in layer))
                         for layer in tml1_layers)

# We should restrict the tile indexes to 8bit, we can have multiple tilesets if needed... I don't think we need more
# Than 255 tiles per tilemap.
tml1_template = '''
#define TML1_TILEMAP_LAYERS {l}
#define TML1_TILEMAP_HEIGHT {h}
#define TML1_TILEMAP_WIDTH {w}
#define TML1_TILEMAP_BG_COLOR {bg_color}

static const uint16_t tml1_tilemap[TML1_TILEMAP_LAYERS][TML1_TILEMAP_HEIGHT][TML1_TILEMAP_WIDTH] = {{
{struct}
}};'''.lstrip()

tml1_def = tml1_template.format(l=len(tml1_layers),
                                h=tml1_height,
                                w=tml1_width,
                                bg_color=tml1_background,
                                struct=tml1_struct)

########################################################################
# Miniwi font
########################################################################

# Load font
from PIL import BdfFontFile

miniwi_path = "miniwi/miniwi-8.bdf"

miniwi_ff = BdfFontFile.BdfFontFile(open(miniwi_path, 'rb'))

miniwi_glyphs = []

for c in range(33, 127):
    glyph = []
    glyph_pix = miniwi_ff.glyph[c][3].load()
    for col in range(0, 4):
        col_byte = 0
        for row in range(0, 8):
            if glyph_pix[col, row] != 0:
                col_byte = col_byte | (0x1 << row)
        glyph.append(col_byte)
    miniwi_glyphs.append(glyph)

miniwi_struct = ",\n".join('{{ {} }}'.format(", ".join('{:#04x}'.format(b)
                                                        for b in glyph))
                           for glyph in miniwi_glyphs)

miniwi_template = '''
#define MINIWI_FONT_WIDTH 4
#define MINIWI_FONT_HEIGHT 8
#define MINIWI_FONT_OFFSET 33
#define MINIWI_FONT_GLYPH_COUNT {l}

static const uint8_t miniwi_font[MINIWI_FONT_GLYPH_COUNT][MINIWI_FONT_WIDTH] = {{
{struct}
}};'''.lstrip()

miniwi_def = miniwi_template.format(l=len(miniwi_glyphs),
                                    struct=miniwi_struct)

########################################################################
# Asset header body and output
########################################################################

template = '''
#ifndef ASSETS_H
#define ASSETS_H

/* This file was automatically generated by {program}.  Do not edit. */

#include <stdint.h>

{ts_def}

{ss_def}

{tml1_def}

{miniwi_def}

#endif /* ASSETS_H */
'''.lstrip()

print(template.format(program=sys.argv[0],
                      ts_def=ts_def,
                      ss_def=ss_def,
                      tml1_def=tml1_def,
                      miniwi_def=miniwi_def))
