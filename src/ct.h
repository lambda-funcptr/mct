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

#pragma once

#include <stdint.h>
#include <unistd.h>
#include "mct.h"

typedef struct {
	uint32_t uid;
	uint32_t uid_start;
	uint32_t uid_count;

	uint32_t gid;
	uint32_t gid_start;
	uint32_t gid_count;
} id_map;

id_map system_id_maps();

void apply_mappings(pid_t target, id_map mapping);

int subexec(mct_options* options);