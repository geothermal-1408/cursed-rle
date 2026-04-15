#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>

#include "rle_format.h"
#include "common.h"
#include "rle_simd.h"

int _file_exists(const char *path)
{
  struct stat ft;
  return (stat(path, &ft) == 0);
}

int _file_is_writable(const char *path)
{
  FILE *f = fopen(path, "rb");
  if (!f)
    return 0;
  fclose(f);
  return 1;
}

int _is_valid_bmp(const char *path)
{
  FILE *f = fopen(path, "rb");
  if (!f)
    return 0;
  unsigned char magic[2];
  fread(magic, 1, 2, f);
  fclose(f);

  return (magic[0] == 'B' && magic[1] == 'M');
}

static int join_path(char *dst, size_t dst_size, const char *base, const char *name)
{
  size_t base_len = strlen(base);
  size_t name_len = strlen(name);
  size_t need = base_len + 1 + name_len + 1;

  if (need > dst_size)
    return -1;

  memcpy(dst, base, base_len);
  dst[base_len] = '/';
  memcpy(dst + base_len + 1, name, name_len + 1);
  return 0;
}

void _ncurses_file_browser(char *out_path, int out_size)
{
  char cwd[512];
  getcwd(cwd, sizeof(cwd));
  char entries[256][512];
  int n_entries = 0;
  int highlight = 0;

  while (1)
  {
    n_entries = 0;
    DIR *dir = opendir(cwd);
    struct dirent *dp;

    snprintf(entries[n_entries++], 512, "..");
    while ((dp = readdir(dir)) != NULL && n_entries < 256)
    {
      if (dp->d_name[0] == '.')
        continue;
      snprintf(entries[n_entries++], 512, "%s", dp->d_name);
    }
    closedir(dir);

    clear();
    mvprintw(1, 2, "FILE BROWSER");
    mvprintw(2, 2, "Locations: %s", cwd);
    mvprintw(3, 2, "====================");
    mvprintw(3 + n_entries + 2, 2, "ENTER=select q=cancel");

    for (int i = 0; i < n_entries; ++i)
    {
      char full[1024];
      if (join_path(full, sizeof(full), cwd, entries[i]) != 0)
      {
        continue;
      }

      struct stat st;
      stat(full, &st);
      int is_dir = S_ISDIR(st.st_mode);

      if (i == highlight)
        attron(COLOR_PAIR(4) | A_BOLD);
      else
        attron(A_NORMAL);

      mvprintw(4 + i, 4, "%s%s", entries[i], is_dir ? "/" : "");
      attroff(COLOR_PAIR(4) | A_BOLD);
    }
    refresh();
    int c = getch();
    switch (c)
    {
    case KEY_UP:
      highlight--;
      if (highlight < 0)
        highlight = 0;
      break;
    case KEY_DOWN:
      highlight++;
      if (highlight >= n_entries)
        highlight = n_entries - 1;
      break;
    case 'q':
      out_path[0] = '\0';
      return;
      break;
    case 10:
    {
      char selected[1024];
      if (join_path(selected, sizeof(selected), cwd, entries[highlight]) != 0)
      {
        break;
      }
      struct stat st;
      stat(selected, &st);
      if (S_ISDIR(st.st_mode))
      {
        snprintf(cwd, sizeof(cwd), "%s", selected);
        highlight = 0;
      }
      else
      {
        snprintf(out_path, out_size, "%s", selected);
        return;
      }
      break;
    }
    }
  }
}

