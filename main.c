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

# include <stdio.h>
# include <stdlib.h>

# if LINUX
# include <unistd.h>
# endif

# include "type.h"
# include "parse.h"
# include "draw.h"
# include "scan.h"
# include "log.h"

int main (int argc, char **argv)
{
     FILE *file = NULL ;
     char *name = NULL ;
     DPM *dpm = NULL ;
     SPK *spk = NULL ;

     int error = 0 ;

     if (argc < 2)
          { error = 1 ; goto quit ; }

     char *path = argv[1] ;

     if (get_name (path, &name) != 0)
          { error = 2 ; goto quit ; }

     file = fopen (path, "rb") ;
     if (file == NULL)
          { error = 3 ; goto quit ; }

     MDS mds = {0} ;

     read_mds (file, &mds) ;

     dpm = calloc (mds.smp, sizeof (DPM)) ;
     if (dpm == NULL)
          { error = 4 ; goto quit ; }

     read_dpm (file, &mds, dpm) ;

     fclose (file) ;
     file = NULL ;

     # if LINUX

     int pid = fork () ;
     if (pid == 0)
     {
          draw_dpm (&mds, dpm, name) ;

          free (dpm) ;
          dpm = NULL ;

          return 0 ;
     }

     # endif

     DSC dsc = {0} ;

     if (eval_dpm (&mds, dpm, &dsc, &spk) == 2)
          { error = 5 ; goto quit ; }

     if (save_log (&mds, dpm, &dsc, spk, name) != 0)
          { error = 6 ; goto quit ; }

     # if WINDOWS

     draw_dpm (&mds, dpm, name) ;

     # endif

     quit :

     if (spk != NULL)
          free (spk) ;
     if (dpm != NULL)
          free (dpm) ;
     if (name != NULL)
          free (name) ;
     if (file != NULL)
          fclose (file) ;
     if (error != 0)
          fprintf (stderr, "\e[1;31mError # %d\e[0m\n", error) ;

     return error ;
}
