#include "tdf.h"
#include "ansiraster.h"
#include "ansicanvas.h"
#include "tdffont.h"


bool is_block_code(uint8_t c)
{
    switch (c) {
    case 0xDB:
    case 0xDC:
    case 0xDD:
    case 0xDE:
    case 0xDF:
        return true;
    default:
        return false;
    }
    return false;
}


const char *get_font_type(int type)
{

    switch(type) {
    case 0:
        return (const char *) "TYPE_OUTLINE";
        break;
    case 1:
        return (const char *) "TYPE_BLOCK";
        break;
    case 2:
        return (const char *) "TYPE_COLOR";
        break;
    }

    return (const char *) "BADFONTTYPE";
}


TDFFont *create_new_font()
{
    int ii = 0;
    TDFFont *new_font = NULL;
    new_font = malloc(sizeof(TDFFont));
    assert((bool)(new_font));
    memset(new_font, 0, sizeof(TDFFont));

    /* we also need to zero out the character structures within the font */

    for (ii = 0; ii < TDF_MAXCHAR; ii++) {
        memset(&new_font->characters[ii], 0, sizeof(TDFCharacter));
    }

    return new_font;

}

bool push_font(TDFHandle *my_tdf, TDFFont *new_font)
{
    TDFFont *last_fontptr = NULL;
    TDFFont *next_fontptr = NULL;
    int count = 1;

    if (my_tdf && new_font) {
        if (!my_tdf->first_font) {
            my_tdf->first_font = new_font;
            return true;
        } else {
            next_fontptr = my_tdf->first_font;
            while (next_fontptr) {
                last_fontptr = next_fontptr;
                next_fontptr = next_fontptr->next_font;
                count++;
            }
            last_fontptr->next_font = new_font;
            return true;
        }
    }
    return false;
}


TDFFont* getfont_by_id(TDFHandle *my_tdf, int id)
{
    int icount = 1;
    TDFFont *fontptr = my_tdf->first_font;

    while (fontptr && icount <= my_tdf->fontcount) {
        if (icount == id) {
            return fontptr;
        }
        fontptr = fontptr->next_font;
        icount++;
    }

    return fontptr;
}

const char *get_font_name(TDFHandle *my_tdf, int id)
{

    TDFFont *fontptr = getfont_by_id(my_tdf, id);
    if (!fontptr) {
        return "MISSING FONT";
    }

    return (const char *) fontptr->name;
}

bool render_glyph(TDFFont *render_font, unsigned c)
{
    TDFCharacter *tdc = NULL;
    uint32_t glyph_offset = 0;
    int rc = 0;

    if (!render_font) {
        return false;
    }

    if (!(c>= TDF_ASCII_LO && c<= TDF_ASCII_HI)) {
        /* glyph index is out of range for a TDF font */
        return false;
    }

    /* get the correct glyph index from the ASCII char value */

    c -= 33;
    assert((bool)(c >= 0 && c <= 93));
    tdc = &render_font->characters[c];
    assert (tdc);

    if (tdc->undefined) {
        return false;
    }

    /* already done */
    if (tdc->prerendered) {
        return true;
    }

    /* check info */

    if (render_font->parent_tdf->debug_level) {
        printf(" glyph ascii char: %c\n", render_font->characters[c].ascii_value);
        printf(" font data offset: %u\n", render_font->offset);
        printf("glyph data offset: %u\n", render_font->characters[c].offset);
    }

    glyph_offset = (uint32_t) (render_font->offset + ((uint32_t) render_font->characters[c].offset));


    if (render_font->parent_tdf->debug_level) {
        printf("      data offset: %u + %u = %u\n",
               render_font->offset, (uint32_t) render_font->characters[c].offset, glyph_offset);
    }

    if (!render_font->data) {
        /* load the font from the file now */
        rc = fseek(render_font->parent_tdf->fh,render_font->offset, SEEK_SET);
        if (render_font->parent_tdf->debug_level) {
            printf("file position is now: %ld (0x%04lx)\n",
                   ftell(render_font->parent_tdf->fh),
                   ftell(render_font->parent_tdf->fh));
        }
        assert (rc != -1);
        render_font->data = malloc(render_font->blocksize);
        rc = fread(render_font->data, render_font->blocksize, 1, render_font->parent_tdf->fh);
        assert((bool)(rc == 1));
    }

    prerender_glyph(render_font, c);

    return true;

}

