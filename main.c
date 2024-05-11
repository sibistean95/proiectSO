#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <libgen.h>
#include <errno.h>

#define MAX_PATH_LENGTH 1024
#define MAX_ENTRIES 1000

typedef struct {
    char name[MAX_PATH_LENGTH];
    mode_t mode;
    off_t size;
    time_t mtime;
} EntryMetadata;

EntryMetadata getEntryMetadata(const char *path) {
    EntryMetadata metadata;
    struct stat st;

    if (lstat(path, &st) == 0) {
        strcpy(metadata.name, path);
        metadata.mode = st.st_mode;
        metadata.size = st.st_size;
        metadata.mtime = st.st_mtime;
    } else {
        perror("Error getting entry metadata");
        exit(EXIT_FAILURE);
    }

    return metadata;
}

void createSnapshot(const char *dirPath, const char *outputDir) {
    EntryMetadata entries[MAX_ENTRIES];
    int numEntries = 0;

    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char entryPath[MAX_PATH_LENGTH];
            snprintf(entryPath, sizeof(entryPath), "%s/%s", dirPath, entry->d_name);
            entries[numEntries++] = getEntryMetadata(entryPath);

            if (S_ISDIR(entries[numEntries-1].mode)) {
                createSnapshot(entryPath, outputDir);
            }
        }
    }

    closedir(dir);

    char *dirName = basename(strdup(dirPath));

    char snapshotFilePath[MAX_PATH_LENGTH];
    snprintf(snapshotFilePath, sizeof(snapshotFilePath), "%s/Snapshot_%s.txt", outputDir, dirName);

    FILE *snapshotFile = fopen(snapshotFilePath, "w");
    if (snapshotFile == NULL) {
        perror("Error creating snapshot file");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < numEntries; i++) {
        fprintf(snapshotFile, "Name: %s\n", entries[i].name);
        fprintf(snapshotFile, "Mode: %o\n", entries[i].mode);
        fprintf(snapshotFile, "Size: %ld bytes\n", entries[i].size);
        fprintf(snapshotFile, "Last modified: %s", ctime(&entries[i].mtime));
        fprintf(snapshotFile, "\n");
    }

    fclose(snapshotFile);

    printf("Snapshot created successfully for directory: %s\n", dirPath);
}

void analyzeFile(const char *filePath, const char *isolatedDir, int pipe_fd) {
    pid_t child_pid = fork();

    if (child_pid == 0) {
        close(pipe_fd);

        char scriptPath[MAX_PATH_LENGTH];
        snprintf(scriptPath, sizeof(scriptPath), "./verify_for_malicious.sh"); 

        dup2(pipe_fd, STDOUT_FILENO);

        execl(scriptPath, "verify_for_malicious.sh", filePath, NULL);

        if (errno == EACCES) {
            printf("Insufficient permissions to read or execute file: %s\n", filePath);
            exit(EXIT_SUCCESS);
        } else {
            perror("Error executing script");
            exit(EXIT_FAILURE);
        }
    } else if (child_pid < 0) {
        perror("Error forking");
        exit(EXIT_FAILURE);
    } else {
        close(pipe_fd);

        int status;
        waitpid(child_pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == EXIT_SUCCESS) {
        } else {
            printf("Analyzing file: %s\n", filePath);
            printf("Script execution failed for file: %s\n", filePath);
        }
    }
}

void moveFile(const char *filePath, const char *isolatedDir) {
    struct stat st;
    if (lstat(filePath, &st) == -1) {
        perror("Error getting file metadata");
        exit(EXIT_FAILURE);
    }

    if ((st.st_mode & S_IRWXU) == 0 && (st.st_mode & S_IRWXG) == 0 && (st.st_mode & S_IRWXO) == 0) {
        char *fileName = basename(strdup(filePath));
        char newFilePath[MAX_PATH_LENGTH];
        snprintf(newFilePath, sizeof(newFilePath), "%s/%s", isolatedDir, fileName);

        if (rename(filePath, newFilePath) != 0) { 
            perror("Error moving file");
            exit(EXIT_FAILURE);
        }

        printf("File %s moved to %s\n", filePath, newFilePath);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 5 || argc > 14) {
        printf("Usage: %s -o <output_directory> -s <isolated_space_dir> <directory1> <directory2> ... (up to 10 directories)\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *outputDir = NULL;
    char *isolatedDir = NULL;
    char *directories[10];
    int numDirectories = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                outputDir = argv[i + 1];
                i++;
            } else {
                printf("Error: Missing output directory path.\n");
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-s") == 0) {
            if (i + 1 < argc) {
                isolatedDir = argv[i + 1];
                i++;
            } else {
                printf("Error: Missing isolated space directory path.\n");
                return EXIT_FAILURE;
            }
        } else {
            if (numDirectories < 10) {
                directories[numDirectories++] = argv[i];
            } else {
                printf("Error: Too many directories provided (maximum 10).\n");
                return EXIT_FAILURE;
            }
        }
    }

    if (outputDir == NULL || isolatedDir == NULL) {
        printf("Error: Output directory or isolated space directory not provided.\n");
        return EXIT_FAILURE;
    }

    if (mkdir(isolatedDir, 0777) == -1 && errno != EEXIST) {
        perror("Error creating isolated space directory");
        return EXIT_FAILURE;
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Error creating pipe");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < numDirectories; i++) {
        createSnapshot(directories[i], outputDir);

        DIR *dir = opendir(directories[i]);
        if (dir == NULL) {
            perror("Error opening directory");
            return EXIT_FAILURE;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {
                char filePath[MAX_PATH_LENGTH];
                snprintf(filePath, sizeof(filePath), "%s/%s", directories[i], entry->d_name);
                analyzeFile(filePath, isolatedDir, pipe_fd[1]);
            }
        }

        closedir(dir);
    }

    close(pipe_fd[1]);

    char buffer[1024];
    ssize_t bytesRead;
    while ((bytesRead = read(pipe_fd[0], buffer, sizeof(buffer))) > 0) {
        write(STDOUT_FILENO, buffer, bytesRead);
    }

    close(pipe_fd[0]);

    for (int i = 0; i < numDirectories; i++) {
        DIR *dir = opendir(directories[i]);
        if (dir == NULL) {
            perror("Error opening directory");
            return EXIT_FAILURE;
        }

        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type == DT_REG) {
                char filePath[MAX_PATH_LENGTH];
                snprintf(filePath, sizeof(filePath), "%s/%s", directories[i], entry->d_name);
                moveFile(filePath, isolatedDir);
            }
        }

        closedir(dir);
    }

    return EXIT_SUCCESS;
}
