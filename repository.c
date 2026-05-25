#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repository.h"
#include <stdint.h>
#include <sys/stat.h>
#include <dirent.h>

int repo_init(void) { // инициализация репозитория
    if (file_exists(".mygit")) {
        printf("Repository already exists.");
        return 1;
    }

    make_dir(".mygit");
    make_dir(".mygit/objects");
    make_dir(".mygit/commits");

    FILE *f = fopen(".mygit/index", "w");
    fclose(f);

    printf("Initialized empty repository in .mygit\n");

    repo_commit("Initialization commit");
    return 0;
}

int repo_add(const char *path) { // добавляем файл или папку в следущий коммит

    struct stat path_stat;

    if (stat(path, &path_stat) != 0) {
        printf("Error: File or directory %s does not exist.\n", path);
        return -1;
    }

    // Если добавляем папку
    if (S_ISDIR(path_stat.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) {
            printf("Error: Could not open directory %s\n", path);
            return -1;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }

            char full_path[1024];
            sprintf(full_path, "%s/%s", path, entry->d_name);

            repo_add(full_path);
        }
        closedir(dir);
        return 0;
    }

    // Если добавляем файл
    char hash[9];
    if (hash_function(path, hash) != 0) return -1;

    char obj_path[512];
    sprintf(obj_path, "%s/%s", ".mygit/objects", hash);
    if (!file_exists(obj_path)) {
        copy_file(path, obj_path);
    }

    FILE *old_index = fopen(".mygit/index", "r");
    FILE *new_index = fopen(".mygit/index.tmp", "w");
    if (!new_index) {
        if (old_index) fclose(old_index);
        return -1;
    }

    int is_updated = 0; 

    if (old_index) {
        char line[512];
        char idx_name[256], idx_hash[9];
        int idx_removed;

        while (fgets(line, sizeof(line), old_index)) {
            if (sscanf(line, "%255s %8s %d", idx_name, idx_hash, &idx_removed) == 3) {
                
                if (strcmp(idx_name, path) == 0) {
                    fprintf(new_index, "%s %s 0\n", path, hash);
                    is_updated = 1;
                } 
                else {
                    fprintf(new_index, "%s %s %d\n", idx_name, idx_hash, idx_removed);
                }
            }
        }
        fclose(old_index);
    }

    if (!is_updated) {
        fprintf(new_index, "%s %s 0\n", path, hash);
    }

    fclose(new_index);

    remove(".mygit/index");
    if (rename(".mygit/index.tmp", ".mygit/index") != 0) {
        printf("Error: Failed to update index for %s\n", path);
        return -1;
    }

    printf("File %s added to index.\n", path);
    return 0;
}

int repo_remove(const char *filename) {
    struct stat path_stat;

    // Папка или файл существуют на диске
    if (stat(filename, &path_stat) == 0) {
        if (S_ISDIR(path_stat.st_mode)) {
            DIR *dir = opendir(filename);
            if (!dir) return -1;

            struct dirent *entry;
            while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }

                char full_path[1024];
                sprintf(full_path, "%s/%s", filename, entry->d_name);
                
                // Рекурсивно вызываем удаление
                repo_remove(full_path);
            }
            closedir(dir);
            return 0; 
        }
    } 
    // Папку уже удалили с диска
    else {
        char folder_prefix[512];
        sprintf(folder_prefix, "%s/", filename);
        size_t prefix_len = strlen(folder_prefix);

        FILE *f_idx = fopen(".mygit/index", "r");
        if (f_idx) {
            char idx_name[256], idx_hash[9];
            int idx_removed;
            int found_any_subfile = 0;

            char paths_to_remove[100][256];
            int count = 0;

            while (fscanf(f_idx, "%255s %8s %d", idx_name, idx_hash, &idx_removed) == 3) {
                if (strncmp(idx_name, folder_prefix, prefix_len) == 0 && count < 100) {
                    strcpy(paths_to_remove[count], idx_name);
                    count++;
                    found_any_subfile = 1;
                }
            }
            fclose(f_idx);
            
            for (int i = 0; i < count; i++) {
                repo_remove(paths_to_remove[i]);
            }

            if (found_any_subfile) {
                return 0;
            }
        }
    }

    // Удаляем файл
    FILE *f_read = fopen(".mygit/index", "r");
    if (f_read) {
        char name[256], hash[9];
        int removed;
        while (fscanf(f_read, "%255s %8s %d", name, hash, &removed) == 3) {
            if (strcmp(name, filename) == 0 && removed == 1) {
                fclose(f_read);
                return 0; 
            }
        }
        fclose(f_read);
    }

    FILE *old_index = fopen(".mygit/index", "r");
    FILE *new_index = fopen(".mygit/index.tmp", "w");
    if (!new_index) {
        if (old_index) fclose(old_index);
        return -1;
    }

    int was_in_index = 0;
    if (old_index) {
        char line[512];
        char idx_name[256], idx_hash[9];
        int idx_removed;

        while (fgets(line, sizeof(line), old_index)) {
            if (sscanf(line, "%255s %8s %d", idx_name, idx_hash, &idx_removed) == 3) {
                if (strcmp(idx_name, filename) == 0 && idx_removed == 0) {
                    was_in_index = 1;
                    continue;
                }
                fprintf(new_index, "%s %s %d\n", idx_name, idx_hash, idx_removed);
            }
        }
        fclose(old_index);
    }

    if (!was_in_index) {
        struct stat s;
        int is_dir = (stat(filename, &s) == 0 && S_ISDIR(s.st_mode));
        if (!is_dir) {
            fprintf(new_index, "%s 00000000 1\n", filename);
            printf("File %s marked to be removed in next commit.\n", filename);
        }
    } else {
        printf("File %s removed from index (add canceled).\n", filename);
    }

    fclose(new_index);
    
    remove(".mygit/index");
    if (rename(".mygit/index.tmp", ".mygit/index") != 0) {
        printf("Error: index update failed for %s\n", filename);
        return -1;
    }

    return 0;
}

