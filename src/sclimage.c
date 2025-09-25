// T2
// ~~~Brilho ----.
// ~~~Contraste --'---> perguntar se é pra ser cumulativo, ou aplicar sempre em cima da original
// Convolução
// resize
// ~~~ flip
// histogram show
// histogram match
// histogram equalizate

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
int sclimage_rotate(Image* image, int argc, char* argv[]);
int sclimage_quantization(Image* image, int argc, char* argv[]);
int sclimage_histogram(Image* image, int argc, char* argv[]);
int sclimage_exit(Image* image, int argc, char* argv[]);
int sclimage_brightness(Image* image, int argc, char* argv[]);
int sclimage_contrast(Image* image, int argc, char* argv[]);

ViewerState g_image_viewer = {0}; // Global viewer state
ViewerState g_histogram_viewer = {0}; // Global viewer state


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
  {"rotate", sclimage_rotate},
  {"quantization", sclimage_quantization},
  {"histogram", sclimage_histogram}, // subcommand with more arguments
  {"brightness", sclimage_brightness},
  {"contrast", sclimage_contrast},
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

const char* rotate_options[] = {
    "--clockwise",
    "--counter",
    "-a",
    NULL // O final da lista deve ser NULL
};

// Gerador que encontra correspondências na lista 'rotate_options'
char* rotate_option_generator(const char* text, int state) {
    static int list_index, len;
    const char* name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    // Procura o próximo nome na lista que corresponde ao texto
    while ((name = rotate_options[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL; // Nenhuma outra correspondência encontrada
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

  if (command && strcmp(command, "rotate") == 0) {
    free(line_copy);
    return rl_completion_matches(text, rotate_option_generator);
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

  if (g_image_viewer.is_running || (!g_image_viewer.close_correctly)) { // If the viewer is running, close it before exiting to prevent memory leak
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
	 "  brightness <b>           - Adjust the brightness by a <b> value in range [-255,255].\n"
	 "  contrast <a>           - Adjust the contrast by an <a> value in range (0,255].\n"
	 "  hflip                    - Flips the image horizontally.\n"
	 "  vflip                    - Flips the image vertically.\n"
	 "  rotate                   - Rotate the image clockwise, use the option -a (or --counter) to flip it ocunter-clockwise.\n"
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
      pthread_mutex_lock(&g_image_viewer.mutex);

      temp = pixels[i*ppl + j];
      pixels[i*ppl + j] = pixels[i*ppl + ppl - j - 1];
      pixels[i*ppl + ppl - j - 1] = temp;

      g_image_viewer.has_changed = 1;
      pthread_mutex_unlock(&g_image_viewer.mutex);
    }
  }

  SDL_UnlockSurface(surface);
  return 0;
}

int sclimage_vflip(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;

  Uint32 hold_buffer[SCLIMAGE_MAX_IMAGE_WIDTH] = {0};
  int ppl = surface->w;

  for (int i = 0; i < (surface->h)/2; i++) {
    pthread_mutex_lock(&g_image_viewer.mutex);

    memcpy(hold_buffer, pixels + i*ppl, sizeof(Uint32)*ppl);
    memcpy(pixels + i*ppl, pixels + ppl*(surface->h - 1) - i*ppl, sizeof(Uint32)*ppl);
    memcpy(pixels + ppl*(surface->h - 1) - i*ppl, hold_buffer, sizeof(Uint32)*ppl);

    g_image_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_image_viewer.mutex);
  }

  SDL_UnlockSurface(surface);

  return 0;
}



// rotate the image clockwise
int sclimage_rotate(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;


  int src_w = surface->w;
  int src_h = surface->h;

  int dest_w = src_h;
  int dest_h = src_w;

  int direction = 1;
  int opt;

    // --- 1. Define the Long Options ---
  static struct option long_options[] = {
    // name,       has_arg,            flag, val
    {"clockwise",         no_argument,   NULL, 'c'},
      {"counter",  no_argument,  NULL, 'a'},
	{0, 0, 0, 0} // Marks the end of the array
  };

  optind = 1;
  // --- 2. The getopt_long() Loop ---
  // The short option string "o:O:g" is still used for short options.
  while ((opt = getopt_long(argc, argv, "ca", long_options, NULL)) != -1) {
    switch (opt) {
    case 'c':
      direction = 1;
      break;
    case 'a':
      direction = -1;
      break;
    }
  }

  // new surface with the new dimensions
  SDL_Surface* dest = SDL_CreateRGBSurfaceWithFormat(0, dest_w, dest_h,
                                                     surface->format->BitsPerPixel,
                                                     surface->format->format);
  if (!dest) {
    fprintf(stderr, "Erro ao criar superfície de destino: %s\n", SDL_GetError());
    return SCLIMAGE_LOAD_ERROR;
  }

  SDL_LockSurface(surface);
  SDL_LockSurface(dest);

  Uint32* src_pixels = (Uint32*)surface->pixels;
  Uint32* dest_pixels = (Uint32*)dest->pixels;

  // 3. Itera sobre cada pixel da imagem ORIGINAL (mapeamento direto)
  for (int sy = 0; sy < src_h; ++sy) {
    for (int sx = 0; sx < src_w; ++sx) {
      pthread_mutex_lock(&g_image_viewer.mutex);
      // 4. Calcula a nova posição do pixel na imagem de destino

      int dx = 0;
      int dy = 0;

      if(direction == -1){
	dx = sy;
	dy = (src_w - 1) - sx;
      } else {
	dx = (src_h - 1) - sy;
	dy = sx;
      }

      // 5. Copia o pixel
      dest_pixels[dy * dest_w + dx] = src_pixels[sy * src_w + sx];

      g_image_viewer.has_changed = 1;
      pthread_mutex_unlock(&g_image_viewer.mutex);
    }
  }

  pthread_mutex_lock(&g_image_viewer.mutex);
  image->surface = dest;
  SDL_FreeSurface(surface);
  g_image_viewer.has_changed = 1;
  pthread_mutex_unlock(&g_image_viewer.mutex);

  return 0;
}



// Apply negative filter to the image
int sclimage_negative(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;
  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_image_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRBGA(pixel, &r, &g, &b, &a);

    r = 255 - r;
    g = 255 - g;
    b = 255 - b;

    pixels[i] = setRGBA(r, g, b, a);

    g_image_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_image_viewer.mutex);
  }

  SDL_UnlockSurface(surface);

  return 0;
}

