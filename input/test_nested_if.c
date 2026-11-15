int main() {
    int x = 50;
    int y = 30;
    int result;
    
    if (x > y) {
        if (x > 100) {
            result = x;
        } else {
            result = y;
        }
    } else {
        result = 0;
    }
    
    return result;
}