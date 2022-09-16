#include "Gene.h"

#include "helpers.h"
#include "settings.h"

Gene::Gene() {
	Gene(-1, 0);
}

Gene::Gene(int type) {
	Gene(type, 0);
//	initValue();
}

Gene::Gene(int type, float value)
{
	this->type = type;
	this->value = value;
}

Gene& Gene::operator=(const Gene& other)
{
	if( this != &other ) {
		this->type = other.type;
		this->value = other.value;
	}
	return *this;
}

/*void Gene::initValue() { //we cannot delegate this initialization to the Gene class yet b/c we have no good way to access the passed world variables
	switch (this->type) {
		case Trait::SPECIESID :			this->value = (float)(randi(-conf::SPECIESID_RANGE, conf::SPECIESID_RANGE));	 break;
		case Trait::KINRANGE :			this->value = (float)(randi(1, conf::SPECIESID_RANGE/10));						 break;
		case Trait::BRAIN_MUTATION_CHANCE : this->value = BRAIN_MUTATION_CHANCE;										 break;
		case Trait::BRAIN_MUTATION_SIZE :	this->value = BRAIN_MUTATION_SIZE;											 break;
		case Trait::GENE_MUTATION_CHANCE :  this->value = GENE_MUTATION_CHANCE;											 break;
		case Trait::GENE_MUTATION_SIZE :	this->value = GENE_MUTATION_SIZE;											 break;
		case Trait::RADIUS :			this->value = randf(MEANRADIUS*0.2,MEANRADIUS*2.2);								 break;
		case Trait::SKIN_RED :			this->value = randf(0,1);														 break;
		case Trait::SKIN_GREEN :		this->value = randf(0,1);														 break;
		case Trait::SKIN_BLUE :			this->value = randf(0,1);														 break;
		case Trait::SKIN_CHAMOVID :		this->value = cap(randf(-0.25,0.5));											 break;
		case Trait::STRENGTH :			this->value = randf(0.01,1);													 break;
		case Trait::THERMAL_PREF :		this->value = cap(randn(1-2.0*abs(pos.y/conf::HEIGHT - 0.5),0.02));				 break;
		case Trait::LUNGS :				this->value = randf(0,1);														 break;
		case Trait::NUM_BABIES :		this->value = (float)(randi(1,7));												 break;
		case Trait::SEX_PROJECT_BIAS :	this->value = -1;																 break;
			//purposefully excluding "male" and "female" biases for random spawns
		case Trait::METABOLISM :		this->value = randf(0.25,0.75);													 break;
		case (Trait::STOMACH + Stomach::PLANT) :	
		case (Trait::STOMACH + Stomach::FRUIT) :	
		case (Trait::STOMACH + Stomach::MEAT) :	this->value = 0;														 break;
			//it is assumed that we set the stomach later for random spawns
		case Trait::CLOCK1_FREQ :		this->value = randf(5, 100);													 break;
		case Trait::CLOCK2_FREQ :		this->value = randf(5, 100);													 break;
		case Trait::EYE_SEE_AGENTS :	this->value = randf(0.3, 3);													 break;
		case Trait::EYE_SEE_CELLS :		this->value = randf(0.3, 3);													 break;
		case Trait::EAR_HEAR_AGENTS :	this->value = randf(0.1, 3);													 break;
		case Trait::BLOOD_SENSE :		this->value = randf(0.1, 3);													 break;
		case Trait::SMELL_SENSE :		this->value = randf(0.01, 1);													 break;

		default :						this->value = randf(0,1);
	}
}*/

void Gene::mutate(float chance, float size)
{
	if (randf(0,1) < chance) {
		this->value = randn(this->value, size);
	}
}

