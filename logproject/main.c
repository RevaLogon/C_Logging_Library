#include "queue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char filepath[256];
    int writes, threads;

    printf("Enter file path: ");
    fgets(filepath, sizeof(filepath), stdin);
    filepath[strcspn(filepath, "\n")] = '\0'; 

    printf("Enter number of writes (max 200.000): ");
    scanf("%d", &writes);

    printf("Enter number of threads (max 10): ");
    scanf("%d", &threads);

    
    setLoggingParameters(filepath, &writes, &threads);

    logtoFile();

    return 0;
}