/* // Apply negative filter to the image */
/* int sclimage_gaussian(Image* image, int argc, char* argv[]){ */

/*   flaot gaussian_filter[9] = {0.0625, 0.125, 0.0625, */
/* 			      0.125,  0.25,  0.125, */
/* 			      0.0625, 0.125, 0.0625}; */

/*   sclimage_convolute(image, gaussian_filter, 3); */

/*   return 0; */
/* } */


// Apply the grayscale filter to the image
int sclimage_grayscale(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;
  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_image_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;

    getRBGA(pixel, &r, &g, &b, &a);

    Uint8 gray = (Uint8)(0.299 * r + 0.587 * g + 0.114 * b);
    r = gray;
    g = gray;
    b = gray;

    pixels[i] = setRGBA(r, g, b, a);

    g_image_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_image_viewer.mutex);
  }

  SDL_UnlockSurface(surface);

  return 0;
}

// Adjust brightness of an image
// Is this right?
int sclimage_brightness(Image* image, int argc, char* argv[]){

  if(argc <= 1){
    return SCLIMAGE_BRIGHTNESS_ARGUMENT_MISSING;
  }

  int scalar = max(-255, min(255, atoi(argv[1])));
  SDL_Surface* surface = image->surface;
  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_image_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;

    getRBGA(pixel, &r, &g, &b, &a);

    r = min(255, max(0, r + scalar));
    g = min(255, max(0, g + scalar));
    b = min(255, max(0, b + scalar));

    pixels[i] = setRGBA(r, g, b, a);

    g_image_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_image_viewer.mutex);
  }

  SDL_UnlockSurface(surface);

  return 0;
}


