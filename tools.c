#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include "tools.h"
#include "repository.h"

// Подключение хедеров для создания директорий
#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
#else
    #include <sys/stat.h>
    #include <sys/types.h>
    #include <unistd.h>
#endif

int hash_function(const char* input, char* output_hash) { // Хеш-функция Дженкинса
    FILE *f = fopen(input, "rb");
    if (!f) return -1;

    uint32_t hash = 0;
    uint8_t buffer[4096];
    int bytes;

    while ((bytes = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        for (int i = 0; i < bytes; i++) {
            hash += buffer[i];
            hash += hash << 10;
            hash ^= hash >> 6;
        }
    }
    
    hash += hash << 3;
    hash ^= hash >> 11;
    hash += hash << 15;

    fclose(f);
    
    sprintf(output_hash, "%08x", hash);
    return 0;
}

int copy_file(const char *from, const char *to) { // копирование файлов
    FILE *source = fopen(from, "rb");
    if (!source) return -1;

    FILE *destination = fopen(to, "wb");
    if(!destination) {
        fclose(source);
        return -1;
    }

    uint8_t buffer[4096];
    int bytes;

    while((bytes = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes, destination);
    }

    fclose(source);
    fclose(destination);

    return 0;
}

// создание директорий
int make_dir(const char *path) {
    #ifdef _WIN32
        return _mkdir(path);
    #else
        return mkdir(path, 0755); // На линуксе нужно дополнительно указывать права доступа 
                                  // 7 - открытие, запись, чтение. 5 - открытие, чтение
    #endif
}

int file_exists(const char *path) {
    if (!path) return 0;
    return (access(path, F_OK) == 0); // проверка существования файла или директории
}

void get_current_time(char *s, int len) {
    time_t t = time(NULL);
    struct tm *tim = localtime(&t);
    strftime(s, len, "%Y-%m-%d %H:%M:%S", tim);
}

int find_file_in_commit(const char *commit_hash, const char *filename, char *out_hash) { // поиск файла в коммите
    char path[512];
    sprintf(path, ".mygit/commits/%s", commit_hash);
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    commit_data to_skip;
    fread(&to_skip, sizeof(commit_data), 1, f);

    char line[512];
    char f_name[256], f_hash[9];
    int is_removed;
    int found = -1;

    while (fgets(line, sizeof(line), f)) {
        if (sscanf(line, "%255s %8s %d", f_name, f_hash, &is_removed) == 3) {
            if (strcmp(f_name, filename) == 0) {
                strncpy(out_hash, f_hash, 9);
                found = is_removed;
                break;
            }
        }
    }
    fclose(f);
    return found; // 0 - ок, 1 - удален, -1 - не найден
}