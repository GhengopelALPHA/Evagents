#ifndef GENE_H
#define GENE_H


class Gene
{
public:
	Gene();
	Gene(int type);
	Gene(int type, float value);

	virtual Gene& operator=(const Gene& other);

	int type; //the Type of the Gene. See "Genes" namespace in settings.h
	float value; //the Value of the Gene. How it is used to calculate traits partially depends on the Type

	//float rate;
	//float power; //controls for age-related application of this Gene to the agent's stats

	//void apply(float &trait, int age); //applies the Gene to the given trait with the given age
	void apply(float &trait); //applies the Gene to the given trait. IMPORTANT: make sure to divide the trait value by number of instances of Gene!
	void mutate(float chance, float size); //use the given chance to mutate the Gene with a given size standard deviation

};

#endif // GENE_H