// Adjust contrast of an image
// Is this right?
int sclimage_contrast(Image* image, int argc, char* argv[]){

  if(argc <= 1){
    return SCLIMAGE_CONTRAST_ARGUMENT_MISSING;
  }

  int scalar = max(1, min(255, atoi(argv[1])));
  SDL_Surface* surface = image->surface;
  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_image_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;

    getRBGA(pixel, &r, &g, &b, &a);

    r = min(255, max(0, r*scalar));
    g = min(255, max(0, g*scalar));
    b = min(255, max(0, b*scalar));
    //    printf("(%d, %d, %d)\n", r, g, b);

    pixels[i] = setRGBA(r, g, b, a);

    g_image_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_image_viewer.mutex);
  }

  SDL_UnlockSurface(surface);

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
    pthread_mutex_lock(&g_image_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRBGA(pixel, &r, &g, &b, &a);

    int f = (int)floor((float)(r-tmin)/bin_size);
    Uint16 new_shade = tmin + (Uint16) f * bin_size + bin_size/2; // Calculates the new shade for the pixel

    r = (Uint8)new_shade;
    g = (Uint8)new_shade;
    b = (Uint8)new_shade;

    pixels[i] = setRGBA(r, g, b, a);

    g_image_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_image_viewer.mutex);

    if(new_histogram[new_shade] == 0){
      new_shades++;
    }
    new_histogram[new_shade]++;
  }

  return 0;
}


// Restart the image to its original form
int sclimage_restart(Image* image, int argc, char* argv[]){

  pthread_mutex_lock(&g_image_viewer.mutex);

  Uint32* pixels_edited = (Uint32*)image->surface->pixels;
  Uint32* pixels_original = (Uint32*)image->original->pixels;
  int pixel_count = image->surface->w * image->surface->h;
  memcpy(pixels_edited, pixels_original, sizeof(Uint32)*pixel_count);

  g_image_viewer.has_changed = 1;
  pthread_mutex_unlock(&g_image_viewer.mutex);

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

  pthread_mutex_lock(&g_image_viewer.mutex); // Lock before touching shared data

  image->surface = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);
  image->original = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);

  SDL_FreeSurface(original_surface); // Free the surface used to load
  if (!(image->surface)) {
    return SCLIMAGE_CONVERT_ERROR;
  }

  if (g_image_viewer.is_running) {
    g_image_viewer.has_changed = 1;
  }

  pthread_mutex_unlock(&g_image_viewer.mutex); // Unlock after updating

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
void* viewer_image_thread_func(void* arg) {

  SDL_Init(SDL_INIT_VIDEO); // Start video to show the image

  SDL_Window* window = SDL_CreateWindow("Original and Edited View", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
					g_image_viewer.image->surface->w + g_image_viewer.image->original->w,
					max(g_image_viewer.image->surface->h, g_image_viewer.image->original->h),
					SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

  // Using streaming textures to update frequently
  SDL_Texture* texture_original = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
						    g_image_viewer.image->original->w, g_image_viewer.image->original->h);
  SDL_Texture* texture_edited = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING,
						  g_image_viewer.image->surface->w, g_image_viewer.image->surface->h);

  SDL_UpdateTexture(texture_original, NULL, g_image_viewer.image->original->pixels, g_image_viewer.image->original->pitch);
  SDL_UpdateTexture(texture_edited, NULL, g_image_viewer.image->surface->pixels, g_image_viewer.image->surface->pitch);

  SDL_Rect original_dest_rect = {0, 0, g_image_viewer.image->original->w, g_image_viewer.image->original->h};
  SDL_Rect edited_dest_rect = {g_image_viewer.image->original->w, 0, g_image_viewer.image->surface->w, g_image_viewer.image->surface->h};


  int current_w = g_image_viewer.image->surface->w + g_image_viewer.image->original->w;
  int current_h = max(g_image_viewer.image->surface->h, g_image_viewer.image->original->h);

  SDL_Event event;
  while (!g_image_viewer.should_quit) {

    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
	g_image_viewer.should_quit = 1;
	g_image_viewer.close_correctly = 0;
      }
    }

    pthread_mutex_lock(&g_image_viewer.mutex);
    if (g_image_viewer.has_changed) { // Checking updates in the image by the functions
      Image* current_image = g_image_viewer.image;

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
	original_dest_rect = (SDL_Rect){0, 0, g_image_viewer.image->original->w, g_image_viewer.image->original->h};
	edited_dest_rect = (SDL_Rect){g_image_viewer.image->original->w, 0, g_image_viewer.image->surface->w, g_image_viewer.image->surface->h};
      }

      SDL_UpdateTexture(texture_original, NULL, current_image->original->pixels, current_image->original->pitch);
      SDL_UpdateTexture(texture_edited, NULL, current_image->surface->pixels, g_image_viewer.image->surface->pitch);
      g_image_viewer.has_changed = 0; // Reset the flag
    }
    pthread_mutex_unlock(&g_image_viewer.mutex);

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

  if (g_image_viewer.is_running) {
    return SCLIMAGE_ALREADY_SHOWING;
  }

  if (image == NULL || image->surface == NULL) {
    return SCLIMAGE_SHOW_NULL;
  }

  g_image_viewer.image = image;
  g_image_viewer.should_quit = 0;
  g_image_viewer.has_changed = 1;
  g_image_viewer.close_correctly = 0;
  pthread_mutex_init(&g_image_viewer.mutex, NULL);

  if (pthread_create(&g_image_viewer.thread_id, NULL, viewer_image_thread_func, NULL) != 0) {
    return SCLIMAGE_SHOW_THREAD_ERROR;
  }

  g_image_viewer.is_running = 1;
  return 0;
}


