//
// Created by Guanyu Wu on 12/11/21.
// Copyright (c) 2021 Guanyu Wu.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

#include "ct.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <syscall.h>
#define _GNU_SOURCE
#include <linux/sched.h>
#include <errno.h>

#undef  _GNU_SOURCE

#include "log.h"

typedef struct { uint32_t ids; uint32_t span; } id_line;

id_line extract_id_span(char* line, size_t length) {
	char* seek = memchr(line, ':', length);

	size_t subid_size = line - seek;
	size_t span_size = length - subid_size - 1;

	char* end = seek + span_size;

	id_line id_map_entry = {
		.ids = strtol(line, &seek, 10),
		.span = strtol(seek + 1, &end, 10)
	};

	return id_map_entry;
}

id_line read_file(FILE* subid_file, char* numeric, size_t num_len, char* match_alpha, size_t alpha_len) {
	char* line = NULL;
	size_t length = 0;
	ssize_t read;
	// Reads the file line by line
	while ((read = getline(&line, &length, subid_file)) != -1) {
		if (strncmp(numeric, line, num_len < read ? num_len : read) == 0) {
			// Matching uid_start
			size_t offset = num_len + 1;
			return extract_id_span(line + offset, read - offset);
		} else if (strncmp(match_alpha, line, alpha_len < read ? alpha_len : read) == 0) {
			// Matching alpha
			size_t offset = alpha_len + 1;
			return extract_id_span(line + offset, read - offset);
		}
	}

	mct_fatal(1, "Unable to read subuid/subgid mappings for current user/group!");
}

id_map system_id_maps() {
	uid_t uid = getuid();
	gid_t gid = getgid();

	size_t uid_str_len = snprintf(NULL, 0, "%d", uid);
	char* uid_str = malloc(uid_str_len + 1);
	snprintf(uid_str, uid_str_len + 1, "%d", uid);


	size_t gid_str_len = snprintf(NULL, 0, "%d", gid);
	char* gid_str = malloc(gid_str_len + 1);
	snprintf(gid_str, gid_str_len + 1, "%d", gid);

	char* usrname = getpwuid(uid)->pw_name;
	char* grpname = getgrgid(gid)->gr_name;

	FILE* subuid_file = fopen("/etc/subuid", "r");
	if (!subuid_file) mct_fatal(2, "Unable to open /etc/subuid for reading.");

	FILE* subgid_file = fopen("/etc/subgid", "r");
	if (!subgid_file) mct_fatal(2, "Unable to open /etc/sbugid for reading.");

	id_line uid_map = read_file(subuid_file, uid_str, uid_str_len, usrname, strlen(usrname));
	id_line gid_map = read_file(subgid_file, gid_str, gid_str_len, grpname, strlen(grpname));

	fclose(subuid_file);
	fclose(subgid_file);

	id_map map = {
			.uid = uid,
			.uid_start = uid_map.ids,
			.uid_count = uid_map.span,
			.gid = gid,
			.gid_start = gid_map.ids,
			.gid_count = gid_map.span
	};

	free(uid_str);
	free(gid_str);

	return map;
};

void apply_mappings(pid_t target, id_map mapping) {
	size_t uid_cmd_len = snprintf(NULL, 0, "newuidmap %d %d %d %d", target, 0, mapping.uid_start, mapping.uid_count) + 1;
	char* uid_cmd = malloc(uid_cmd_len + 1);
	sprintf(uid_cmd, "newuidmap %d %d %d %d", target, 0, mapping.uid_start, mapping.uid_count);

	size_t gid_cmd_len = snprintf(NULL, 0, "newgidmap %d %d %d %d", target, 0, mapping.gid_start, mapping.gid_count) + 1;
	char* gid_cmd = malloc(gid_cmd_len + 1);
	snprintf(gid_cmd, gid_cmd_len, "newgidmap %d %d %d %d", target, 0, mapping.uid_start, mapping.uid_count);

	system(uid_cmd);
	system(gid_cmd);

	free(uid_cmd);
	free(gid_cmd);
}



int subexec(mct_options* options) {
	// Unshare and chroots(?) if we have a non-root root for container.
	// TODO: Move to a proper system that isn't chroot.
	if (strcmp(options->root, "/") != 0) {
		int fs_unshare_status = syscall(SYS_unshare, CLONE_FS | CLONE_FILES);
		if (fs_unshare_status != 0) {
			mct_fatal(2, "Unable to unshare filesystem namespace!");
		}

		int chroot_status = chroot(options->root);
		if (chroot_status != 0) {
			mct_fatal(2, "Unable to chroot to target root filesystem!");
		}
		chdir("/");
	}

	char** exec_argv = malloc(sizeof(size_t) * (options->exec_argc));

	memcpy(exec_argv, &(options->exec_args[0]), sizeof(size_t) * options->exec_argc);

	int exec_status = execv(options->exec_args[0], exec_argv);
	if (exec_status != 0)
		mct_error("Error invoking program!");

	free(exec_argv);

	return exec_status == 0 ? 0 : errno;
}