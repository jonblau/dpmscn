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

# include "log.h"

static int save_dsc (FILE *file, MDS *mds, DPM *dpm, DSC *dsc)
{
     fprintf (file, "Format     \t %s %s\n\n", mds->cd ? "CD" : "DVD", mds->mod) ;

     fprintf (file, "Size       \t %ld sectors\n", mds->sct) ;
     fprintf (file, "Interval   \t %d sectors\n", mds->itv) ;
     fprintf (file, "Measure    \t %d samples\n\n", mds->smp) ;

     if (mds->lay == 2)
     {
          fprintf (file, "Path       \t %s\n\n", dsc->trk_pth) ;

          fprintf (file, "Layer      \t # 0\n") ;
          fprintf (file, "Timing     \t %d [%d - %d]\n", dsc->lay_0_avg, dpm[0].tim, dpm[dsc->brk_smp].tim) ;
          fprintf (file, "Variation  \t %d\n", dsc->lay_0_sum) ;
          fprintf (file, "Curve      \t %.2f %%\n\n", dsc->lay_0_rat) ;

          fprintf (file, "Break      \t LBA ~ %ld\n\n", dsc->brk_lba) ;

          fprintf (file, "Layer      \t # 1\n") ;
          fprintf (file, "Timing     \t %d [%d - %d]\n", dsc->lay_1_avg, dpm[dsc->brk_smp+1].tim, dpm[mds->smp-1].tim) ;
          fprintf (file, "Variation  \t %d\n", dsc->lay_1_sum) ;
          fprintf (file, "Curve      \t %.2f %%\n\n", dsc->lay_1_rat) ;
     }
     else
     {
          fprintf (file, "Timing     \t %d [%d - %d]\n", dsc->tim_avg, dpm[0].tim, dpm[mds->smp-1].tim) ;
          fprintf (file, "Variation  \t %d\n", dsc->var_sum) ;
          fprintf (file, "Curve      \t %.2f %%\n\n", dsc->var_rat) ;
     }

     if (mds->itv == 50)
     {
          fprintf (file, "Accuracy   \t %d errors\n\n", dsc->err_cnt) ;
     }

     fprintf (file, "Region     \t %d starts\n", dsc->stt_cnt) ;
     fprintf (file, "           \t %d stops\n\n", dsc->stp_cnt) ;

     fprintf (file, "Spike      \t %d increases\n", dsc->inc_cnt) ;
     fprintf (file, "           \t %d decreases\n\n", dsc->dec_cnt) ;

     fprintf (file, "Amplitude  \t %+d to %+d\n", dsc->inc_amp[0], dsc->inc_amp[1]) ;
     fprintf (file, "           \t %+d to %+d\n\n", dsc->dec_amp[0], dsc->dec_amp[1]) ;

     return 0 ;
}

static int save_reg (FILE *file, DSC *dsc)
{
     if (dsc->stp_cnt == 0)
          return 1 ;

     unsigned char reg_cnt = dsc->stp_cnt ;
     unsigned char spr_cnt = dsc->dec_cnt / dsc->stp_cnt ;

     fprintf (file, "Layout     \t %d x %d\n\n", reg_cnt, spr_cnt) ;

     unsigned long reg_len = 0 ;

     for (int i = 0 ; i < reg_cnt ; i++)
     {
          reg_len = dsc->stp_lba[i] - dsc->stt_lba[i] ;

          fprintf (file, "Region %d   \t LBA = [%ld - %ld]\n", i+1, dsc->stt_lba[i], dsc->stp_lba[i]) ;
          fprintf (file, "            \t %ld sectors\n", reg_len) ;
     }

     fprintf (file, "\n") ;

     unsigned long itv_len = 0 ;

     for (int i = 0 ; i < reg_cnt - 1 ; i++)
     {
          itv_len = dsc->stt_lba[i+1] - dsc->stp_lba[i] ;

          fprintf (file, "Gap %d      \t %ld sectors\n", i+1, itv_len) ;
     }

     if (reg_cnt > 1)
          fprintf (file, "\n") ;

     return 0 ;
}

static int save_spk (FILE *file, DSC *dsc, SPK *spk)
{
     if (dsc->stp_cnt == 0)
          return 1 ;

     unsigned char spr_cnt = dsc->dec_cnt / dsc->stp_cnt ;

     for (int i = 0 ; i < spr_cnt ; i++)
     {
          fprintf (file, "Spike %d    \t avg = %.0f \t dev = %.0f\n", i+1, spk[i].avg, spk[i].dev) ;
     }

     fprintf (file, "\n") ;

     return 0 ;
}

static int save_dpm (FILE *file, MDS *mds, DPM *dpm, DSC *dsc)
{
     unsigned long sector = 0 ;
     unsigned char inc_num = 0 ;
     unsigned char dec_num = 0 ;
     char mark = '|' ;

     for (int i = 0 ; i < mds->smp ; i++)
     {
          sector += mds->itv ;

          if (inc_num < dsc->inc_cnt && sector == dsc->inc_lba[inc_num])
          {
               fprintf (file, "\t\t\t\t\t\t\t\t   INCREASE # %d\n", inc_num + 1) ;
               mark = '>' ;
               inc_num += 1 ;
          }
          else if (dec_num < dsc->dec_cnt && sector == dsc->dec_lba[dec_num])
          {
               fprintf (file, "\t\t\t\t\t\t\t\t   DECREASE # %d\n", dec_num + 1) ;
               mark = '|' ;
               dec_num += 1 ;
          }

          fprintf (file, "[%07ld - %07ld] %08ld %d %+d \t %c\n",
                   sector - mds->itv, sector, dpm[i].raw, dpm[i].tim, dpm[i].var, mark) ;
     }

     return 0 ;
}

int save_log (MDS *mds, DPM *dpm, DSC *dsc, SPK *spk)
{
     FILE *file = fopen ("scan.log", "w") ;
     if (file == NULL)
          return 1 ;

     save_dsc (file, mds, dpm, dsc) ;
     save_reg (file, dsc) ;
     save_spk (file, dsc, spk) ;
     save_dpm (file, mds, dpm, dsc) ;

     fclose (file) ;

     return 0 ;
}