void Gene::birthMutate(float basechance, float basesize) {
	float chance = basechance;
	float size = basesize;

	switch (this->type) {
		// here be magic numbers - don't worry about it, at least all the gene mutation rates are here in one place
		case Trait::SPECIESID :				chance *= 10;		size = (0.5 + basesize*50);								 break;
		case Trait::KINRANGE :				chance *= 5;	size = (1 + basesize*3*(this->value + 1)); break;
		case Trait::BRAIN_MUTATION_CHANCE : chance = conf::META_MUTCHANCE*2;	size = conf::META_MUTSIZE*3;			 break;
		case Trait::BRAIN_MUTATION_SIZE :	chance = conf::META_MUTCHANCE;		size = conf::META_MUTSIZE;				 break;
		case Trait::GENE_MUTATION_CHANCE :  chance = conf::META_MUTCHANCE*2;	size = conf::META_MUTSIZE*3;			 break;
		case Trait::GENE_MUTATION_SIZE :	chance = conf::META_MUTCHANCE;		size = conf::META_MUTSIZE;				 break;
		case Trait::RADIUS :				chance *= 8;		size *= 16;												 break;
		case Trait::SKIN_RED :				chance *= 4;		size *= 2;												 break;
		case Trait::SKIN_GREEN :			chance *= 4;		size *= 2;												 break;
		case Trait::SKIN_BLUE :				chance *= 4;		size *= 2;												 break;
		case Trait::SKIN_CHAMOVID :			chance *= 2;																 break;
		case Trait::STRENGTH :									size *= 4;												 break;
		case Trait::THERMAL_PREF :			chance *= 2;		size /= 2;												 break;
		case Trait::LUNGS :					chance *= 4;		size /= 2;												 break;
		case Trait::NUM_BABIES :			chance /= 2;		size = (0.1 + basesize*50);								 break;
		case Trait::SEX_PROJECT_BIAS :		chance *= 4;		size *= 5;												 break;
		case Trait::METABOLISM :			chance /= 2;		size *= 2;												 break;
		case (Trait::STOMACH + Stomach::PLANT) :	
		case (Trait::STOMACH + Stomach::FRUIT) :	
		case (Trait::STOMACH + Stomach::MEAT): chance *= 6;		size *= 2;												 break;
		case Trait::CLOCK1_FREQ : /*no changes to base rates, still mutates*/											 break;
		case Trait::CLOCK2_FREQ :																						 break;
		case Trait::EYE_SEE_AGENTS: 							size *= 4;												 break;
		case Trait::EYE_SEE_CELLS :								size *= 4;												 break;
		case Trait::EAR_HEAR_AGENTS:							size *= 4;												 break;
		case Trait::BLOOD_SENSE :								size *= 4;												 break;
		case Trait::SMELL_SENSE :								size *= 4;												 break;

		default : break; // don't modify either chance or size for all other cases
	}
	this->mutate(chance, size);
}


void Gene::liveMutate(float basechance, float basesize) {
	float chance = basechance / 2;
	float size = basesize / 4;

	switch (this->type) {
		case Trait::SPECIESID :				chance = 0; /*disable mutation*/											 break;
		case Trait::KINRANGE :									size *= 10;												 break;
		case Trait::BRAIN_MUTATION_CHANCE : chance = conf::META_MUTCHANCE;		size = conf::META_MUTSIZE*20;			 break;
		case Trait::BRAIN_MUTATION_SIZE :	chance = conf::META_MUTCHANCE;		size = conf::META_MUTSIZE/2;			 break;
		case Trait::GENE_MUTATION_CHANCE :  chance = conf::META_MUTCHANCE;		size = conf::META_MUTSIZE*20;			 break;
		case Trait::GENE_MUTATION_SIZE :	chance = conf::META_MUTCHANCE;		size = conf::META_MUTSIZE/2;			 break;
		case Trait::RADIUS :				chance = 0; /*disable mutation*/											 break;
		case Trait::SKIN_RED :									size *= 2;												 break;
		case Trait::SKIN_GREEN :								size *= 2;												 break;
		case Trait::SKIN_BLUE :									size *= 2;												 break;
		case Trait::SKIN_CHAMOVID :																						 break;
		case Trait::STRENGTH :				chance *= 2;		size *= 2;												 break;
		case Trait::THERMAL_PREF :			chance *= 2;																 break;
		case Trait::LUNGS :					chance *= 2;		size /= 2;												 break;
		case Trait::NUM_BABIES :			chance = 0; /*disable mutation*/											 break;
		case Trait::SEX_PROJECT_BIAS :		chance *= 2;		size *= 2;												 break;
		case Trait::METABOLISM :			chance *= 2;		size /= 2;												 break;
		case (Trait::STOMACH + Stomach::PLANT) :	
		case (Trait::STOMACH + Stomach::FRUIT) :	
		case (Trait::STOMACH + Stomach::MEAT): chance *= 2;		size *= 3;												 break;
		case Trait::CLOCK1_FREQ :			chance *= 2;		size /= 2;												 break;
		case Trait::CLOCK2_FREQ :			chance *= 2;		size /= 2;												 break;
		case Trait::EYE_SEE_AGENTS: 							size *= 4;												 break;
		case Trait::EYE_SEE_CELLS :								size *= 4;												 break;
		case Trait::EAR_HEAR_AGENTS:							size *= 4;												 break;
		case Trait::BLOOD_SENSE :								size *= 4;												 break;
		case Trait::SMELL_SENSE :								size *= 4;												 break;

		default : break; // don't modify either chance or size for all other cases
	}
	this->mutate(chance, size);
}


