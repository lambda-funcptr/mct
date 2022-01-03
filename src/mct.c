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

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <syscall.h>
#include <signal.h>

#define _GNU_SOURCE
#include <linux/sched.h>
#undef  _GNU_SOURCE
#include <wait.h>

#include "mct.h"
#include "ct.h"
#include "log.h"

mct_options parse_opts(int argc, char* argv[]) {
	mct_options options = {
			.root = NULL,
			.exec_argc = 0,
			.exec_args = NULL,
			.share_opts = 0
	};

	// Scan for the '-' literal in the command line that species subexec args
	uint16_t separator;
	bool separator_found = false;
	for (int arg_it = 0; arg_it < argc; arg_it++) {
		if (strcmp(argv[arg_it], "-") == 0) {
			separator = arg_it;
			separator_found = true;
			break;
		}
	}

	if (!separator_found) {
		mct_fatal(2, "No command specified!");
	}

	options.exec_argc = argc - separator - 1;
	options.exec_args = &(argv[separator + 1]);

	int flag;
	while ((flag = getopt(separator, argv, "R:")) != -1) {
		switch (flag) {
			case 'R':
				options.root = optarg;
			default:
				break;
		}
	}

	if (options.exec_argc == 0)
		mct_fatal(1, "No command detected.");

	return options;
}



int main(int argc, char* argv[]) {
	// Print usage and exit
	if (argc == 1) {
		mct_error("mct: A simple \"container\" runtime");
		mct_error("usage: mct -R /path/too/root - /path/to/executable [args ...]");
		exit(1);
	}

	mct_options options = parse_opts(argc, argv);

	id_map uid_ns_map = system_id_maps();

	struct clone_args args = {
		.flags = CLONE_NEWUSER,
		.pidfd = (__u64) NULL,
		.child_tid = (__u64) NULL,
		.parent_tid = (__u64) NULL,
		.exit_signal = SIGCHLD,
		.stack = (__u64) NULL,
		.stack_size = 0,
	};

	long child = syscall(SYS_clone3, &args, sizeof(args));

	if (child == 0){
		raise(SIGSTOP);

		setuid(0);
		setgid(0);

		return subexec(&options);
	}

	int child_status;
	waitpid(child, &child_status, WUNTRACED);

	apply_mappings(child, uid_ns_map); // NOLINT(cppcoreguidelines-narrowing-conversions)

	kill(child, SIGCONT); // NOLINT(cppcoreguidelines-narrowing-conversions)

	return wait(NULL);
}