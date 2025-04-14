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

# ifndef LOG_H
# define LOG_H

# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include "type.h"

static int save_dsc (FILE *file, MDS *mds, DPM *dpm, DSC *dsc) ;
static int save_reg (FILE *file, DSC *dsc) ;
static int save_spk (FILE *file, DSC *dsc, SPK *spk) ;
static int save_dpm (FILE *file, MDS *mds, DPM *dpm, DSC *dsc) ;

int save_log (MDS *mds, DPM *dpm, DSC *dsc, SPK *spk, char *name) ;

# endif
