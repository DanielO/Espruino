/*
 * This file is part of Espruino, a JavaScript interpreter for Microcontrollers
 *
 * Copyright (C) 2013 Gordon Williams <gw@pur3.co.uk>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * ----------------------------------------------------------------------------
 * Graphics Backend for drawing to ArrayBuffer
 * ----------------------------------------------------------------------------
 */
#include "jswrap_arraybuffer.h"
#include "lcd_arraybuffer.h"
#include "jsvar.h"
#include "jsvariterator.h"

// returns the BIT index, so the bottom 3 bits specify the bit in the byte
size_t lcdGetPixelIndex_ArrayBuffer(JsGraphics *gfx, int x, int y, int pixelCount) {
  if (gfx->data.flags & JSGRAPHICSFLAGS_ARRAYBUFFER_ZIGZAG) {
    if (y&1) x = gfx->data.width - (x+pixelCount);
  }
  if (gfx->data.flags & JSGRAPHICSFLAGS_ARRAYBUFFER_VERTICAL_BYTE)
    return (size_t)(((x + (y>>3)*gfx->data.width)<<3) | (y&7));
  else
    return (size_t)((x + y*gfx->data.width)*gfx->data.bpp);
}

unsigned int lcdGetPixel_ArrayBuffer(JsGraphics *gfx, short x, short y) {
  unsigned int col = 0;
  JsVar *buf = jsvObjectGetChild(gfx->graphicsVar, "buffer", 0);
  if (buf && jsvIsArrayBuffer(buf)) {
    size_t idx = lcdGetPixelIndex_ArrayBuffer(gfx,x,y,1);
    JsvArrayBufferIterator it;
    jsvArrayBufferIteratorNew(&it, buf, idx>>3 );
    if (gfx->data.bpp&7/*not a multiple of one byte*/) {
      idx = idx & 7;
      unsigned int mask = (unsigned int)(1<<gfx->data.bpp)-1;
      unsigned int existing = (unsigned int)jsvArrayBufferIteratorGetIntegerValue(&it);
      col = ((existing>>idx)&mask);
    } else {
      int i;
      for (i=0;i<gfx->data.bpp;i+=8) {
        col |= ((unsigned int)jsvArrayBufferIteratorGetIntegerValue(&it)) << i;
        jsvArrayBufferIteratorNext(&it);
      }
    }
    jsvArrayBufferIteratorFree(&it);
  }
  jsvUnLock(buf);
  return col;
}

// set pixelCount pixels starting at x,y
void lcdSetPixels_ArrayBuffer(JsGraphics *gfx, short x, short y, short pixelCount, unsigned int col) {
  JsVar *buf = jsvObjectGetChild(gfx->graphicsVar, "buffer", 0);
  if (buf && jsvIsArrayBuffer(buf)) {
    size_t idx = lcdGetPixelIndex_ArrayBuffer(gfx,x,y,pixelCount);
    JsvArrayBufferIterator it;
    jsvArrayBufferIteratorNew(&it, buf, idx>>3 );
    short p;
    for (p=0;p<pixelCount;p++) { // writing individual bits
      if (gfx->data.bpp&7/*not a multiple of one byte*/) {
        idx = idx & 7;
        unsigned int mask = (unsigned int)(1<<gfx->data.bpp)-1;
        unsigned int existing = (unsigned int)jsvArrayBufferIteratorGetIntegerValue(&it);
        jsvArrayBufferIteratorSetIntegerValue(&it, (JsVarInt)((existing&~(mask<<idx)) | ((col&mask)<<idx)));
        if (gfx->data.flags & JSGRAPHICSFLAGS_ARRAYBUFFER_VERTICAL_BYTE) {
          jsvArrayBufferIteratorNext(&it);
        } else {
          idx += gfx->data.bpp;
          if (idx>=8) jsvArrayBufferIteratorNext(&it);
        }
      } else { // we're writing whole bytes
        int i;
        for (i=0;i<gfx->data.bpp;i+=8) {
          jsvArrayBufferIteratorSetIntegerValue(&it, (JsVarInt)(col >> i));
          jsvArrayBufferIteratorNext(&it);
        }
      }
    }
    jsvArrayBufferIteratorFree(&it);
    jsvUnLock(buf);
  }
}


void lcdSetPixel_ArrayBuffer(JsGraphics *gfx, short x, short y, unsigned int col) {
  lcdSetPixels_ArrayBuffer(gfx,x,y,1,col);
}

void  lcdFillRect_ArrayBuffer(struct JsGraphics *gfx, short x1, short y1, short x2, short y2) {
  short y;
  for (y=y1;y<=y2;y++)
    lcdSetPixels_ArrayBuffer(gfx, x1, y, (short)(1+x2-x1), gfx->data.fgColor);
}

void lcdInit_ArrayBuffer(JsGraphics *gfx) {
  // create buffer
  JsVar *buf = jswrap_arraybuffer_constructor((gfx->data.width * gfx->data.height * gfx->data.bpp + 7) >> 3);
  jsvUnLock(jsvAddNamedChild(gfx->graphicsVar, buf, "buffer"));
  jsvUnLock(buf);
}

void lcdSetCallbacks_ArrayBuffer(JsGraphics *gfx) {
  gfx->setPixel = lcdSetPixel_ArrayBuffer;
  gfx->getPixel = lcdGetPixel_ArrayBuffer;
  gfx->fillRect = lcdFillRect_ArrayBuffer;
}
