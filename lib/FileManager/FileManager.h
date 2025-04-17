#ifndef FILEMANAGER_H
#define FILEMANAGER_H

#include <stddef.h> // For size_t

#ifdef __cplusplus
extern "C" {
#endif

void file_manager_init();
void file_manager_deinit();
int file_manager_write_data(const char* app, const char* filename, const char* data);
int file_manager_append_data(const char* app, const char* filename, const char* data);
char* file_manager_read_data_root(const char* path);
char* file_manager_read_data(const char* app, const char* filename);
void file_manager_get_files_in_directory(const char* app, char*** files, int* count);

#ifdef __cplusplus
}
#endif

#endif // FILEMANAGER_H
