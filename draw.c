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

# include "draw.h"

static int calc_tim_crv (MDS *mds, DPM *dpm, SDL_Point *timing, int smp_stt, int smp_stp)
{
     if (smp_stt >= mds->smp || smp_stp >= mds->smp)
          return 1 ;

     float zoom_x = (float) mds->smp / (smp_stp - smp_stt + 1) ;
     unsigned long sector = 0 ;

     for (int i = smp_stt ; i <= smp_stp ; i++)
     {
          sector += mds->itv ;

          timing[i].x = sector * zoom_x * 640 / mds->sct ;
          timing[i].y = 480 - (dpm[i].tim * 480 / dpm[0].tim) + 140 ;
     }

     return 0 ;
}

static int calc_var_crv (MDS *mds, DPM *dpm, SDL_Point *variation, int smp_stt, int smp_stp)
{
     if (smp_stt >= mds->smp || smp_stp >= mds->smp)
          return 1 ;

     float zoom_x = (float) mds->smp / (smp_stp - smp_stt + 1) ;
     unsigned long sector = 0 ;

     for (int i = smp_stt ; i <= smp_stp ; i++)
     {
          sector += mds->itv ;

          variation[i].x = sector * zoom_x * 640 / mds->sct ;
          variation[i].y = - dpm[i].var + 60 ;
     }

     return 0 ;
}

bool draw_dpm (MDS *mds, DPM *dpm)
{
     SDL_Window *window = NULL ;
     SDL_Renderer *renderer = NULL ;
     SDL_Texture *texture_1 = NULL ;
     SDL_Texture *texture_2 = NULL ;
     SDL_Point *timing = NULL ;
     SDL_Point *variation = NULL ;

     /* initializing */

     int action = 0 ;
     bool error = false ;

     action = SDL_Init (SDL_INIT_VIDEO) ;
     if (action != 0) { error = true ; goto quit ; }

     window = SDL_CreateWindow ("DPM", 0, 0, 660, 720, SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS) ;
     if (window == NULL) { error = true ; goto quit ; }
     renderer = SDL_CreateRenderer (window, -1, SDL_RENDERER_SOFTWARE) ;
     if (renderer == NULL) { error = true ; goto quit ; }

     texture_1 = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480) ;
     if (texture_1 == NULL) { error = true ; goto quit ; }
     texture_2 = SDL_CreateTexture (renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 640, 480) ;
     if (texture_2 == NULL) { error = true ; goto quit ; }

     timing = malloc (mds->smp * sizeof (SDL_Point)) ;
     if (timing == NULL) { error = true ; goto quit ; }
     variation = malloc (mds->smp * sizeof (SDL_Point)) ;
     if (variation == NULL) { error = true ; goto quit ; }

     /* drawing */

     SDL_SetRenderTarget (renderer, NULL) ;

          action = SDL_SetRenderDrawColor (renderer, 55, 55, 55, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }
          SDL_RenderClear (renderer) ;

          SDL_Rect border_1 = {0, 0, 660, 720} ;
          SDL_Rect border_2 = {9, 9, 642, 442} ;
          SDL_Rect border_3 = {9, 459, 642, 252} ;

          action = SDL_SetRenderDrawColor (renderer, 35, 35, 35, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }
          action = SDL_RenderDrawRect (renderer, &border_1) ;
          if (action != 0) { error = true ; goto quit ; }
          action = SDL_RenderDrawRect (renderer, &border_2) ;
          if (action != 0) { error = true ; goto quit ; }
          action = SDL_RenderDrawRect (renderer, &border_3) ;
          if (action != 0) { error = true ; goto quit ; }

     unsigned int count = 0 ;

     SDL_SetRenderTarget (renderer, texture_1) ;

          action = SDL_SetRenderDrawColor (renderer, 0, 0, 0, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }
          SDL_RenderClear (renderer) ;

          count = mds->smp ;

          calc_tim_crv (mds, dpm, timing, 0, count - 1) ;
          calc_var_crv (mds, dpm, variation, 0, count - 1) ;

          action = SDL_SetRenderDrawColor (renderer, 255, 0, 0, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }
          action = SDL_RenderDrawLines (renderer, timing, count) ;
          if (action != 0) { error = true ; goto quit ; }

          action = SDL_SetRenderDrawColor (renderer, 0, 0, 255, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }
          action = SDL_RenderDrawLines (renderer, variation, count) ;
          if (action != 0) { error = true ; goto quit ; }

     SDL_SetRenderTarget (renderer, texture_2) ;

          action = SDL_SetRenderDrawColor (renderer, 0, 0, 0, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }
          SDL_RenderClear (renderer) ;

          count = 750 ;
          if (count > mds->smp)
               count = mds->smp ;

          calc_tim_crv (mds, dpm, timing, 0, count - 1) ;
          calc_var_crv (mds, dpm, variation, 0, count - 1) ;

          action = SDL_SetRenderDrawColor (renderer, 255, 0, 0, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }
          action = SDL_RenderDrawLines (renderer, timing, count) ;
          if (action != 0) { error = true ; goto quit ; }

          action = SDL_SetRenderDrawColor (renderer, 0, 0, 255, SDL_ALPHA_OPAQUE) ;
          if (action != 0) { error = true ; goto quit ; }
          action = SDL_RenderDrawLines (renderer, variation, count) ;
          if (action != 0) { error = true ; goto quit ; }

     /* rendering */

     SDL_SetRenderTarget (renderer, NULL) ;

          SDL_Rect area_1 = {0, 0, 640, 440} ;
          SDL_Rect area_2 = {10, 10, 640, 440} ;
          SDL_Rect area_3 = {0, 0, 640, 250} ;
          SDL_Rect area_4 = {10, 460, 640, 250} ;

          SDL_RenderCopy (renderer, texture_1, &area_1, &area_2) ;
          SDL_RenderCopy (renderer, texture_2, &area_3, &area_4) ;

          SDL_RenderPresent (renderer) ;

     /* exporting */

     SDL_Surface *surface = NULL ;

     surface = SDL_GetWindowSurface (window) ;
     if (surface == NULL) { error = true ; goto quit ; }

     action = SDL_SaveBMP (surface, "scan.bmp") ;
     if (action != 0) { error = true ; goto quit ; }

     /* waiting */

     SDL_Event event = {0} ;
     bool execution = true ;

     while (execution)
     {
          while (SDL_PollEvent (&event))
          {
               switch (event.type)
               {
                    case SDL_KEYDOWN :
                         switch (event.key.keysym.sym)
                         {
                              case SDLK_ESCAPE :
                                   execution = false ;
                                   break ;
                         }
                         break ;

                    case SDL_QUIT :
                         execution = false ;
                         break ;
               }
          }
     }

     /* exiting */

     quit :

     if (error == true)       SDL_Log ("%s\n", SDL_GetError()) ;
     if (variation != NULL)   free (variation) ;
     if (timing != NULL)      free (timing) ;
     if (texture_2 != NULL)   SDL_DestroyTexture (texture_2) ;
     if (texture_1 != NULL)   SDL_DestroyTexture (texture_1) ;
     if (renderer != NULL)    SDL_DestroyRenderer (renderer) ;
     if (window != NULL)      SDL_DestroyWindow (window) ; // also frees the attached surface

     SDL_Quit() ;

     return error ;
}
