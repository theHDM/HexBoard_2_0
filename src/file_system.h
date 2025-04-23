#pragma once
#include "LittleFS.h"       // code to use flash drive space as a file system -- not implemented yet, as of May 2024

bool mount_file_system(bool format_if_corrupt) {
  if (LittleFS.begin()) return true;
  if (format_if_corrupt) {
    if (LittleFS.format()) return true;
  }  
  return false;
}
File load_from_file(const char* filename) {
  return LittleFS.open(filename, "r+");
}
File save_to_file(const char* filename) {
  return LittleFS.open(filename, "w+");
}



const char* settingFileName = "temp222.dat";
namespace Boot_Flags {
  bool fs_mounted = false;
  bool ini_exists = false;
  bool safe_mode = false;
}