bool prerender_glyph(TDFFont *font, unsigned char c)
{
    unsigned char *ptr = NULL;
    uint8_t width = 0;
    uint8_t height = 0;
    uint8_t type = 0;
    off_t offset = 0;
    uint32_t limit = font->parent_tdf->limit;
    uint8_t byteval = 0;
    uint8_t color = 0;
    uint8_t bg = 0;
    uint8_t fg = 0;
    uint8_t x = 0;
    uint8_t y = 0;
    bool suppress = false;
    ANSIRaster *r = NULL;
    TDFCharacter *tdc = &font->characters[c];

    ptr = font->data + tdc->offset;

    width = ptr[0];
    height = ptr[1];
    type = font->type;
    ptr +=2;

    if (font->parent_tdf->debug_level) {
        printf("[width = %u, height = %u, type = %u (%s)]\n", width, height, type, get_font_type(type));
    }

    assert((bool)(1 <= width && width <= 30));
    assert((bool)(1 <= height && height <= 12));

    while (ptr[0] != '\0' && offset < limit) {

        /*
        if (!(y <= MAX_LINES)) {
            printf("%s: y > MAX_LINES, y = %u\n", font->name, y);
            exit(1);
            }
        */
        assert((bool)(y <= MAX_LINES));

        switch(type) {
        case TYPE_OUTLINE:
            //printf("+ Unhandled font_type = %d\n", type);
            assert((bool)(type != TYPE_OUTLINE));
            return false;
            break;
        case TYPE_BLOCK:
            byteval = ptr[0];
            x++;
            //assert((bool)(y < MAX_LINES));
            assert((bool)(tdc));
            assert((bool)(tdc->char_rasters[y]));
            r = tdc->char_rasters[y];
            assert((bool)(r));
            if (!suppress) {
                if (byteval >= 32) {
                    //putchar(byteval);
                    /* TODO: the foreground and background colors should be #defined */
                    raster_append_byte(r, byteval, 7, 0, false);
                    ptr ++;
                    offset ++;
                } else {
                    if (byteval == 0x0D) {
                        suppress = true;
                        ptr ++;
                        offset ++;
                    } else {
                        /* disabled for now, but not sure about this ... */
                        //assert (byteval >= 32 && byteval <= 255);
                    }
                }
            } else {
                raster_append_byte(r, ' ', 7, 0, false);
            }
            if (x > width) {
                //printf("\n");
                x = 0;
                y++;
                suppress = false;
            }
            break;

        case TYPE_COLOR:
            byteval = ptr[0];
            x++;
            color = ptr[1];
            fg = color;
            fg = ( fg & 0x0F ) % 0x0F;
            //printf("fg >= 0x08 = %d\n", fg);

            //assert (y < MAX_LINES);
            assert((bool)(tdc));
            assert((bool)(tdc->char_rasters[y]));
            r = tdc->char_rasters[y];
            assert((bool)(r));

            assert((bool)(fg >= 0 && fg <= 16));
            bg = color;
            bg = ((bg & 0xF0) >> 4) % 0x08;
            assert((bool)(bg >= 0 && bg <= 7));

//            printf("[0x%02x][0x%02x] [%02x][%02x] %c\n", byteval, color, bg, fg, byteval);
            if (byteval >= 32) {
                //printf("^[%u;%um",40 + bg, 30 + fg);
                if (!suppress) {

                    /* Ok. Here's some weirdness. Some block codes are often used with a foreground color of 0 (black)
                       which doesn't make sense since you can't see it. So if we see this combination convert the fg=0 to fg=0x0f (black to high-intensity white) */

                    if (is_block_code(byteval) && fg == 0x00) {
                        fg = 0x0F;
                    }

                    raster_append_byte(r, byteval, fg, bg, false);
                }
                ptr += 2;
                offset += 2;
            } else {
                if (byteval == 0x0D) {
                    //printf("<S>");
                    if (!suppress) {
                        /* reset to transparent */
                    }
                    suppress = true;
                    //raster_append_byte(r, ' ', false);
                    ptr ++;
                    offset ++;
                } else {
                    /* ok it's strange but some fonts seem to have sub-ASCII values */
                    //if (!(byteval >= 32 && byteval <= 255)) {
                    //printf("prerender_glyph: byteval = %u (0x%02x)\n", byteval);
                    //exit(1);
                    //}
                    //assert (byteval >= 32 && byteval <= 255);
                }
            }
            if (x > width) {
                //printf("<H>\n");
                //printf("\n");
                x = 0;
                y++;
                suppress = false;
            }
            break;
        default:
            printf("+ Unhandled font_type = %d?? Not expected to reach here.\n", type);
            return false;
            break;
        }
    }

    /* check that the number of bytes in the raster is equivalent to the width - some fonts
       seem to have a short count on the final line and need to be padded */


    /* ensure there is no overrun */

    assert((bool) (r != NULL));

    if (r != NULL && r->bytes > width) {
        if (font->parent_tdf->debug_level) {
            printf("+ overrun, actual length = %u,  expected = %u\n", r->bytes, width);
        }
        width = r->bytes;
        if (font->parent_tdf->debug_level) {
            display_glyph(font, c, false);
        }
    }

    assert((bool)(r->bytes <= width));

    while (r->bytes < width) {
        if (font->parent_tdf->debug_level) {
            printf("+ actual length = %u,  expected = %u\n", r->bytes, width);
            fflush(NULL);
        }
        raster_append_byte(r, ' ', 7, 0, false);
    }

    assert((bool)(r->bytes == width));

    y++;
    tdc->height = height;
    tdc->width = width;
    tdc->prerendered = true;
    tdc->discovered_height = y+1;
    if (font->parent_tdf->debug_level) {
        printf("\n");
        printf("(%ld bytes; %d lines)\n", offset, y);
    }
    return true;

}

