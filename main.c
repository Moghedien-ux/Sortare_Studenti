#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>

#define MAX_NAME_LENGTH 50
#define MAX_FACULTY_NAME_LENGTH 255
#define MAX_LEVEL_LENGTH 10
#define MAX_CRITERIA_FUNCTIONS 10
#define MAX_CRITERIA_DEFINITION 255
#define LINE_BUFFER_SIZE 1024

typedef enum {
    ARE_CNP       = (1 << 0),
    ARE_INALTIME  = (1 << 1),
    ARE_NUME_MAMA = (1 << 2),
    ARE_NUME_TATA = (1 << 3)
} DateOptionale;

typedef struct {
    uint8_t varsta;
    uint8_t ID_Student;

    char *Nume;
    char *Prenume;

    struct {
        char *Facultate;
        char *Nivel;
        uint8_t an;
    } Date;

    uint8_t flags;
    uint8_t date[];
} student;

typedef int (*cmp_func)(const void *, const void *);

typedef struct {
    char *Criteriu;
    cmp_func func;
} crit;

static void swap(void *a, void *b, size_t elem_size) {
    void *temp = malloc(elem_size);
    if (!temp) return;
    memcpy(temp, a, elem_size);
    memcpy(a, b, elem_size);
    memcpy(b, temp, elem_size);
    free(temp);
}

static size_t partition(void *arr, size_t count, size_t elem_size, cmp_func cmp) {
    char *base = (char *)arr;
    void *pivot = base + (count - 1) * elem_size;

    size_t i = 0;
    for (size_t j = 0; j < count - 1; j++) {
        void *current = base + j * elem_size;
        if (cmp(current, pivot) <= 0) {
            swap(base + i * elem_size, current, elem_size);
            i++;
        }
    }
    swap(base + i * elem_size, pivot, elem_size);
    return i;
}

static void quick_sort(void *arr, const size_t arr_size, const size_t arr_type_size, cmp_func cmp) {
    if (arr_size <= 1) return;

    size_t pivot_index = partition(arr, arr_size, arr_type_size, cmp);
    quick_sort(arr, pivot_index, arr_type_size, cmp);

    char *base = (char *)arr;
    void *right_subarray = base + (pivot_index + 1) * arr_type_size;
    size_t right_size = arr_size - pivot_index - 1;

    quick_sort(right_subarray, right_size, arr_type_size, cmp);
}

static inline int crit_0(const void *a, const void *b) {
    const student *s1 = *(const student **)a;
    const student *s2 = *(const student **)b;
    return strcmp(s1->Nume, s2->Nume);
}

static inline int crit_1(const void *a, const void *b) {
    const student *s1 = *(const student **)a;
    const student *s2 = *(const student **)b;
    return (int)s1->varsta - (int)s2->varsta;
}

static inline int crit_2(const void *a, const void *b) {
    const student *s1 = *(const student **)a;
    const student *s2 = *(const student **)b;
    return (int)s1->Date.an - (int)s2->Date.an;
}

static void afisare_studenti(FILE *fout, student** studenti, const size_t nr_studenti) {
    for (size_t i = 0; i < nr_studenti; i++) {
        const student *s = studenti[i];
        fprintf(fout, "Student %zu : %s %s | Facultate: %s | Varsta: %hhu | An: %hhu\n",
                i + 1, s->Nume, s->Prenume, s->Date.Facultate, s->varsta, s->Date.an);

        const uint8_t *ptr = s->date;

        if (s->flags & ARE_CNP) {
            uint64_t cnp;
            memcpy(&cnp, ptr, sizeof(uint64_t));
            ptr += sizeof(uint64_t);
            fprintf(fout, "   -> CNP: %llu\n", (unsigned long long)cnp);
        }
        if (s->flags & ARE_INALTIME) {
            float inaltime;
            memcpy(&inaltime, ptr, sizeof(float));
            ptr += sizeof(float);
            fprintf(fout, "   -> Inaltime: %.2f m\n", inaltime);
        }
        if (s->flags & ARE_NUME_MAMA) {
            fprintf(fout, "   -> Nume Mama: %s\n", (const char*)ptr);
            ptr += strlen((const char*)ptr) + 1;
        }
        if (s->flags & ARE_NUME_TATA) {
            fprintf(fout, "   -> Nume Tata: %s\n", (const char*)ptr);
            ptr += strlen((const char*)ptr) + 1;
        }
    }
    fprintf(fout, "\n");
}

static inline void free_studenti(student** studenti, const size_t nr_studenti) {
    for (size_t i = 0; i < nr_studenti; i++) {
        if (studenti[i]) {
            free(studenti[i]->Nume);
            free(studenti[i]->Prenume);
            free(studenti[i]->Date.Facultate);
            free(studenti[i]->Date.Nivel);
            free(studenti[i]);
        }
    }
    free(studenti);
}

static inline crit *NewCrit(const char *type, cmp_func func) {
    crit* new = (crit*)malloc(sizeof(crit));
    if (!new) return NULL;
    new->Criteriu = (char*)malloc(MAX_CRITERIA_DEFINITION * sizeof(char));
    if (!new->Criteriu) {
        free(new);
        return NULL;
    }
    strcpy(new->Criteriu, type);
    new->func = func;
    return new;
}

