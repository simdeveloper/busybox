/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 2005, 2006 Rob Landley <rob@landley.net>
 * Copyright (C) 2004 Erik Andersen <andersen@codepoet.org>
 * Copyright (C) 2001 Matt Krai
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include "libbb.h"

/* This function reads an entire line from a text file, up to a newline
 * or NUL byte, inclusive.  It returns a malloc'ed char * which
 * must be free'ed by the caller.  If end is NULL '\n' isn't considered
 * end of line.  If end isn't NULL, length of the chunk read is stored in it.
 * Return NULL if EOF/error */
char* FAST_FUNC bb_get_chunk_from_file(FILE *file, int *end)
{
	int ch;
	int idx = 0;
	char *linebuf = NULL;
	int linebufsz = 0;

	while ((ch = getc(file)) != EOF) {
		/* grow the line buffer as necessary */
		if (idx >= linebufsz) {
			linebufsz += 80;
			linebuf = xrealloc(linebuf, linebufsz);
		}
		linebuf[idx++] = (char) ch;
		if (!ch || (end && ch == '\n'))
			break;
	}
	if (end)
		*end = idx;
	if (linebuf) {
		// huh, does fgets discard prior data on error like this?
		// I don't think so....
		//if (ferror(file)) {
		//	free(linebuf);
		//	return NULL;
		//}
		linebuf = xrealloc(linebuf, idx + 1);
		linebuf[idx] = '\0';
	}
	return linebuf;
}

/* Get line, including trailing \n if any */
char* FAST_FUNC xmalloc_fgets(FILE *file)
{
	int i;

	return bb_get_chunk_from_file(file, &i);
}

/* Get line.  Remove trailing \n */
char* FAST_FUNC xmalloc_fgetline(FILE *file)
{
	int i;
	char *c = bb_get_chunk_from_file(file, &i);

	if (i && c[--i] == '\n')
		c[i] = '\0';

	return c;
}

/* Faster routines (~twice as fast). +170 bytes. Unused as of 2008-07.
 *
 * NB: they stop at NUL byte too.
 * Performance is important here. Think "grep 50gigabyte_file"...
 * Iironically, grep can't use it because of NUL issue.
 * We sorely need C lib to provide fgets which reports size!
 */

static char* xmalloc_fgets_internal(FILE *file, int *sizep)
{
	int len;
	int idx = 0;
	char *linebuf = NULL;

	while (1) {
		char *r;

		linebuf = xrealloc(linebuf, idx + 0x100);
		r = fgets(&linebuf[idx], 0x100, file);
		if (!r) {
			/* need to terminate in case this is error
			 * (EOF puts NUL itself) */
			linebuf[idx] = '\0';
			break;
		}
		/* stupid. fgets knows the len, it should report it somehow */
		len = strlen(&linebuf[idx]);
		idx += len;
		if (len != 0xff || linebuf[idx - 1] == '\n')
			break;
	}
	*sizep = idx;
	if (idx) {
		/* xrealloc(linebuf, idx + 1) is up to caller */
		return linebuf;
	}
	free(linebuf);
	return NULL;
}

/* Get line, remove trailing \n */
char* FAST_FUNC xmalloc_fgetline_fast(FILE *file)
{
	int sz;
	char *r = xmalloc_fgets_internal(file, &sz);
	if (r && r[sz - 1] == '\n')
		r[--sz] = '\0';
	return r; /* not xrealloc(r, sz + 1)! */
}

#if 0
char* FAST_FUNC xmalloc_fgets(FILE *file)
{
	int sz;
	return xmalloc_fgets_internal(file, &sz);
}

/* Get line, remove trailing \n */
char* FAST_FUNC xmalloc_fgetline(FILE *file)
{
	int sz;
	char *r = xmalloc_fgets_internal(file, &sz);
	if (!r)
		return r;
	if (r[sz - 1] == '\n')
		r[--sz] = '\0';
	return xrealloc(r, sz + 1);
}
#endif
