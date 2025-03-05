#ifndef CONFIG_H
#define CONFIG_H

typedef struct {
  int screen_width;
  int screen_height;
  int use_bloom;
  int trilinear_filter;
  int disable_mipmaps;
  int language;
  int crouch_toggle;
  int character_shadows;
  int drop_highest_lod;
  int show_weapon_menu;
  float decal_limit;
  float debris_limit;
  char mod_file[256];
} Config;

extern Config config;

int read_config(const char *file);
int write_config(const char *file);

#endif // CONFIG_H