void Gene::limit(int OVERRIDE_KINRANGE)
{
	//Limits on all the different gene types should be defined below
	switch (this->type) {
		case Trait::SPECIESID :				this->value = floorf(this->value);															 break;
		case Trait::KINRANGE :		this->value = (float)(OVERRIDE_KINRANGE >= 0 ? OVERRIDE_KINRANGE : floorf(abs((int)this->value)));	 break;
		case Trait::BRAIN_MUTATION_CHANCE : this->value = abs(this->value);																 break;
		case Trait::BRAIN_MUTATION_SIZE :	this->value = abs(this->value);																 break;
		case Trait::GENE_MUTATION_CHANCE :  this->value = abs(this->value);																 break;
		case Trait::GENE_MUTATION_SIZE :	this->value = abs(this->value);																 break;
		case Trait::RADIUS :				this->value = lower(this->value, 1.0);														 break;
		case Trait::SKIN_RED :				this->value = cap(abs(this->value));														 break;
		case Trait::SKIN_GREEN :			this->value = cap(abs(this->value));														 break;
		case Trait::SKIN_BLUE :				this->value = cap(abs(this->value));														 break;
		case Trait::SKIN_CHAMOVID :			this->value = cap(this->value);																 break;
		case Trait::STRENGTH :				this->value = cap(abs(this->value));														 break;
		case Trait::THERMAL_PREF :			this->value = cap(this->value);																 break;
		case Trait::LUNGS :					this->value = cap(this->value);																 break;
		case Trait::NUM_BABIES :			this->value = lower(floorf(this->value), 1.0);												 break;
		case Trait::SEX_PROJECT_BIAS :		this->value = clamp(this->value, -1.0, 1.0);												 break;
		case Trait::METABOLISM :			this->value = cap(this->value);																 break;
		case (Trait::STOMACH + Stomach::PLANT) :	
		case (Trait::STOMACH + Stomach::FRUIT) :	
		case (Trait::STOMACH + Stomach::MEAT): this->value = cap(this->value);															 break;
		case Trait::CLOCK1_FREQ :			this->value = lower(this->value, 2);														 break;
		case Trait::CLOCK2_FREQ :			this->value = lower(this->value, 2);														 break;
		case Trait::EYE_SEE_AGENTS :		this->value = abs(this->value);																 break;
		case Trait::EYE_SEE_CELLS :			this->value = abs(this->value);																 break;
		case Trait::EAR_HEAR_AGENTS :		this->value = abs(this->value);																 break;
		case Trait::BLOOD_SENSE :			this->value = abs(this->value);																 break;
		case Trait::SMELL_SENSE :			this->value = abs(this->value);																 break;

		default : break; //for all other types, do nothing. Not advised typically but feel free to go crazy
	}
}

std::string Gene::getTextType() {
	std::string typetext;
	switch (this->type) {
		case Trait::SPECIESID :				typetext = "SPECIESID";											 break;
		case Trait::KINRANGE :				typetext = "KINRANGE";											 break;
		case Trait::BRAIN_MUTATION_CHANCE : typetext = "BRAIN_MUTATION_CHANCE";								 break;
		case Trait::BRAIN_MUTATION_SIZE :	typetext = "BRAIN_MUTATION_SIZE";								 break;
		case Trait::GENE_MUTATION_CHANCE :  typetext = "GENE_MUTATION_CHANCE";								 break;
		case Trait::GENE_MUTATION_SIZE :	typetext = "GENE_MUTATION_SIZE";								 break;
		case Trait::RADIUS :				typetext = "RADIUS";											 break;
		case Trait::SKIN_RED :				typetext = "SKIN_RED";											 break;
		case Trait::SKIN_GREEN :			typetext = "SKIN_GREEN";										 break;
		case Trait::SKIN_BLUE :				typetext = "SKIN_BLUE";											 break;
		case Trait::SKIN_CHAMOVID :			typetext = "SKIN_CHAMOVID";										 break;
		case Trait::STRENGTH :				typetext = "STRENGTH";											 break;
		case Trait::THERMAL_PREF :			typetext = "THERMAL_PREF";										 break;
		case Trait::LUNGS :					typetext = "LUNGS";												 break;
		case Trait::NUM_BABIES :			typetext = "NUM_BABIES";										 break;
		case Trait::SEX_PROJECT_BIAS :		typetext = "SEX_PROJECT_BIAS";									 break;
		case Trait::METABOLISM :			typetext = "METABOLISM";										 break;
		case (Trait::STOMACH + Stomach::PLANT) : typetext = "STOMACH::PLANT";								 break;
		case (Trait::STOMACH + Stomach::FRUIT) : typetext = "STOMACH::FRUIT";								 break;
		case (Trait::STOMACH + Stomach::MEAT) :	typetext = "STOMACH::MEAT";									 break;
		case Trait::CLOCK1_FREQ :			typetext = "CLOCK1_FREQ";										 break;
		case Trait::CLOCK2_FREQ :			typetext = "CLOCK2_FREQ";										 break;
		case Trait::EYE_SEE_AGENTS :		typetext = "EYE_SEE_AGENTS";									 break;
		case Trait::EYE_SEE_CELLS :			typetext = "EYE_SEE_CELLS";										 break;
		case Trait::EAR_HEAR_AGENTS :		typetext = "EAR_HEAR_AGENTS";									 break;
		case Trait::BLOOD_SENSE :			typetext = "BLOOD_SENSE";										 break;
		case Trait::SMELL_SENSE :			typetext = "SMELL_SENSE";										 break;
		default :							typetext = "Trait unknown";
	}

	return typetext;
}