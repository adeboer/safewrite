/* safewrite.c

	Copyright (C) 2003 Anthony de Boer

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
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>

char *template;

void cleanup() {
	if (template) unlink(template);
	}

void say(char *s) {
	write (2, s, strlen(s));
	}

void die(char *s) {
	say(s);
	cleanup();
	exit(1);
	}

void sysdie(char *s) {
	perror(s);
	cleanup();
	exit(1);
	}

int main (int argc, char **argv) {

	int i, fd, fildes[2], pid, wpid, status;
	int doit = 1;
	struct stat sbuf;

	template = NULL;

	if (argc < 3) die("usage: safewrite targetfile command [args]\n");

	i = strlen(argv[1]);
	template = malloc(i+8);
	if (!template) die("malloc failed\n");
	memcpy(template, argv[1], i);
	memcpy(template+i, "XXXXXX\0", 7);

	fd = mkstemp(template);
	if (fd == -1) sysdie(template);

	if (pipe(fildes)) sysdie("pipe");

	if (signal(SIGCHLD, SIG_DFL) == SIG_ERR) die("signal() failure\n");
	pid = fork();

	switch(pid) {
		case 0:
			if (dup2(fildes[1], 1) == -1) sysdie("dup2 stdout\n");
			close(fildes[0]);
			close(fildes[1]);
			close(fd);
			execvp(argv[2], argv+2);
			sysdie(argv[2]);
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
		char buffer[4096];
		int rd = read(fildes[0], buffer, sizeof(buffer));
		int wr;
		switch(rd) {
			case -1:
				sysdie("read failed");
				;;
			case 0:
				doit = 0;
				;;
			default:
				wr = write(fd, buffer, rd);
				if (wr == -1) sysdie("write failed\n");
				if (wr != rd) die("incomplete write\n");
				;;
			}
		}

	close(fildes[0]);

	if (fsync(fd) == -1) sysdie("fsync\n");
	close(fd);

	if (stat(argv[1], &sbuf) == 0) {
		uid_t fuid;
		if (chown(template, getuid() ? -1 : sbuf.st_uid, sbuf.st_gid) == -1) sysdie(template);
		if (chmod(template, sbuf.st_mode) == -1) sysdie(template);
		}
	else if (errno != ENOENT) {
		sysdie(argv[1]);
		}

	wpid = wait(&status);
	if (wpid == -1) sysdie("wait error\n");
	if (wpid != pid) die("wrong pid?!?\n");

	if (WIFEXITED(status)) {
		int es = WEXITSTATUS(status);
		if (es) {
			cleanup();
			exit(es);
			}
		}
	else {
		die("command killed\n");
		}

	if (rename(template, argv[1])) sysdie("rename failed\n");

	return 0;
	}
