#include <assert.h>
#include <ctype.h>
#include "common.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "thirdparty/stb_image.h"
#include "thirdparty/stb_image_write.h"

char *rle_encode(const char *str)
{
  int n = strlen(str);
  char *output = (char*)malloc(1024);
  int output_idx = 0;
  assert(output != NULL);
  for(int i = 0; i < n; ++i) {
    int count = 1;
    while(i < n-1 && str[i] == str[i+1]) {
      count++;
      i++;
    }
    output[output_idx++] = str[i];
    int _char_written = snprintf(output + output_idx, 12, "%d", count);
    output_idx += _char_written;
  }
  output[output_idx] = '\0';
  return output;
  
}

char *rle_decode(const char *str)
{
  char *decoded_str = (char*)malloc(1024);
    if (!decoded_str) return NULL;
    
    int n = strlen(str);
    int i = 0;         
    int dest_idx = 0;  

    while (i < n) {    
        char ch = str[i];
        i++; 
        int count = 0;
        
        while (i < n && isdigit(str[i])) {    
            count = (count * 10) + (str[i] - '0');
            i++; 
        }
	
        if (count == 0) {
            count = 1;
        }
        memset(decoded_str + dest_idx, ch, count);
        dest_idx += count; 
    }

    decoded_str[dest_idx] = '\0'; 
    return decoded_str;
}

void handle_input_screen(const char *title, int is_encode)
{
  char input_buffer[512];
  clear();
  mvprintw(2, 4, "=== %s ===", title);
  mvprintw(4, 4, "type your string and press ENTER:");
  move(5,4);

  echo();
  curs_set(1);

  getnstr(input_buffer, 511);

  noecho();
  curs_set(0);

  char *res = NULL;
  if(is_encode) res = rle_encode(input_buffer);
  else res = rle_decode(input_buffer);

  mvprintw(7, 4, "Result: ");
  attron(A_BOLD);
  mvprintw(8, 4, "%s", res);
  attroff(A_BOLD);

  mvprintw(11, 4, "press any key to return to menu...");
  free(res);
  getch();
}

void handle_compress_screen(void)
{
  char input_path[512];
  char out_path[512];

 pick_bmp:
  input_path[0] = '\0';
  clear();
  mvprintw(2, 4, "COMPRESS IMAGE");
  mvprintw(4, 4, "Opening file picker...");
  refresh();
  endwin();

  int picked = _open_file_picker(input_path, sizeof(input_path), "bmp");
  //ncurses_file_browser(input_path, sizeof(input_path));
  if(picked != 0 || input_path[0] == '\0') return;

  clear();
  refresh();
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  /*
  if(picked != 0) {
    mvprintw(7, 4, "No file selected");
    mvprintw(9, 4, "Press any key to return");
    getch();
    return;
  }
  */
  mvprintw(6, 4, "Selected: %s", input_path);
  refresh();
  
  if(!_file_exists(input_path)) {
    mvprintw(7, 4, "Error: file does not exist.");
    mvprintw(8, 4, "Path : %s", input_path);
    mvprintw(10, 4, "press any key to return...");
    getch();
    goto pick_bmp;
  }

  if(!_is_valid_bmp(input_path)) {
    mvprintw(7, 4, "Error: not a valid BMP file.");
    mvprintw(8, 4, "Only uncompressed .bmp files are supported.");
    mvprintw(10, 4, "press any key to return...");
    getch();
    goto pick_bmp;
  }
  
  mvprintw(7, 4, "Processing... please wait.");
  refresh();

  int w,h,n;
  unsigned char *data = stbi_load(input_path, &w, &h, &n, 0);
  if(!data) {
    mvprintw(9, 4, "Error: could not open '%s'", input_path);
    mvprintw(11, 4, "press any key to return");
    getch();
    return;
  }

  int pixel_data_len = w * h * n;
  
  unsigned char *encoded = malloc(pixel_data_len * 2);
  int enc_length = rle_encode_binary(data, pixel_data_len, encoded, pixel_data_len * 2, n);
  
  if(enc_length >= pixel_data_len) {
    mvprintw(9, 4, "Warning: compressed file >= orginal");
    mvprintw(10, 4, "RLE is not effective");
    mvprintw(12, 4, "press any key to return");
    free(encoded);
    stbi_image_free(data);
    getch();
    return;
  }

  rle_make_filename(input_path, out_path, sizeof(out_path));
  int res = rle_write_file(out_path, w, h, n, encoded, enc_length);

  if(res != 0) {
    mvprintw(9, 4, "Error: could not write '%s'", out_path);
  } else {
    float ratio = (float)pixel_data_len / enc_length;
    float saved = 100.0f * (1.0f - (float)enc_length/pixel_data_len);

    mvprintw(7, 4, "                    ");
    mvprintw(7, 4, "Done!");
    mvprintw(9, 4, "Orginal : %d bytes\n",pixel_data_len);
    mvprintw(10,4, "Compressed : %d bytes\n",enc_length);
    mvprintw(11,4, "Ratio : %.2fx (%.1f%% saved)\n",ratio, saved);
    mvprintw(12, 4, "Output : %s", out_path);
  }

  mvprintw(14, 4, "press any key to return...");
  free(encoded);
  stbi_image_free(data);
  getch();
}

