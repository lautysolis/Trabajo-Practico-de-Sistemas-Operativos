#include "paths.h"

char* path_file_dir(char* file){
    return string_from_format("%s/%s", fs_paths->path_files, file);
}
char* path_tag_dir(char* file, char* tag){
    return string_from_format("%s/%s/%s", fs_paths->path_files, file, tag);
}
char* path_tag_metadata(char* file, char* tag){
    return string_from_format("%s/%s/%s/metadata.config", fs_paths->path_files, file, tag);
}
char* path_tag_logical_dir(char* file, char* tag){
    return string_from_format("%s/%s/%s/logical_blocks", fs_paths->path_files, file, tag);
}
char* path_physical_block(int phys){
    return string_from_format("%s/block%04d.dat", fs_paths->path_physicalblocks, phys);
}
char* path_logical_block(char* file, char* tag, int logicalIdx){
    return string_from_format("%s/%s/%s/logical_blocks/%06d.dat", fs_paths->path_files, file, tag, logicalIdx);
}