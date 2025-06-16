#include <stdio.h>

int fibo(int n) {
    if (n <= 1) return n;
    return fibo(n - 1) + fibo(n - 2);
}

int main() {
    int n;
    if (scanf("%d", &n) != 1) {
        return 1;
    }

    for (int i = 0; i < n; i++) {
        printf("%d", fibo(i));
        if (i != n - 1) printf(" ");
    }
    printf("\n");
    return 0;
}
