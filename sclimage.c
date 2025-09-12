#include "sclimage.h"
#include <pthread.h> // Include the threads library

// Signatures for the functions of the application
int sclimage_help(Image* image, int argc, char* argv[]);
int sclimage_vflip(Image* image, int argc, char* argv[]);
int sclimage_hflip(Image* image, int argc, char* argv[]);
int sclimage_show(Image* image, int argc, char* argv[]);
int sclimage_hide(Image* image, int argc, char* argv[]);
int sclimage_load(Image* image, int argc, char* argv[]);
int sclimage_save(Image* image, int argc, char* argv[]);
int sclimage_grayscale(Image* image, int argc, char* argv[]);
int sclimage_negative(Image* image, int argc, char* argv[]);
int sclimage_restart(Image* image, int argc, char* argv[]);
int sclimage_quantization(Image* image, int argc, char* argv[]);
int sclimage_exit(Image* image, int argc, char* argv[]);


typedef struct {
    pthread_t thread_id;
    int is_running;
    int should_quit;
    int has_changed; // Flag to signal a redraw is needed
    Image* image;
    pthread_mutex_t mutex; // For safe data access between threads
} ViewerState;

ViewerState g_viewer = {0}; // Initialize our global viewer state

// --- The struct that pairs a command name with a function pointer ---
typedef struct command {
    const char* name;
    // Pointer to a function that takes Image state and arguments, and returns and int
    int (*handler)(Image* image, int argc, char* argv[]);
} Command;

