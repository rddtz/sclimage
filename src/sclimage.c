#include "sclimage.h"

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

ViewerState g_viewer = {0}; // Global viewer state

// Command structe with the name and function pointer
typedef struct command {
    const char* name;
    int (*handler)(Image* image, int argc, char* argv[]);
} Command;

// All available commands
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
  {"close", sclimage_hide},
  {"hflip", sclimage_hflip},
  {"vflip", sclimage_vflip},
  {"quit", sclimage_exit},
  {"exit", sclimage_exit}
};

// Total commands amount
int num_commands = sizeof(command_table) / sizeof(Command);

// This is a auto-complete function for the readline library that complete with sclimage commands.
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

// This calls the autocomplete depending on the CLI state
char** smart_completer(const char* text, int start, int end) {
  // This prevents readline from trying to complete from a static list by default.
  rl_attempted_completion_over = 1;

  // If start is 0 we are at the beginning of the line so we complete with the command list.
  if (start == 0) {
    return rl_completion_matches(text, command_generator);
  }

  // If we are not at the start, we are completing an argument.
  char* line_copy = strdup(rl_line_buffer);
  char* command = strtok(line_copy, " ");

  // Check if the command is one that takes a filename as a argument.
  if (command && (strcmp(command, "load") == 0 || strcmp(command, "open") == 0)) {
    free(line_copy);
    // Use readline built-in filename completer.
    return rl_completion_matches(text, rl_filename_completion_function);
  }

  // If the command is something else, return.
  free(line_copy);
  return NULL;
}


int main(int argc, char** argv) {

  char* line;
  char* argv_int[SCLIMAGE_MAX_ARGS];
  int argc_int = 0;
  int status = 0;
  int stop = 0;

  IMG_Init(IMG_INIT_JPG); // Set up image JPG loading

  printf("Welcome to the Simple Command Line Interface for Image Processing (SCLIMAGE)!\nType 'help' for commands.\n");

  rl_attempted_completion_function = smart_completer;
  Image image = {NULL, NULL, ""}; // This instance of Image represents the current image

  // Read loop for command line interface
  while(!stop){

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
    char line_copy[SCLIMAGE_MAX_LINE_LEN];
    strncpy(line_copy, line, SCLIMAGE_MAX_LINE_LEN - 1);
    line_copy[SCLIMAGE_MAX_LINE_LEN - 1] = '\0';

    // Tokenize the input, creating a argc and argv for each command
    argc_int = 0;
    char* token = strtok(line_copy, " \t");
    while (token != NULL && argc_int < SCLIMAGE_MAX_ARGS - 1) {
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
	status = command_table[i].handler(&image, argc_int, argv_int);

	stop = sclimage_error(status);

	found_command = 1;
	break;
      }
    }

    if (!found_command) {
      printf("Unknown command: '%s'. Type 'help' for a list of commands.\n", argv_int[0]);
    }

    free(line);
  }

  if (g_viewer.is_running || (!g_viewer.close_correctly)) { // If the viewer is running, close it before exiting to prevent memory leak
    sclimage_hide(NULL, 0, NULL);
  }

  // Freeing the image used and exiting the IMG enviroment
  SDL_FreeSurface(image.surface);
  SDL_FreeSurface(image.original);
  IMG_Quit();
  SDL_Quit();

  return 0;
}


// Show help massage
int sclimage_help(Image* img, int argc, char* argv[]) {
  printf("Available commands:\n"
	 "  load <filepath>          - Loads an image from a path.\n"
	 "  open <filepath>          - Same as load.\n"
	 "  save <name>              - Save as <name>. If <name> argument not informed, replace original image.\n"
	 "  show                     - Show the original image and the edition side by side.\n"
	 "  view                     - Same as show.\n"
	 "  close                    - Close the image viewer.\n"
	 "  hide                     - Same as close.\n"
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


// Flips the image horizontally
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


// Flip the image vertically
int sclimage_vflip(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;

  Uint32 hold_buffer[SCLIMAGE_MAX_IMAGE_WIDTH] = {0};
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


// Apply negative filter to the image
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


// Apply the grayscale filter to the image
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

    g_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_viewer.mutex);
  }

  SDL_UnlockSurface(surface);

  return 0;
}


// Restart the image to its original form
int sclimage_restart(Image* image, int argc, char* argv[]){

  pthread_mutex_lock(&g_viewer.mutex);

  Uint32* pixels_edited = (Uint32*)image->surface->pixels;
  Uint32* pixels_original = (Uint32*)image->original->pixels;
  int pixel_count = image->surface->w * image->surface->h;
  memcpy(pixels_edited, pixels_original, sizeof(Uint32)*pixel_count);

  g_viewer.has_changed = 1;
  pthread_mutex_unlock(&g_viewer.mutex);

  return 0;
}


// Quantizes the image to have only n tones, where n is the first and only argument of the function
int sclimage_quantization(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  if(argc <= 1){
    return SCLIMAGE_QUANTIZATION_ARGUMENT_MISSING;
  }

  if(!is_grayscale(image)){
    fprintf(stderr, "Warning: Quantization only works for images in grayscale (at the moment). The image will be converted to grayscale before contining.\n");
    sclimage_grayscale(image, 0, NULL);
  }

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  int shades = atoi(argv[1]); // first and only argument is the new amount of shades of the image
  int tmax = -1;
  int tmin = 256;
  int image_shades = 0;
  int histogram[SCLIMAGE_GRAYSCALE_RANGE] = {0};

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

  float bin_size = (float) (tmax - tmin + 1)/shades; // Bin interval

  int new_histogram[SCLIMAGE_GRAYSCALE_RANGE*2] = {0};
  int new_shades = 0;

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRBGA(pixel, &r, &g, &b, &a);

    int f = (int)floor((float)(r-tmin)/bin_size);
    Uint16 new_shade = tmin + (Uint16) f * bin_size + bin_size/2; // Calculates the new shade for the pixel

    r = (Uint8)new_shade;
    g = (Uint8)new_shade;
    b = (Uint8)new_shade;

    pixels[i] = setRGBA(r, g, b, a);

    g_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_viewer.mutex);

    if(new_histogram[new_shade] == 0){
      new_shades++;
    }
    new_histogram[new_shade]++;
  }

  return 0;
}


