// T2
// ~~~Brilho ----.
// ~~~Contraste --'---> perguntar se é pra ser cumulativo, ou aplicar sempre em cima da original
// ~~ Convolução ~~
// resize
// ~~~ flip
// ~~histogram show~~
// ~~histogram match~~
// ~~histogram equalizate~~

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
int sclimage_convolution(Image* image, int argc, char* argv[]);
int sclimage_zoom_in(Image* image, int argc, char* argv[]);
int sclimage_zoom_out(Image* image, int argc, char* argv[]);
int sclimage_histogram_show(Image* image, int argc, char* argv[]);
int sclimage_histogram_close(Image* image, int argc, char* argv[]);
int sclimage_histogram_equalization(Image* image, int argc, char* argv[]);
int sclimage_histogram_matching(Image* image, int argc, char* argv[]);
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
  {"zoom_in", sclimage_zoom_in},
  {"zoom_out", sclimage_zoom_out},
  {"quantization", sclimage_quantization},
  {"histogram", sclimage_histogram}, // subcommand with more arguments
  {"brightness", sclimage_brightness},
  {"contrast", sclimage_contrast},
  {"convolution", sclimage_convolution},
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


const char* convolution_options[] = {
    "gaussian",
    "laplacian",
    "high_pass",
    "prewitt_hx",
    "prewitt_hy",
    "sobel_hx",
    "sobel_hy",
    NULL // O final da lista deve ser NULL
};

// Gerador que encontra correspondências na lista 'rotate_options'
char* convolution_option_generator(const char* text, int state) {
    static int list_index, len;
    const char* name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    // Procura o próximo nome na lista que corresponde ao texto
    while ((name = convolution_options[list_index++])) {
        if (strncmp(name, text, len) == 0) {
            return strdup(name);
        }
    }

    return NULL; // Nenhuma outra correspondência encontrada
}


//
const char* histogram_subcommands[] = {
    "show",
    "close",
    "equalization",
    "matching",
    NULL
};


