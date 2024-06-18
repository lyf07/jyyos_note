#include <unistd.h>
#include <stdio.h>

int global = 0;

int main() {
    for (int i = 0; i < 2; i++) {
        fork();
        printf("global = %d\n", global++);
    }
}