// Stop showing the image (ends the showing thread)
int sclimage_hide(Image* image, int argc, char* argv[]) {
  if (!g_image_viewer.is_running) {
    return 0;
  }

  g_image_viewer.should_quit = 1;
  pthread_join(g_image_viewer.thread_id, NULL);

  pthread_mutex_destroy(&g_image_viewer.mutex);
  g_image_viewer.is_running = 0;
  g_image_viewer.close_correctly = 1;
  return 0;
}


// Protótipos das funções auxiliares
void calculate_histogram(SDL_Surface* surface, unsigned int histogram[256]);
unsigned int normalize_histogram(unsigned int raw_histogram[256], int normalized_histogram[256], int graph_height);
SDL_Texture* create_text_texture(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color);


void* viewer_histogram_thread_func(void* arg);

// Shows the image by calling a thread to do so, doing this prevents the terminal to freeze
int sclimage_histogram(Image* image, int argc, char* argv[]) {

  if (g_histogram_viewer.is_running) {
    return SCLIMAGE_ALREADY_SHOWING;
  }

  if (image == NULL || image->surface == NULL) {
    return SCLIMAGE_SHOW_NULL;
  }

  g_histogram_viewer.image = image;
  g_histogram_viewer.should_quit = 0;
  g_histogram_viewer.has_changed = 1;
  g_histogram_viewer.close_correctly = 0;
  pthread_mutex_init(&g_histogram_viewer.mutex, NULL);

  if (pthread_create(&g_histogram_viewer.thread_id, NULL, viewer_histogram_thread_func, NULL) != 0) {
    return SCLIMAGE_SHOW_THREAD_ERROR;
  }

  g_histogram_viewer.is_running = 1;
  return 0;
}