char* histogram_subcommands_generator(const char* text, int state) {
    static int list_index, len;
    const char* name;

    if (!state) {
        list_index = 0;
        len = strlen(text);
    }

    // Procura o próximo nome na lista que corresponde ao texto
    while ((name = histogram_subcommands[list_index++])) {
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

  // Se não, analisamos a linha para descobrir o contexto
  char* context_line = strndup(rl_line_buffer, start);
  if (!context_line) return NULL; // falha de alocação de memória

  char* words[SCLIMAGE_MAX_ARGS];
  int word_count = 0;

  char* token = strtok(context_line, " \t");
  while (token != NULL && word_count < SCLIMAGE_MAX_ARGS) {
    words[word_count++] = token;
    token = strtok(NULL, " \t");
  }

  // Se não houver palavras (linha com espaços), não faz nada
  if (word_count == 0) {
    free(context_line); // Liberamos a cópia
    return NULL;
  }

  char* command = words[0];

  if(word_count == 1){
    // Check if the command is one that takes a filename as a argument.
    if ((strcmp(command, "load") == 0 || strcmp(command, "open") == 0)) {
      free(context_line); // Liberamos a cópia
      return rl_completion_matches(text, rl_filename_completion_function);
    }

    if (strcmp(command, "rotate") == 0) {
      free(context_line); // Liberamos a cópia
      return rl_completion_matches(text, rotate_option_generator);
    }

    if (strcmp(command, "convolution") == 0) {
      free(context_line); // Liberamos a cópia
      return rl_completion_matches(text, convolution_option_generator);
    }

    if (command && strcmp(command, "histogram") == 0) {
      free(context_line); // Liberamos a cópia
      return rl_completion_matches(text, histogram_subcommands_generator);
    }
  }

  if (word_count == 2) {
    if (strcmp(command, "histogram") == 0 && strcmp(words[1], "matching") == 0) {
      free(context_line); // Liberamos a cópia
      return rl_completion_matches(text, rl_filename_completion_function);
    }
  }

  free(context_line); // Liberamos a cópia
  return NULL;
}


int main(int argc, char** argv) {

  char* line;
  char* argv_int[SCLIMAGE_MAX_ARGS];
  int argc_int = 0;
  int status = 0;
  int stop = 0;

  IMG_Init(IMG_INIT_JPG); // Set up image JPG loading
  SDL_Init(SDL_INIT_VIDEO);

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

int convolution(Image* image, float filter[3][3], int clamping){

  SDL_Surface* surface = image->surface;
  Uint32* pixels = (Uint32*)surface->pixels;

  float r_mean = 0, g_mean = 0, b_mean = 0;

  SDL_LockSurface(surface);
  int h = surface->h;
  int w = surface->w;

  int buffer_size = w * h * sizeof(Uint32);
  Uint32* pixels_copy = malloc(buffer_size);

  memcpy(pixels_copy, pixels, buffer_size);

  for(int i = 1; i < h - 1; i++){
    for(int j = 1; j < w - 1; j++){
      pthread_mutex_lock(&g_image_viewer.mutex);

      r_mean = g_mean = b_mean = 0;
      for(int y = 0; y < 3; y++){
	for(int x = 0; x < 3; x++){

	  Uint8 r, g, b, a;
	  getRGBA(pixels_copy[(i + y - 1) * w + (j + x - 1)], &r, &g, &b, &a);

	  r_mean += (float)filter[y][x]*r;
	  g_mean += (float)filter[y][x]*g;
	  b_mean += (float)filter[y][x]*b;

	}
      }

      r_mean = (Uint8)max(0, min(255, r_mean + clamping*127));
      g_mean = (Uint8)max(0, min(255, g_mean + clamping*127));
      b_mean = (Uint8)max(0, min(255, b_mean + clamping*127));

      pixels[i * w + j] = setRGBA(r_mean, g_mean, b_mean, 255);

      g_image_viewer.has_changed = 1;
      pthread_mutex_unlock(&g_image_viewer.mutex);
    }
  }

  free(pixels_copy);
  SDL_UnlockSurface(surface);

  g_image_viewer.has_changed = 1;
  pthread_mutex_unlock(&g_image_viewer.mutex);

  return 0;

}

// apply a convolution filter to the image
int sclimage_convolution(Image* image, int argc, char* argv[]){

  if(argc <= 1){
    printf("Warning: need the name for the convolution filter or its coeficients");
  }

  if(argc == 10){
    int c = 1;
    float filter[3][3] = {0};

    for(int y = 0; y < 3; y++){
      for(int x = 0; x < 3; x++){
	filter[y][x] = atof(argv[c++]);
      }
    }
    convolution(image, filter, 0);
  } else {

    if(strcmp("gaussian", argv[1]) == 0){
      float apply_filter[3][3] = {{0.0625, 0.125, 0.0625},
				  {0.125,  0.25,  0.125},
				  {0.0625, 0.125, 0.0625}};
      return convolution(image, apply_filter, 0);
    }

    if(strcmp("laplacian", argv[1]) == 0){
      float apply_filter[3][3] = {{0,  -1,  0},
				  {-1,  4, -1},
				  {0,  -1,  0}};
      return convolution(image, apply_filter, 1);
    }

     if(strcmp("high_pass", argv[1]) == 0){
      float apply_filter[3][3] = {{-1, -1, -1},
				  {-1,  8, -1},
				  {-1, -1, -1}};
      return convolution(image, apply_filter, 0);
    }

     if(strcmp("prewitt_hx", argv[1]) == 0){
       float apply_filter[3][3] = {{-1, 0, 1},
				   {-1, 0, 1},
				   {-1, 0, 1}};
       return convolution(image, apply_filter, 1);
     }

     if(strcmp("prewitt_hy", argv[1]) == 0){
      float apply_filter[3][3] = {{-1, -1, -1},
				  { 0,  0,  0},
				  { 1,  1,  1}};
      return convolution(image, apply_filter, 1);
    }


     if(strcmp("sobel_hx", argv[1]) == 0){
       float apply_filter[3][3] = {{-1, 0, 1},
				   {-2, 0, 2},
				   {-1, 0, 1}};
       return convolution(image, apply_filter, 1);
     }

     if(strcmp("sobel_hy", argv[1]) == 0){
       float apply_filter[3][3] = {{-1, -1, -1},
				   { 0,  0,  0},
				   { 1,  2,  1}};
       return convolution(image, apply_filter, 1);
     }
  }

  return -23918412;
}


int sclimage_zoom_out(Image* image, int argc, char* argv[]){

  if(argc <= 2){
    printf("Warning: need the values for sx and sy\n");
    return -29814912;
  }

  SDL_Surface* surface = image->surface;

  int sx = atoi(argv[1]);
  int sy = atoi(argv[2]);

  int w = surface->w;
  int h = surface->h;

  int dest_w = w/sx;
  int dest_h = h/sy;

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
  float r_mean = 0, g_mean = 0, b_mean = 0;

  int dest_x = 0, dest_y = 0;
  for (int i = 0; i < h; i += sy) {

    dest_x = 0;
    for (int j = 0; j < w; j += sx) {

      r_mean = b_mean = g_mean = 0;
      int counter = 0;
      for (int y = 0; y < sy; y++) {
	for (int x = 0; x < sx; x++) {
	  if(i + y < h && j + x < w){
	    Uint8 r, g, b, a;
	    getRGBA(src_pixels[(i+y) * w + j+x], &r, &g, &b, &a);
	    r_mean += r;
	    g_mean += g;
	    b_mean += b;
	    counter++;
	  }

	}
      }

      printf("[MAX %d %d] %d %d <- %f %f %f\n", dest_h, dest_w, dest_y, dest_x, r_mean/(sx*sy), g_mean/(sx*sy), b_mean/(sx*sy));
      if(counter > 0){
	dest_pixels[dest_y * dest_w + dest_x] = setRGBA(r_mean/(counter), g_mean/(counter), b_mean/(counter), 255);
      }
      dest_x = min(dest_x + 1, dest_w - 1);
    }
    dest_y = min(dest_y + 1, dest_h - 1);
  }

  SDL_UnlockSurface(surface);
  SDL_UnlockSurface(dest);

  pthread_mutex_lock(&g_image_viewer.mutex);

  SDL_FreeSurface(surface);
  image->surface = dest;

  pthread_mutex_unlock(&g_image_viewer.mutex);
  g_image_viewer.has_changed = 1;

  return 0;
}


// resize the image
int sclimage_zoom_in(Image* image, int argc, char* argv[]){

  SDL_Surface* surface = image->surface;

  int src_w = surface->w;
  int src_h = surface->h;

  int dest_w = src_w*2 - 1;
  int dest_h = src_h*2 - 1;

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

  for (int i = 0; i < src_h; i++) {
    for (int j = 0; j < dest_w; j++) {
      pthread_mutex_lock(&g_image_viewer.mutex);

      // indices pares
      if(j % 2 == 0){
	dest_pixels[i*2 * dest_w + j] = src_pixels[i * src_w + (int)j/2];
      } else {

	Uint8 r1, g1, b1, a1;
	Uint8 r2, g2, b2, a2;

	getRGBA(src_pixels[i * src_w + (int)(j - 1)/2], &r1, &g1, &b1, &a1);
	getRGBA(src_pixels[i * src_w + (int)(j + 1)/2], &r2, &g2, &b2, &a2);

	dest_pixels[i*2 * dest_w + j] = setRGBA((r1+r2)/2, (g1+g2)/2, (b1+b2)/2, (a1+a2)/2);
      }

      g_image_viewer.has_changed = 1;
      pthread_mutex_unlock(&g_image_viewer.mutex);
    }
  }

  /* printf("aqui fora\n"); */
  for (int j = 0; j < dest_w; j++) {
    for (int i = 0; i < dest_h; i++) {
      pthread_mutex_lock(&g_image_viewer.mutex);

      if(i % 2 == 1){
	Uint8 r1, g1, b1, a1;
	Uint8 r2, g2, b2, a2;

	getRGBA(dest_pixels[(i - 1) * dest_w + j], &r1, &g1, &b1, &a1);
	getRGBA(dest_pixels[(i + 1) * dest_w + j], &r2, &g2, &b2, &a2);

	dest_pixels[i * dest_w + j] = setRGBA((r1+r2)/2, (g1+g2)/2, (b1+b2)/2, (a1+a2)/2);
      }

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
    getRGBA(pixel, &r, &g, &b, &a);

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

    getRGBA(pixel, &r, &g, &b, &a);

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

    getRGBA(pixel, &r, &g, &b, &a);

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

  float scalar = max(0, min(255, atof(argv[1])));
  SDL_Surface* surface = image->surface;
  SDL_LockSurface(surface);

  Uint32* pixels = (Uint32*)surface->pixels;
  int pixel_count = surface->w * surface->h;

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_image_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;

    getRGBA(pixel, &r, &g, &b, &a);

    r = (Uint8)min(255, max(0, r*scalar));
    g = (Uint8)min(255, max(0, g*scalar));
    b = (Uint8)min(255, max(0, b*scalar));
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

  for (int i = 0; i < pixel_count; i++) {

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRGBA(pixel, &r, &g, &b, &a);

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
    getRGBA(pixel, &r, &g, &b, &a);

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

  SDL_FreeSurface(image->surface);
  image->surface = SDL_CreateRGBSurfaceWithFormat(0, image->original->w, image->original->h,
						     image->original->format->BitsPerPixel,
						     image->original->format->format);

  Uint32* pixels_original = (Uint32*)image->original->pixels;
  Uint32* pixels_image = (Uint32*)image->surface->pixels;
  int pixel_count = image->original->w * image->original->h;
  memcpy(pixels_image, pixels_original, sizeof(Uint32)*pixel_count);

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

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture_original, NULL, &original_dest_rect);
    SDL_RenderCopy(renderer, texture_edited, NULL, &edited_dest_rect);
    SDL_RenderPresent(renderer);

    pthread_mutex_unlock(&g_image_viewer.mutex);
  }


  SDL_DestroyTexture(texture_original);
  SDL_DestroyTexture(texture_edited);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);

  return NULL;
}



// Equalizate the histogram of the image
int sclimage_histogram_equalization(Image* image, int argc, char* argv[]){

  if(!is_grayscale(image)){
    fprintf(stderr, "Warning: Histogram equalization only works for images in grayscale (at the moment). The image will be converted to grayscale before contining.\n");
    sclimage_grayscale(image, 0, NULL);
  }

  SDL_Surface* surface = image->surface;
  Uint32* pixels = (Uint32*)surface->pixels;

  int pixel_count = surface->w * surface->h;
  double scaling_factor = 255.0 / pixel_count;

  int histogram[SCLIMAGE_GRAYSCALE_RANGE] = {0};
  int histogram_sum[SCLIMAGE_GRAYSCALE_RANGE] = {0};

  SDL_LockSurface(surface);

  calculate_histogram(surface, histogram, '_');
  cumulative_histogram(histogram, histogram_sum, scaling_factor);

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_image_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRGBA(pixel, &r, &g, &b, &a);

    Uint8 new_shade = histogram_sum[r]; // Calculates the new shade for the pixel

    r = g = b = (Uint8)new_shade;

    pixels[i] = setRGBA(r, g, b, a);
    g_image_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_image_viewer.mutex);
  }

  return 0;
}


