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

# include "draw.h"

int calc_crv (MDS *mds, DPM *dpm, SDL_Point *timing, int smp_stt, int smp_stp)
{
     int zoom = mds->smp / (smp_stp + 1 - smp_stt) ;

     unsigned long sector = 0 ;

     for (int i = smp_stt ; i <= smp_stp ; i++)
     {
          sector += mds->itv ;

          timing[i].x = (sector * 640 * zoom / mds->sct) ;
          timing[i].y = (480 - (dpm[i].tim * 480 / dpm[0].tim)) * 1 + 480 / 10 ;
     }

     return 0 ;
}

bool draw_crv (MDS *mds, DPM *dpm)
{
     SDL_Window *window = NULL ;
     SDL_Renderer *renderer = NULL ;
     SDL_Texture *texture_1 = NULL ;
     SDL_Texture *texture_2 = NULL ;
     SDL_Point *timing = NULL ;

     bool error = false ;

     timing = malloc (mds->smp * sizeof (SDL_Point)) ;
     if (timing == NULL) { error = true ; goto quit ; }

     /* initializing */

     int action = 0 ;

     action = SDL_Init (SDL_INIT_VIDEO) ;
     if (action != 0) { error = true ; goto quit ; }

     window = SDL_CreateWindow ("DPM curve and pattern", 0, 0, 640, 540, SDL_WINDOW_SHOWN) ;
     if (window == NULL) { error = true ; goto quit ; }

     renderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_SOFTWARE) ;
     if (renderer == NULL) { error = true ; goto quit ; }

     texture_1 = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480) ;
     if (texture_1 == NULL) { error = true ; goto quit ; }

     texture_2 = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480) ;
     if (texture_2 == NULL) { error = true ; goto quit ; }

     /* drawing texture_1 */

     SDL_SetRenderTarget (renderer, texture_1) ;

          action = SDL_SetRenderDrawColor (renderer, 55, 55, 55, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }

          SDL_RenderClear (renderer) ;

          action = SDL_SetRenderDrawColor (renderer, 255, 0, 0, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }

          calc_crv (mds, dpm, timing, 0, mds->smp - 1) ;

          action = SDL_RenderDrawLines (renderer, timing, mds->smp) ;
          if (action != 0) { error = true ; goto quit ; }

     /* drawing texture_2 */

     SDL_SetRenderTarget (renderer, texture_2) ;

          action = SDL_SetRenderDrawColor (renderer, 65, 65, 65, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }

          SDL_RenderClear (renderer) ;

          action = SDL_SetRenderDrawColor (renderer, 255, 0, 0, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }

          calc_crv (mds, dpm, timing, 0, 750 - 1) ;

          action = SDL_RenderDrawLines (renderer, timing, 750) ;
          if (action != 0) { error = true ; goto quit ; }

     /* rendering */

     SDL_SetRenderTarget (renderer, NULL) ;

          SDL_Rect area_1 = {0, 0, 640, 360} ;
          SDL_Rect area_2 = {0, 0, 640, 180} ;
          SDL_Rect area_3 = {0, 360, 640, 180} ;

          SDL_RenderCopy (renderer, texture_1, &area_1, &area_1) ;
          SDL_RenderCopy (renderer, texture_2, &area_2, &area_3) ;

          SDL_RenderPresent (renderer) ;

     /* waiting */

     SDL_Event event = {0} ;

     bool execution = true ;

     while (execution)
     {
          while (SDL_PollEvent (&event))
          {
               switch (event.type)
               {
                    case SDL_QUIT :
                    execution = false ;
                    break ;
               }
          }
     }

     /* exiting */

     quit :

     if (error == true)       SDL_Log ("%s\n", SDL_GetError()) ;
     if (timing != NULL)      free (timing) ;

     if (texture_2 != NULL)   SDL_DestroyTexture (texture_2) ;
     if (texture_1 != NULL)   SDL_DestroyTexture (texture_1) ;
     if (renderer != NULL)    SDL_DestroyRenderer (renderer) ;
     if (window != NULL)      SDL_DestroyWindow (window) ;

     SDL_Quit() ;

     return error ;
}