int repo_commit(const char *message) { // создание коммита
    commit_data new_commit;
    memset(&new_commit, 0, sizeof(commit_data));

    strcpy(new_commit.parent_hash, "none");
    FILE *cur_file = fopen(".mygit/CURRENT", "r");
    if (cur_file) {
        fscanf(cur_file, "%8s", new_commit.parent_hash);
        fclose(cur_file);
    }

    get_current_time(new_commit.time, sizeof(new_commit.time));
    strncpy(new_commit.message, message, sizeof(new_commit.message) - 1);

    uint32_t h = 0;
    for (int i = 0; new_commit.message[i]; i++) {
        h += new_commit.message[i];
        h += h << 10;
        h ^= h >> 6;
    }
    for (int i = 0; new_commit.time[i]; i++) {
        h += new_commit.time[i];
        h += h << 10;
        h ^= h >> 6;
    }
    h += h << 3;
    h ^= h >> 11;
    h += h << 15;
    sprintf(new_commit.hash, "%08x", h);

    char commit_path[512];
    sprintf(commit_path, ".mygit/commits/%s", new_commit.hash);
    
    FILE *commit_file = fopen(commit_path, "wb");
    if (!commit_file) return -1;

    fwrite(&new_commit, sizeof(commit_data), 1, commit_file);

    // Если есть родительский коммит — переносим файлы из него
    if (strcmp(new_commit.parent_hash, "none") != 0) {
        char parent_path[512];
        sprintf(parent_path, ".mygit/commits/%s", new_commit.parent_hash);

        FILE *parent_file = fopen(parent_path, "rb");
        if (parent_file) {
            commit_data to_skip;
            fread(&to_skip, sizeof(commit_data), 1, parent_file);

            char parent_line[512];
            char parent_name[256], parent_hash[9];
            int parent_removed;

            while (fgets(parent_line, sizeof(parent_line), parent_file)) {
                if (sscanf(parent_line, "%255s %8s %d", parent_name, parent_hash, &parent_removed) == 3) {
                    if (parent_removed) continue; 

                    FILE *idx_check = fopen(".mygit/index", "r");
                    int changed_in_index = 0;
                    if (idx_check) {
                        char idx_name[256], idx_hash[9];
                        int idx_removed;
                        while (fscanf(idx_check, "%255s %8s %d", idx_name, idx_hash, &idx_removed) == 3) {
                            if (strcmp(parent_name, idx_name) == 0) {
                                changed_in_index = 1;
                                break;
                            }

                            char dir_prefix[512];
                            sprintf(dir_prefix, "%s/", idx_name);
                            if (strncmp(parent_name, dir_prefix, strlen(dir_prefix)) == 0) {
                                changed_in_index = 1;
                                break;
                            }
                        }
                        fclose(idx_check);
                    }

                    if (!changed_in_index) {
                        fprintf(commit_file, "%s %s 0\n", parent_name, parent_hash);
                    }
                } 
            }
            fclose(parent_file);
        }
    }

    // Дописываем новые файлы из индекса
    FILE *idx = fopen(".mygit/index", "r");
    if (idx) {
        char i_name[256], i_hash[9];
        int i_removed;
        while (fscanf(idx, "%255s %8s %d", i_name, i_hash, &i_removed) == 3) {
            if (i_removed == 0) {
                fprintf(commit_file, "%s %s 0\n", i_name, i_hash);
            }
        }
        fclose(idx);
    }
    fclose(commit_file);

    FILE *cur = fopen(".mygit/CURRENT", "w");
    if (cur) {
        fprintf(cur, "%s", new_commit.hash);
        fclose(cur);
    }

    // Очищаем индекс
    FILE *clear_idx = fopen(".mygit/index", "w");
    if (clear_idx) fclose(clear_idx);

    printf("Commit [%s] created successfully.\n", new_commit.hash);
    return 0;
}

