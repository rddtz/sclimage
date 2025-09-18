#ifndef _SCLIMAGE_H
#define _SCLIMAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h> // Library used to image handle
#include <SDL2/SDL_image.h> // <-----'
#include <SDL2/SDL_ttf.h> // <--- Used to create the histogram

#include <readline/readline.h> // Library to CLI
#include <readline/history.h> // <-----'
#include <getopt.h> //  <-----------'

#include <pthread.h> // Thread Library to show the image while using the terminal

// Constants definition
#define SCLIMAGE_MAX_LINE_LEN 256
#define SCLIMAGE_MAX_ARGS 16
#define SCLIMAGE_MAX_FILENAME_LEN 128
#define SCLIMAGE_MAX_IMAGE_WIDTH 100000
#define SCLIMAGE_GRAYSCALE_RANGE 256

// Error code definitions
#define SCLIMAGE_EXIT 1
#define SCLIMAGE_LOAD_MISSING_PATH -2
#define SCLIMAGE_LOAD_ERROR -3
#define SCLIMAGE_CONVERT_ERROR -4
#define SCLIMAGE_SAVE_ERROR -5
#define SCLIMAGE_SAVE_NULL -6
#define SCLIMAGE_ALREADY_SHOWING -7
#define SCLIMAGE_SHOW_NULL -8
#define SCLIMAGE_SHOW_THREAD_ERROR -9
#define SCLIMAGE_QUANTIZATION_ARGUMENT_MISSING -10
#define SCLIMAGE_BRIGHTNESS_ARGUMENT_MISSING -11
#define SCLIMAGE_OK 0



#define max(a,b)				\
({ __typeof__ (a) _a = (a);			\
__typeof__ (b) _b = (b);			\
_a > _b ? _a : _b; })

#define min(a,b)				\
({ __typeof__ (a) _a = (a);			\
__typeof__ (b) _b = (b);			\
_a < _b ? _a : _b; })


typedef struct image{
  SDL_Surface* surface;
  SDL_Surface* original;
  char filename[SCLIMAGE_MAX_FILENAME_LEN];
} Image;


typedef struct {
  pthread_t thread_id;
  int is_running;
  int close_correctly; // Flag to see if closed correctly, preventing memory leak
  int should_quit;
  int has_changed; // Flag to signal a redraw is needed
  Image* image;
  pthread_mutex_t mutex; // For safe data access between threads
} ViewerState;


// My pixel decoder
void getRBGA(Uint32 pixel, Uint8 *r, Uint8 *g, Uint8 *b, Uint8 *a){

  Uint8 mask = 0xFF;
  *r = (pixel & mask);

  pixel = pixel >> 8;
  *g = pixel & mask;

  pixel = pixel >> 8;
  *b = pixel & mask;

  pixel = pixel >> 8;
  *a = pixel & mask;

  return;
}


// My pixel encoder
Uint32 setRGBA(Uint8 r, Uint8 g, Uint8 b, Uint8 a){

  Uint32 my_pixel = 0x00000000;

  my_pixel += a;

  my_pixel = my_pixel << 8;
  my_pixel = my_pixel + b;

  my_pixel = my_pixel << 8;
  my_pixel += g;

  my_pixel = my_pixel << 8;
  my_pixel += r;

  return my_pixel;
}


// Verify if a image is in gray scale
int is_grayscale(Image* image){

  SDL_Surface* surface = image->surface;
  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; i++) {

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;

    getRBGA(pixel, &r, &g, &b, &a);
    if(r != b || r != g || b != g){
      return 0; // Is not in grayscale
    }
  }

  return 1; // is gray scale
}

// Print error mesasges and returns if program needs to be closed
int sclimage_error(int status){

  char error_msg[SCLIMAGE_MAX_LINE_LEN] = "";

  switch (status) {
  case SCLIMAGE_OK:
    return 0;
    break;

  case SCLIMAGE_EXIT:
    return 1;
    break;

  case SCLIMAGE_LOAD_ERROR:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN, "Error: canceling operation due to problem when loading image (%s)\n", IMG_GetError());
    break;

  case SCLIMAGE_LOAD_MISSING_PATH:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN, "Error: missing image path.\n");
    break;

  case SCLIMAGE_CONVERT_ERROR:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN, "Error: canceling operation due to problem when converting surface (%s)\n", SDL_GetError());
    break;

  case SCLIMAGE_SAVE_ERROR:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN, "Error: canceling operation due to problem when saving image (%s)\n", IMG_GetError());
    break;

  case SCLIMAGE_SAVE_NULL:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN,  "Error: tried to save null image, try loading an image first.\n");
    break;

  case SCLIMAGE_ALREADY_SHOWING:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN, "Error: already showing.\n");
    break;

  case SCLIMAGE_SHOW_NULL:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN,  "Error: no image loaded to show.\n");
    break;

  case SCLIMAGE_SHOW_THREAD_ERROR:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN, "Error creating the thread to show.\n");
    break;

  case SCLIMAGE_QUANTIZATION_ARGUMENT_MISSING:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN, "Error: amount of shades not informed, try again with 'quantization <shades>'.\n");
    break;

  case SCLIMAGE_BRIGHTNESS_ARGUMENT_MISSING:
    snprintf(error_msg, SCLIMAGE_MAX_LINE_LEN, "Error: brightness scalar not informed, try again with 'brightness <scalar>'.\n");
    break;
  }


  fprintf(stderr, error_msg);
  return 0;
}

#endif
