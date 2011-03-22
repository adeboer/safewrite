/* safewrite.c

	Copyright (C) 2003,2008 Anthony de Boer

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

#define BUFSIZE 16384

char *template;			/* temp filename */
mode_t mymask;			/* permissions for new file */
int fd;				/* file descriptor for temp file */
int oldfd;			/* file descriptor for diffing old file */
unsigned long same = 0;		/* identical bytes passed over */
char *basename;			/* original filename */
int diffing = 0;		/* true if diffing old file */
char buff1[BUFSIZE];		/* new data from command */
char buff2[BUFSIZE];		/* used to handle data from old file */

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
	die("usage: safewrite [ -m mode ] targetfile command [args]\n");
}

void opentemp()
{
	int i = strlen(basename);
	template = malloc(i+8);
	if (!template) die("malloc failed\n");
	memcpy(template, basename, i);
	memcpy(template+i, "XXXXXX", 7);
	mymask = umask(0077);
	fd = mkstemp(template);
	if (fd == -1) sysdie(template);
	umask(mymask);
}

void different()
{
	opentemp();
	diffing = 0;
	if (same) {
		if (lseek(oldfd, 0, SEEK_SET) == -1) sysdie("lseek");
		while(same > 0) {
			int bwant = BUFSIZE;
			if (bwant > same) bwant = same;
			int rd = read(oldfd, buff2, bwant);
			if (rd == -1) sysdie("reread");
			if (rd < 1) die("short reread");
			int wr = write(fd, buff2, rd);
			if (wr == -1) sysdie("write failed");
			if (wr != rd) die("incomplete write");
			same -= rd;
		}
	}
}

int main (int argc, char **argv)
{
	int fildes[2], pid, wpid, status, opt;
	int doit = 1;
	int modeopt = 0;
	struct stat sbuf;
	mode_t mymode;

	template = NULL;

	while ((opt = getopt(argc, argv, "+m:s")) != -1) {
		switch(opt) {
		case 'm':
			mymode = strtol(optarg,(char **)NULL, 8);
			modeopt = 1;
			break;
		case 's':
			diffing = 1;
			break;
		default:
			usage();
		}
	}

	if (argc < optind+2) usage();
	basename = argv[optind];

	if (pipe(fildes)) sysdie("pipe");
	if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) die("signal() failure\n");

	if (diffing) {
		oldfd = open(basename, O_RDONLY);
		if (oldfd == -1) diffing = 0;
	}

	if (!diffing) opentemp();

	pid = fork();

	switch(pid) {
		case 0:
			if (dup2(fildes[1], 1) == -1) sysdie("dup2 stdout");
			close(fildes[0]);
			close(fildes[1]);
			if (!diffing) close(fd);
			execvp(argv[optind+1], argv+optind+1);
			sysdie(argv[optind+1]);
			;;
		case -1:
			sysdie("cannot fork");
			;;
		default:
			close(fildes[1]);
			close(0);
			close(1);
			;;
	}

	while(doit) {
		int rd = read(fildes[0], buff1, sizeof(buff1));
		switch(rd) {
		case -1:
			sysdie("read failed");
			/* NOTREACHED */
		case 0:
			doit = 0;
			break;
		default:
			if (diffing) {
				int rx = read(oldfd, buff2, rd);
				int ok = (rx == rd);
				while (ok && rx--) {
					ok = (buff2[rx] == buff1[rx]);
				}
				if (ok) {
					same += rd;
				} else {
					different();
				}
			}
			if (!diffing) {
				int wr = write(fd, buff1, rd);
				if (wr == -1) sysdie("write failed");
				if (wr != rd) die("incomplete write\n");
			}
			break;
		}
	}

	if (diffing) {
		int rd = read(oldfd, buff2, 1);
		if (rd > 0) different();
	}

	close(fildes[0]);

	wpid = wait(&status);
	if (wpid == -1) sysdie("wait error");
	if (wpid != pid) die("wrong pid?!?\n");

	if (WIFEXITED(status)) {
		int es = WEXITSTATUS(status);
		if (es) {
			cleanup();
			exit(es);
		}
	} else {
		die("command killed\n");
	}

	if (diffing) exit(0);

	if (fsync(fd) == -1) sysdie("fsync");

	if (stat(basename, &sbuf) == 0) {
#ifdef HAVE_CHOWN
		if (fchown(fd, getuid() ? -1 : sbuf.st_uid, sbuf.st_gid) == -1) sysdie(template);
#endif
		if (modeopt == 0) mymode = sbuf.st_mode;
	} else if (errno == ENOENT) {
		if (modeopt == 0) mymode = ~mymask;
	} else {
		sysdie(basename);
	}

	if (fchmod(fd, mymode & 0777) == -1) sysdie(template);

	if (close(fd) == -1) sysdie("close");

	if (rename(template, basename)) sysdie("rename failed");

	return 0;
}
