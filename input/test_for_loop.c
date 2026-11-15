int sum_to(int n) {
    int total = 0;
    for (int i = 1; i <= n; i = i + 1) {
        total = total + i;
    }
    return total;
}

int main() {
    int evens = 0;
    for (int k = 0; k < 10; k = k + 1) {
        if (k % 2 == 0) {
            evens = evens + 1;
        }
    }
    return sum_to(evens);
}