int repo_status(void) { // просмотр содержимого индекса
    FILE *f = fopen(".mygit/index", "r");
    if (!f) {
        printf("Index empty. Nothing to commit.\n");
        return 0;
    }

    char cur_hash[9];
    int has_cur = 0;
    FILE *h_file = fopen(".mygit/CURRENT", "r");
    if (h_file) {
        if (fscanf(h_file, "%8s", cur_hash) == 1) {
            has_cur = 1;
        }
        fclose(h_file);
    }

    char filename[256];
    char hash[9];
    int removed;
    int has_changes = 0;

    while (fscanf(f, "%255s %8s %d", filename, hash, &removed) == 3) {
        if (!has_changes) {
            printf("Changes to be committed:\n");
            has_changes = 1;
        }
        
        if (removed) {
            printf("  (deleted)   %s\n", filename);
        } 
        else if (has_cur) {
            char old_hash[9];
            int res = find_file_in_commit(cur_hash, filename, old_hash);
            
            if (res == -1 || res == 1) {
                printf("  (created)   %s\n", filename);
            } 
            else if (strcmp(hash, old_hash) != 0) {
                printf("  (modified)  %s\n", filename);
            } 
            else {
                printf("  (unchanged) %s\n", filename);
            }
        } 
        else {
            printf("  (created)   %s\n", filename);
        }
    }
    
    fclose(f);

    if (!has_changes) {
        printf("Index empty. Nothing to commit.\n");
    }

    return 0;
}

void repo_log(int n, const char *start_commit_hash) { // просмотр лога
    char current_hash[9];

    if (start_commit_hash == NULL) {
        FILE *h = fopen(".mygit/CURRENT", "r");
        if (!h) {
            printf("No commits yet.\n");
            return;
        }

        fscanf(h, "%8s", current_hash);
        fclose(h);
    }
    else {
        strncpy(current_hash, start_commit_hash, 9);
    }

    int count = 0;
    while (strcmp(current_hash, "none") != 0) {
        if (n != -1 && count >= n) break;

        char path[512];
        sprintf(path, ".mygit/commits/%s", current_hash);
        FILE *commit_file = fopen(path, "rb");
        if (!commit_file) break;

        commit_data data;

        if (fread(&data, sizeof(commit_data), 1, commit_file)) {
            printf("--------------------------------\n");
            printf("Commit:  %s\n", data.hash);
            printf("Time:    %s\n", data.time);
            printf("Message: %s\n", data.message);
            strcpy(current_hash, data.parent_hash);
        }
        else {
            fclose(commit_file);
            break;
        }
        fclose(commit_file);
        count++;
    }
}

