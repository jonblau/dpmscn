// DPM SCN
// Disc image utility that displays and analyzes DPM timings from MDS files
// Copyright (c) 2025 Jon Blau

// SPDX-License-Identifier: GPL-3.0-or-later

// This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.

// This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
//  along with this program. If not, see <https://www.gnu.org/licenses/>.

# ifndef DRAW_H
# define DRAW_H

# include <stdbool.h>
# include <stdlib.h>
# include <string.h>
# include <SDL.h>

# include "type.h"

static int calc_tim_crv (MDS *mds, DPM *dpm, SDL_Point *timing, int smp_stt, int smp_stp) ;
static int calc_var_crv (MDS *mds, DPM *dpm, SDL_Point *variation, int smp_stt, int smp_stp) ;

bool draw_dpm (MDS *mds, DPM *dpm, char *name) ;

# endif
