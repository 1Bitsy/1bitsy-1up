#!/usr/bin/env python3

from itertools import groupby
import sys

from PIL import Image
from PIL import BdfFontFile
from xml.etree import ElementTree

white_pixel = 255

defs = []

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
# Definition functions
########################################################################

def pixmap(path, name):
    img = Image.open(path)
    img = img.convert("RGBA")
    pix = img.load()
    w, h = img.size
    packed = [[pack_rgb565(pix[x, y])
        for x in range(w)]
        for y in range(h)]
    data = ',\n'.join('    {{ {} }}'
            .format(',\n      '.join(', '.join('{:3d}'.format(b)
                                               for b in line)
                                     for line in by_n(8, row)))
            for row in packed)
    template = '''
#define {NAME}_PIXMAP_HEIGHT {h}
#define {NAME}_PIXMAP_WIDTH {w}

static const uint16_t {name}_pixmap[{NAME}_PIXMAP_HEIGHT][{NAME}_PIXMAP_WIDTH] = {{
{data}
}};'''.lstrip()
    return template.format(name=name, NAME=name.upper(), h=h, w=w, data=data)

def tilemap(path, name):
    e = ElementTree.parse(path).getroot()
    w = int(e.attrib['width'])
    h = int(e.attrib['height'])
    background = rgb888_to_rgb565(int(e.attrib['backgroundcolor']
        .replace("#", ""), 16))
    def filter_empty(x):
        return list(filter(lambda d: len(d) > 0, x))
    # Split the data apart and remove empty stuff while doing it and convert the numbers to ints.
    layers = list(map(lambda l: list(map(lambda d: list(map(lambda i: int(i), filter_empty(d.split(",")))),
                                         filter_empty(l.find('data').text.split("\n")))), e.findall('layer')))
    # Convert the gathered layer data into a usable C array.
    struct = ',\n'.join('{{ {} }}'
                        .format(',\n  '
                                .join('{{ {} }}'
                                      .format(',\n    '.join(','.join('{:3d}'.format(b)
                                                                  for b in line)
                                                         for line in by_n(16, row)))
                                      for row in layer))
                        for layer in layers)
    # We should restrict the tile indexes to 8bit, we can have multiple tilesets if needed... I don't think we need more
    # Than 255 tiles per tilemap.
    template = '''
#define {NAME}_TILEMAP_LAYERS {l}
#define {NAME}_TILEMAP_HEIGHT {h}
#define {NAME}_TILEMAP_WIDTH {w}
#define {NAME}_TILEMAP_BG_COLOR {bg_color}

static const uint16_t {name}_tilemap[{NAME}_TILEMAP_LAYERS][{NAME}_TILEMAP_HEIGHT][{NAME}_TILEMAP_WIDTH] = {{
{struct}
}};'''.lstrip()
    return template.format(name=name, NAME=name.upper(), l=len(layers), h=h, w=w,
                           bg_color=background, struct=struct)

def font(path, name):
    ff = BdfFontFile.BdfFontFile(open(path, 'rb'))
    glyphs = []
    for c in range(33, 127):
        glyph = []
        glyph_pix = ff.glyph[c][3].load()
        for col in range(0, 4):
            col_byte = 0
            for row in range(0, 8):
                if glyph_pix[col, row] != 0:
                    col_byte = col_byte | (0x1 << row)
            glyph.append(col_byte)
        glyphs.append(glyph)
    struct = ",\n".join('{{ {} }}'.format(", ".join('{:#04x}'.format(b)
                                                    for b in glyph))
                        for glyph in glyphs)
    template = '''
#define {NAME}_FONT_WIDTH 4
#define {NAME}_FONT_HEIGHT 8
#define {NAME}_FONT_OFFSET 33
#define {NAME}_FONT_GLYPH_COUNT {l}

static const uint8_t {name}_font[{NAME}_FONT_GLYPH_COUNT][{NAME}_FONT_WIDTH] = {{
{struct}
}};'''.lstrip()
    return template.format(name=name, NAME=name.upper(), l=len(glyphs),
                           struct=struct)

########################################################################
# Game 1
########################################################################

defs += [
        pixmap('tilesheet.png', 'ts'),
        pixmap('spritesheet.png', 'ss'),
        tilemap('1bitsy-1up-game1.tmx', 'tml1'),
        ]

########################################################################
# Falling blocks
########################################################################

defs += [
        pixmap('fblocks.png', 'fblocks'),
        ]

########################################################################
# Miniwi font
########################################################################

defs += [
    font('miniwi/miniwi-8.bdf', 'miniwi'),
    ]

########################################################################
# Asset header body and output
########################################################################

template = '''
#ifndef ASSETS_H
#define ASSETS_H

/* This file was automatically generated by {program}.  Do not edit. */

#include <stdint.h>

{defs}

#endif /* ASSETS_H */
'''.lstrip()

print(template.format(program=sys.argv[0], defs='\n\n'.join(defs)))
