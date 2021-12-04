#include <stdlib.h>

#ifndef USEFULFUNCTIONS_H
#define USEFULFUNCTIONS_H


// https://stackoverflow.com/questions/5289613/generate-random-float-between-two-floats/5289624
// Generates a random float between two values.
// Does not care if the largest value is first or last
float randomBetween(float a, float b) {
    if(a > b) {
        float temp = a;
        a = b;
        b = temp;
    }
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b-a;
    float r = random * diff;
    return a + r;
}

#endif