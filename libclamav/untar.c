/*
 *  Copyright (C) 2004 Nigel Horne <njh@bandsman.co.uk>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * Much of this code is based on minitar.c which is in the public domain.
 * Author: Charles G. Waldman (cgw@pgt.com),  Aug 4 1998
 *
 * Change History:
 * $Log: untar.c,v $
 * Revision 1.4  2004/09/06 08:45:44  nigelhorne
 * Code Tidy
 *
 * Revision 1.3  2004/09/06 08:34:47  nigelhorne
 * Randomise extracted file names from tar file
 *
 * Revision 1.2  2004/09/05 18:58:21  nigelhorne
 * Extract files completed
 *
 * Revision 1.1  2004/09/05 15:28:10  nigelhorne
 * First draft
 *
 */
static	char	const	rcsid[] = "$Id: untar.c,v 1.4 2004/09/06 08:45:44 nigelhorne Exp $";

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/param.h>        /* for NAME_MAX */

#include "clamav.h"
#include "others.h"
#include "untar.h"
#include "blob.h"

#define BLOCKSIZE 512

static int
octal(const char *str)
{
	int ret = -1;

	sscanf(str, "%o", &ret);
	return ret;
}

int
cli_untar(const char *dir, int desc)
{
	int size = 0;
	int in_block = 0;
	char fullname[NAME_MAX + 1];
	FILE *outfile = (FILE*)0;

	cli_dbgmsg("In untar(%s, %d)\n", dir ? dir : "", desc);

	for(;;) {
		char block[BLOCKSIZE];
		const int nread = read(desc, block, sizeof(block));

		if(!in_block && nread == 0)
			break;

		if(nread != BLOCKSIZE) {
			cli_errmsg("cli_untar: incomplete block read\n");
			return CL_EIO;
		}

		if(!in_block) {
			char type;
			const char *suffix;
			size_t suffixLen = 0;
			int fd, directory;
			char magic[7], name[101], osize[13];

			if(outfile) {
				if(fclose(outfile)) {
					cli_errmsg("cli_untar: cannot close file %s\n",
					    fullname);
					return CL_EIO;
				}
				outfile = (FILE*)0;
			}

			if(block[0] == '\0')  /* We're done */
				break;

			strncpy(magic, block+257, 6);
			magic[6] = '\0';
			if(strcmp(magic, "ustar ") != 0) {
				cli_errmsg("Incorrect magic number in tar header\n");
				return CL_EDSIG;
			}

			type = block[156];

			switch(type) {
				case '0':
				case '\0':
					directory = 0;
					break;
				case '5':
					directory = 1;
					break;
				default:
					cli_errmsg("cli_untar: unknown type flag %c\n", type);
					return CL_EIO;
			}

			if(directory)
				continue;

			strncpy(name, block, 100);
			name[100] = '\0';

			/*
			 * see also fileblobSetFilename()
			 * TODO: check if the suffix needs to be put back
			 */
			sanitiseName(name);
			suffix = strrchr(name, '.');
			if(suffix == NULL)
				suffix = "";
			else {
				suffixLen = strlen(suffix);
				if(suffixLen > 4) {
					/* Found a full stop which isn't a suffix */
					suffix = "";
					suffixLen = 0;
				}
			}
			snprintf(fullname, sizeof(fullname) - 1 - suffixLen, "%s/%.*sXXXXXX", dir,
				(int)(sizeof(fullname) - 9 - suffixLen - strlen(dir)), name);
#if	defined(C_LINUX) || defined(C_BSD) || defined(HAVE_MKSTEMP) || defined(C_SOLARIS) || defined(C_CYGWIN)
			fd = mkstemp(fullname);
#else
			(void)mktemp(fullname);
			fd = open(fullname, O_WRONLY|O_CREAT|O_EXCL|O_TRUNC|O_BINARY, 0600);
#endif

			if(fd < 0) {
				cli_errmsg("Can't create temporary file %s: %s\n", fullname, strerror(errno));
				cli_dbgmsg("%lu %d %d\n", suffixLen, sizeof(fullname), strlen(fullname));
				return CL_ETMPFILE;
			}

			cli_dbgmsg("cli_untar: extracting %s\n", fullname);

			in_block = 1;
			if((outfile = fdopen(fd, "wb")) == NULL) {
				cli_errmsg("cli_untar: cannot create file %s\n",
				    fullname);
				close(fd);
				return CL_ETMPFILE;
			}

			strncpy(osize, block+124, 12);
			osize[12] = '\0';
			size = octal(osize);
			if(size < 0){
				cli_errmsg("Invalid size in tar header\n");
				fclose(outfile);
				return CL_EDSIG;
			}
		} else { /* write or continue writing file contents */
			const int nbytes = size>512? 512:size;
			const int nwritten = fwrite(block, 1, nbytes, outfile);

			if(nwritten != nbytes) {
				cli_errmsg("cli_untar: only wrote %d bytes to file %s\n",
					nwritten, fullname);
			}
			size -= nbytes;
			if (size == 0)
				in_block = 0;
		}
	}
	if(outfile)
		fclose(outfile);
	return CL_CLEAN;
}
