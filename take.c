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

#include "system.h"

char *template;

void cleanup()
{
	if (template) unlink(template);
}

void say(char *s)
{
	int rc = write (2, s, strlen(s));
	/* who exactly were you going to tell that STDERR was hosed ?!? */
	(void)rc;
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

void usage()
{
	die("usage: take [ -m mode ] file1 [file2...]\n");
}

int main (int argc, char **argv)
{
	template = NULL;

	mode_t mymask = umask(0077);
	mode_t mymode = 0666 & ~mymask;
	int opt;

	while ((opt = getopt(argc, argv, "+m:")) != -1) {
		switch(opt) {
		case 'm':
			mymode = strtol(optarg,(char **)NULL, 8);
			break;
		default:
			usage();
			}
		}

	if (optind >= argc) usage();

	while (optind < argc) {
		int i = strlen(argv[optind]);
		template = malloc(i+8);
		if (!template) die("malloc failed\n");
		memcpy(template, argv[optind], i);
		memcpy(template+i, "XXXXXX\0", 7);

		int ifd = open(argv[optind], O_RDONLY);
		if (ifd == -1)  sysdie(argv[optind]);
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
		if (chmod(template, mymode) == -1) sysdie(template);
		if (rename(template, argv[optind])) sysdie("rename failed\n");
		free(template);
		template = NULL;
		optind++;
	}

	return 0;
}
