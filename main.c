/*
Subiectul proiectului este monitorizarea modificarilor aparute in directoare de-a lungul timpului, prin realizarea de capturi (snapshots) la
cererea utilizatorului.

Cerintele programului:
    Se va dezvolta un program în limbajul C, prin care utilizatorul va putea specifica directorul de monitorizat ca argument în linia de
comandă, iar programul va urmări schimbările care apar în acesta și în subdirectoarele sale.
    La fiecare rulare a programului, se va actualiza captura (snapshot-ul) directorului, stocând metadatele fiecărei intrări.

Pentru a reprezenta grafic un director și schimbările survenite înainte și după rularea programului de monitorizare a modificărilor, putem
utiliza un arbore de fișiere simplu.

Directorul inainte de rularea programului:

Director Principal
|_ Subdirector 1
|  |_ Fisier1.txt
|  |_ Fisier2.txt
|_ Subdirector 2
|  |_ Fisier3.txt
|_ Fisier4.txt

Directorul dupa rularea programului:

// Versiunea 1
Director Principal
|_ Subdirector 1
|  |_ Fisier1.txt
|  |_ Fisier2.txt
|_ Subdirector 2
|  |_ Fisier3.txt
|_ Fisier4.txt
|_ Snapshot.txt (sau alt format specificat)

// Versiunea 2
Director Principal
|_ Subdirector 1
|  |_ Fisier1.txt
|  |_ Fisier1_snapshot.txt
|  |_ Fisier2.txt
|  |_ Fisier2_snapshot.txt
|_ Subdirector 2
|  |_ Fisier3.txt
|  |_ Fisier3_snapshot.txt
|_ Fisier4.txt
|_ Fisier4_snapshot.txt

Nota: S-au prezentat două variante ca exemplu, însă organizarea directorului și a fișierelor de snapshot poate fi realizată în orice fel dorit,
fără a exista restricții impuse.

Explicatie versiunea 1: După rularea programului, în directorul principal este creat un nou fișier, denumit "Snapshot.txt" (sau alt format
specificat), care conține metadatele fiecărei intrări (fișiere și subdirectoare) din directorul principal și subdirectoarele sale. Acest snapshot
este actualizat la fiecare rulare a programului pentru a reflecta modificările survenite.

Nota: Selectarea metadatelor este complet la latitudinea utilizatorului. Este important să includem informații care să diferențieze fiecare
intrare din director într-un mod unic și să evidențieze modificările efectuate.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// declar lstat extern
int lstat(const char *path, struct stat *buf);

void creeaza_actualizeaza_snapshot(char *caleDirector) { 
    struct stat st;
    if(lstat(caleDirector, &st) != 0) { // primesc eroare la lstat daca nu o declar extern
        perror("Eroare la obtinerea informatiilor din director!\n");
        exit(EXIT_FAILURE);
    }

    if(!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "%s nu este director!\n", caleDirector);
        exit(EXIT_FAILURE);
    }

    DIR *director = opendir(caleDirector);
    if (director == NULL) {
        perror("Eroare la deschiderea directorului");
        exit(EXIT_FAILURE);
    }

    char caleSnapshot[256];
    snprintf(caleSnapshot, sizeof(caleSnapshot), "%s/Snapshot.txt", caleDirector);

    FILE *fisierSnapshot = fopen(caleSnapshot, "w"); // schimbare
    if (fisierSnapshot == NULL) {
        perror("Eroare la crearea fisierului de snapshot");
        closedir(director);
        exit(EXIT_FAILURE);
    }

    struct dirent *intrare;
    while ((intrare = readdir(director)) != NULL) {
        if (strcmp(intrare->d_name, ".") == 0 || strcmp(intrare->d_name, "..") == 0) {
            continue;
        }

        char caleIntrare[512];
        snprintf(caleIntrare, sizeof(caleIntrare), "%s/%s", caleDirector, intrare->d_name);

        struct stat informatii;
        if (lstat(caleIntrare, &informatii) != 0) { // lstat
            perror("Eroare la obtinerea informatiilor despre intrare");
            fclose(fisierSnapshot);
            closedir(director);
            exit(EXIT_FAILURE);
        }

        fprintf(fisierSnapshot, "%s: ", intrare->d_name);
        fprintf(fisierSnapshot, "dimensiune=%ld, mod=%o, ultima modificare=%I64d\n", informatii.st_size, informatii.st_mode, informatii.st_mtime);

        if (S_ISDIR(informatii.st_mode)) {
            if (strcmp(intrare->d_name, ".") != 0 && strcmp(intrare->d_name, "..") != 0) {
                creeaza_actualizeaza_snapshot(caleIntrare); // apelez recursiv functia pe un fisier, nu pe un director
            }
        }
    }

    closedir(director);
    fclose(fisierSnapshot);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Utilizare: %s <director>\n", argv[0]);
        return EXIT_FAILURE;
    }

    creeaza_actualizeaza_snapshot(argv[1]);

    return 0;
}

/* 
Cerinta saptamana 2 de proiect:

Se actualizeazaa functionalitatea programului astfel incat sa primeasca un numar nespecificat de argumente
dar nu mai mult de 10, cu mentiunea ca niciun argument nu se va repeta. Programul proceseaza doar directoarele (de completat)
Programul primeste un argument suplimentar care reprez directorul de iesire in care vor fi stocate toate snapshot-urile.
*/