// Match the histogram of the image to another file
int sclimage_histogram_matching(Image* image, int argc, char* argv[]){

  if(argc <= 1){
    printf("Needs a image path as argument\n");
    return -2421841;
  }

  printf("Loading %s\n", argv[1]);

  SDL_Surface* original_surface = IMG_Load(argv[1]);
  if (!original_surface) {
    return SCLIMAGE_LOAD_ERROR;
  }

  SDL_Surface* match_surface = SDL_ConvertSurfaceFormat(original_surface, SDL_PIXELFORMAT_RGBA32, 0);

  SDL_FreeSurface(original_surface); // Free the surface used to load
  if (!(match_surface)) {
    return SCLIMAGE_CONVERT_ERROR;
  }

  Image match_image = {match_surface, match_surface, ""}; // This instance of Image represents the current image

  int color = 0;
  int opt;
  // --- 1. Define the Long Options ---
  static struct option long_options[] = {
    // name,       has_arg,            flag, val
    {"color",         no_argument,   NULL, 'c'},
      {0, 0, 0, 0} // Marks the end of the array
  };

  optind = 1;
  // --- 2. The getopt_long() Loop ---
  // The short option string "o:O:g" is still used for short options.
  while ((opt = getopt_long(argc, argv, "ca", long_options, NULL)) != -1) {
    switch (opt) {
    case 'c':
      color = 1;
      break;
    }
  }

  SDL_Surface* surface = image->surface;
  Uint32* pixels = (Uint32*)surface->pixels;

  int pixel_count = surface->w * surface->h;

  double scaling_factor = 255.0 / pixel_count;
  double scaling_factor_target = 255.0 / (match_surface->w * match_surface->h);

  int histogram_r[SCLIMAGE_GRAYSCALE_RANGE] = {0};
  int histogram_sum_r[SCLIMAGE_GRAYSCALE_RANGE] = {0};
  int histogram_target_r[SCLIMAGE_GRAYSCALE_RANGE] = {0};
  int histogram_sum_target_r[SCLIMAGE_GRAYSCALE_RANGE] = {0};

  int HMr[SCLIMAGE_GRAYSCALE_RANGE] = {0};
  int HMg[SCLIMAGE_GRAYSCALE_RANGE] = {0};
  int HMb[SCLIMAGE_GRAYSCALE_RANGE] = {0};

  if(!color){
    sclimage_grayscale(image, 0, NULL);
    sclimage_grayscale(&match_image, 0, NULL);
  }

  calculate_histogram(surface, histogram_r, 'r');
  calculate_histogram(match_surface, histogram_target_r, 'r');

  cumulative_histogram(histogram_r, histogram_sum_r, scaling_factor);
  cumulative_histogram(histogram_target_r, histogram_sum_target_r, scaling_factor_target);

  calculate_HM(HMr, histogram_sum_r, histogram_sum_target_r);

  if(color){
    int histogram_g[SCLIMAGE_GRAYSCALE_RANGE] = {0};
    int histogram_sum_g[SCLIMAGE_GRAYSCALE_RANGE] = {0};
    int histogram_target_g[SCLIMAGE_GRAYSCALE_RANGE] = {0};
    int histogram_sum_target_g[SCLIMAGE_GRAYSCALE_RANGE] = {0};

    int histogram_b[SCLIMAGE_GRAYSCALE_RANGE] = {0};
    int histogram_sum_b[SCLIMAGE_GRAYSCALE_RANGE] = {0};
    int histogram_target_b[SCLIMAGE_GRAYSCALE_RANGE] = {0};
    int histogram_sum_target_b[SCLIMAGE_GRAYSCALE_RANGE] = {0};

    calculate_histogram(surface, histogram_g, 'g');
    calculate_histogram(match_surface, histogram_target_g, 'g');

    calculate_histogram(surface, histogram_b, 'b');
    calculate_histogram(match_surface, histogram_target_b, 'b');

    cumulative_histogram(histogram_g, histogram_sum_g, scaling_factor);
    cumulative_histogram(histogram_target_g, histogram_sum_target_g, scaling_factor_target);

    cumulative_histogram(histogram_b, histogram_sum_b, scaling_factor);
    cumulative_histogram(histogram_target_b, histogram_sum_target_b, scaling_factor_target);

    calculate_HM(HMg, histogram_sum_g, histogram_sum_target_g);
    calculate_HM(HMb, histogram_sum_b, histogram_sum_target_b);
    printf("here\n");
  }

  SDL_LockSurface(surface);

  for (int i = 0; i < pixel_count; i++) {
    pthread_mutex_lock(&g_image_viewer.mutex);

    Uint32 pixel = pixels[i];
    Uint8 r, g, b, a;
    getRGBA(pixel, &r, &g, &b, &a);

    r = HMr[r];
    if(color){
      g = HMg[g];
      b = HMb[b];
    } else {
      g = HMr[g];
      b = HMr[b];
    }

    pixels[i] = setRGBA(r, g, b, a);
    g_image_viewer.has_changed = 1;
    pthread_mutex_unlock(&g_image_viewer.mutex);
  }

  SDL_UnlockSurface(surface);
  SDL_FreeSurface(match_surface);

  return 0;
  }

