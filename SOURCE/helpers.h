#ifndef HELPERS_H
#define HELPERS_H

#include <cmath>
#include <vector>
#include <algorithm>
#include <cassert>
#include "vmath.h"

inline float expnf(float x, int n = 8) {
    x = 1.0 + x / std::pow(2.0f, n);
    while (n >= 0) {
        x *= x;
        n--;
    }
    return x;
}

// uniform random in [a,b)
inline float randf(float a, float b) { return ((b - a) * (static_cast<float>(std::rand()) / RAND_MAX)) + a; }

// uniform random int in [a,b)
inline int randi(int a, int b) { return (std::rand() % (b - a)) + a; }

// normalvariate random N(mu, sigma)
inline double randn(double mu, double sigma) {
    static bool deviateAvailable = false; // flag
    static float storedDeviate;           // deviate from previous calculation
    double polar, rsquared, var1, var2;
    if (!deviateAvailable) {
        do {
            var1 = 2.0 * (static_cast<double>(std::rand()) / RAND_MAX) - 1.0;
            var2 = 2.0 * (static_cast<double>(std::rand()) / RAND_MAX) - 1.0;
            rsquared = var1 * var1 + var2 * var2;
        } while (rsquared >= 1.0 || rsquared == 0.0);
        polar = std::sqrt(-2.0 * std::log(rsquared) / rsquared);
        storedDeviate = var1 * polar;
        deviateAvailable = true;
        return var2 * polar * sigma + mu;
    } else {
        deviateAvailable = false;
        return storedDeviate * sigma + mu;
    }
}

// cap value between 0 and 1
inline float cap(float a) {
    return std::min(std::max(a, 0.0f), 1.0f);
}

// return a unless greater than upper, then return upper
inline float upper(float a, float upper) {
    return (a > upper) ? upper : a;
}

// return a unless less than lower, then return lower
inline float lower(float a, float lower) {
    return (a < lower) ? lower : a;
}

// clamp function, combines lower & upper. Note: returns low if high is also less than than low
inline float clamp(float a, float low, float high) {
    return lower(upper(a, high), low);
}

inline int fround(float a) {
    return (a - (static_cast<int>(std::abs(a)))) >= 0.5 ? static_cast<int>(a) + 1 : static_cast<int>(a);
}

// distance between point and a line connecting two other points
inline float pointline(Vector2f posA, Vector2f posB, Vector2f posC) {
    // points A & B are of the line; point C is the other
    float normalLength = (posA - posB).length();
    return std::fabs((posC.x - posA.x) * (posB.y - posA.y) - (posC.y - posA.y) * (posB.x - posA.x)) / normalLength;
}

// delete the first item in a vector, treating it like a queue
template <typename T>
void pop_front(std::vector<T>& v) {
    assert(!v.empty());
    v.erase(v.begin());
}

// sum up the second in a vector of pairs
template <typename T>
float getSum(std::vector< std::pair< T, float > > values) {
	float sum= 0;
	for(int i=0; i<values.size(); i++){
		sum+= values[i].second;
	}
	return sum;
}

// Passed arrays store different data types
template <typename T, typename U, int size1, int size2>
inline bool matchArrays(T (&arr1)[size1], U (&arr2)[size2]) {
    return false;
}

// Passed arrays store the SAME data types
template <typename T, int size1, int size2>
inline bool matchArrays(T (&arr1)[size1], T (&arr2)[size2]) {
    if (size1 == size2) {
        return std::equal(arr1, arr1 + size1, arr2);
    }
    return false;
}

#endif