/* quick and dirty way to just show a single glyph. deprecating soon */

bool display_glyph(TDFFont *tdf, uint8_t c, bool use_unicode)
{
    TDFCharacter *tdc = NULL;
    ANSIRaster *tdr = NULL;
    int ii = 0, jj = 0;

    if (tdf->parent_tdf->debug_level) {
        printf("display_glyph('%c')\n", c);
    }

    if (!tdf) return false;

    c -= 33;
    assert((bool)((c >= 0 && c <= 93)));
    tdc = &tdf->characters[c];

    assert((bool)(tdc));

    if (tdc->undefined) {
        return false;
    }

    assert((bool)(!tdc->undefined));
    assert((bool)(tdc->prerendered));

    /* get correct character */
    for (ii = 0; ii < tdc->discovered_height; ii++) {
        assert ((bool) (ii < MAX_LINES));
        assert((bool)(tdc));
        assert((bool)(tdc->char_rasters[ii]));
        tdr = tdc->char_rasters[ii];
        assert((bool)(tdr));

        if (tdr->bytes && tdr->chardata) {
            //printf("%s", tdr->chardata);
            raster_output(tdr, false, use_unicode);
            printf(" (%u,%u/%u)\n", tdr->bytes, ii+1, tdc->discovered_height);
        } else {
            /* blank raster */
            for (jj = 0; jj < tdc->width; jj++) {
                putchar(' ');
            }
            printf(" (%u,%u/%u)\n", tdr->bytes, ii+1, tdc->discovered_height);
        }
    }

    return true;
}

