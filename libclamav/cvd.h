/*
 *  Copyright (C) 2007-2010 Sourcefire, Inc.
 *
 *  Authors: Tomasz Kojm
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#ifndef __CVD_H
#define __CVD_H

#include <stdio.h>
#include <zlib.h>
#include "clamav.h"

struct cli_dbio {
    gzFile gzs;
    FILE *fs;
    unsigned int size, bread;
    char *buf, *bufpt, *readpt;
    unsigned int usebuf, bufsize, readsize;
    unsigned int chkonly;
    EVP_MD_CTX hashctx;
};

int cli_cvdload(FILE *fs, struct cl_engine *engine, unsigned int *signo, unsigned int options, unsigned int dbtype, const char *filename, unsigned int chkonly);
int cli_cvdunpack(const char *file, const char *dir);

#endif