void* viewer_histogram_thread_func(void* arg) {

    // --- INICIALIZAÇÃO ---
    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
    TTF_Init();

    // --- CARREGAMENTO E CÁLCULO ---
    SDL_Surface* surface = g_histogram_viewer.image->surface;

    const int GRAPH_BASE_WIDTH = 256;
    const int GRAPH_BASE_HEIGHT = 200;
    const int SCALE_X = 2;
    const int MARGIN_TOP = 40, MARGIN_BOTTOM = 40, MARGIN_LEFT = 60, MARGIN_RIGHT = 20;
    const int GRAPH_WIDTH = GRAPH_BASE_WIDTH * SCALE_X;
    const int GRAPH_HEIGHT = GRAPH_BASE_HEIGHT;
    const int WINDOW_WIDTH = GRAPH_WIDTH + MARGIN_LEFT + MARGIN_RIGHT;
    const int WINDOW_HEIGHT = GRAPH_HEIGHT + MARGIN_TOP + MARGIN_BOTTOM;

    unsigned int raw_histogram[256];
    int normalized_histogram[256];

    calculate_histogram(surface, raw_histogram);
    unsigned int max_pixel_count = normalize_histogram(raw_histogram, normalized_histogram, GRAPH_HEIGHT);
    //    SDL_FreeSurface(surface);

    // --- JANELA, RENDERER E FONTE ---
    SDL_Window* window = SDL_CreateWindow("Histograma Estilizado", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf", 16);
    if (!font) font = TTF_OpenFont("font.ttf", 16);

    // --- CRIAÇÃO DOS RÓTULOS DE TEXTO ---
    SDL_Color text_color = {0, 0, 0, 255};

    // Armazena as strings dos rótulos em buffers
    char max_text[20], mid_text[20], zero_text[] = "0";
    char x0_text[] = "0", x128_text[] = "128", x255_text[] = "255";
    snprintf(max_text, 20, "%u", max_pixel_count);
    snprintf(mid_text, 20, "%u", max_pixel_count / 2);

    // Cria as texturas a partir dos buffers de string
    SDL_Texture* label_max_y = create_text_texture(renderer, font, max_text, text_color);
    SDL_Texture* label_mid_y = create_text_texture(renderer, font, mid_text, text_color);
    SDL_Texture* label_0_y = create_text_texture(renderer, font, zero_text, text_color);
    SDL_Texture* label_0_x = create_text_texture(renderer, font, x0_text, text_color);
    SDL_Texture* label_128_x = create_text_texture(renderer, font, x128_text, text_color);
    SDL_Texture* label_255_x = create_text_texture(renderer, font, x255_text, text_color);

    // --- POSICIONAMENTO DOS RÓTULOS (CORRIGIDO) ---
    SDL_Rect r_max_y, r_mid_y, r_0_y, r_0_x, r_128_x, r_255_x;

    // Usa as strings originais para calcular o tamanho
    TTF_SizeText(font, max_text, &r_max_y.w, &r_max_y.h);
    TTF_SizeText(font, mid_text, &r_mid_y.w, &r_mid_y.h);
    TTF_SizeText(font, zero_text, &r_0_y.w, &r_0_y.h);
    TTF_SizeText(font, x0_text, &r_0_x.w, &r_0_x.h);
    TTF_SizeText(font, x128_text, &r_128_x.w, &r_128_x.h);
    TTF_SizeText(font, x255_text, &r_255_x.w, &r_255_x.h);

    // Define as coordenadas (x, y)
    r_max_y.x = MARGIN_LEFT - r_max_y.w - 5; r_max_y.y = MARGIN_TOP - (r_max_y.h / 2);
    r_mid_y.x = MARGIN_LEFT - r_mid_y.w - 5; r_mid_y.y = MARGIN_TOP + (GRAPH_HEIGHT / 2) - (r_mid_y.h / 2);
    r_0_y.x = MARGIN_LEFT - r_0_y.w - 5;   r_0_y.y = MARGIN_TOP + GRAPH_HEIGHT - (r_0_y.h / 2);

    r_0_x.x = MARGIN_LEFT; r_0_x.y = MARGIN_TOP + GRAPH_HEIGHT + 5;
    r_128_x.x = MARGIN_LEFT + (GRAPH_WIDTH / 2) - (r_128_x.w / 2); r_128_x.y = MARGIN_TOP + GRAPH_HEIGHT + 5;
    r_255_x.x = MARGIN_LEFT + GRAPH_WIDTH - r_255_x.w; r_255_x.y = MARGIN_TOP + GRAPH_HEIGHT + 5;

    // --- LOOP DE RENDERIZAÇÃO ---
    int quit = 0;
    SDL_Event event;
    while (!quit) {
        while (SDL_PollEvent(&event)) { if (event.type == SDL_QUIT) quit = 1; }

        // Gradiente do fundo
        for (int y = 0; y < WINDOW_HEIGHT; ++y) {
	  Uint8 c = 255;// - (Uint8)(y * (20.0 / WINDOW_HEIGHT));
            SDL_SetRenderDrawColor(renderer, c, c, c, 255);
            SDL_RenderDrawLine(renderer, 0, y, WINDOW_WIDTH, y);
        }
        // Desenho dos eixos
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawLine(renderer, MARGIN_LEFT, MARGIN_TOP, MARGIN_LEFT, MARGIN_TOP + GRAPH_HEIGHT);
        SDL_RenderDrawLine(renderer, MARGIN_LEFT, MARGIN_TOP + GRAPH_HEIGHT, MARGIN_LEFT + GRAPH_WIDTH, MARGIN_TOP + GRAPH_HEIGHT);
        // Barras do histograma
        for (int i = 0; i < GRAPH_BASE_WIDTH; ++i) {
            int bar_height = normalized_histogram[i];
            if (bar_height > 0) {
                SDL_Rect bar_rect = {MARGIN_LEFT + (i * SCALE_X), MARGIN_TOP + GRAPH_HEIGHT - bar_height, SCALE_X, bar_height};
                /* Uint8 r = 100 + (bar_height * 155 / GRAPH_HEIGHT); */
                /* Uint8 b = 255 - (bar_height * 155 / GRAPH_HEIGHT); */
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &bar_rect);
            }
        }
        // Desenho dos rótulos
        SDL_RenderCopy(renderer, label_max_y, NULL, &r_max_y);
        SDL_RenderCopy(renderer, label_mid_y, NULL, &r_mid_y);
        SDL_RenderCopy(renderer, label_0_y, NULL, &r_0_y);
        SDL_RenderCopy(renderer, label_0_x, NULL, &r_0_x);
        SDL_RenderCopy(renderer, label_128_x, NULL, &r_128_x);
        SDL_RenderCopy(renderer, label_255_x, NULL, &r_255_x);

        SDL_RenderPresent(renderer);
        SDL_Delay(30);
    }

    // --- LIMPEZA ---
    SDL_DestroyTexture(label_max_y);
    SDL_DestroyTexture(label_mid_y);
    // ... (destruir todas as outras texturas de rótulo) ...
    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();

    return NULL;
}