/* right now file picker has been implemeted using AppleScript.
   So, it's only supports macOS. In future, need to add custom ncurses
   based file browser.
*/
int _open_file_picker(char *out_path, int out_size, const char *file_type)
{
#if defined(__APPLE__) && defined(__MACH__)

  char script[512];
  snprintf(script, sizeof(script),
           "osascript -e 'POSIX path of (choose file of type {\"%s\"} "
           "with prompt \"Select a %s file\")' 2>/dev/null",
           file_type, file_type);

  FILE *fp = popen(script, "r");
  if (!fp)
    return -1;

  if (fgets(out_path, out_size, fp) == NULL)
  {
    pclose(fp);
    return -1;
  }

  out_path[strcspn(out_path, "\n")] = '\0';
  pclose(fp);
  return 0;

#elif defined(__linux__)

  char script[512];
  snprintf(script, sizeof(script),
           "zenity --file-selection --file-filter = '*.%s' 2>/dev/null "
           "|| kdialog --getopenfilename . '*.%s' 2>/dev/null",
           file_type, file_type);

  FILE *fp = popen(script, "r");
  if (fp)
  {
    if (fgets(out_path, out_size, fp) != NULL)
    {
      out_path[strcspn(out_path, "\n")] = '\0';
      pclose(fp);
      return 0;
    }
    pclose(fp);
  }

  _ncurses_file_browser(out_path, out_size);
  if (out_path[0] == '\0')
    return -1;
  return 0;

#else
  _ncurses_file_browser(out_path, out_size);
  if (out_path[0] == '\0')
    return -1;
  return 0;

#endif
}

int rle_write_file(const char *output,
                   int width, int height, int channels,
                   const unsigned char *enc_data,
                   int enc_len)
{
  FILE *f = fopen(output, "wb");

  if (!f)
    return -1;

  RLEheader hdr;
  hdr.magic[0] = RLE_MAGIC0;
  hdr.magic[1] = RLE_MAGIC1;
  hdr.magic[2] = RLE_MAGIC2;
  hdr.version = RLE_VERSION;
  hdr.org_width = (uint32_t)width;
  hdr.org_height = (uint32_t)height;
  hdr.channels = (uint8_t)channels;
  hdr.org_filesize = (uint32_t)(width * height * channels);

  fwrite(&hdr, sizeof(RLEheader), 1, f);
  fwrite(enc_data, 1, enc_len, f);

  fclose(f);
  return 0;
}

int rle_read_file(const char *input,
                  RLEheader *hdr_out,
                  unsigned char **data_out,
                  int *data_len_out)
{
  FILE *f = fopen(input, "rb");
  if (!f)
    return -1;

  fread(hdr_out, sizeof(RLEheader), 1, f);

  if (hdr_out->magic[0] != RLE_MAGIC0 ||
      hdr_out->magic[1] != RLE_MAGIC1 ||
      hdr_out->magic[2] != RLE_MAGIC2)
  {
    printf("ERROR: not a vaild .rle file\n");
    fclose(f);
    return -2;
  }

  if (hdr_out->version != RLE_VERSION)
  {
    printf("ERROR: unsupported version\n");
    fclose(f);
    return -3;
  }

  fseek(f, 0, SEEK_END);
  long file_size = ftell(f);
  long data_size = file_size - sizeof(RLEheader);
  fseek(f, sizeof(RLEheader), SEEK_SET);

  *data_out = malloc(data_size);
  if (!*data_out)
  {
    fclose(f);
    return -4;
  }

  fread(*data_out, 1, data_size, f);
  *data_len_out = (int)data_size;

  fclose(f);
  return 0;
}

void rle_make_filename(const char *input, char *output, int out_size)
{
  snprintf(output, out_size, "%s.rle", input);
}

int rle_encode_binary(const unsigned char *input,
                      int input_len,
                      unsigned char *output,
                      int output_max,
                      int channels)
{
  int i = 0;
  int offset = 0;
  int pixel_count = input_len / channels;

  while (i < pixel_count)
  {
    size_t pixeli = (size_t)i * (size_t)channels;
    int count = rle_count_run_simd(&input[pixeli], pixel_count - i, 255, channels);

    if (offset + 1 + channels > output_max)
      return -1;

    output[offset++] = (unsigned char)count;
    memcpy(&output[offset], &input[(size_t)i * (size_t)channels], channels);
    offset += channels;
    i += count;
  }
  return offset;
}

int rle_decode_binary(const unsigned char *input,
                      int input_len,
                      unsigned char *output,
                      int output_max,
                      int channels)
{
  int i = 0;
  int offset = 0;

  while (i + 1 < input_len)
  {
    int count = (int)input[i++];

    if (i + channels > input_len)
      return -1;

    if (offset + count * channels > output_max)
      return -1;

    rle_fill_pixel_simd(&output[offset], &input[i], count, channels);
    
    offset += count * channels;
    i += channels;
  }
  
  return offset;
}