void handle_decompress_screen(void)
{
  char input_path[512];
  char out_path[512];

 pick_bmp:
  input_path[0] = '\0';
  clear();
  mvprintw(2, 4, "DECOMPRESS IMAGE");
  mvprintw(4, 4, "Opening file picker...");
  refresh();
  endwin();

  int picked = _open_file_picker(input_path, sizeof(input_path), "rle");
  //ncurses_file_browser(input_path, sizeof(input_path));
  if(picked != 0 ||  input_path[0] == '\0') return;

  clear();
  refresh();
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);

  /*
  if(picked != 0) {
    mvprintw(7, 4, "No file selected");
    mvprintw(9, 4, "Press any key to return");
    getch();
    return;
  }
  */
  
  mvprintw(6, 4, "Selected: %s", input_path);
  refresh();
 
  if (!_file_exists(input_path)) {
    mvprintw(7, 4, "Error: file does not exist.");
    mvprintw(8, 4, "Path : %s", input_path);
    mvprintw(10, 4, "press any key to return...");
    getch();
    goto pick_bmp;
  }

  if (!strstr(input_path, ".rle")) {
    mvprintw(7, 4, "Error: file does not have .rle extension.");
    mvprintw(8, 4, "Only files compressed by this tool are supported.");
    mvprintw(10, 4, "press any key to return...");
    getch();
    goto pick_bmp;
  }
  
  mvprintw(7, 4, "Processing... please wait.");
  refresh();

  RLEheader hdr;
  unsigned char *encoded = NULL;
  int enc_len = 0;
  int res = rle_read_file(input_path, &hdr, &encoded, &enc_len);
  
  if (res != 0) {
    mvprintw(9, 4, "couldn't read file %s", input_path);
    if(res == -2) mvprintw(10, 4, "not a vaild .rle file");
    if(res == -3) mvprintw(10, 4, "unsupported version");
    if(res == -4) mvprintw(10, 4, "out of memory");
    mvprintw(12, 4, "press any key to return");
    getch();
    return;
  }

  unsigned char *restored = malloc(hdr.org_filesize);
  int restored_len = rle_decode_binary(encoded, enc_len, restored, hdr.org_filesize, hdr.channels);
   if (restored_len != (int)hdr.org_filesize) {
    mvprintw(9,  4, "Error: decoded size mismatch!");
    mvprintw(10, 4, "Expected %d bytes, got %d bytes.", hdr.org_filesize, restored_len);
    mvprintw(11, 4, "File may be corrupt or truncated.");
    mvprintw(13, 4, "press any key to return...");
    free(encoded);
    free(restored);
    getch();
    return;
  }

  snprintf(out_path, sizeof(out_path), "%s", input_path);
  char *ext = strstr(out_path, ".bmp.rle");
  if(ext) *ext = '\0';
  strncat(out_path, "_restored.bmp",sizeof(out_path) - strlen(out_path) - 1);
  
  stbi_write_bmp(out_path, hdr.org_width, hdr.org_height, hdr.channels, restored);

  mvprintw(7	, 4, "                         ");
  mvprintw(7	, 4, "Done!");
  mvprintw(9	, 4, "Width :%d", hdr.org_width);
  mvprintw(10	, 4, "Height :%d", hdr.org_height);
  mvprintw(11	, 4, "Channels :%d", hdr.channels);
  mvprintw(12	, 4, "Output :%s", out_path);
  mvprintw(14	, 4, "press any key to return...");
  
  free(encoded);
  free(restored);
  getch();
}

int main(int argc, char **argv)
{
  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);

  if (has_colors()) {
    start_color();
    use_default_colors(); 
  }
  int item_colors[] = {
    COLOR_YELLOW,      
    COLOR_CYAN  ,       
    COLOR_GREEN ,       
    COLOR_MAGENTA,
    COLOR_RED ,
  };
  
  int choice = 0;
  const char *choices[] = {
    "Encode ",
    "Decode ",
    "Compress Image",
    "Decompress Image",
    "Exit"
  };
  int n_choice =(sizeof(choices)) / sizeof(char *);
  int highlight = 0;
  int c;

  for(int i = 0; i < n_choice; ++i) {
    init_pair(10 + i, item_colors[i], -1);
  }
  init_pair(1, COLOR_WHITE, -1);
  init_pair(4, COLOR_BLACK, COLOR_WHITE);
  
  while(1) {
    erase();

    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(2, 4, "RLE Utility");
    mvprintw(3, 4, "==============================");
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    for(int i = 0; i < n_choice; ++i) {
      if(i == highlight) {
	attron(COLOR_PAIR(4) | A_BOLD);
	mvprintw(5 + i, 6, "%s", choices[i]);
	attroff(COLOR_PAIR(4) | A_BOLD);
      } else {
	attron(COLOR_PAIR(10 + i) | A_BOLD);
	mvprintw(5 + i, 6, "%s", choices[i]);
	attroff(COLOR_PAIR(10 + i) | A_BOLD);
      }
    }

    refresh();
    
    c = getch();
    switch(c) {
      
    case KEY_UP:
      highlight--;
      if(highlight < 0) highlight = 0;
      break;
      
    case KEY_DOWN:
      highlight++;
      if(highlight >= n_choice) highlight = n_choice - 1;
      break;

    case 10:
      choice = highlight;
      if(choice == 0) handle_input_screen("Encode String", 1);
      else if(choice == 1) handle_input_screen("Decode String", 0);
      else if(choice == 2) handle_compress_screen();
      else if(choice == 3) handle_decompress_screen();
      else if(choice == 4) {
	endwin();
	return 0;
      }
      break;

    default:
      break;
    }
  }
  endwin();
  return 0;
}
