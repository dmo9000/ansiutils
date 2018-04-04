#include <stdint.h>
#include "tdf.h"
#include "ansiraster.h"
#include "utf8.h"

ANSIRaster *create_new_raster()
{
    ANSIRaster *new_raster = NULL;
    new_raster = malloc(sizeof(ANSIRaster));
    assert(new_raster);
    memset(new_raster, 0, sizeof(ANSIRaster));
    return new_raster;

}


bool raster_append_bytes(ANSIRaster *r, unsigned char *data, uint8_t bytes, ansicolor_t fg, ansicolor_t bg, attributes_t attr, bool debug)
{

    int ii = 0;
    /* uncomment this assert to filter some dogdy fonts, need to know why though */

    for (ii = 0; ii < bytes; ii++) {
        if (!raster_append_byte(r, data[ii], fg, bg, attr, debug)) {
            return false;
        };
    }
    return true;

}

bool raster_resize(ANSIRaster *r, uint16_t size)
{
    unsigned char *chardata_realloc = NULL;
    unsigned char *fgcolors_realloc = NULL;
    unsigned char *bgcolors_realloc = NULL;
    unsigned char *attribs_realloc = NULL;

    /* raster must be valid */
    assert(r);
    /* must request resize by at least one byte */
    assert(size);
    /* size must be only a single byte (for now) */
    assert(size == r->bytes + 1);
    /* new size must be larger than old size */
    assert(size > r->bytes);
    /* new size must not cause raster to exceed maximum length */
    assert(size + r->bytes <= 65535);

    /* if the raster is empty, allocate an initial byte */

    if (!r->bytes) {
        /* if the raster is empty, allocate an initial byte */
        r->bytes = 1;
        /* TODO - since we're planning not to just printf() anymore,  probably no need to add the extra byte for much longer */
        r->chardata = malloc(2);
        r->fgcolors = malloc(2);
        r->bgcolors = malloc(2);
        r->attribs = malloc(2);
        r->chardata[0] = 0;
        r->fgcolors[0] = 7;
        r->bgcolors[0] = 0;
        r->attribs[0] = ATTRIB_NONE;
        r->chardata[1] = '\0'; /* NULL terminate, so that the rasters can be printed with C library functions */
        r->fgcolors[1] = '\0';
        r->bgcolors[1] = '\0';
        r->attribs[1] = ATTRIB_NONE;
        r->bytes = 1;
        return true;
    }

    /* otherwise we are resizing an existing raster */
    /* ensure we already have raster data */
    assert(r->chardata);
    /* ensure we already have foreground color data */
    assert(r->fgcolors);
    /* ensure we already have background color data */
    assert(r->bgcolors);
    /* assert that the raster already has a length greater than 0 */
    assert(r->bytes);

    r->bytes ++;
    chardata_realloc = realloc(r->chardata, r->bytes+1);
    fgcolors_realloc = realloc(r->fgcolors, r->bytes+1);
    bgcolors_realloc = realloc(r->bgcolors, r->bytes+1);
    attribs_realloc = realloc(r->attribs, r->bytes+1);

    /* ensure realloc() didn't fail */
    assert(chardata_realloc);
    assert(fgcolors_realloc);
    assert(bgcolors_realloc);
    assert(attribs_realloc);

    /* assign the realloc'd regions to the relevant pointers */
    r->chardata = chardata_realloc;
    r->fgcolors = fgcolors_realloc;
    r->bgcolors = bgcolors_realloc;
    r->attribs = attribs_realloc;
    /* ensure the new raster does not have length 0 */
    assert(r->bytes-1);

    return true;
}

bool raster_append_byte(ANSIRaster *r, unsigned char data, ansicolor_t fg, ansicolor_t bg, attributes_t attr, bool debug)
{
    ANSIRaster *tdr = r;

    /* raster must exist */
    assert(r);
    /* do not push null byte, ever, since it's a string terminator  */

    if (data < 0x20) {
        /* use 'X' instead for now */
        data = 'X';
    }

    assert(data);

    if (!tdr->bytes) {
        /* TODO - since we're planning not to just printf() anymore,  probably no need to add the extra byte for much longer */
        raster_resize(tdr, tdr->bytes+1);
        tdr->chardata[0] = data;
        tdr->fgcolors[0] = fg;
        tdr->bgcolors[0] = bg;
        tdr->attribs[0] = attr;
        tdr->chardata[1] = '\0'; /* NULL terminate, so that the rasters can be printed with C library functions */
        tdr->fgcolors[1] = '\0';
        tdr->bgcolors[1] = '\0';
        tdr->attribs[1] = ATTRIB_NONE;
        return true;
    } else {
        raster_resize(tdr, tdr->bytes+1);
        tdr->chardata[tdr->bytes-1] = data;
        tdr->fgcolors[tdr->bytes-1] = fg;
        tdr->bgcolors[tdr->bytes-1] = bg;
        tdr->attribs[tdr->bytes-1] = attr;
        tdr->chardata[tdr->bytes] = '\0';
        tdr->fgcolors[tdr->bytes] = '\0';
        tdr->bgcolors[tdr->bytes] = '\0';
        tdr->attribs[tdr->bytes] = ATTRIB_NONE;
    }

    return true;
}

