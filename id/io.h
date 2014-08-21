/**
 * @file io.h
 * @brief i/o routines for communicating with other landslide processes
 * @author Ben Blum <bblum@andrew.cmu.edu>
 */

#ifndef __ID_IO_H
#define __ID_IO_H

#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"

// FIXME make more flexible
#define LANDSLIDE_PROGNAME "landslide"
#define LANDSLIDE_PATH "../pebsim"

struct file {
	int fd;
	char *filename;
};

#define EXPECT(cond, ...) do {						\
		if (!(cond)) {						\
			int errno0 = errno;				\
			ERR("Assertion failed: '%s'\n", #cond);		\
			ERR(__VA_ARGS__);				\
			ERR("Error: %s\n", strerror(errno0));		\
		}							\
	} while (0)

// FIXME: convert to eval-argument-once form ("int __arg = (arg);")

// FIXME: deal with short writes
#define XWRITE(file, ...) do {						\
		char __buf[BUF_SIZE];					\
		int __len = scnprintf(__buf, BUF_SIZE, __VA_ARGS__);	\
		int __ret = write((file)->fd, __buf, __len);		\
		EXPECT(__ret == __len, "failed write to file '%s'\n",	\
		       (file)->filename);				\
	} while (0)

#define XCLOSE(fd) do {							\
		int __ret = close(fd);					\
		EXPECT(__ret == 0, "failed close fd %d\n", fd);		\
	} while (0)

#define XREMOVE(filename) do {						\
		int __ret = remove(filename);				\
		EXPECT(__ret == 0, "failed remove '%s'\n", (filename));	\
	} while (0)

#define XPIPE(pipefd) do {						\
		int __ret = pipe2(pipefd, O_CLOEXEC);			\
		EXPECT(__ret == 0, "failed create pipe\n");		\
	} while (0)

#define XCHDIR(path) do {						\
		int __ret = chdir(path);				\
		EXPECT(__ret == 0, "failed chdir to '%s'\n", (path));	\
	} while (0)

#define XRENAME(oldpath, newpath) do {					\
		int __ret = rename((oldpath), (newpath));		\
		EXPECT(__ret == 0, "failed rename '%s' to '%s'\n",	\
		       (oldpath), (newpath));				\
	} while (0)

#define XDUP2(oldfd, newfd) do {					\
		int __newfd = (newfd);					\
		int __ret = dup2((oldfd), __newfd);			\
		EXPECT(__ret == __newfd, "failed dup2 %d <- %d\n",	\
		       (oldfd), __newfd);				\
	} while (0)

void create_file(struct file *f, const char *template);
void delete_file(struct file *f, bool do_remove);
void move_file_to(struct file *f, const char *dirpath);

void unset_cloexec(int fd);

#endif