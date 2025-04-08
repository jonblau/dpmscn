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

# include "scan.h"

int seek_brk (MDS *mds, DPM *dpm, DSC *dsc)
{
     if (mds->cd || mds->lay < 2)
          return 0 ;

     // inferior and superior limits of the layer break area

     unsigned int smp_inf = (mds->sct / 2) / mds->itv - 1 ;
     unsigned int smp_sup = (2294922) / mds->itv - 1 ;

     unsigned int brk_tim = dpm[smp_inf].tim ;

     for (int i = smp_inf ; i <= smp_sup ; i++)
     {
          if (brk_tim >= dpm[i].tim)
          {
               brk_tim = dpm[i].tim ;
               dsc->brk_smp = i ;
          }
     }

     dsc->brk_lba = (dsc->brk_smp + 1) * mds->itv ;

     if (abs (dpm[smp_inf].tim - dpm[smp_sup+1].tim) < 100)
          sprintf (dsc->trk_pth, "opposite") ;
     else sprintf (dsc->trk_pth, "parallel") ;

     return 0 ;
}

int seek_spk (MDS *mds, DPM *dpm, DSC *dsc, int layer)
{
     unsigned int var_min = 0 ;
     unsigned int var_max = 0 ;

     switch (mds->itv)
     {
          case 256 :
               var_min = 10 ;
               var_max = 60 ;
               break ;
          case 500 :
          case 2048 :
               var_min = 100 ;
               var_max = 400 ;
               break ;
     }

     unsigned int smp_stt = 0 ;
     unsigned int smp_stp = mds->smp - 1 ;

     switch (layer)
     {
          case 0 :
               smp_stt = 0 ;
               smp_stp = dsc->brk_smp ;
               break ;
          case 1 :
               smp_stt = dsc->brk_smp + 1 ;
               smp_stp = mds->smp - 1 ;
               break ;
     }

     unsigned long sector = smp_stt * mds->itv ;
     dsc->var_sum = 0 ;

     for (int i = smp_stt ; i <= smp_stp ; i++)
     {
          sector += mds->itv ;

          // spike increase detection

          if (dpm[i].var > var_min && dpm[i].var < var_max)
          {
               if (dsc->inc_cnt == 200)
               {
                    fprintf (stderr, "Abnormal increase count, exiting...\n") ;
                    exit (EXIT_FAILURE) ;
               }

               dsc->inc_lba[dsc->inc_cnt] = sector ;
               dsc->inc_cnt += 1 ;
               i += 1 ;
               sector += mds->itv ;
               continue ;
          }

          // spike decrease detection

          if (dpm[i].var < -var_min && dpm[i].var > -var_max)
          {
               if (dsc->dec_cnt == 200)
               {
                    fprintf (stderr, "Abnormal decrease count, exiting...\n") ;
                    exit (EXIT_FAILURE) ;
               }

               dsc->dec_lba[dsc->dec_cnt] = sector ;
               dsc->dec_cnt += 1 ;
               i += 1 ;
               sector += mds->itv ;
               continue ;
          }

          dsc->var_sum += abs (dpm[i].var) ;
     }

     dsc->var_rat = (float) abs (dpm[smp_stt].tim - dpm[smp_stp].tim) * 100 / dsc->var_sum ;

     switch (layer)
     {
          case 0 :
               dsc->lay_0_sum = dsc->var_sum ;
               dsc->lay_0_rat = dsc->var_rat ;
               break ;
          case 1 :
               dsc->lay_1_sum = dsc->var_sum ;
               dsc->lay_1_rat = dsc->var_rat ;
               break ;
     }

     return 0 ;
}