Command command_table[] = {
  {"help", sclimage_help},
  {"load", sclimage_load},
  {"open", sclimage_load},
  {"save", sclimage_save},
  {"grayscale", sclimage_grayscale},
  {"negative", sclimage_negative},
  {"restart", sclimage_restart},
  {"quantization", sclimage_quantization},
  {"show", sclimage_show},
  {"view", sclimage_show},
  {"hide", sclimage_hide},
  {"hflip", sclimage_hflip},
  {"vflip", sclimage_vflip},
  {"quit", sclimage_exit},
  {"exit", sclimage_exit}
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
    if (command && (strcmp(command, "load") == 0 || strcmp(command, "open") == 0)) {
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

    IMG_Init(IMG_INIT_JPG); // Set up image JPG loading

    printf("Welcome to the Simple Command Line Interface for Image Processing (SCLIMAGE)!\nType 'help' for commands.\n");

    rl_attempted_completion_function = smart_completer;
    Image image = {NULL, NULL, ""}; // This instance of Image represents the current image

    // Read loop for command line interface
    while(1){
      line = readline("(sclimage) > ");

      if (!line) { // this handles the EOF (Ctrl+D) signal and exits the program
	printf("exit\n");
	break;
      }

      if (strlen(line) > 0) { // If the line isn't empty, add to history of commands
	add_history(line);
      }

      // Remove the \n from the input
      line[strcspn(line, "\n")] = 0;

      // Copying the line to a new array that can be used to tokenize the arguments
      char line_copy[MAX_LINE_LEN];
      strncpy(line_copy, line, MAX_LINE_LEN - 1);
      line_copy[MAX_LINE_LEN - 1] = '\0';

      // Tokenize the input, creating a argc and argv for each command
      argc_int = 0;
      char* token = strtok(line_copy, " \t");
      while (token != NULL && argc_int < MAX_ARGS - 1) {
	argv_int[argc_int++] = token;
	token = strtok(NULL, " \t");
      }
      argv_int[argc_int] = NULL;

      if (argc_int == 0) { // If the line was empty (after tokenization), free and continue
	free(line);
	continue;
      }

      // Loop to match the input command with the available one
      int found_command = 0;
      for (int i = 0; i < num_commands; i++) {
	if (strcmp(argv_int[0], command_table[i].name) == 0) {

	  // If find, call the function
	  int status = command_table[i].handler(&image, argc_int, argv_int);

	  // Verify if an error happened when handling the command
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

    // Freeing the image used and exiting the IMG enviroment
    SDL_FreeSurface(image.surface);
    SDL_FreeSurface(image.original);
    sclimage_hide(NULL, 0, NULL);
    IMG_Quit();

    return 0;
}


int sclimage_help(Image* img, int argc, char* argv[]) {
    printf("Available commands:\n"
           "  load <filepath>          - Loads an image from a path.\n"
	   "  open <filepath>          - Same as load.\n"
	   "  show                     - Show the original image and the edition side by side.\n"
	   "  view                     - Same as show.\n"
	   "  hide                     - Close the showed image.\n"
           "  grayscale                - Applies the grayscale filter to image.\n"
	   "  quantization <shades>    - Quantizes the image to only use <shades> shades.\n"
	   "  negative                 - Applies the negative filter to the image.\n"
	   "  hflip                    - Flips the image horizontally.\n"
	   "  vflip                    - Flips the image vertically.\n"
           "  exit                     - Exits the shell.\n"
	   "  quit                     - Same as exit.\n"
           "  help                     - Shows this help message.\n");

    return 0;
}


int sclimage_hflip(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  SDL_LockSurface(surface);

  // Get a pointer to the pixel data and cast it to the format we need.
  Uint32* pixels = (Uint32*)surface->pixels;

  int ppl = surface->w; // pixels per line
  Uint32 temp = 0;

  for (int i = 0; i < surface->h; i++){
    for(int j = 0; j < ppl/2; j++){
      pthread_mutex_lock(&g_viewer.mutex);

      temp = pixels[i*ppl + j];
      pixels[i*ppl + j] = pixels[i*ppl + ppl - j - 1];
      pixels[i*ppl + ppl - j - 1] = temp;

      g_viewer.has_changed = 1;
      pthread_mutex_unlock(&g_viewer.mutex);
    }
  }

  SDL_UnlockSurface(surface);
  return 0;
}


int sclimage_vflip(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;

  Uint32 hold_buffer[MAX_IMAGE_WIDTH] = {0};
  int ppl = surface->w;

  for (int i = 0; i < (surface->h)/2; i++) {
    pthread_mutex_lock(&g_viewer.mutex);

    memcpy(hold_buffer, pixels + i*ppl, sizeof(Uint32)*ppl);
    memcpy(pixels + i*ppl, pixels + ppl*(surface->h - 1) - i*ppl, sizeof(Uint32)*ppl);
    memcpy(pixels + ppl*(surface->h - 1) - i*ppl, hold_buffer, sizeof(Uint32)*ppl);

    g_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_viewer.mutex);
  }

  SDL_UnlockSurface(surface);

  return 0;
}


int sclimage_negative(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;
  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRBGA(pixel, &r, &g, &b, &a);

    r = 255 - r;
    g = 255 - g;
    b = 255 - b;

    pixels[i] = setRGBA(r, g, b, a);

    g_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_viewer.mutex);
  }

  SDL_UnlockSurface(surface);

  return 0;
}

int sclimage_grayscale(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;
  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;

    getRBGA(pixel, &r, &g, &b, &a);

    Uint8 gray = (Uint8)(0.299 * r + 0.587 * g + 0.114 * b);
    r = gray;
    g = gray;
    b = gray;

    pixels[i] = setRGBA(r, g, b, a);

    g_viewer.has_changed = 1; // 3. Set the dirty flag
    pthread_mutex_unlock(&g_viewer.mutex); // 4. Unlock
  }

  SDL_UnlockSurface(surface);

  return 0;
}

/* Restart the image to its original form */
int sclimage_restart(Image* image, int argc, char* argv[]){

  pthread_mutex_lock(&g_viewer.mutex); // 1. Lock

  Uint32* pixels_edited = (Uint32*)image->surface->pixels;
  Uint32* pixels_original = (Uint32*)image->original->pixels;
  int pixel_count = image->surface->w * image->surface->h;
  memcpy(pixels_edited, pixels_original, sizeof(Uint32)*pixel_count);

  g_viewer.has_changed = 1; // 3. Set the dirty flag
  pthread_mutex_unlock(&g_viewer.mutex); // 4. Unlock

  return 0;
}

