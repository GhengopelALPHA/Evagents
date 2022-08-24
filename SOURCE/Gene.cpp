#include "Gene.h"

#include "helpers.h"

Gene::Gene() {
	Gene(-1, 0);
}

Gene::Gene(int type) {
	Gene(type, 0);
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

void Gene::mutate(float chance, float size)
{
	if (randf(0,1) < chance) {
		this->value = randn(this->value, size);
	}
}