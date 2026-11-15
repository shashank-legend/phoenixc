// This file has intentional errors for testing self-healing

int main() {
    int a = 5
    int b = 10;
    int c = a + b;
    
    x = c * 2;  // x not declared
    
    return c;
}