#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "../structs/fat32.h"
#include "../commands/cmd.h"

#define MAX_PATH_LEN 512
#define MAX_LFN_LEN  255

extern FAT32Partition *root;
extern char absolute_image_path[256];
extern char program_path[256];
extern char current_path[256];

bool file_exists(const char *path);
void lfn_extract_part(const LFNEntry *lfn, char *dest, size_t maxDestSize);
void str_to_upper(char *str);
void to_lowercase(char *str);
uint8_t calc_lfn_checksum(const char shortName[11]);
void format_short_name(const char *newname, char dest[11]);
int update_lfn_entries(uint8_t *buffer, int mainIndex, const char *newname); 
int is_directory_empty(FAT32Partition *partition, uint32_t cluster);
void recursive_delete_dir(FAT32Partition *partition, uint32_t cluster, const char *basePath);
void extract_sfn(DirectoryEntry entry, char *name);  
void extract_lfn(DirectoryEntry entry, char *lfnBuffer, int *lfnIndex);
int get_directory_entry(const char *path, DirectoryEntry *entry_out);

void normalize_path(char *path); 
char* get_filename(const char *path);
int is_directory(FAT32Partition *partition, const char *path);
int read_internal_file(const char *path, uint8_t **data, size_t *size);
int write_internal_file(const char *path, uint8_t *data, size_t size);
char *get_parent_directory(const char *path);
int create_directory(const char *path);
char *to_absolute_path(const char *path);

int find_directory_cluster(const char *path);
int find_file_cluster(const char *path);
int read_file_from_image(int startCluster, uint8_t **buffer, size_t *size);
int write_file_to_image(const char *path, uint8_t *data, size_t size);
void format_fat_date_time(uint16_t date, uint16_t time, char *out, size_t out_size);


#endif // UTILS_H