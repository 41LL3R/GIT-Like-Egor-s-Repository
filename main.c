#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repository.h"

void help_me_plz() {
    printf("Usage: mygit <command> [<args>]\n");
    printf("Available commands:\n");
    printf("  repo_init                          Initialize a new repository\n");
    printf("  repo_add <filename>                Add a file to the index\n");
    printf("  repo_remove <filename>             Mark a file for removal\n");
    printf("  repo_commit <message>              Create a new commit with a message\n");
    printf("  repo_status                        Show the current index\n");
    printf("  repo_log --n <number> <hash>       Show commit history (number and hash optional)\n");
    printf("  repo_diff <hash>                   Show difference between current and target commit\n");
    printf("  repo_checkout <hash> <filename>    Restore a file from a specific commit\n");
    return;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        help_me_plz();
        return 1;
    }

    const char* command = argv[1];

    if (strcmp(command, "repo_init") == 0) {
        return repo_init();
    }

    if (strcmp(command, "repo_add") == 0) {
        if (argc < 3) {
            printf("Error: 'repo_add' command requires a filename.\n");
            printf("Usage: ./mygit repo_add <filename>\n");
            return 1;
        }
        const char* filename = argv[2];
        return repo_add(filename);
    }
    if (strcmp(command, "repo_remove") == 0) {
        if (argc < 3) {
            printf("Error: 'repo_remove' command requires a filename.\n");
            printf("Usage: ./mygit repo_remove <filename>\n");
            return 1;
        }
        const char* filename = argv[2];
        return repo_remove(filename);
    }

    if (strcmp(command, "repo_commit") == 0) {
        if (argc < 3) {
            printf("Error: 'repo_commit' command requires a message.\n");
            printf("Usage: ./mygit repo_commit \"your message\"\n");
            return 1;
        }
        const char* message = argv[2];
        return repo_commit(message);
    }

    if (strcmp(command, "repo_status") == 0) {
        return repo_status();
    }

    if (strcmp(command, "repo_log") == 0) {
        int num_limit = -1;
        const char *start_hash = NULL;

        for (int i = 2; i < argc; i++) {
            if (strcmp(argv[i], "--n") == 0) {
                if (i + 1 < argc) {
                    num_limit = atoi(argv[i + 1]);
                    i++;
                } else {
                    printf("Error: Missing number after --n option.\n");
                    return 1;
                }
            } else {
                start_hash = argv[i];
            }
        }

        repo_log(num_limit, start_hash);
        return 0;
    }

    if (strcmp(command, "repo_diff") == 0) {
        if (argc < 3) {
            printf("Error: 'repo_diff' command requires a target commit hash.\n");
            printf("Usage: ./mygit repo_diff <hash>\n");
            return 1;
        }
        return repo_diff(argv[2]);
    }

    if (strcmp(command, "repo_checkout") == 0) {
        if (argc < 4) {
            printf("Error: 'repo_checkout' command requires both a commit hash and a filename.\n");
            printf("Usage: ./mygit repo_checkout <hash> <filename>\n");
            return 1;
        }
        return repo_checkout(argv[2], argv[3]);
    }

    printf("Error: Unknown command '%s'.\n\n", command);
    help_me_plz();
    return 1;
}