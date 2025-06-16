#include <stdio.h>

int fibo(int n) {
    int  f, f1 = 0, f2 = 1;
    for (int i = 1; i < n; i++){
        printf("%d", f2);
        f = f1;
        f1 = f2;
        f2 = f + f1;
    }
    return f2;
}

int main() {
    int n;
    if (scanf("%d", &n) != 1) {
        return 1;
    }

    printf("%d", fibo(n));
    return 0;
}