bool raster_output(ANSIRaster *r, bool debug_mode, bool use_unicode, bool compress, FILE *fh)
{

    int jj = 0;
    ansicolor_t fg = 0, bg = 0;
    uint8_t attr = ATTRIB_NONE;
    ansicolor_t last_fg = 0, last_bg = 0;
    uint8_t last_attr = ATTRIB_NONE;
    unsigned char last_char = 0;
    bool bold = false;
    uint8_t compress_flags = COMPRESSION_DISABLED;
    uint16_t compress_hspace_count = 0;

    if (compress) {
        compress_flags |= COMPRESSION_ENABLED;
    }

    assert(compress_flags == COMPRESSION_ENABLED || compress_flags == COMPRESSION_DISABLED);

    for (jj = 0; jj < r->bytes; jj++) {
        bold = false;
        last_fg = fg;
        last_bg = bg;
        last_attr = attr;

        fg = r->fgcolors[jj];
        bg = r->bgcolors[jj];
        attr = r->attribs[jj];
        bold = attr  & ATTRIB_BOLD;

        if (debug_mode) {
            printf("[%u/%03u:%c:%X/%X:%s]\n", jj, r->chardata[jj],
                   r->chardata[jj], r->bgcolors[jj], r->fgcolors[jj], ((bold) ? "BOLD" : "NOBOLD"));
        } else {
            /* if compression is enabled, check whether the colors are repeated and set/clear compression flags appropriately */
            if (compress_flags & COMPRESSION_ENABLED) {
                if (last_fg == fg && last_bg == bg) {
                    compress_flags |= COMPRESSION_COLORS;
                } else {
                    fprintf(fh, (char *) "\x1b\x5b""%u;%um", 40 + bg, 30 + fg);
                    compress_flags &= ~COMPRESSION_COLORS;
                }
            } else {
                /* compression disabled, output color codes for every byte */
                assert(!compress_flags);
                fprintf(fh, (char *) "\x1b\x5b""%u;%um", 40 + bg, 30 + fg);
            }

            if (compress_flags & COMPRESSION_ENABLED) {
                /* if compression is enabled, check whether the attributes are repeated and set/clear compression flags appropriately */
                if (last_attr == attr) {
                    compress_flags |= COMPRESSION_ATTRIBS;
                } else {
                    compress_flags &= ~COMPRESSION_ATTRIBS;
                    if (bold) {
                        fprintf(fh, "\x1b\x5b""1m");
                    } else {
                        fprintf(fh, "\x1b\x5b""21m");
                    }
                }
            } else {
                /* no compression */
                assert(!compress_flags);
                if (bold) {
                    fprintf(fh, "\x1b\x5b""1m");
                } else {
                    fprintf(fh, "\x1b\x5b""21m");
                }
            }

            /* check that COMPRESSION_HRELEASE isn't set (for now), since it can't be set here */
            assert(!(compress_flags & COMPRESSION_HRELEASE));

            if (compress_flags & COMPRESSION_ENABLED) {
                if ((compress_flags & COMPRESSION_COLORS) && (compress_flags & COMPRESSION_ATTRIBS)) {
                    /* colors and attributes are compressed. consider whether we can compress a row of spaces into a MOVE CURSOR RIGHT n SPACES sequence */
                    if (last_char == 0x020) {
                        /* last character was a space ... */
                        if (last_char == r->chardata[jj]) {
                            /* this one is too. set the HSPACE compression flag and increment the counter */
                            compress_flags |= COMPRESSION_HSPACE;
                            compress_hspace_count ++;
                        } else {
                            /* this one isn't. set the HRELEASE flag */
                            compress_flags |= COMPRESSION_HRELEASE;
                        }
                    } else {
                        /* colors and attributes were compressed, by last character wasn't a space. don't consider compression for the current byte */
                        compress_flags &= ~COMPRESSION_HSPACE;
                        compress_hspace_count = 0;
                    }
                } else {
                    /* colors and attributes are not currently compressed (they changed). accordingly can't compress horizontal whitespace either */
                    compress_flags &= ~COMPRESSION_HSPACE;
                    compress_hspace_count = 0;
                }
            } else {
                /* compression is disabled - no horizontal space compression */
                assert(!compress_hspace_count);
            }

            // brake lever
            assert(!(compress_flags & COMPRESSION_HRELEASE));

            if (compress_flags & COMPRESSION_HRELEASE) {
                //printf("+++ COMPRESSION_HRELEASE set, counter = %u\n", compress_hspace_count);
								/* dump the HSPACES to output */	
								fprintf(fh, "\x1b\x5b""%uC", compress_hspace_count);
						//		fprintf(fh, "
								}

                /* check that COMPRESSION_HRELEASE isn't set (for now) */
                assert(!(compress_flags & COMPRESSION_HRELEASE));

                if (use_unicode) {
                    if (r->chardata[jj] < 128) {
                        fputc(r->chardata[jj], fh);
                    } else {
                        fprintf(fh, "%s", utf8_string(r->chardata[jj]));
                    }
                } else {
                    fputc(r->chardata[jj], fh);
                }

                last_char = r->chardata[jj];
            }
        }

    fprintf(fh, "\x1b\x5b""0m");
    return true;

}