int seek_spk_50 (MDS *mds, DPM *dpm, DSC *dsc)
{
     unsigned long sector = 0 ;

     for (int i = 0 ; i < mds->smp ; i++)
     {
          sector += mds->itv ;

          // spike increase detection

          if (dpm[i].var > 3 && dpm[i].var < 33 && dpm[i].var + dpm[i+1].var > 13)
          {
               // false positive caused by variation artifact

               if (dpm[i-2].var + dpm[i-1].var < -9 || dpm[i+2].var + dpm[i+3].var < -9)
               {
                    dsc->err_cnt += 1 ;
                    dsc->var_sum += abs (dpm[i].var) ;
                    continue ;
               }

               // false positive caused by previous increase

               if (dpm[i-1].var > 9)
               {
                    dsc->err_cnt += 1 ;
                    dsc->var_sum += abs (dpm[i].var) ;
                    continue ;
               }

               // true positive
               // now determining the first increase sector

               if (dsc->inc_cnt == 200)
               {
                    fprintf (stderr, "Abnormal increase count, exiting...\n") ;
                    exit (EXIT_FAILURE) ;
               }

               dsc->inc_lba[dsc->inc_cnt] = sector ;
               dsc->inc_cnt += 1 ;
               i += 2 ;
               sector += mds->itv * 2 ;
               continue ;
          }

          // spike decrease detection

          if (dpm[i].var < -3 && dpm[i].var > -33 && dpm[i].var + dpm[i+1].var < -13)
          {
               // false positive caused by variation artifact

               if (dpm[i-2].var + dpm[i-1].var > 9 || dpm[i+2].var + dpm[i+3].var > 9)
               {
                    dsc->err_cnt += 1 ;
                    dsc->var_sum += abs (dpm[i].var) ;
                    continue ;
               }

               // false positive caused by previous decrease

               if (dpm[i-1].var < -9)
               {
                    dsc->err_cnt += 1 ;
                    dsc->var_sum += abs (dpm[i].var) ;
                    continue ;
               }

               // true positive
               // now determining the last decrease sector

               if (dsc->dec_cnt == 200)
               {
                    fprintf (stderr, "Abnormal decrease count, exiting...\n") ;
                    exit (EXIT_FAILURE) ;
               }

               dsc->dec_lba[dsc->dec_cnt] = sector ;

               if (dpm[i+1].var < -3)
                    dsc->dec_lba[dsc->dec_cnt] += mds->itv ;

               if (dpm[i+1].var < -3 && dpm[i+2].var < -3)
                    dsc->dec_lba[dsc->dec_cnt] += mds->itv ;

               dsc->dec_cnt += 1 ;
               i += 2 ;
               sector += mds->itv * 2 ;
               continue ;
          }

          dsc->var_sum += abs (dpm[i].var) ;
     }

     dsc->var_rat = (float) (dpm[0].tim - dpm[mds->smp-1].tim) * 100 / dsc->var_sum ;

     return 0 ;
}

int calc_inc_amp (MDS *mds, DPM *dpm, DSC *dsc)
{
     unsigned int fis = dsc->inc_lba[0] / mds->itv - 1 ;
     unsigned int lis = dsc->inc_lba[dsc->inc_cnt-1] / mds->itv - 1 ;

     dsc->inc_amp[0] = dpm[fis].var + dpm[fis+1].var + dpm[fis+2].var ;
     dsc->inc_amp[1] = dpm[lis].var + dpm[lis+1].var + dpm[lis+2].var ;

     return 0 ;
}

int calc_dec_amp (MDS *mds, DPM *dpm, DSC *dsc)
{
     unsigned int fds = dsc->dec_lba[0] / mds->itv - 1 ;
     unsigned int lds = dsc->dec_lba[dsc->dec_cnt-1] / mds->itv - 1 ;

     dsc->dec_amp[0] = dpm[fds].var + dpm[fds-1].var + dpm[fds-2].var ;
     dsc->dec_amp[1] = dpm[lds].var + dpm[lds-1].var + dpm[lds-2].var ;

     return 0 ;
}

int seek_reg (MDS *mds, DSC *dsc)
{
     unsigned int threshold = 0 ;

     if (mds->cd)
          threshold = 4000 ;
     else if (mds->dvd)
          threshold = 40000 ;

     // region start detection

     if (dsc->inc_cnt > 0)
     {
          dsc->stt_lba[0] = dsc->inc_lba[0] ;
          dsc->stt_cnt += 1 ;
     }

     for (int i = 1 ; i < dsc->inc_cnt ; i++)
     {
          if (dsc->inc_lba[i] - dsc->inc_lba[i-1] > threshold)
          {
               if (dsc->stt_cnt == 10)
               {
                    fprintf (stderr, "Abnormal start count, exiting...\n") ;
                    exit (EXIT_FAILURE) ;
               }

               dsc->stt_lba[dsc->stt_cnt] = dsc->inc_lba[i] ;
               dsc->stt_cnt += 1 ;
          }
     }

     // region stop detection

     for (int i = 1 ; i < dsc->dec_cnt ; i++)
     {
          if (dsc->dec_lba[i] - dsc->dec_lba[i-1] > threshold)
          {
               if (dsc->stp_cnt == 9)
               {
                    fprintf (stderr, "Abnormal stop count, exiting...\n") ;
                    exit (EXIT_FAILURE) ;
               }

               dsc->stp_lba[dsc->stp_cnt] = dsc->dec_lba[i-1] ;
               dsc->stp_cnt += 1 ;
          }
     }

     if (dsc->dec_cnt > 0)
     {
          dsc->stp_lba[dsc->stp_cnt] = dsc->dec_lba[dsc->dec_cnt-1] ;
          dsc->stp_cnt += 1 ;
     }

     return 0 ;
}