int repo_diff(const char *target_hash) { // вывод разницы между текущим и выбранным коммитом
    char cur_hash[9];
    FILE *cur_file = fopen(".mygit/CURRENT", "r");
    if (!cur_file) {
        printf("Error: No commits in CURRENT yet.\n");
        return -1;
    }
    fscanf(cur_file, "%8s", cur_hash);
    fclose(cur_file);

    if (strcmp(cur_hash, target_hash) == 0) {
        printf("No changes. Target commit is the same as CURRENT.\n");
        return 0;
    }

    char target_path[512];
    sprintf(target_path, ".mygit/commits/%s", target_hash);
    FILE *target_file = fopen(target_path, "rb");
    if (!target_file) {
        printf("Error: Target commit %s not found.\n", target_hash);
        return -1;
    }

    commit_data to_skip;
    fread(&to_skip, sizeof(commit_data), 1, target_file);

    printf("Comparing commit [%s], current commit [%s]:\n", target_hash, cur_hash);
    printf("%-20s %-12s %-12s  %s\n", "File Name", "Target Hash", "Current Hash", "Status");
    printf("-----------------------------------------------------------------\n");

    char line[512];
    char file_name[256], t_hash[9];
    int t_removed;
    int has_changes = 0;

    // Ищем измененные и удаленные файлы из целевого коммита
    while (fgets(line, sizeof(line), target_file)) {
        if (sscanf(line, "%s %s %d", file_name, t_hash, &t_removed) == 3) {
            if (t_removed) continue;

            char h_hash[9];
            int h_removed = find_file_in_commit(cur_hash, file_name, h_hash);

            if (h_removed == -1 || h_removed == 1) {
                printf("%-20s %-12s %-12s  %s\n", file_name, t_hash, "--------", "removed");
                has_changes = 1;
            } 
            else if (strcmp(t_hash, h_hash) != 0) {
                printf("%-20s %-12s %-12s  %s\n", file_name, t_hash, h_hash, "modified");
                has_changes = 1;
            }
        }
    }
    fclose(target_file);

    // Ищем новые файлы в текущем коммите
    char cur_path[512];
    sprintf(cur_path, ".mygit/commits/%s", cur_hash);
    FILE *cur_file_scan = fopen(cur_path, "rb");
    if (cur_file_scan) {
        fread(&to_skip, sizeof(commit_data), 1, cur_file_scan);
        char c_name[256], c_hash[9];
        int c_removed;

        while (fgets(line, sizeof(line), cur_file_scan)) {
            if (sscanf(line, "%s %s %d", c_name, c_hash, &c_removed) == 3) {
                if (c_removed) continue;

                char temp_hash[9];

                int res = find_file_in_commit(target_hash, c_name, temp_hash);
                if (res == -1 || res == 1) {
                    printf("%-20s %-12s %-12s  %s\n", c_name, "--------", c_hash, "was added");
                    has_changes = 1;
                }
            }
        }
        fclose(cur_file_scan);
    }

    if (!has_changes) {
        printf("Commits are absolutely identical.\n");
    }

    return 0;
}

int repo_checkout(const char *hash, const char *filename) { // Восстановление выбранного файла или папки
    char commit_path[512];
    sprintf(commit_path, ".mygit/commits/%s", hash);

    FILE *commit_file = fopen(commit_path, "rb");
    if (!commit_file) {
        printf("Error: Commit %s not found.\n", hash);
        return -1;
    }

    commit_data to_skip;
    fread(&to_skip, sizeof(commit_data), 1, commit_file);

    char line[512];
    char f_name[256], f_hash[9];
    int is_removed;
    int found_any = 0;

    char dir_prefix[512];
    sprintf(dir_prefix, "%s/", filename);
    size_t prefix_len = strlen(dir_prefix);

    while (fgets(line, sizeof(line), commit_file)) {
        if (sscanf(line, "%255s %8s %d", f_name, f_hash, &is_removed) == 3) {

            if (strcmp(f_name, filename) == 0 || strncmp(f_name, dir_prefix, prefix_len) == 0) {
                
                if (is_removed) {
                    if (strcmp(f_name, filename) == 0) {
                        printf("Error: File %s was deleted in commit %s.\n", filename, hash);
                        fclose(commit_file);
                        return -1;
                    }
                    continue;
                }

                found_any = 1;

                // Создаем папки для файла, если их стерли с диска
                create_parent_dirs(f_name);

                char obj_path[512];
                sprintf(obj_path, ".mygit/objects/%s", f_hash);
                
                if (copy_file(obj_path, f_name) == 0) {
                    printf("Restored: %s from commit %s\n", f_name, hash);
                }
                else {
                    printf("Error: Could not restore %s (object %s missing)\n", f_name, f_hash);
                }
            }
        }
    }

    if (!found_any) {
        printf("Error: File or directory %s not found in commit %s.\n", filename, hash);
    }

    fclose(commit_file);
    return found_any ? 0 : -1;
}