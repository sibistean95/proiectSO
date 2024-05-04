#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>

#define MAX_PATH_LENGTH 1024
#define MAX_ENTRIES 1000

// Structura pentru a stoca metadatele fiecărei intrări din director
typedef struct {
    char name[MAX_PATH_LENGTH];
    mode_t mode;
    off_t size;
    time_t mtime; // Timpul ultimei modificări
} EntryMetadata;

// Funcția pentru a obține metadatele unei intrări din director folosind lstat()
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

// Funcția pentru a crea un snapshot al directorului specificat
void createSnapshot(const char *dirPath) {
    EntryMetadata entries[MAX_ENTRIES];
    int numEntries = 0;

    // Deschide directorul
    DIR *dir = opendir(dirPath);
    if (dir == NULL) {
        perror("Error opening directory");
        exit(EXIT_FAILURE);
    }

    // Parcurge fiecare intrare din director
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            char entryPath[MAX_PATH_LENGTH];
            snprintf(entryPath, sizeof(entryPath), "%s/%s", dirPath, entry->d_name);
            entries[numEntries++] = getEntryMetadata(entryPath);
        }
    }

    // Închide directorul
    closedir(dir);

    // Creează fișierul de snapshot
    FILE *snapshotFile = fopen("Snapshot.txt", "w");
    if (snapshotFile == NULL) {
        perror("Error creating snapshot file");
        exit(EXIT_FAILURE);
    }

    // Scrie metadatele în fișierul de snapshot
    for (int i = 0; i < numEntries; i++) {
        fprintf(snapshotFile, "Name: %s\n", entries[i].name);
        fprintf(snapshotFile, "Mode: %o\n", entries[i].mode);
        fprintf(snapshotFile, "Size: %ld bytes\n", entries[i].size);
        fprintf(snapshotFile, "Last modified: %s", ctime(&entries[i].mtime)); // Convertește timpul într-un șir de caractere
        fprintf(snapshotFile, "\n");
    }

    // Închide fișierul de snapshot
    fclose(snapshotFile);

    printf("Snapshot created successfully!\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <directory_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *dirPath = argv[1];

    createSnapshot(dirPath);

    return EXIT_SUCCESS;
}
