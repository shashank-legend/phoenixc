int add(int x, int y) {
    return x + y;
}

int multiply(int x, int y) {
    return x * y;
}

int main() {
    int a = 5;
    int b = 10;
    int sum = add(a, b);
    int product = multiply(a, b);
    return sum + product;
}