bool push_glyph(ANSICanvas *my_canvas, TDFFont *tdf, uint8_t c)
{
    unsigned char dummy_spacing[30];
    TDFCharacter *tdc = NULL;
    ANSIRaster *src_raster = NULL;
    ANSIRaster *dst_raster = NULL;
    int ii = 0;
    int jj = 0;

    assert(my_canvas);
    assert(tdf);

    /* FIXME: special handling for space! it would be nice to handle this in a more graceful way */

    if (c == 32) {
        memset(&dummy_spacing, 0, 30);
        assert((bool) (1 <= tdf->spacing && tdf->spacing <= 30));
        assert((bool) (1 <= tdf->average_width && tdf->average_width <= 30));
        memset(&dummy_spacing, ' ', tdf->average_width);

        /* fake it, baby */

//        assert(tdf->average_height);
        assert(tdf->maximum_height);

        for (ii = 0; ii < tdf->maximum_height; ii++) {
            dst_raster = canvas_get_raster(my_canvas, ii);

            while (!dst_raster) {
                dst_raster = canvas_add_raster(my_canvas);
            }
            assert(dst_raster);
            assert(ii == dst_raster->index);
            assert(ii <= my_canvas->lines);
            assert(tdf);
            assert(tdf->average_width);
            /* TODO: don't use constants for colors here, use #defines */
            assert(raster_append_bytes(dst_raster, (unsigned char*) &dummy_spacing, tdf->average_width, 7, 0, false));
        }
        return true;
    }

    /* otherwise it's not a space */

    memset(&dummy_spacing, 0, 30);
    assert((bool) (1 <= tdf->spacing && tdf->spacing <= 30));
    memset(&dummy_spacing, ' ', tdf->spacing);


    //printf("c = '%c'\n", c);

    c -= 33;
    assert(c >= 0 && c <= 93);
    tdc = &tdf->characters[c];

    assert(tdc);

    /* make sure character will fit on canvas vertically */

    if (tdc->undefined) {
        /* if the glyph is undefined, just skip it */
        return true;
    }


    assert(!tdc->undefined);
    assert(tdc->prerendered);

    if (tdf->maximum_height > MAX_LINES) {
        printf("%s: tdf->maximum_height = %u is larger than MAX_LINES = %u\n", tdf->name, tdf->maximum_height, MAX_LINES);
        exit(1);
    }

    assert(tdf->maximum_height <= MAX_LINES);
    for (ii = 0; ii < tdf->maximum_height; ii++) {
        assert(tdc);
        assert(tdc->char_rasters[ii]);
        src_raster = tdc->char_rasters[ii];
        dst_raster = canvas_get_raster(my_canvas, ii);
        while (!dst_raster) {
            dst_raster = canvas_add_raster(my_canvas);
        }
        assert(dst_raster);
        assert(ii == dst_raster->index);
        assert(ii <= my_canvas->lines);
        assert(src_raster);


        if ((!src_raster->chardata || !src_raster->bytes)) {
            /* the glyph is prerendered, and not a space character,
            	 but this particular raster doesn't have any data.
            	 we fill it with dummy spacing data instead */
            memset(&dummy_spacing, 0, 30);
            assert((bool) (1 <= tdc->width && tdc->width <= 30));
            memset(&dummy_spacing, ' ', tdc->width);
            assert(raster_append_bytes(dst_raster, (unsigned char*) &dummy_spacing, tdc->width, 7, 0, false));
            /* don't forget the font-level spacing as well */
            memset(&dummy_spacing, 0, 30);
            assert((bool) (1 <= tdf->spacing && tdf->spacing <= 30));
            memset(&dummy_spacing, ' ', tdf->spacing);
            assert(raster_append_bytes(dst_raster, (unsigned char*) &dummy_spacing, tdf->spacing, 7, 0, false));
            return true;
        }

        assert(src_raster->chardata);
        assert(src_raster->bytes);

        /* FIXME: its seems we need a copy_raster() function that preserves fg/bg colors, as
                well as a raster_append_space[s]() */

        for (jj = 0; jj < src_raster->bytes ; jj++) {
            assert(raster_append_byte(dst_raster, src_raster->chardata[jj], src_raster->fgcolors[jj], src_raster->bgcolors[jj], false));
        }
        assert(raster_append_bytes(dst_raster, (unsigned char*) &dummy_spacing, tdf->spacing, 7, 0, false));
    }
    return true;

}
