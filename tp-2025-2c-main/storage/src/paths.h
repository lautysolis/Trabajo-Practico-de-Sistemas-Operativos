#ifndef PATHS_H_
#define PATHS_H_

#include <commons/string.h>
typedef struct{
    char* path_superblock;
    char* path_bitmap;
    char* path_physicalblocks;
    char* path_blocks_hash_index;
    char* initialfile;
    char* path_files;
} t_paths_fs;

extern t_paths_fs* fs_paths;

char* path_file_dir(char* file);
char* path_tag_dir(char* file, char* tag);
char* path_tag_metadata(char* file, char* tag);
char* path_tag_logical_dir(char* file, char* tag);
char* path_physical_block(int phys);
char* path_logical_block(char* file, char* tag, int logicalIdx);

#endif