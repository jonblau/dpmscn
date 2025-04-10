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