int eval_reg (DSC *dsc)
{
     // evaluate disc density layout consistency using count homogeneity

     if (dsc->stt_cnt == 0 && dsc->stp_cnt == 0)
     {
          printf ("Layout     \t Normal density\n") ;
          return 1 ;
     }
     else if (dsc->inc_cnt != dsc->dec_cnt || dsc->stt_cnt != dsc->stp_cnt)
     {
          fprintf (stderr, "Layout     \t Unreliable\n") ;
          return 1 ;
     }
     else if (dsc->dec_cnt % dsc->stp_cnt != 0)
     {
          fprintf (stderr, "Layout     \t Unreliable\n") ;
          return 1 ;
     }

     unsigned char reg_cnt = dsc->stp_cnt ;
     unsigned char spk_cnt = dsc->dec_cnt ;
     unsigned char spr_cnt = dsc->dec_cnt / dsc->stp_cnt ;

     unsigned char ipr_cnt = 0 ;
     unsigned char dpr_cnt = 0 ;
     unsigned char inc_num = 0 ;
     unsigned char dec_num = 0 ;

     for (int i = 0 ; i < reg_cnt ; i++)
     {
          ipr_cnt = 0 ;
          dpr_cnt = 0 ;

          for (int j = inc_num ; j < spk_cnt ; j++)
          {
               if (dsc->inc_lba[j] >= dsc->stt_lba[i] && dsc->inc_lba[j] < dsc->stp_lba[i])
                    ipr_cnt += 1 ;
               else break ;
          }

          inc_num += ipr_cnt ;

          for (int j = dec_num ; j < spk_cnt ; j++)
          {
               if (dsc->dec_lba[j] <= dsc->stp_lba[i] && dsc->dec_lba[j] > dsc->stt_lba[i])
                    dpr_cnt += 1 ;
               else break ;
          }

          dec_num += dpr_cnt ;

          if (ipr_cnt != spr_cnt || dpr_cnt != spr_cnt)
          {
               fprintf (stderr, "Layout     \t Unreliable\n") ;
               return 1 ;
          }
     }

     return 0 ;
}

int eval_spk (DSC *dsc, SPK *spk)
{
     // evaluate timing spike length consistency using standard deviation

     if (dsc->stp_cnt == 0)
          return 1 ;

     unsigned char reg_cnt = dsc->stp_cnt ;
     unsigned char spr_cnt = dsc->dec_cnt / dsc->stp_cnt ;

     unsigned char spk_num = 0 ;
     float avg_dev = 0 ;

     for (int i = 0 ; i < spr_cnt ; i++)
     {
          for (int j = 0 ; j < reg_cnt ; j++)
          {
               spk_num = i + j * spr_cnt ;

               spk[i].len[j] = dsc->dec_lba[spk_num] - dsc->inc_lba[spk_num] ;
               spk[i].avg += (float) spk[i].len[j] ;
          }

          spk[i].avg /= (float) reg_cnt ;

          for (int j = 0 ; j < reg_cnt ; j++)
          {
               avg_dev = spk[i].len[j] - spk[i].avg ;
               spk[i].dev += avg_dev * avg_dev ;
          }

          spk[i].dev = sqrtf (spk[i].dev / reg_cnt) ;
     }

     return 0 ;
}

int eval_dpm (MDS *mds, DPM *dpm, DSC *dsc)
{
     seek_brk (mds, dpm, dsc) ;

     if (mds->itv == 50)
     {
          seek_spk_50 (mds, dpm, dsc) ;
     }
     else switch (mds->lay)
     {
          case 0 :
          case 1 :
               // analyze whole disc
               seek_spk (mds, dpm, dsc, -1) ;
               break ;
          case 2 :
               // analyze layer # 0
               seek_spk (mds, dpm, dsc, 0) ;
               // analyze layer # 1
               seek_spk (mds, dpm, dsc, 1) ;
               break ;
     }

     if (dsc->inc_cnt)
          calc_inc_amp (mds, dpm, dsc) ;

     if (dsc->dec_cnt)
          calc_dec_amp (mds, dpm, dsc) ;

     seek_reg (mds, dsc) ;
     eval_reg (dsc) ;

     unsigned char spr_cnt = 0 ;

     if (dsc->stp_cnt)
          spr_cnt = dsc->dec_cnt / dsc->stp_cnt ;

     SPK *spk = calloc (spr_cnt, sizeof (SPK)) ;
     if (spk == NULL)
     {
          free (dpm) ;
          dpm = NULL ;
          exit (EXIT_FAILURE) ;
     }

     eval_spk (dsc, spk) ;

     save_log (mds, dpm, dsc, spk) ;

     free (spk) ;
     spk = NULL ;

     return 0 ;
}

int main (int argc, char **argv)
{
     if (argc < 2)
          exit (EXIT_FAILURE) ;

     char *path = argv[1] ;

     FILE *file = fopen (path, "rb") ;
     if (file == NULL)
          exit (EXIT_FAILURE) ;

     MDS mds = {0} ;

     read_mds (file, &mds) ;

     DPM *dpm = calloc (mds.smp, sizeof (DPM)) ;
     if (dpm == NULL)
          exit (EXIT_FAILURE) ;

     read_dpm (file, &mds, dpm) ;

     fclose (file) ;
     file = NULL ;

     # if LINUX

     int pid = fork () ;
     if (pid == 0)
     {
          draw_dpm (&mds, dpm) ;

          free (dpm) ;
          dpm = NULL ;

          return 0 ;
     }

     # else

     draw_dpm (&mds, dpm) ;

     # endif

     DSC dsc = {0} ;

     eval_dpm (&mds, dpm, &dsc) ;

     free (dpm) ;
     dpm = NULL ;

     return 0 ;
}
