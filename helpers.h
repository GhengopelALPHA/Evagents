#ifndef HELPERS_H
#define HELPERS_H
#include <stdlib.h>
#include <math.h>
#include "vmath.h"

//uniform random in [a,b)
inline float randf(float a, float b){return ((b-a)*((float)rand()/RAND_MAX))+a;}

//uniform random int in [a,b)
inline int randi(int a, int b){return (rand()%(b-a))+a;}

//normalvariate random N(mu, sigma)
inline double randn(double mu, double sigma) {
	static bool deviateAvailable=false;	//	flag
	static float storedDeviate;			//	deviate from previous calculation
	double polar, rsquared, var1, var2;
	if (!deviateAvailable) {
		do {
			var1=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
			var2=2.0*( double(rand())/double(RAND_MAX) ) - 1.0;
			rsquared=var1*var1+var2*var2;
		} while ( rsquared>=1.0 || rsquared == 0.0);
		polar=sqrt(-2.0*log(rsquared)/rsquared);
		storedDeviate=var1*polar;
		deviateAvailable=true;
		return var2*polar*sigma + mu;
	}
	else {
		deviateAvailable=false;
		return storedDeviate*sigma + mu;
	}
}

//cap value between 0 and 1
inline float cap(float a){ 
	if (a<0) return 0;
	if (a>1) return 1;
	return a;
}

inline float capm(float a, float low, float hi){
	if (a<low) return low;
	if (a>hi) return hi;
	return a;
}

inline int fround(float a){
	if(a - ((int) (abs(a)))>=0.5) return (int) (a) + 1;
	else return (int) (a);
}

//distance between point and a line connecting two other points
inline float pointline(Vector2f posA, Vector2f posB, Vector2f posC){
	//points A & B are of line; point C is other
	float normalLength=(posA-posB).length();
	return fabs((posC.x-posA.x)*(posB.y-posA.y) - (posC.y-posA.y)*(posB.x-posA.x))/normalLength;
}
#endif