/* Quantizes the image to have only n tones, where n is the first and only argument of the function */
int sclimage_quantization(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  if(argc <= 1){
    printf("This function takes one argument: the shades to quantizate.\n");
    return -1;
  }

  if(!is_grayscale(image)){
    printf("Quantization only works for images in grayscale (at the moment). The image will be converted to grayscale before contining.\n");
    sclimage_grayscale(image, 0, NULL);
  }


  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;


  int shades = atoi(argv[1]); // first and only argument is the new amount of shades of the image
  int tmax = -1;
  int tmin = 256;
  int image_shades = 0;
  int histogram[GRAYSCALE_RANGE] = {0};

  SDL_LockSurface(surface);

  for (int i = 0; i < pixel_count; ++i) {

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRBGA(pixel, &r, &g, &b, &a);

    if(r > tmax) tmax = r;
    if(r < tmin) tmin = r;

    if(histogram[r] == 0){
      image_shades++; // Get the true amount of shades of the image
    }
    histogram[r]++; // Calculate the histogram
  }

  if(shades >= image_shades){ // If the image has less shades then the function argument, exits
    return 0;
  }

  float bin_size = (float) (tmax - tmin + 1)/shades;

  int new_histogram[GRAYSCALE_RANGE*2] = {0};
  int new_shades = 0;


  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_viewer.mutex); // 1. Lock

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRBGA(pixel, &r, &g, &b, &a);

    int f = (int)floor((float)(r-tmin)/bin_size);
    Uint16 new_shade = tmin + (Uint16) f * bin_size + bin_size/2; // Calculates the new shade for the pixel

    r = (Uint8)new_shade;
    g = (Uint8)new_shade;
    b = (Uint8)new_shade;

    pixels[i] = setRGBA(r, g, b, a);

    g_viewer.has_changed = 1; // 3. Set the dirty flag
    pthread_mutex_unlock(&g_viewer.mutex); // 4. Unlock

    if(new_histogram[new_shade] == 0){
      new_shades++;
    }
    new_histogram[new_shade]++;
  }

  /* if(new_shades != shades){ */
  /*   printf("%d -> %d -> %d", image_shades, shades, new_shades); */
  /*   printf("Error: an error occurred while quantizating the image\n"); */
  /*   return -1; */
  /* } */

  return 0;
}


int sclimage_load(Image* image, int argc, char* argv[]){

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

  pthread_mutex_lock(&g_viewer.mutex); // Lock before touching shared data

  image->surface = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);
  image->original = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);

  SDL_FreeSurface(original_surface); // Free the surface used to load
  if (!(image->surface)) {
    fprintf(stderr, "Error: problem when converting surface (%s)\n", SDL_GetError());
    return -1;
  }

  if (g_viewer.is_running) {
    g_viewer.has_changed = 1;
  }

  pthread_mutex_unlock(&g_viewer.mutex); // Unlock after updating

  strncpy(image->filename, argv[1], MAX_FILENAME_LEN - 1);
  image->filename[strlen(image->filename) - 1] = '\0'; // Save the name of the file


  return 0;
}


int sclimage_save(Image* image, int argc, char* argv[]){

  char name[MAX_FILENAME_LEN] = "";
  int quality = 100;

  if(argc > 1){ // If a argument is passed, use the argument as filepath to save
    strcpy(name, argv[1]);
  } else { // Else, replace the image on the original filepath
    strcpy(name, image->filename);
  }

  if(image->surface == NULL){
    printf("Error: tried to save null image, try loading an image first.\n");
    return 0;
  }

  printf("Saving as %s\n", name);

  if( IMG_SaveJPG(image->surface, name, quality) ){
    fprintf(stderr, "Error: problem when saving image (%s)\n", IMG_GetError());
    return -1;
  }

  return 0;
}

int sclimage_exit(Image* image, int argc, char* argv[]){

  sclimage_hide(image, argc, argv);

  SDL_FreeSurface(image->surface);
  SDL_FreeSurface(image->original);

  IMG_Quit();
  SDL_Quit();
  exit(0);

  return 0;
}