// Load a new image to modify
int sclimage_load(Image* image, int argc, char* argv[]){

  if(argc < 2){
    return SCLIMAGE_LOAD_MISSING_PATH;
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
    return SCLIMAGE_LOAD_ERROR;
  }

  pthread_mutex_lock(&g_viewer.mutex); // Lock before touching shared data

  image->surface = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);
  image->original = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);

  SDL_FreeSurface(original_surface); // Free the surface used to load
  if (!(image->surface)) {
    return SCLIMAGE_CONVERT_ERROR;
  }

  if (g_viewer.is_running) {
    g_viewer.has_changed = 1;
  }

  pthread_mutex_unlock(&g_viewer.mutex); // Unlock after updating

  strncpy(image->filename, argv[1], SCLIMAGE_MAX_FILENAME_LEN - 1);
  image->filename[strlen(image->filename) - 1] = '\0'; // Save the name of the file

  return 0;
}


// Save the current image
int sclimage_save(Image* image, int argc, char* argv[]){

  char name[SCLIMAGE_MAX_FILENAME_LEN] = "";
  int quality = 100;

  if(argc > 1){ // If a argument is passed, use the argument as filepath to save
    strcpy(name, argv[1]);
    if(argc > 2){
      quality = atoi(argv[2]);
    }
  } else { // Else, replace the image on the original filepath
    strcpy(name, image->filename);
  }

  if(image->surface == NULL){
    return SCLIMAGE_SAVE_NULL;
  }

  printf("Saving as %s\n", name);

  if( IMG_SaveJPG(image->surface, name, quality) ){
    return SCLIMAGE_SAVE_ERROR;
  }

  return 0;
}


// Return the status to exit the program
int sclimage_exit(Image* image, int argc, char* argv[]){
  return SCLIMAGE_EXIT;
}


// This is a thread function to show the image using SDL Library whitout freezing the terminal
void* viewer_thread_func(void* arg) {

  SDL_Init(SDL_INIT_VIDEO); // Start video to show the image

  SDL_Window* window = SDL_CreateWindow("Original and Edited View", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					g_viewer.image->surface->w + g_viewer.image->original->w,
					max(g_viewer.image->surface->h, g_viewer.image->original->h),
					SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Using streaming textures to update frequently
  SDL_Texture* texture_original = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
						    g_viewer.image->original->w, g_viewer.image->original->h);
  SDL_Texture* texture_edited = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
						  g_viewer.image->surface->w, g_viewer.image->surface->h);

  SDL_UpdateTexture(texture_original, NULL, g_viewer.image->original->pixels, g_viewer.image->original->pitch);
  SDL_UpdateTexture(texture_edited, NULL, g_viewer.image->surface->pixels, g_viewer.image->surface->pitch);

  SDL_Rect original_dest_rect = {0, 0, g_viewer.image->original->w, g_viewer.image->original->h};
  SDL_Rect edited_dest_rect = {g_viewer.image->original->w, 0, g_viewer.image->surface->w, g_viewer.image->surface->h};


  int current_w = g_viewer.image->surface->w + g_viewer.image->original->w;
  int current_h = max(g_viewer.image->surface->h, g_viewer.image->original->h);

  SDL_Event event;
  while (!g_viewer.should_quit) {

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	g_viewer.should_quit = 1;
	g_viewer.close_correctly = 0;
      }
    }

    pthread_mutex_lock(&g_viewer.mutex);
    if (g_viewer.has_changed) { // Checking updates in the image by the functions
      Image* current_image = g_viewer.image;

      // Check if the image dimensions have changed, if so, resize the window
      if ((current_image->surface->w + current_image->original->w) != current_w ||
	  max(current_image->surface->h, current_image->original->h) != current_h) {


	current_w = current_image->surface->w + current_image->original->w;
	current_h = max(current_image->original->h, current_image->surface->h);


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
  SDL_Quit(); // Close video

  return NULL;
}


// Shows the image by calling a thread to do so, doing this prevents the terminal to freeze
int sclimage_show(Image* image, int argc, char* argv[]) {

  if (g_viewer.is_running) {
    return SCLIMAGE_ALREADY_SHOWING;
  }

  if (image == NULL || image->surface == NULL) {
    return SCLIMAGE_SHOW_NULL;
  }

  g_viewer.image = image;
  g_viewer.should_quit = 0;
  g_viewer.has_changed = 1;
  g_viewer.close_correctly = 0;
  pthread_mutex_init(&g_viewer.mutex, NULL);

  if (pthread_create(&g_viewer.thread_id, NULL, viewer_thread_func, NULL) != 0) {
    return SCLIMAGE_SHOW_THREAD_ERROR;
  }

  g_viewer.is_running = 1;
  return 0;
}


// Stop showing the image (ends the showing thread)
int sclimage_hide(Image* image, int argc, char* argv[]) {
  if (!g_viewer.is_running) {
    return 0;
  }

  g_viewer.should_quit = 1;
  pthread_join(g_viewer.thread_id, NULL);

  pthread_mutex_destroy(&g_viewer.mutex);
  g_viewer.is_running = 0;
  g_viewer.close_correctly = 1;
  return 0;
}
