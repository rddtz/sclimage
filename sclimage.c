// See why last (and first) pixel is not chaning

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL2/SDL.h> // Library used to image handle
#include <SDL2/SDL_image.h> // <-----'

#include <readline/readline.h> // Library to CLI
#include <readline/history.h> // <-----'

#define MAX_LINE_LEN 256
#define MAX_ARGS 16
#define MAX_FILENAME_LEN 128
#define MAX_IMAGE_WIDTH 100000

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
  char filename[MAX_FILENAME_LEN];
} Image;

int handle_vflip(Image* image, int argc, char* argv[]);
int handle_hflip(Image* image, int argc, char* argv[]);
int handle_show(Image* image, int argc, char* argv[]);
int handle_load(Image* image, int argc, char* argv[]);
int handle_grayscale(Image* image, int argc, char* argv[]);
int handle_exit(Image* image, int argc, char* argv[]);

// --- The struct that pairs a command name with a function pointer ---
typedef struct command {
    const char* name;
    // Pointer to a function that takes Image state and arguments, and returns and int
    int (*handler)(Image* image, int argc, char* argv[]);
} Command;

Command command_table[] = {
  //    {"help", handle_help},
  {"load", handle_load},
  {"grayscale", handle_grayscale},
  {"show", handle_show}, //,
  {"hflip", handle_hflip},
  {"vflip", handle_vflip},
  // Save
  // vflip
  // hflip
  {"quit", handle_exit},
  {"exit", handle_exit}
};

const int num_commands = sizeof(command_table) / sizeof(Command);

char* command_generator(const char* text, int state) {
    static int list_index, len;
    const char* name;
    if (!state) {
        list_index = 0;
        len = strlen(text);
    }
    while (list_index < num_commands) {
        name = command_table[list_index].name;
        list_index++;
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }
    return NULL;
}

char** smart_completer(const char* text, int start, int end) {
    // This prevents readline from trying to complete from a static list by default.
    rl_attempted_completion_over = 1;

    // If 'start' is 0, we are at the beginning of the line, completing the command.
    if (start == 0) {
        return rl_completion_matches(text, command_generator);
    }

    // If we are not at the start, we are completing an argument.
    // We need to check what the command (the first word) is.
    // Make a copy of the line buffer because strtok is destructive.
    char* line_copy = strdup(rl_line_buffer);
    char* command = strtok(line_copy, " ");

    // Check if the command is one that takes a filename.
    if (command && strcmp(command, "load") == 0) {
        free(line_copy);
        // Use readline's built-in filename completer.
        return rl_completion_matches(text, rl_filename_completion_function);
    }

    // If the command is something else (like 'blur'), do nothing.
    free(line_copy);
    return NULL;
}



int main(int argc, char** argv) {

    char* line;
    char* argv_int[MAX_ARGS];
    int argc_int = 0;

    // --- 1. SETUP AND IMAGE LOADING ---
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG | IMG_INIT_JPG);

    printf("Welcome to the Simple Command Line Interface for Image Processing (SCLIMAGE)!\nType 'help' for commands.\n");

    rl_attempted_completion_function = smart_completer;
    Image image = {NULL, NULL, ""}; // Image showned

    while(1){

      line = readline("(sclimage) > ");

      //fflush(stdout); // Ensure the prompt is displayed before reading input

      // Read a line of input
      if (!line) {
	printf("exit\n"); // Handle Ctrl+D (EOF)
	break;
      }

      if (strlen(line) > 0) {
	add_history(line);
      }

      // Remove the trailing newline character from fgets
      line[strcspn(line, "\n")] = 0;

      char line_copy[MAX_LINE_LEN];
      strncpy(line_copy, line, MAX_LINE_LEN - 1);
      line_copy[MAX_LINE_LEN - 1] = '\0';

      argc_int = 0;
      char* token = strtok(line_copy, " \t");
      while (token != NULL && argc_int < MAX_ARGS - 1) {
	argv_int[argc_int++] = token;
	token = strtok(NULL, " \t");
      }
      argv_int[argc_int] = NULL;

      if (argc_int == 0) {
	free(line);
	continue;
      }

      // --- Find and execute the command ---
      int found_command = 0;
      for (int i = 0; i < num_commands; i++) {
	if (strcmp(argv_int[0], command_table[i].name) == 0) {
	  // Call the associated function pointer

	  int status = command_table[i].handler(&image, argc_int, argv_int);

	  if(status != 0){
	    exit(status);
	  }

	  found_command = 1;
	  break;
	}
      }

      if (!found_command) {
	printf("Unknown command: '%s'. Type 'help' for a list of commands.\n", argv_int[0]);
      }

      free(line);
    }


    SDL_FreeSurface(image.surface);
    SDL_FreeSurface(image.original);
    IMG_Quit();
    SDL_Quit();

    return 0;
}


