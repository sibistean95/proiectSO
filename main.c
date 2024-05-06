#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

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
        }
    }

    closedir(dir);

    char snapshotFilePath[MAX_PATH_LENGTH];
    snprintf(snapshotFilePath, sizeof(snapshotFilePath), "%s/Snapshot.txt", outputDir);
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

    printf("Snapshot created successfully!\n");
}

void compareAndUpdateSnapshots(const char *oldSnapshotPath, const char *newSnapshotPath) {
    FILE *oldSnapshotFile = fopen(oldSnapshotPath, "r");
    FILE *newSnapshotFile = fopen(newSnapshotPath, "r");

    if (oldSnapshotFile == NULL || newSnapshotFile == NULL) {
        perror("Error opening snapshot files");
        exit(EXIT_FAILURE);
    }

    char oldLine[MAX_PATH_LENGTH];
    char newLine[MAX_PATH_LENGTH];
    while (fgets(oldLine, sizeof(oldLine), oldSnapshotFile) != NULL && fgets(newLine, sizeof(newLine), newSnapshotFile) != NULL) {
        if (strcmp(oldLine, newLine) != 0) {
            fclose(oldSnapshotFile);

            FILE *tempFile = fopen(oldSnapshotPath, "w");
            if (tempFile == NULL) {
                perror("Error creating temporary file");
                exit(EXIT_FAILURE);
            }

            rewind(newSnapshotFile);
            int c;
            while ((c = fgetc(newSnapshotFile)) != EOF) {
                fputc(c, tempFile);
            }

            fclose(tempFile);
            fclose(newSnapshotFile);

            printf("Snapshot updated successfully!\n");
            return;
        }
    }

    fclose(oldSnapshotFile);
    fclose(newSnapshotFile);

    printf("Snapshots are identical. No update needed.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 3 || argc > 12) {
        printf("Usage: %s -o <output_directory> <directory1> <directory2> ... (up to 10 directories)\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *outputDir = NULL;
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
        } else {
            directories[numDirectories++] = argv[i];
        }
    }

    if (outputDir == NULL) {
        printf("Error: Missing output directory.\n");
        return EXIT_FAILURE;
    }

    for(int i = 0; i < numDirectories; i++) {
        pid_t child_pid = fork();   // creare proces copil nou
        if(child_pid == 0) {
            createSnapshot(directories[i], outputDir);
            exit(EXIT_SUCCESS);     // terminarea procesului copil cu succes
        } else if(child_pid < 0) {      // cazul de eroare al apelului sistem fork
            perror("Error forking!");
            exit(EXIT_FAILURE);
        }
    }

    // procesul parinte asteapta ca toate procesele copil sa sa termine
    int status;
    pid_t wpid;
    while((wpid = wait(&status)) > 0) {
        if(WIFEXITED(status)) {
            printf("Child Process %d terminated with PID %d and exit code %d.\n", (int)wpid, (int)wpid, WEXITSTATUS(status));
        } else {
            printf("Child Process %d terminated abnormally.\n", (int)wpid);
        }
    }

    return EXIT_SUCCESS;
}
