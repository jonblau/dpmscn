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

# ifndef TYPE_H
# define TYPE_H

# include <stdbool.h>

typedef struct mds
{
     bool cd ;
     bool dvd ;
     unsigned int ptr ;
     unsigned int lay ;
     char mod[14] ;
     unsigned char loc ;
     unsigned int itv ;
     unsigned int smp ;
     unsigned long sct ;
}
MDS ;

typedef struct dpm
{
     unsigned long raw ;
     unsigned int tim ;
     signed int var ;
}
DPM ;

typedef struct dsc
{
     unsigned long inc_lba[200] ;
     unsigned long dec_lba[200] ;
     unsigned char inc_cnt ;
     unsigned char dec_cnt ;
     unsigned long stt_lba[10] ;
     unsigned long stp_lba[10] ;
     unsigned char stt_cnt ;
     unsigned char stp_cnt ;
     signed int inc_amp[2] ;
     signed int dec_amp[2] ;
     unsigned int brk_smp ;
     unsigned long brk_lba ;
     char trk_pth[9] ;
     unsigned int tim_avg ;
     unsigned int lay_0_avg ;
     unsigned int lay_1_avg ;
     unsigned int err_cnt ;
     unsigned int var_sum ;
     unsigned int lay_0_sum ;
     unsigned int lay_1_sum ;
     float var_rat ;
     float lay_0_rat ;
     float lay_1_rat ;
     unsigned int dpm_cat ;
}
DSC ;

typedef struct spk
{
     unsigned long len[4] ;
     float avg ;
     float dev ;
}
SPK ;

# endif
