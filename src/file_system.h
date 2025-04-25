#pragma once
#include "LittleFS.h"       // code to use flash drive space as a file system -- not implemented yet, as of May 2024

bool mount_file_system(bool format_if_corrupt) {
  if (LittleFS.begin()) return true;
  if (format_if_corrupt) {
    if (LittleFS.format()) return true;
  }  
  return false;
}
File open_file_at_path(const char* filename) {
  return LittleFS.open(filename, "r+");
}
File new_file_at_path(const char* filename) {
  return LittleFS.open(filename, "w+");
}

namespace Boot_Flags {
  bool fs_mounted = false;
  bool calibrate_mode = false;
  bool safe_mode = false;
}