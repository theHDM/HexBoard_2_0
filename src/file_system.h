#pragma once
#include "LittleFS.h"       // code to use flash drive space as a file system -- not implemented yet, as of May 2024
#include "debug.h"
bool fileSystemExists;
bool mount_file_system(bool format_if_corrupt) {
  if (LittleFS.begin()) return true;
  debug.add("LittleFS could not mount.\n");
  if (format_if_corrupt) {
    debug.add("Re-formatting storage space...\n");
    if (LittleFS.format()) return true;
    debug.add("Couldn't re-format -- check that you enabled a flash partition.\n");
  }  
  return false;
}
File load_from_file(const char* filename) {
  return LittleFS.open(filename, "r+");
}
File save_to_file(const char* filename) {
  return LittleFS.open(filename, "w+");
}