# include <stdio.h>
# include <stdlib.h>
# include <stdbool.h>
# include <string.h>
# include <math.h>

/*
 DPM SCN 0.1
 PC disc image utility that displays and analyzes DPM timings from MDS files
 Copyright (c) 2025 Jon Blau

 SPDX-License-Identifier: GPL-3.0-or-later

 This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

 This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
  GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

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

typedef struct disc
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
     unsigned int err_cnt ;
     unsigned int var_sum ;
     unsigned int lay_0_sum ;
     unsigned int lay_1_sum ;
     float var_rat ;
     float lay_0_rat ;
     float lay_1_rat ;
}
DISC ;

typedef struct spike
{
     unsigned long len[4] ;
     float avg ;
     float dev ;
}
SPIKE ;

int read_mds (FILE *file, MDS *mds)
{
     char file_id[16] ;

     fseek (file, 0x00, SEEK_SET) ;
     fread (file_id, 16, 1, file) ;

     if (memcmp ("MEDIA DESCRIPTOR", file_id, 16))
     {
          fprintf (stderr, "No MDS\n") ;
          exit (EXIT_FAILURE) ;
     }

     unsigned char byte = 0 ;

     fseek (file, 0x11, SEEK_SET) ;
     fread (&byte, 1, 1, file) ;

     if (byte != 0x05)
     {
          fprintf (stderr, "Unsupported file version\n") ;
          exit (EXIT_FAILURE) ;
     }

     fseek (file, 0x12, SEEK_SET) ;
     fread (&byte, 1, 1, file) ;

     switch (byte)
     {
          case 0x00 :
               mds->cd = true ;
               break ;
          case 0x10 :
               mds->dvd = true ;
               break ;
          default :
               fprintf (stderr, "Unknown disc format\n") ;
               exit (EXIT_FAILURE) ;
     }

     fseek (file, 0x54, SEEK_SET) ;
     fread (&mds->ptr, 2, 1, file) ;

     switch (mds->ptr)
     {
          case 0x0000 :
               fprintf (stderr, "No DPM\n") ;
               exit (EXIT_FAILURE) ;
          case 0x10E8 :
               mds->lay = 1 ;
               break ;
          case 0x20EC :
               mds->lay = 2 ;
               break ;
     }

     if (mds->cd)
     {
          fseek (file, 0x168, SEEK_SET) ;
          fread (&byte, 1, 1, file) ;

          byte &= 0x0F ;

          switch (byte)
          {
               case 0x09 :
                    sprintf (mds->mod, "audio") ;
                    break ;
               case 0x0A :
                    sprintf (mds->mod, "mode 1") ;
                    break ;
               case 0x0B :
                    sprintf (mds->mod, "mode 2") ;
                    break ;
               case 0x0C :
                    sprintf (mds->mod, "mode 2 form 1") ;
                    break ;
               case 0x0D :
                    sprintf (mds->mod, "mode 2 form 2") ;
                    break ;
          }
     }
     else if (mds->dvd && mds->lay == 1)
          sprintf (mds->mod, "single layer") ;
     else if (mds->dvd && mds->lay == 2)
          sprintf (mds->mod, "double layer") ;

     unsigned int offset = 0 ;

     fseek (file, mds->ptr, SEEK_SET) ;
     fread (&mds->loc, 1, 1, file) ;

     switch (mds->loc)
     {
          case 0x01 :
               offset = mds->ptr + 16 ;
               break ;
          case 0x02 :
               offset = mds->ptr + 20 ;
               break ;
          default :
               fprintf (stderr, "Unknown header structure\n") ;
               exit (EXIT_FAILURE) ;
     }

     fseek (file, offset, SEEK_SET) ;
     fread (&mds->itv, 2, 1, file) ;

     offset += 4 ;

     fseek (file, offset, SEEK_SET) ;
     fread (&mds->smp, 2, 1, file) ;

     switch (mds->itv)
     {
          case 50 :
          case 500 :
               offset = 100 ;
               break ;
          case 256 :
          case 2048 :
               offset = mds->ptr - 128 ;
               break ;
          default :
               fprintf (stderr, "Unknown interval value\n") ;
               exit (EXIT_FAILURE) ;
     }

     fseek (file, offset, SEEK_SET) ;
     fread (&mds->sct, 3, 1, file) ;

     return 0 ;
}

int read_dpm (FILE *file, MDS *mds, DPM *dpm)
{
     unsigned int offset = 0 ;

     if (mds->loc == 0x01)
          offset = mds->ptr + 24 ;
     else if (mds->loc == 0x02)
          offset = mds->ptr + 28 ;

     fseek (file, offset, SEEK_SET) ;
     fread (&dpm[0].raw, 4, 1, file) ;

     dpm[0].tim = dpm[0].raw ;
     dpm[0].var = 0 ;

     for (int i = 1 ; i < mds->smp ; i++)
     {
          fread (&dpm[i].raw, 4, 1, file) ;

          dpm[i].tim = dpm[i].raw - dpm[i-1].raw ;
          dpm[i].var = dpm[i].tim - dpm[i-1].tim ;
     }

     return 0 ;
}

int seek_brk (MDS *mds, DPM *dpm, DISC *dsc)
{
     if (mds->cd || mds->lay < 2)
          return 0 ;

     // defining inferior and superior limits of the layer break area
     // 4.7 GB as 4700000000 is ~ 2294922 sectors of 2048 bytes/sector

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

int seek_spk (MDS *mds, DPM *dpm, DISC *dsc, const int lay_num)
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

     switch (lay_num)
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

     switch (lay_num)
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

int seek_spk_high_precision (MDS *mds, DPM *dpm, DISC *dsc)
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

int calc_inc_amp (MDS *mds, DPM *dpm, DISC *dsc)
{
     unsigned int fis = dsc->inc_lba[0] / mds->itv - 1 ;
     unsigned int lis = dsc->inc_lba[dsc->inc_cnt-1] / mds->itv - 1 ;

     dsc->inc_amp[0] = dpm[fis].var + dpm[fis+1].var + dpm[fis+2].var ;
     dsc->inc_amp[1] = dpm[lis].var + dpm[lis+1].var + dpm[lis+2].var ;

     return 0 ;
}

int calc_dec_amp (MDS *mds, DPM *dpm, DISC *dsc)
{
     unsigned int fds = dsc->dec_lba[0] / mds->itv - 1 ;
     unsigned int lds = dsc->dec_lba[dsc->dec_cnt-1] / mds->itv - 1 ;

     dsc->dec_amp[0] = dpm[fds].var + dpm[fds-1].var + dpm[fds-2].var ;
     dsc->dec_amp[1] = dpm[lds].var + dpm[lds-1].var + dpm[lds-2].var ;

     return 0 ;
}

int seek_reg (MDS *mds, DISC *dsc)
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

int eval_dpm (MDS *mds, DPM *dpm, DISC *dsc)
{
     seek_brk (mds, dpm, dsc) ;

     if (mds->itv == 50)
     {
          seek_spk_high_precision (mds, dpm, dsc) ;
     }
     else switch (mds->lay)
          {
               case 0 :
               case 1 :
                    // analyze whole disc
                    seek_spk (mds, dpm, dsc, -1) ;
                    break ;
               case 2 :
                    // analyze layer number 0
                    seek_spk (mds, dpm, dsc, 0) ;
                    // analyze layer number 1
                    seek_spk (mds, dpm, dsc, 1) ;
                    break ;
          }

     if (dsc->inc_cnt)
          calc_inc_amp (mds, dpm, dsc) ;

     if (dsc->dec_cnt)
          calc_dec_amp (mds, dpm, dsc) ;

     seek_reg (mds, dsc) ;

     return 0 ;
}

int show_dpm (MDS *mds, DPM *dpm, DISC *dsc)
{
     unsigned long sector = 0 ;
     unsigned char inc_num = 0 ;
     unsigned char dec_num = 0 ;
     char mark = ' ' ;

     for (int i = 0 ; i < mds->smp ; i++)
     {
          sector += mds->itv ;

          if (inc_num < dsc->inc_cnt && sector == dsc->inc_lba[inc_num])
          {
               printf ("\t\t\t\t\t\t\tINCREASE # %d\n", inc_num + 1) ;
               mark = '*' ;
               inc_num += 1 ;
          }
          else if (dec_num < dsc->dec_cnt && sector == dsc->dec_lba[dec_num])
          {
               printf ("\t\t\t\t\t\t\tDECREASE # %d\n", dec_num + 1) ;
               mark = ' ' ;
               dec_num += 1 ;
          }

          printf ("%+d \t %d \t %ld   \t [%ld - %ld]   \t%c\n",
                  dpm[i].var, dpm[i].tim, dpm[i].raw, sector - mds->itv, sector, mark) ;
     }

     printf ("\n") ;

     return 0 ;
}

int show_dsc (MDS *mds, DPM *dpm, DISC *dsc)
{
     printf ("Format     \t %s %s\n\n", mds->cd ? "CD" : "DVD", mds->mod) ;

     printf ("Size       \t %ld sectors\n", mds->sct) ;
     printf ("Interval   \t %d sectors\n", mds->itv) ;
     printf ("Measure    \t %d samples\n\n", mds->smp) ;

     if (dsc->brk_lba)
     {
          printf ("Break      \t LBA ~ %ld\n", dsc->brk_lba) ;
          printf ("Path       \t %s\n\n", dsc->trk_pth) ;
     }

     if (mds->lay == 2)
     {
          printf ("Layer      \t # 0\n") ;
          printf ("Timing     \t %d to %d\n", dpm[0].tim, dpm[dsc->brk_smp].tim) ;
          printf ("Variation  \t %d\n", dsc->lay_0_sum) ;
          printf ("Curve      \t %.2f %%\n\n", dsc->lay_0_rat) ;

          printf ("Layer      \t # 1\n") ;
          printf ("Timing     \t %d to %d\n", dpm[dsc->brk_smp+1].tim, dpm[mds->smp-1].tim) ;
          printf ("Variation  \t %d\n", dsc->lay_1_sum) ;
          printf ("Curve      \t %.2f %%\n\n", dsc->lay_1_rat) ;
     }
     else
     {
          printf ("Timing     \t %d to %d\n", dpm[0].tim, dpm[mds->smp-1].tim) ;
          printf ("Variation  \t %d\n", dsc->var_sum) ;
          printf ("Curve      \t %.2f %%\n\n", dsc->var_rat) ;
     }

     if (mds->itv == 50)
     {
          printf ("Accuracy   \t %d errors\n\n", dsc->err_cnt) ;
     }

     printf ("Region     \t %d starts\n", dsc->stt_cnt) ;
     printf ("           \t %d stops\n\n", dsc->stp_cnt) ;

     printf ("Spike      \t %d increases\n", dsc->inc_cnt) ;
     printf ("           \t %d decreases\n\n", dsc->dec_cnt) ;

     printf ("Amplitude  \t %+d to %+d\n", dsc->inc_amp[0], dsc->inc_amp[1]) ;
     printf ("           \t %+d to %+d\n\n", dsc->dec_amp[0], dsc->dec_amp[1]) ;

     return 0 ;
}

int eval_reg (DISC *dsc)
{
     // evaluate disc density layout consistency using count homogeneity

     if (dsc->stt_cnt == 0 && dsc->stp_cnt == 0)
     {
          printf ("Layout     \t Normal density\n") ;
          exit (EXIT_SUCCESS) ;
     }
     else if (dsc->inc_cnt != dsc->dec_cnt || dsc->stt_cnt != dsc->stp_cnt)
     {
          fprintf (stderr, "Layout     \t Unreliable\n") ;
          exit (EXIT_FAILURE) ;
     }
     else if (dsc->dec_cnt % dsc->stp_cnt != 0)
     {
          fprintf (stderr, "Layout     \t Unreliable\n") ;
          exit (EXIT_FAILURE) ;
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
               exit (EXIT_FAILURE) ;
          }
     }

     return 0 ;
}

int show_reg (DISC *dsc)
{
     unsigned char reg_cnt = dsc->stp_cnt ;
     unsigned char spr_cnt = dsc->dec_cnt / dsc->stp_cnt ;

     printf ("Layout     \t %d x %d\n\n", reg_cnt, spr_cnt) ;

     unsigned long reg_len = 0 ;

     for (int i = 0 ; i < reg_cnt ; i++)
     {
          reg_len = dsc->stp_lba[i] - dsc->stt_lba[i] ;

          printf ("Region %d \t LBA = [%ld - %ld]\n", i+1, dsc->stt_lba[i], dsc->stp_lba[i]) ;
          printf ("          \t %ld sectors\n", reg_len) ;
     }

     printf ("\n") ;

     unsigned long itv_len = 0 ;

     for (int i = 0 ; i < reg_cnt - 1 ; i++)
     {
          itv_len = dsc->stt_lba[i+1] - dsc->stp_lba[i] ;

          printf ("Gap %d    \t %ld sectors\n", i+1, itv_len) ;
     }

     if (reg_cnt > 1)
          printf ("\n") ;

     return 0 ;
}

int eval_spk (DISC *dsc, SPIKE *spk)
{
     // evaluate timing spike length consistency using standard deviation

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

int show_spk (DISC *dsc, SPIKE *spk)
{
     unsigned char spr_cnt = dsc->dec_cnt / dsc->stp_cnt ;

     for (int i = 0 ; i < spr_cnt ; i++)
     {
          printf ("Spike %d \t avg = %.0f \t dev = %.0f\n", i+1, spk[i].avg, spk[i].dev) ;
     }

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

     DISC dsc = {0} ;

     eval_dpm (&mds, dpm, &dsc) ;
     show_dpm (&mds, dpm, &dsc) ;

     show_dsc (&mds, dpm, &dsc) ;

     free (dpm) ;
     dpm = NULL ;

     eval_reg (&dsc) ;
     show_reg (&dsc) ;

     unsigned char spr_cnt = dsc.dec_cnt / dsc.stp_cnt ;

     SPIKE *spk = calloc (spr_cnt, sizeof (SPIKE)) ;
     if (spk == NULL)
          exit (EXIT_FAILURE) ;

     eval_spk (&dsc, spk) ;
     show_spk (&dsc, spk) ;

     free (spk) ;
     spk = NULL ;

     return 0 ;
}