int handle_vflip(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  SDL_LockSurface(surface);
  // Get a pointer to the pixel data and cast it to the format we need.
  Uint32* pixels = (Uint32*)surface->pixels;

  //  int pixel_count = surface->w * surface->h;
  int ppl = surface->w; // pixels per line

  Uint32 temp = 0;

  for (int i = 0; i < surface->h; i++) {
    for(int j = 0; j < (surface->w)/2; j++){

      //      printf("Trading pixel at position (%d, %d) with (%d, %d)\n", i, j, i, ppl - j);
      temp = pixels[i*ppl + j];
      pixels[i*ppl + j] = pixels[i*ppl + ppl - j - 1];
      pixels[i*ppl + ppl - j - 1] = temp;

    }
  }

  SDL_UnlockSurface(surface);

  return 0;
}

int handle_hflip(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  SDL_LockSurface(surface);
  // Get a pointer to the pixel data and cast it to the format we need.

  Uint32* pixels = (Uint32*)surface->pixels;
  //  int pixel_count = surface->w * surface->h;

  Uint32 hold_buffer[MAX_IMAGE_WIDTH] = {0};
  int ppl = surface->w; // pixels per line

  for (int i = 0; i < (surface->h)/2; i++) {

    memcpy(hold_buffer, pixels + i*ppl, sizeof(Uint32)*ppl);
    memcpy(pixels + i*ppl, pixels + ppl*(surface->h - 1) - i*ppl, sizeof(Uint32)*ppl);
    memcpy(pixels + ppl*(surface->h - 1) - i*ppl, hold_buffer, sizeof(Uint32)*ppl);

  }

  SDL_UnlockSurface(surface);

  return 0;
}

int handle_grayscale(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  SDL_LockSurface(surface);
  // Get a pointer to the pixel data and cast it to the format we need.

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; ++i) {
    // GET THE PIXEL and its individual color components
    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    SDL_GetRGBA(pixel, surface->format, &r, &g, &b, &a);

    // MANIPULATE THE PIXEL (convert to grayscale using luminosity formula)
    Uint8 gray = (Uint8)(0.299 * r + 0.587 * g + 0.114 * b);
    r = gray;
    g = gray;
    b = gray;

    // WRITE THE NEW PIXEL back to the surface
    pixels[i] = SDL_MapRGBA(surface->format, r, g, b, a);
  }

  SDL_UnlockSurface(surface);

  return 0;
}


int handle_load(Image* image, int argc, char* argv[]){

  if(argc < 2){
    printf("Error: image path missing.\n");
    return 0;
  }

  if(argc > 2){
    printf("Warning: more than one argument found for load function.\n");
  }

  // Free previous surface
  if(image->surface != NULL){
    SDL_FreeSurface(image->surface);
    SDL_FreeSurface(image->original);
  }

  printf("Loading %s\n", argv[1]);

  SDL_Surface* original_surface = IMG_Load(argv[1]);
  if (!original_surface) {
    fprintf(stderr, "Error: problem when loading image (%s)\n", IMG_GetError());
    return -1;
  }

  fflush(stdout);

  image->surface = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);
  image->original = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);

  SDL_FreeSurface(original_surface); // Free the original surface
  if (!(image->surface)) {
    fprintf(stderr, "Error: problem when converting surface (%s)\n", SDL_GetError());
    return -1;
  }

  strncpy(image->filename, argv[1], MAX_FILENAME_LEN - 1);
  image->filename[strlen(image->filename) - 1] = '\0'; // Ensure null-termination

  return 0;
}


int handle_show(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;
  SDL_Surface* original = image->original;

  if(surface == NULL){
    printf("Error: showing null image, try loading an image first.\n");
    return 0;
  }

  SDL_Window* window = SDL_CreateWindow("Original and Edited View", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					surface->w + original->w, max(surface->h, original->h),
					SDL_WINDOW_SHOWN);


  /* SDL_Window* window = SDL_CreateWindow("Edited View", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, */
  /* 					surface->w, surface->h, */
  /* 					SDL_WINDOW_SHOWN); */


  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  SDL_Texture* texture_original = SDL_CreateTextureFromSurface(renderer, original);
  SDL_Texture* texture_edited = SDL_CreateTextureFromSurface(renderer, surface);

  // Rectangle for the original image (left side)
  SDL_Rect original_dest_rect;
  original_dest_rect.x = 0;
  original_dest_rect.y = 0;
  original_dest_rect.w = original->w; // Use edited_surface w/h for consistency
  original_dest_rect.h = original->h;

  // Rectangle for the edited image (right side)
  SDL_Rect edited_dest_rect;
  edited_dest_rect.x = original_dest_rect.w; // Position it right next to the original
  edited_dest_rect.y = 0;
  edited_dest_rect.w = surface->w;
  edited_dest_rect.h = surface->h;


  int quit = 0;
  SDL_Event event;
  while (!quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) quit = 1;
    }
    SDL_RenderClear(renderer);

    /* SDL_RenderCopy(renderer, texture_edited, NULL, NULL); */
    SDL_RenderCopy(renderer, texture_original, NULL, &original_dest_rect);
    SDL_RenderCopy(renderer, texture_edited, NULL, &edited_dest_rect);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture_original);
  SDL_DestroyTexture(texture_edited);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return 0;
}

int handle_exit(Image* image, int argc, char* argv[]){
    IMG_Quit();
    SDL_Quit();
    exit(0);

    return 0;
}
