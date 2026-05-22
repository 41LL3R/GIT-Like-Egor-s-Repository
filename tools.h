#pragma once

#include <stdio.h>

int hash_function(const char* input, char* output_hash);

int copy_file(const char *from, const char *to);

int make_dir(const char *path);

int file_exists(const char *path);

void get_current_time(char *s, int len);

int find_file_in_commit(const char *commit_hash, const char *filename, char *out_hash);