void calculate_histogram(SDL_Surface* surface, unsigned int histogram[256]) {
    memset(histogram, 0, 256 * sizeof(unsigned int));
    SDL_LockSurface(surface);
    Uint32* pixels = (Uint32*)surface->pixels;
    for (int i = 0; i < surface->w * surface->h; ++i) {
        Uint8 r, g, b, a;
        SDL_GetRGBA(pixels[i], surface->format, &r, &g, &b, &a);
        Uint8 luminance = (Uint8)(0.299 * r + 0.587 * g + 0.114 * b);
        histogram[luminance]++;
    }
    SDL_UnlockSurface(surface);
}

unsigned int normalize_histogram(unsigned int raw_histogram[256], int normalized_histogram[256], int graph_height) {
    unsigned int max_value = 0;
    for (int i = 0; i < 256; ++i) {
        if (raw_histogram[i] > max_value) max_value = raw_histogram[i];
    }
    if (max_value == 0) {
        memset(normalized_histogram, 0, 256 * sizeof(int));
        return 0;
    }
    for (int i = 0; i < 256; ++i) {
        normalized_histogram[i] = (int)(((double)raw_histogram[i] / max_value) * graph_height);
    }
    return max_value;
}

SDL_Texture* create_text_texture(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color) {
    SDL_Surface* text_surface = TTF_RenderText_Blended(font, text, color);
    if (!text_surface) {
        printf("Erro ao renderizar texto: %s\n", TTF_GetError());
        return NULL;
    }
    SDL_Texture* text_texture = SDL_CreateTextureFromSurface(renderer, text_surface);
    SDL_FreeSurface(text_surface);
    return text_texture;
}
