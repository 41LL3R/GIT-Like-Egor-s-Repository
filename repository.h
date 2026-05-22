#pragma once

#include "tools.h"

typedef struct {
    char hash[9];
    char parent_hash[9];
    char message[256];
    char time[20];
} commit_data;

int repo_init(void);

int repo_add(const char *filename);
int repo_remove(const char *filename);

int repo_commit(const char *message);

int repo_status(void);

void repo_log(int n, const char *start_commit_hash);

int repo_diff(const char *target_hash);

int repo_checkout(const char *hash, const char *filename);