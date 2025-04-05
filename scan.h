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

# ifndef SCAN_H
# define SCAN_H

# include <math.h>
# include <stdbool.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>

# include "draw.h"
# include "parse.h"
# include "type.h"

int seek_brk (MDS *mds, DPM *dpm, DSC *dsc) ;
int seek_spk (MDS *mds, DPM *dpm, DSC *dsc, int layer) ;
int seek_spk_50 (MDS *mds, DPM *dpm, DSC *dsc) ;
int calc_inc_amp (MDS *mds, DPM *dpm, DSC *dsc) ;
int calc_dec_amp (MDS *mds, DPM *dpm, DSC *dsc) ;
int seek_reg (MDS *mds, DSC *dsc) ;
int eval_dpm (MDS *mds, DPM *dpm, DSC *dsc) ;
int save_log (MDS *mds, DPM *dpm, DSC *dsc) ;
int show_dsc (MDS *mds, DPM *dpm, DSC *dsc) ;
int eval_reg (DSC *dsc) ;
int show_reg (DSC *dsc) ;
int eval_spk (DSC *dsc, SPK *spk) ;
int show_spk (DSC *dsc, SPK *spk) ;

# endif
