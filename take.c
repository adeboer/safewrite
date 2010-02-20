/* take.c

	Copyright (C) 2003, 2009 Anthony de Boer

	This program is free software; you can redistribute it and/or modify
	it under the terms of version 2 of the GNU General Public License as
	published by the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
	USA
*/

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

char *template;

void cleanup()
{
	if (template) unlink(template);
}

void say(char *s)
{
	write (2, s, strlen(s));
}

void die(char *s)
{
	say(s);
	cleanup();
	exit(1);
}

void sysdie(char *s)
{
	perror(s);
	cleanup();
	exit(1);
}

int main (int argc, char **argv)
{
	int fno;

	template = NULL;

	if (argc < 2) die("usage: take [files]\n");

	for (fno = 1; fno < argc; fno++) {
		int i = strlen(argv[fno]);
		template = malloc(i+8);
		if (!template) die("malloc failed\n");
		memcpy(template, argv[fno], i);
		memcpy(template+i, "XXXXXX\0", 7);

		int ifd = open(argv[fno], O_RDONLY);
		if (ifd == -1) sysdie(argv[fno]);
		int ofd = mkstemp(template);
		if (ofd == -1) sysdie(template);
		int doit = 1;

		do {
			char buffer[4096];
			int rd = read(ifd, buffer, sizeof(buffer));
			int wr;
			switch(rd) {
			case -1:
				sysdie("read failed");
				break;
			case 0:
				doit = 0;
				break;
			default:
				wr = write(ofd, buffer, rd);
				if (wr == -1) sysdie("write failed\n");
				if (wr != rd) die("incomplete write\n");
				break;
			}
		} while (doit);

		close(ifd);
		if (fsync(ofd) == -1) sysdie("fsync\n");
		close(ofd);
		if (rename(template, argv[fno])) sysdie("rename failed\n");
		free(template);
		template = NULL;
	}
	return 0;
}
