#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SIZE 10000000

long long int stringHash(char* s) {
    if (s == NULL) return 0;
    static const int p = 127;
    static const int mod = 1e9 + 7;
    int n = strlen(s);
    long long int hash = 0;
    for (int i = 0; i < n; ++i) {
        if (!isalpha(s[i])) continue;
        hash *= p;
        hash += tolower(s[i]);
        hash %= mod;
    }
    return hash;
}

long long int ngramHash(long long int a, long long int b, long long int c) {
    static const int q = 997;
    return a * q * q + b * q + c;
}

// returns the sorted array of ngram hashes

char buffer[BUFFER_SIZE];
long long int hashBuffer[BUFFER_SIZE] = {0};
int length = 0;

void sort(long long int* a, int l, int r) {
    if (l >= r) return;
    int m = (l + r) >> 1;
    sort(a, l, m);
    sort(a, m + 1, r);
    int i = l;
    int j = m + 1;
    int cur = 0;
    while (i <= m && j <= r) {
        if (a[i] > a[j]) {
            hashBuffer[cur++] = a[j++];
        } else {
            hashBuffer[cur++] = a[i++];
        }
    }
    while (i <= m) {
        hashBuffer[cur++] = a[i++];
    }
    while (j <= r) {
        hashBuffer[cur++] = a[j++];
    }
    for (i = l; i <= r; ++i) {
        a[i] = hashBuffer[i - l];
    }
}

long long int* workFile(char* fileName) {
    long long int* a;

    FILE* f = fopen(fileName, "r");

    if (f == NULL) {
        printf("null\n");
        return NULL;
    }

    int index = 2;
    while (fgets(buffer, BUFFER_SIZE, f) != NULL) {
        char* token = strtok(buffer, " ,.\n");
        while (token != NULL) {
            // filtering short words
            if (strlen(token) > 2)
                hashBuffer[index++] = stringHash(token);
            token = strtok(NULL, " ,.\n");
        }
    }

    fclose(f);

    a = (long long int*)malloc((index - 2) * sizeof(long long int));
    for (int i = 0; i + 2 < index; ++i) {
        a[i] = ngramHash(hashBuffer[i], hashBuffer[i + 1], hashBuffer[i + 2]);
    }

    sort(a, 0, index - 3);

    for (int i = 0; i < index - 3; ++i) {
        assert(a[i] <= a[i + 1]);
    }

    length = index - 2;
    return a;
}

float findSimilarity(long long int* a, int n, long long int* b, int m) {

    if (!a || !b) {
        return 0.0;
    }

    int newLength = n + m;
    assert(newLength > 0);

    long long int* elements =
        (long long int*)malloc(newLength * sizeof(long long int));

    int indexA = 0;
    int indexB = 0;
    int cur = 0;
    while (indexA < n && indexB < m) {
        if (a[indexA] <= b[indexB]) {
            elements[cur++] = a[indexA++];
        } else {
            elements[cur++] = b[indexB++];
        }
    }
    while (indexA < n) {
        elements[cur++] = a[indexA++];
    }
    while (indexB < m) {
        elements[cur++] = b[indexB++];
    }

    assert(newLength == cur);

    cur = 0;
    int index = 0;
    while (index < newLength) {
        elements[cur++] = elements[index];
        long long int currentElement = elements[index];
        while (index < newLength && elements[index] == currentElement) ++index;
    }

    assert(cur > 0);
    newLength = cur;

    int* freqA = (int*)malloc(newLength * sizeof(int));
    int* freqB = (int*)malloc(newLength * sizeof(int));

    int f;

    int curA = 0;
    for (int i = 0; i < newLength; ++i) {
        f = 0;
        while (curA < n && a[curA] == elements[i]) ++curA, ++f;
        freqA[i] = f;
    }

    int curB = 0;
    for (int i = 0; i < newLength; ++i) {
        f = 0;
        while (curB < m && b[curB] == elements[i]) ++curB, ++f;
        freqB[i] = f;
    }

    double cross = 0.0;
    double A = 0.0;
    double B = 0.0;

    for (int i = 0; i < newLength; ++i) {
        A += freqA[i] * freqA[i];
        cross += freqA[i] * freqB[i];
        B += freqB[i] * freqB[i];
    }

    long double denominator = sqrtl(A) * sqrtl(B);
    if (denominator < 1) denominator = 1;
    float similarity = cross / denominator;

    free(freqA);
    free(freqB);
    free(elements);
    return (similarity);
}

int main(int argc, char* argv[]) {
    long long int* processedQuery = workFile(argv[1]);
    int processedQueryLength = length;

    DIR* d = opendir(argv[2]);
    struct dirent* directory;

    if (d) {
        while ((directory = readdir(d)) != NULL) {
            if (directory->d_type != DT_REG) continue;

            char* fileName =
                malloc(strlen(argv[2]) + strlen(directory->d_name) + 1);
            strcpy(fileName, argv[2]);
            strcat(fileName, directory->d_name);

            long long int* processedTextFile = workFile(fileName);
            free(fileName);
            int processedTextFileLength = length;

            // should be in percentage
            float similarity =
                100 * findSimilarity(processedQuery, processedQueryLength,
                                     processedTextFile,
                                     processedTextFileLength);

            printf("%s %f%%\n", directory->d_name, similarity);

            free(processedTextFile);
        }

        closedir(d);

    } else {
        printf("Invalid directory path\n");
    }

    free(processedQuery);
    return 0;
}
