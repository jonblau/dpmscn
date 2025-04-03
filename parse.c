// DPM SCN 0.1
// PC disc image utility that displays and analyzes DPM timings from MDS files
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

# include "parse.h"

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
