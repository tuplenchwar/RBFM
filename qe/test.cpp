#include <iostream>
using namespace std;

int main() {
    // Write C++ code here
    cout << "Try programiz.pro";
    int varcharTupleCount = 100;
    for (int i = 0; i < varcharTupleCount; ++i) {
        int a = i + 20;
        int length = (i % 26) + 1;
        string b = string(length, '\0');
        for (int j = 0; j < length; j++) {
            b[j] = 96 + length;
        }
        cout<<"i::"<<i<<"a::"<<a<<" string::"<<b<<"\n";
    }
    return 0;
}