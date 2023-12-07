#pragma once

#include "settings.h"
#include <string>

//extern const float DEFAULT_TRAITS[Trait::TRAIT_TYPES];

class Gene
{
public:
	Gene();
	explicit Gene(int type);
	Gene(int type, float value);

	Gene& operator=(const Gene& other);

	//void initValue(); //set the initial value for a gene given its type. UNUSED, challenges discovered in implementation
	void apply(float& trait);               //applies the Gene to the given trait. IMPORTANT: make sure to divide the trait value by number of instances of Gene!
	
    // Mutation functions
    void birthMutate(float basechance, float basesize);
	void liveMutate(float basechance, float basesize);
	void mutate(float chance, float size);  //use the given chance to mutate the Gene with a given size standard deviation
	void limit(int OVERRIDE_KINRANGE = -1); //based on rules, force a given gene's value in a certain range for its type

	std::string getTextType();              // used to help Agent.printSelf

    // Gene parameters
    int type;       //the Type of the Gene. See "Genes" namespace in settings.h
	float value;    //the Value of the Gene. How it is used to calculate traits partially depends on the Type
	//float rate;
	//float power;  //controls for age-related application of this Gene to the agent's stats

};