// Shows the image by calling a thread to do so, doing this prevents the terminal to freeze
int sclimage_show(Image* image, int argc, char* argv[]) {

  if (g_image_viewer.is_running && g_image_viewer.should_quit && !g_image_viewer.close_correctly) {
    sclimage_hide(NULL, 0, NULL);
  }

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
unsigned int normalize_histogram(int raw_histogram[256], int normalized_histogram[256], int graph_height);
SDL_Texture* create_text_texture(SDL_Renderer* renderer, TTF_Font* font, const char* text, SDL_Color color);


// All available commands
Command histogram_command_table[] = {
  {"show", sclimage_histogram_show},
  {"equalization", sclimage_histogram_equalization},
  {"matching", sclimage_histogram_matching},
  {"close", sclimage_histogram_close}
};

int num_subcommands_histogram = sizeof(histogram_command_table) / sizeof(Command);

int sclimage_histogram(Image* image, int argc, char* argv[]) {

  if(argc <= 1){
    printf("Missing argument for histogram menu\n");
    return -93138921;
  }

  int status = 0;

  int found_command = 0;
  for (int i = 0; i < num_subcommands_histogram; i++) {
    if (strcmp(argv[1], histogram_command_table[i].name) == 0) {

      // If find, call the function
      status = histogram_command_table[i].handler(image, argc - 1, argv + 1);

      found_command = 1;
      break;
    }
  }

  if (!found_command) {
    printf("Unknown command: 'histogram %s'. Type 'help' for a list of commands.\n", argv[1]);
  }

  return status;

}


void* viewer_histogram_thread_func(void* arg);

// Shows the image by calling a thread to do so, doing this prevents the terminal to freeze
int sclimage_histogram_show(Image* image, int argc, char* argv[]) {

  if (g_histogram_viewer.is_running && g_histogram_viewer.should_quit && !g_histogram_viewer.close_correctly) {
    sclimage_histogram_close(NULL, 0, NULL);
  }

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


// Shows the image by calling a thread to do so, doing this prevents the terminal to freeze
int sclimage_histogram_close(Image* image, int argc, char* argv[]) {
  if (!g_histogram_viewer.is_running) {
    return 0;
  }

  g_histogram_viewer.should_quit = 1;
  pthread_join(g_histogram_viewer.thread_id, NULL);

  pthread_mutex_destroy(&g_histogram_viewer.mutex);
  g_histogram_viewer.is_running = 0;
  g_histogram_viewer.close_correctly = 1;
  return 0;
}


void* viewer_histogram_thread_func(void* arg) {

  SDL_Init(SDL_INIT_VIDEO);
  IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
  TTF_Init();

  SDL_Surface* surface = g_histogram_viewer.image->surface;

  int GRAPH_BASE_WIDTH = 256;
  int GRAPH_BASE_HEIGHT = 200;
  int SCALE_X = 2;
  int MARGIN_TOP = 40, MARGIN_BOTTOM = 40, MARGIN_LEFT = 60, MARGIN_RIGHT = 20;
  int GRAPH_WIDTH = GRAPH_BASE_WIDTH * SCALE_X;
  int GRAPH_HEIGHT = GRAPH_BASE_HEIGHT;
  int WINDOW_WIDTH = GRAPH_WIDTH + MARGIN_LEFT + MARGIN_RIGHT;
  int WINDOW_HEIGHT = GRAPH_HEIGHT + MARGIN_TOP + MARGIN_BOTTOM;

  int raw_histogram[256];
  int normalized_histogram[256];

  calculate_histogram(surface, raw_histogram, '_');
  unsigned int max_pixel_count = normalize_histogram(raw_histogram, normalized_histogram, GRAPH_HEIGHT);


  SDL_Window* window = SDL_CreateWindow("Histograma Estilizado", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/liberation/LiberationMono-Regular.ttf", 16);
  if (!font) font = TTF_OpenFont("font.ttf", 16);

  SDL_Color text_color = {0, 0, 0, 255};

  // strings dos rótulos em buffers
  char max_text[20], mid_text[20], zero_text[] = "0";
  char x0_text[] = "0", x128_text[] = "128", x255_text[] = "255";
  snprintf(max_text, 20, "%u", max_pixel_count);
  snprintf(mid_text, 20, "%u", max_pixel_count / 2);

  // texturas a partir dos buffers de string
  SDL_Texture* label_max_y = create_text_texture(renderer, font, max_text, text_color);
  SDL_Texture* label_mid_y = create_text_texture(renderer, font, mid_text, text_color);
  SDL_Texture* label_0_y = create_text_texture(renderer, font, zero_text, text_color);
  SDL_Texture* label_0_x = create_text_texture(renderer, font, x0_text, text_color);
  SDL_Texture* label_128_x = create_text_texture(renderer, font, x128_text, text_color);
  SDL_Texture* label_255_x = create_text_texture(renderer, font, x255_text, text_color);

  // posicionamento dos rótulos
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
  while (!quit && !g_histogram_viewer.should_quit) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT){
	quit = 1;
      }
    }

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
    for (int i = 0; i < GRAPH_BASE_WIDTH; i++) {
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


  SDL_DestroyTexture(label_max_y);
  SDL_DestroyTexture(label_mid_y);
  TTF_CloseFont(font);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  g_histogram_viewer.is_running = 0;
  TTF_Quit();
  IMG_Quit();
  SDL_Quit();

  return NULL;
}

unsigned int normalize_histogram(int raw_histogram[256], int normalized_histogram[256], int graph_height) {
  unsigned int max_value = 0;
  for (int i = 0; i < 256; i++) {
    if (raw_histogram[i] > max_value) max_value = raw_histogram[i];
  }
  if (max_value == 0) {
    memset(normalized_histogram, 0, 256 * sizeof(int));
    return 0;
  }
  for (int i = 0; i < 256; i++) {
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