int main() {
    FILE *fin = fopen("Studenti.in", "r");
    if (!fin) return -1;

    FILE *fout = fopen("Studenti.out", "w");
    if (!fout) {
        fclose(fin);
        return -1;
    }

    char buffer[LINE_BUFFER_SIZE];
    if (!fgets(buffer, sizeof(buffer), fin)) {
        fclose(fin);
        fclose(fout);
        return -1;
    }

    char *endptr;
    errno = 0;
    unsigned long val_nr = strtoul(buffer, &endptr, 10);
    if (errno != 0 || endptr == buffer) {
        fclose(fin);
        fclose(fout);
        return -1;
    }
    size_t nr_studenti = (size_t)val_nr;

    student **studenti_UNSTPB = (student**)malloc(nr_studenti * sizeof(student*));
    if (!studenti_UNSTPB) {
        fclose(fin);
        fclose(fout);
        return -1;
    }

    for (size_t i = 0; i < nr_studenti; i++) {
        char temp_nume[MAX_NAME_LENGTH];
        char temp_prenume[MAX_NAME_LENGTH];
        char temp_facultate[MAX_FACULTY_NAME_LENGTH];
        char temp_nivel[MAX_LEVEL_LENGTH];

        if (fscanf(fin, "%49s %49s", temp_nume, temp_prenume) != 2) break;

        char num_buf[64];

        if (fscanf(fin, "%63s", num_buf) != 1) break;
        uint8_t varsta = (uint8_t)strtoul(num_buf, NULL, 10);

        if (fscanf(fin, "%63s", num_buf) != 1) break;
        uint8_t id_student = (uint8_t)strtoul(num_buf, NULL, 10);

        if (fscanf(fin, "%254s %9s", temp_facultate, temp_nivel) != 2) break;

        if (fscanf(fin, "%63s", num_buf) != 1) break;
        uint8_t an = (uint8_t)strtoul(num_buf, NULL, 10);

        if (fscanf(fin, "%63s", num_buf) != 1) break;
        uint8_t flags = (uint8_t)strtoul(num_buf, NULL, 10);

        size_t payload_size = 0;
        uint64_t temp_cnp = 0;
        float temp_inaltime = 0.0f;
        char temp_mama[MAX_NAME_LENGTH] = {0};
        char temp_tata[MAX_NAME_LENGTH] = {0};

        if (flags & ARE_CNP) {
            if (fscanf(fin, "%63s", num_buf) == 1) {
                temp_cnp = (uint64_t)strtoull(num_buf, NULL, 10);
            }
            payload_size += sizeof(uint64_t);
        }
        if (flags & ARE_INALTIME) {
            if (fscanf(fin, "%63s", num_buf) == 1) {
                temp_inaltime = strtof(num_buf, NULL);
            }
            payload_size += sizeof(float);
        }
        if (flags & ARE_NUME_MAMA) {
            fscanf(fin, "%49s", temp_mama);
            payload_size += strlen(temp_mama) + 1;
        }
        if (flags & ARE_NUME_TATA) {
            fscanf(fin, "%49s", temp_tata);
            payload_size += strlen(temp_tata) + 1;
        }

        studenti_UNSTPB[i] = (student*)malloc(sizeof(student) + payload_size);
        if (!studenti_UNSTPB[i]) break;

        studenti_UNSTPB[i]->varsta = varsta;
        studenti_UNSTPB[i]->ID_Student = id_student;
        studenti_UNSTPB[i]->Date.an = an;
        studenti_UNSTPB[i]->flags = flags;

        studenti_UNSTPB[i]->Nume = _strdup(temp_nume);
        studenti_UNSTPB[i]->Prenume = _strdup(temp_prenume);
        studenti_UNSTPB[i]->Date.Facultate = _strdup(temp_facultate);
        studenti_UNSTPB[i]->Date.Nivel = _strdup(temp_nivel);

        uint8_t *p = studenti_UNSTPB[i]->date;

        if (flags & ARE_CNP) {
            memcpy(p, &temp_cnp, sizeof(uint64_t));
            p += sizeof(uint64_t);
        }
        if (flags & ARE_INALTIME) {
            memcpy(p, &temp_inaltime, sizeof(float));
            p += sizeof(float);
        }
        if (flags & ARE_NUME_MAMA) {
            strcpy((char*)p, temp_mama);
            p += strlen(temp_mama) + 1;
        }
        if (flags & ARE_NUME_TATA) {
            strcpy((char*)p, temp_tata);
            p += strlen(temp_tata) + 1;
        }
    }

    size_t nr_criterii_de_sortare = 3;
    crit **criterii_de_sortare = (crit **)malloc(nr_criterii_de_sortare * sizeof(crit*));
    if (criterii_de_sortare) {
        *(criterii_de_sortare) = NewCrit("Dupa Nume", crit_0);
        *(criterii_de_sortare + 1) = NewCrit("Dupa Varsta", crit_1);
        *(criterii_de_sortare + 2) = NewCrit("Dupa an de studiu", crit_2);

        for (size_t i = 0; i < nr_criterii_de_sortare; ++i) {
            if (*(criterii_de_sortare + i)) {
                fprintf(fout, "Sortare in functie de: %s\n", criterii_de_sortare[i]->Criteriu);
                quick_sort(studenti_UNSTPB, nr_studenti, sizeof(student*), (*(criterii_de_sortare + i))->func);
                afisare_studenti(fout, studenti_UNSTPB, nr_studenti);
                free((*(criterii_de_sortare + i))->Criteriu);
                free(*(criterii_de_sortare + i));
            }
        }
        free(criterii_de_sortare);
    }

    free_studenti(studenti_UNSTPB, nr_studenti);

    fclose(fin);
    fclose(fout);
    return 0;
}