void* viewer_thread_func(void* arg) {

  SDL_Window* window = SDL_CreateWindow("Original and Edited View", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					g_viewer.image->surface->w + g_viewer.image->original->w,
					max(g_viewer.image->surface->h, g_viewer.image->original->h),
					SDL_WINDOW_SHOWN);

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Using streaming textures to update frequently
  SDL_Texture* texture_original = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
						    g_viewer.image->original->w, g_viewer.image->original->h);
  SDL_Texture* texture_edited = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
						  g_viewer.image->surface->w, g_viewer.image->surface->h);

  int current_w = g_viewer.image->surface->w + g_viewer.image->original->w;
  int current_h = max(g_viewer.image->surface->h, g_viewer.image->original->h);

  SDL_UpdateTexture(texture_original, NULL, g_viewer.image->original->pixels, g_viewer.image->original->pitch);
  SDL_UpdateTexture(texture_edited, NULL, g_viewer.image->surface->pixels, g_viewer.image->surface->pitch);

  SDL_Rect original_dest_rect = {0, 0, g_viewer.image->original->w, g_viewer.image->original->h};

  SDL_Rect edited_dest_rect = {g_viewer.image->original->w, 0, g_viewer.image->surface->w, g_viewer.image->surface->h};

  SDL_Event event;
  while (!g_viewer.should_quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	g_viewer.should_quit = 1;
      }
    }

    pthread_mutex_lock(&g_viewer.mutex);
    if (g_viewer.has_changed) { // Checking updates for the functions
      Image* current_image = g_viewer.image;

      // Check if the image dimensions have changed
      if ((current_image->surface->w + current_image->original->w) != current_w ||
	  max(current_image->surface->h, current_image->original->h) != current_h) {


	current_w = current_image->surface->w + current_image->original->w;
	current_h = max(current_image->original->h, current_image->surface->h);

	//	printf("Image dimensions changed. Resizing window and textures...\n");

	SDL_SetWindowSize(window, current_w, current_h);

	SDL_DestroyTexture(texture_original);
	SDL_DestroyTexture(texture_edited);

	// Create new textures
	texture_original = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
					     current_image->original->w,
					     current_image->original->h);
	texture_edited = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
					   current_image->surface->w,
					   current_image->surface->h);

	// Update squares
	original_dest_rect = (SDL_Rect){0, 0, g_viewer.image->original->w, g_viewer.image->original->h};
	edited_dest_rect = (SDL_Rect){g_viewer.image->original->w, 0, g_viewer.image->surface->w, g_viewer.image->surface->h};
      }


      SDL_UpdateTexture(texture_original, NULL, current_image->original->pixels, current_image->original->pitch);
      SDL_UpdateTexture(texture_edited, NULL, current_image->surface->pixels, g_viewer.image->surface->pitch);
      g_viewer.has_changed = 0; // Reset the flag
    }
    pthread_mutex_unlock(&g_viewer.mutex);

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture_original, NULL, &original_dest_rect);
    SDL_RenderCopy(renderer, texture_edited, NULL, &edited_dest_rect);
    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(texture_original);
  SDL_DestroyTexture(texture_edited);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  g_viewer.should_quit = 1;
  pthread_join(g_viewer.thread_id, NULL);

  pthread_mutex_destroy(&g_viewer.mutex);
  g_viewer.is_running = 0;
  /* printf("Viewer thread finished.\n"); */
  return NULL;
}


int sclimage_show(Image* image, int argc, char* argv[]) {

  if (g_viewer.is_running) {
    printf("Already showing.\n");
    return 0;
  }

  if (image == NULL || image->surface == NULL) {
    printf("Error: No image loaded to show.\n");
    return 0;
  }

  g_viewer.image = image;
  g_viewer.should_quit = 0;
  g_viewer.has_changed = 1;
  pthread_mutex_init(&g_viewer.mutex, NULL);

  if (pthread_create(&g_viewer.thread_id, NULL, viewer_thread_func, NULL) != 0) {
    printf("Error creating the thread to show.\n");
    return -1;
  }

  g_viewer.is_running = 1;
  /* printf("Viewer launched in the background.\n"); */
  return 0;
}

// A new "hide" command to gracefully shut down the viewer
int sclimage_hide(Image* image, int argc, char* argv[]) {
  if (!g_viewer.is_running) {
    /* printf("Viewer is not running.\n"); */
    return 0;
  }

  g_viewer.should_quit = 1;
  pthread_join(g_viewer.thread_id, NULL);

  pthread_mutex_destroy(&g_viewer.mutex);
  g_viewer.is_running = 0;
  /* printf("Viewer has been closed.\n"); */
  return 0;
}
