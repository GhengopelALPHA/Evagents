#include "Agent.h"

#include "helpers.h"
#include <stdio.h>

using namespace std;
Agent::Agent(
	int NUMBOXES, 
	int NUMINITCONNS, 
	bool SPAWN_MIRROR_EYES,
	float EYE_SENSE_MAX_FOV, 
	float BRAIN_MUTATION_CHANCE, 
	float BRAIN_MUTATION_SIZE,
	float GENE_MUTATION_CHANCE,
	float GENE_MUTATION_SIZE
){
	//basics
	id= 0;
	setPosRandom(conf::WIDTH, conf::HEIGHT);
	angle= randf(-M_PI,M_PI);

	//genes
	genes.clear();
	//gene creation is handled after agent creation

	//sense structures
	//eyes
	for(int i=0;i<NUMEYES;i++) {
		Eye eye = Eye(EYE_SENSE_MAX_FOV);
		eyes.push_back(eye);
	}
	for(int i=0;i<NUMEYES;i++) {
		if(SPAWN_MIRROR_EYES && i%2==1) { //if enabled, every other eye mirrors the angle of the last, and copies the fov
			eyes[i].dir= 2*M_PI-eyes[i-1].dir;
			eyes[i].fov = eyes[i-1].fov;
		}
	}

	//ears
	for(int i=0;i<NUMEARS;i++) {
		Ear ear = Ear();
		ears.push_back(ear);
	}
	for(int i=0;i<NUMEARS;i++) {
		if(i%NUMEARS == NUMEARS/2) { //every NUMEARS/2 ear is a mirrored position
			ears[i].dir = 2*M_PI - ears[i-1].dir;
		} else if(i%2 == 1) { //every other ear matches the last's position, leaving us with NUMEARS/2 sensors on either side of the agent
			ears[i].dir= ears[i-1].dir;
		}
	}

	//brain matters
	brain = CPBrain(NUMBOXES, NUMINITCONNS);
	in.resize(Input::INPUT_SIZE, 0);
	out.resize(Output::OUTPUT_SIZE, 0);
	mutations.clear();
	damages.clear();


	//triggers, counters, and stats
	health = 1+randf(0.5,0.75);
	exhaustion = 0;
	age = 0;
	freshkill = 0;
	gencount = 0;
	parentid= -1;
	near = false;
	discomfort = 0;
	encumbered = 0;
	
	carcasscount = -1;


	//output mechanical values
	w1= 0;
	w2= 0;
	boost= false;
	zvelocity= 0;
	real_red= 0.5;
	real_gre= 0.5;
	real_blu= 0.5;
	volume= 1;
	tone= 0.5;
	give= 0;
	spikeLength= 0;
	jawPosition= 0;
	jawMaxRecent = 0;
	jawOldOutput= 0;
	grabID= -1;
	grabbing= 0;
	grabangle= 0;
	sexproject= 0;
	clockf3 = 5;

	//stats
	hybrid= false;
	damages.clear();
	death= "Killed by Unknown Factor!"; //default death message, just in case
	children= 0;
	killed= 0;
	hits= 0;
	indicator= 0;
	ir= 0;
	ig= 0;
	ib= 0;
	jaw_render_timer= 0;
	centerrender= 0.5;
	dhealth= 0;
}

Agent::Agent(){
}

void Agent::populateGenes(
	float MEANRADIUS,
	float REP_PER_BABY,
	float BRAIN_MUTATION_CHANCE, 
	float BRAIN_MUTATION_SIZE,
	float GENE_MUTATION_CHANCE,
	float GENE_MUTATION_SIZE,
	int OVERRIDE_KINRANGE
){
	//TODO: add a random number of genes initiated randomly on top of at least one copy for every trait
	for (int i = 0; i < Trait::TRAIT_TYPES; i++) { // TODO: replace later with expanded logic for many copies of genes
		float value;

		switch (i) { //we cannot delegate this initialization to the Gene class yet b/c we have no good way to access the passed world variables
			case Trait::SPECIESID :			value = (float)(randi(-conf::SPECIESID_RANGE, conf::SPECIESID_RANGE));	 break;
			case Trait::KINRANGE : value = OVERRIDE_KINRANGE >= 0 ? OVERRIDE_KINRANGE : (float)(randi(1, conf::SPECIESID_RANGE/10)); break;
			case Trait::BRAIN_MUTATION_CHANCE : value = BRAIN_MUTATION_CHANCE;										 break;
				/* later TODO: add some variation here */
			case Trait::BRAIN_MUTATION_SIZE :	value = BRAIN_MUTATION_SIZE;										 break;
				/* later TODO: add some variation here */
			case Trait::GENE_MUTATION_CHANCE :  value = GENE_MUTATION_CHANCE;										 break;
				/* later TODO: add some variation here */
			case Trait::GENE_MUTATION_SIZE :	value = GENE_MUTATION_SIZE;											 break;
				/* later TODO: add some variation here */
			case Trait::RADIUS :			value = randf(MEANRADIUS*0.2,MEANRADIUS*2.2);							 break;
			case Trait::SKIN_RED :			value = randf(0,1);														 break;
			case Trait::SKIN_GREEN :		value = randf(0,1);														 break;
			case Trait::SKIN_BLUE :			value = randf(0,1);														 break;
			case Trait::SKIN_CHAMOVID :		value = cap(randf(-0.25,0.5));											 break;
			case Trait::STRENGTH :			value = randf(0.01,1);													 break;
			case Trait::THERMAL_PREF :		value = cap(randn(1-2.0*abs(pos.y/conf::HEIGHT - 0.5),0.02));			 break;
			case Trait::LUNGS :				value = randf(0,1);														 break;
			case Trait::NUM_BABIES :		value = (float)(randi(1,8));											 break;
			case Trait::SEX_PROJECT_BIAS :	value = -1; 	 break;
				/*purposefully excluding "male" and "female" biases for random spawns*/
			case Trait::METABOLISM :		value = randf(0.25,0.75);												 break;
			case (Trait::STOMACH + Stomach::PLANT) :	
			case (Trait::STOMACH + Stomach::FRUIT) :	
			case (Trait::STOMACH + Stomach::MEAT) :	value = 0;														 break;
				/*it is assumed that we set the stomach later for random spawns*/
			case Trait::CLOCK1_FREQ :		value = randf(5, 100);													 break;
			case Trait::CLOCK2_FREQ :		value = randf(5, 100);													 break;
			case Trait::EYE_SEE_AGENTS :	value = randf(0.3, 4);													 break;
			case Trait::EYE_SEE_CELLS :		value = randf(0.3, 4);													 break;
			case Trait::EAR_HEAR_AGENTS :	value = randf(0.1, 3);													 break;
			case Trait::BLOOD_SENSE :		value = randf(0.1, 3);													 break;
			case Trait::SMELL_SENSE :		value = randf(0.01, 1);													 break;

			default :		value = randf(0,1);		 break;
		}
		Gene newgene(i, value);
		genes.push_back(newgene);
	}

	//need to force stomach genes to pick random, but complementary, values
	populateStomach();

	//finally, express all genes as traits; initialization is now done!
	expressGenes();	

	resetRepCounter(MEANRADIUS, REP_PER_BABY); //make sure numbabies is set before this!
}

void Agent::populateStomach()
{
	float temp_stomach[Stomach::FOOD_TYPES];
	for (int i=0; i < Stomach::FOOD_TYPES; i++) temp_stomach[i] = 0;

	float budget= 1.5; //give ourselves a budget that's over 100%
	int overtype;
	while (budget > 0){ 
		overtype = randi(0, Stomach::FOOD_TYPES); //pick a random food type
		float budget_taken = randf(0, budget*1.25); //take a value that could be more than our budget (but will be capped)
		temp_stomach[overtype] = cap(temp_stomach[overtype] + budget_taken);
		budget -= budget_taken; //set stomach of type selected and reduce budget, allowing both variety and specialized stomachs
	}

	for (int i = 0; i < genes.size(); i++) {
		if (this->genes[i].type == Trait::STOMACH + Stomach::PLANT) this->genes[i].value = temp_stomach[Stomach::PLANT]; // set all relevant genes to these forced values
		if (this->genes[i].type == Trait::STOMACH + Stomach::MEAT) this->genes[i].value = temp_stomach[Stomach::MEAT];
		if (this->genes[i].type == Trait::STOMACH + Stomach::FRUIT) this->genes[i].value = temp_stomach[Stomach::FRUIT];
	}
}

void Agent::countGenes()
{
	//reset
	for (int i = 0; i < Trait::TRAIT_TYPES; i++) {
		traits[i] = 0;
		counts[i] = 0;
	}

	//for every Gene in our genelist, we calculate our agent's traits and add up counts
	for (int i = 0; i < genes.size(); i++) {
		traits[genes[i].type] += genes[i].value;
		counts[genes[i].type]++;
	}
}

void Agent::expressGenes()
{
	//first, reset our trait and count lists
	this->countGenes();

	//after we add them all up, we divide by counts of instances (pure average). TODO: implement age factors here and periodically recalc
	//and finally, apply limits on the trait values that get expressed depending on trait
	for (int i = 0; i < Trait::TRAIT_TYPES; i++) {
		float value;
		if (counts[i] == 0) {
			printf("DEBUG: count of a trait (%i) was == 0!", i);
			//value = DEFAULT_TRAITS[i]; //load up a default value for a given trait, see Gene.cpp for details
		} else { 
			value = traits[i] / counts[i];
		}

		traits[i] = value;
	}
}

void Agent::mutateGenesBirth(float basechance, float basesize, int OVERRIDE_KINRANGE)
{
	//we need to count genes for delete process
	this->countGenes();
	
	/* Not ready for variable sized arrays of genes
	//let's have some fun; add or remove Genes!
	for (int i = 0; i < 6; i++) {
		if (randf(0,1) < this->traits[Trait::GENE_MUTATION_CHANCE]) {
			//sometimes duplicate Genes (that's how it works in nature!), up to 6 times
			int source_index = randi(0, genes.size());
			genes.push_back(genes[source_index]);
		}
	}

	//delete Genes (ONLY if there is more than 1 copy! This is why we run countGenes prior)
	if (genes.size() > Trait::TRAIT_TYPES) {
		vector<int> match_type_options;
		for (int i = 0; i < Trait::TRAIT_TYPES; i++) {
			if (this->counts[i] > 1) match_type_options.push_back(i); //push the type (i) into a list to randomly select from
		}

		for (int i = 0; i < match_type_options.size(); i++) {
			if (randf(0,1) < this->traits[Trait::GENE_MUTATION_CHANCE]) continue;

			int match_type = match_type_options[i];

			bool has_erased = false;
			vector<Gene>::iterator gene = this->genes.begin();
			while (gene != this->genes.end()) {
				if (gene->type == match_type && !has_erased) {
					gene = this->genes.erase(gene);
					has_erased = true;
				} else {
					++gene;
				}
			}
		}
	} */
	
	for (int i = 0; i < genes.size(); i++) {
		genes[i].birthMutate(basechance, basesize); //all traits can mutate with birth mutations
		genes[i].limit(OVERRIDE_KINRANGE);
	}
}

void Agent::mutateGenesLive(float basechance, float basesize, int OVERRIDE_KINRANGE)
{
	for (int i = 0; i < genes.size(); i++) {
		genes[i].liveMutate(basechance, basesize); //lower chance and size for live mutations, some are disallowed
		genes[i].limit(OVERRIDE_KINRANGE);
	}
}

Agent Agent::reproduce(
	Agent that,
	bool PRESERVE_MIRROR_EYES,
	int OVERRIDE_KINRANGE,
	float MEANRADIUS,
	float REP_PER_BABY,
	int baby,
	float EYE_SENSE_MAX_FOV
){
	//moved muterate gets into reproduce because thats where its needed and used, no reason to go through world
	//choose a value of our agent's mutation and their saved mutation value, can be anywhere between the parents'
	float BMR= randf( min(this->traits[Trait::BRAIN_MUTATION_CHANCE], that.traits[Trait::BRAIN_MUTATION_CHANCE]),
		max(this->traits[Trait::BRAIN_MUTATION_CHANCE], that.traits[Trait::BRAIN_MUTATION_CHANCE]));
	float BMR2= randf( min(this->traits[Trait::BRAIN_MUTATION_SIZE], that.traits[Trait::BRAIN_MUTATION_SIZE]),
		max(this->traits[Trait::BRAIN_MUTATION_SIZE], that.traits[Trait::BRAIN_MUTATION_SIZE]));
	float GMR= randf( min(this->traits[Trait::GENE_MUTATION_CHANCE], that.traits[Trait::GENE_MUTATION_CHANCE]),
		max(this->traits[Trait::GENE_MUTATION_CHANCE], that.traits[Trait::GENE_MUTATION_CHANCE]));
	float GMR2= randf( min(this->traits[Trait::GENE_MUTATION_SIZE], that.traits[Trait::GENE_MUTATION_SIZE]),
		max(this->traits[Trait::GENE_MUTATION_SIZE], that.traits[Trait::GENE_MUTATION_SIZE]));

	//create baby. Note that if the bot selects itself to mate with, this function acts also as assexual reproduction
	//NOTES: Agent "this" is mother, Agent "that" is father, Agent "a2" is daughter
	//if a single parent's trait is required, use the mother's (this->)
	Agent a2(
		this->brain.boxes.size(),
		this->brain.conns.size(),
		PRESERVE_MIRROR_EYES,
		EYE_SENSE_MAX_FOV,
		BMR,
		BMR2,
		GMR,
		GMR2
		);
	//Agent::Agent

	//spawn the baby somewhere closeby behind the mother
	//we want to spawn behind so that agents dont accidentally kill their young right away
	//note that this relies on bots actally driving forward, not backward. We'll let natural selection choose who lives and who dies
	Vector3f fb(this->traits[Trait::RADIUS]*(randf(1.9, 2.1) + 0.3*this->traits[Trait::NUM_BABIES]), 0, 0);
	float floatrange = 0.45;
	fb.rotate(0, 0, this->angle + M_PI*(1 + floatrange - 2*floatrange*(baby+1)/(this->traits[Trait::NUM_BABIES]+1)));
	a2.pos= this->pos + fb;
	a2.dpos= a2.pos;
	a2.borderRectify();

	//basic stat inheritance
	a2.gencount = max(this->gencount,that.gencount) + 1;
	a2.parentid = this->id; //parent ID is strictly inherited from mothers
	a2.discomfort = this->discomfort; //meh, just get mom's temp discomfort and use that for now

	//gene inheritence, per trait (more complicated version later after Genes fully implemented to allow Genes to control this? TODO)
	int maxgenes = this->genes.size();
	if (that.genes.size() > maxgenes) maxgenes = that.genes.size();

	for (int i = 0; i < maxgenes; i++) {
		if (i > this->genes.size()) a2.genes.push_back(that.genes[i]); //if current gene index only exists in father, inherit it
		else if (i > that.genes.size()) a2.genes.push_back(this->genes[i]); //else if current gene index only exists in mother, inherit it
		else a2.genes.push_back( randf(0,1)<0.5 ? this->genes[i] : that.genes[i]); //otherwise if current gene index exists in both, random tossup
		//IMPORTANT NOTE: This method does NOT guarentee all possible traits are inherited. see setDefaultTraits() for details
	}
	
	for (int i=0; i<ears.size(); i++) {
		//inherrit individual ears
		if(randf(0,1) < 0.5) {
			a2.ears[i] = this->ears[i];
		} else {
			a2.ears[i] = that.ears[i];
		}
	}

	for (int i=0; i<eyes.size(); i++) {
		//inherrit individual eyes
		if(randf(0,1) < 0.5) {
			a2.eyes[i] = this->eyes[i];
		} else {
			a2.eyes[i] = that.eyes[i];
		}
	}

	//---MUTATIONS---//
	a2.mutateGenesBirth(GMR, GMR2, OVERRIDE_KINRANGE);		

	//finish by calculating gene expression and setting repcounter
	a2.expressGenes();
	a2.resetRepCounter(MEANRADIUS, REP_PER_BABY);

	for (int i=0 ; i < eyes.size(); i++) {
		if(PRESERVE_MIRROR_EYES && i%2==1) {
			a2.eyes[i].dir = 2*M_PI - a2.eyes[i-1].dir;
			a2.eyes[i].fov = a2.eyes[i-1].fov;
		} else {
			if (randf(0,1)<GMR/60) {
				//LOW-CHANCE EYE COPY MUTATION
				int origeye = randi(0,eyes.size());
				a2.eyes[i].dir = a2.eyes[origeye].dir;
				if(randf(0,1)<GMR*3) a2.eyes[i].fov = a2.eyes[origeye].fov;
			}
			if (randf(0,1)<GMR/40) {
				//MIRROR EYE COPY MUTATION
				int origeye= randi(0,eyes.size());
				a2.eyes[i].dir= 2*M_PI - a2.eyes[origeye].dir;
				if(randf(0,1)<GMR*3) a2.eyes[i].fov= a2.eyes[origeye].fov;
			}

			if(randf(0,1)<GMR) a2.eyes[i].fov = abs(randn(a2.eyes[i].fov, GMR2));
			if(a2.eyes[i].fov>M_PI) a2.eyes[i].fov = M_PI; //eyes cannot wrap around agent

			if(randf(0,1)<GMR) a2.eyes[i].dir = randn(a2.eyes[i].dir, GMR2*4);
			if(a2.eyes[i].dir<0) a2.eyes[i].dir = 0;
			if(a2.eyes[i].dir>2*M_PI) a2.eyes[i].dir = 2*M_PI;
			//not going to loop coordinates; 0,2pi is agents' front again, so it provides a good point to "bounce" off of
		}
	}

	for(int i=0;i<ears.size();i++){
		if(i%2==0) {
			if (randf(0,1)<GMR/40) {
				//LOW-CHANCE COPY EVENT, only copies the hearing ranges
				int origear = randi(0,ears.size());
				if(randf(0,1)<GMR*3) a2.ears[i].low = a2.ears[origear].low;
				if(randf(0,1)<GMR*3) a2.ears[i].high = a2.ears[origear].high;
			}

			if(randf(0,1)<GMR) a2.ears[i].dir = randn(a2.ears[i].dir, GMR2*4);
			if(a2.ears[i].dir < 0) a2.ears[i].dir = 0;
			if(a2.ears[i].dir > 2*M_PI) a2.ears[i].dir = 2*M_PI;
		} else { //IMPLEMENT WORLD SETTING FOR CONTROLLING THIS
			ears[i].dir = ears[i-1].dir;
		}
		
		if(randf(0,1)<GMR/2) a2.ears[i].low = randn(a2.ears[i].low, GMR2*2);
		if(randf(0,1)<GMR/2) a2.ears[i].high = randn(a2.ears[i].high, GMR2*2);

		//restore order if lost
		a2.ears[i].order();
	}
	
	//create brain here
	a2.brain= this->brain.crossover(that.brain);
	a2.brain.initMutate(BMR,BMR2);

	a2.initSplash(conf::RENDER_MAXSPLASHSIZE*0.5,0.8,0.8,0.8); //grey event means we were just born! Welcome!
	
	return a2;
}

void Agent::liveMutate(int MUTMULT, int OVERRIDE_KINRANGE)
{
	initSplash(conf::RENDER_MAXSPLASHSIZE*0.75,0.5,0,1.0);
	
	float BMR= this->traits[Trait::BRAIN_MUTATION_CHANCE]*MUTMULT;
	float BMR2= this->traits[Trait::BRAIN_MUTATION_SIZE];
	float GMR= this->traits[Trait::GENE_MUTATION_CHANCE]*MUTMULT;
	float GMR2= this->traits[Trait::GENE_MUTATION_SIZE];

	//mutate between 1 and 5 brain boxes, plus a ratio from the brain_mutation_chance (+1 at a chance of 0.05)
	int randboxes = randi(1,6) + 20*BMR;
	for(int i= 0; i < randboxes; i++){
		this->brain.liveMutate(BMR, BMR2, this->out);
	}

	//mutate some Genes according to limited rules of the live mutation
	this->mutateGenesLive(GMR, GMR2, OVERRIDE_KINRANGE);
	this->mutations.push_back("");

	this->expressGenes();

	//change traits that are live-mutable here. Things like entropic decay of senses
/*	for (int i = 0; i < traits.size(); i++) {
		float value = trait[i];

		switch (i) {
//			case Trait::SPECIESID :				value = (int)value;											 break;
			case Trait::KINRANGE :				value = OVERRIDE_KINRANGE >= 0 ? OVERRIDE_KINRANGE : (int)abs((int)value); break;
			case Trait::BRAIN_MUTATION_CHANCE : value = abs(value);											 break;
			case Trait::BRAIN_MUTATION_SIZE :	value = abs(value);											 break;
			case Trait::GENE_MUTATION_CHANCE :  value = abs(value);											 break;
			case Trait::GENE_MUTATION_SIZE :	value = abs(value);											 break;
			case Trait::RADIUS :				value = lower(value, 1.0);									 break;
			case Trait::SKIN_RED :				value = cap(abs(value));									 break;
			case Trait::SKIN_GREEN :			value = cap(abs(value));									 break;
			case Trait::SKIN_BLUE :				value = cap(abs(value));									 break;
			case Trait::SKIN_CHAMOVID :			value = cap(value);											 break;
			case Trait::STRENGTH :				value = cap(abs(value));									 break;
			case Trait::THERMAL_PREF :			value = cap(value);											 break;
			case Trait::LUNGS :					value = cap(value);											 break;
			case Trait::NUM_BABIES :			value = lower(value, 1);									 break;
			case Trait::SEX_PROJECT_BIAS :		value = clamp(value, -1.0, 1.0);							 break;
			case Trait::METABOLISM :			value = cap(value);											 break;
			case (Trait::STOMACH + Stomach::PLANT) :	
			case (Trait::STOMACH + Stomach::FRUIT) :	
			case (Trait::STOMACH + Stomach::MEAT) :	value = cap(value);										 break;
			case Trait::CLOCK1_FREQ :			value = lower(value, 2);									 break;
			case Trait::CLOCK2_FREQ :			value = lower(value, 2);									 break;
			case Trait::EYE_SEE_AGENTS :		value = abs(value);											 break;
			case Trait::EYE_SEE_CELLS :			value = abs(value);											 break;
			case Trait::EAR_HEAR_AGENTS :		value = abs(value);											 break;
			case Trait::BLOOD_SENSE :			value = abs(value);											 break;
			case Trait::SMELL_SENSE :					value = abs(value);											 break;

			default : break; //for all other types, do nothing. Not advised typically but feel free to go crazy
		}*/


	//sensory failure: rare chance that the value of a sense gets cut down by roughly half, depending on mutation size
/*	if (randf(0,1)<GMR/30) this->traits[Trait::SMELL_SENSE] /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR/30) this->traits[Trait::EAR_HEAR_AGENTS] /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR/30) this->traits[Trait::EYE_SEE_AGENTS] /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR/30) this->traits[Trait::EYE_SEE_CELLS] /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR/30) this->traits[Trait::BLOOD_SENSE] /= randf(1, 2+GMR2*50); //suspending these mutations until Genes are implemented TODO*/

}

void Agent::tick()
{
	brain.tick(in, out);
}

void Agent::printSelf()
{
	printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	//print all Variables
	//bot basics
	printf("SELECTED AGENT SNAPSHOT REPORT, ID: %i\n", this->id);
	printf("pos & angle: (%f, %f, %f), %f\n", this->pos.x, this->pos.y, this->pos.z, this->angle);
	printf("health, age, & gencount: %f, %.1f days, %i\n", this->health, (float)this->age/10, this->gencount);
	printf("repcounter: %f\n", this->repcounter);
	printf("exhaustion: %f\n", this->exhaustion);

	//traits
	printf("======================= traits =======================\n");
	printf("Genes count: %i\n", genes.size());
	for (int i = 0; i < genes.size(); i++) {
		printf(" gene #%i: type: %s, val: %f\n", i, genes[i].getTextType().c_str(), genes[i].value);
	}
	printf("brain_mutation_chance: %f, brain_mutation_size: %f\n", this->traits[Trait::BRAIN_MUTATION_CHANCE], this->traits[Trait::BRAIN_MUTATION_SIZE]);
	printf("gene_mutation_chance: %f, gene_mutation_size: %f\n", this->traits[Trait::GENE_MUTATION_CHANCE], this->traits[Trait::GENE_MUTATION_SIZE]);
	printf("species id: %f, kin_range: +/-%f\n", this->traits[Trait::SPECIESID], this->traits[Trait::KINRANGE]);
	printf("radius: %f\n", this->traits[Trait::RADIUS]);
	printf("strength: %f\n", this->traits[Trait::STRENGTH]);
	printf("camo: %f, ", this->traits[Trait::SKIN_CHAMOVID]);
	printf("gene_red: %f, gene_gre: %f, gene_blu: %f\n", this->traits[Trait::SKIN_RED], this->traits[Trait::SKIN_GREEN], this->traits[Trait::SKIN_BLUE]);
	printf("num_babies: %f\n", this->traits[Trait::NUM_BABIES]);
	printf("max_repcounter: %f\n", this->maxrepcounter);
	printf("sex_project_bias: %f\n", this->traits[Trait::SEX_PROJECT_BIAS]);
	printf("temp_preference: %f\n", this->traits[Trait::THERMAL_PREF]);
	printf("temp_discomfort: %f\n", this->discomfort);
	printf("lungs: %f\n", this->traits[Trait::LUNGS]);
	printf("metabolism: %f\n", this->traits[Trait::METABOLISM]);
	for(int i=0; i<Stomach::FOOD_TYPES; i++){
		printf("stomach %i value: %f\n", i, this->traits[Trait::STOMACH + i]);
	}
	if (this->isHerbivore()) printf(" Herbivore\n");
	if (this->isCarnivore()) printf(" Carnivore\n");
	if (this->isFrugivore()) printf(" Frugivore\n");
	
	//senses
	printf("======================= senses ========================\n");
	printf("eye_see_agent_mod: %f\n", this->traits[Trait::EYE_SEE_AGENTS]);
	printf("eye_see_cell_mod: %f\n", this->traits[Trait::EYE_SEE_CELLS]);
	printf("eye structs: [");
	for (int i=0; i < this->eyes.size(); i++) {
		printf("(dir: %f, fov: %f),", this->eyes[i].dir, this->eyes[i].fov);
	}
	printf("]");
	printf("hear_mod: %f\n", this->traits[Trait::EAR_HEAR_AGENTS]);
//	std::vector<float> eardir; //position of ears
//	std::vector<float> hearlow; //low values of hearing ranges
//	std::vector<float> hearhigh; //high values of hearing ranges
	printf("clockf1: %f, clockf2: %f, clockf3: %f\n", this->traits[Trait::CLOCK1_FREQ], this->traits[Trait::CLOCK2_FREQ], this->clockf3);
	printf("blood_mod: %f\n", this->traits[Trait::BLOOD_SENSE]);
	printf("smell_mod: %f\n", this->traits[Trait::SMELL_SENSE]);

	//the brain
	printf("====================== the brain ======================\n");
	for(int i=0; i < Output::OUTPUT_SIZE; i++){
		switch(i){
			case Output::BLU :			printf("- blue");				break;
			case Output::BOOST :		printf("- boost");				break;
			case Output::CLOCKF3 :		printf("- clock3 freq.");		break;
			case Output::GIVE :			printf("- give");				break;
			case Output::GRAB :			printf("- grab");				break;
			case Output::GRAB_ANGLE :	printf("- grab angle");			break;
			case Output::GRE :			printf("- green");				break;
			case Output::JAW :			printf("- jaw");				break;
			case Output::JUMP :			printf("- jump");				break;
			case Output::LEFT_WHEEL_B :	printf("- left wheel (B)");		break;
			case Output::LEFT_WHEEL_F :	printf("- left wheel (F)");		break;
			case Output::PROJECT :		printf("- sexual proj.");		break;
			case Output::RED :			printf("- red");				break;
			case Output::REVERSE :		printf("- reverse wheels");		break;
			case Output::RIGHT_WHEEL_B : printf("- right wheel (B)");	break;
			case Output::RIGHT_WHEEL_F : printf("- right wheel (F)");	break;
			case Output::SPIKE :		printf("- spike");				break;
			case Output::STIMULANT :	printf("- stimulant");			break;
			case Output::TONE :			printf("- voice tone");			break;
			case Output::VOLUME :		printf("- voice volume");		break;
			case Output::WASTE_RATE :	printf("- waste");				break;
		}
		printf(" output: %f\n", this->out[i]);
	}
	
	//outputs
	printf("wheel 1 & 2 sums: %f, %f\n", this->w1, this->w2);
	printf("reverse wheels? %s\n", this->out[Output::REVERSE]>0.5 ? "true" : "false");
	printf("boost? %s\n", this->out[Output::BOOST]>0.5 ? "true" : "false");
	printf("jump? %s\n", this->out[Output::JUMP]>0.5 ? "true" : "false");
	printf("pos.z (jump height): %f\n", this->pos.z);
	printf("spikeLength: %f\n", this->spikeLength);
	printf("jawPosition: %f\n", this->jawPosition);
	printf("jawOldOutput: %f\n", this->jawOldOutput);
	printf("jawMaxRecent: %f\n", this->jawMaxRecent);
	printf("volume: %f\n", this->volume);
	printf("tone: %f\n", this->tone);
	printf("give: %f\n", this->give);
	printf("grabID: %i\n", this->grabID);
	printf("grabbing: %f\n", this->grabbing);
	printf("grabangle: %f\n", this->grabangle);
	//more detailed grabangle output?
	printf("sexproject TOTAL: %f\n", this->sexproject);
	
	//stats
	printf("======================== stats ========================\n");
	if(this->near) printf("PRESENTLY NEAR\n");
	else printf("PRESENTLY NOT NEAR (not interaction-processed)\n");
	printf("freshkill: %i\n", this->freshkill);
	printf("carcasscount: %i\n", this->carcasscount);
	if (this->encumbered > 0) printf("encumbered x%i\n", this->encumbered);
	else printf("not encumbered\n");
	if(this->hybrid) printf("is hybrid\n");
	else printf("is budded\n");
	if(this->isDead()) printf("Killed by %s\n", this->death);
	else printf("STILL ALIVE (o)\n");
	printf("parent ID: %i\n", this->parentid);
	printf("children, killed, hits: %i, %i, %i\n", this->children, this->killed, this->hits);
	printf("indicator size, rgb: %f, %f, %f, %f\n", this->indicator, this->ir, this->ig, this->ib); 
	printf("give health gfx magnitude, pos: %f, (%f,%f)\n", this->dhealth, this->dhealthx, this->dhealthy);
	printf("grab gfx pos: (%f,%f)\n", this->grabx, this->graby);
	printf("jaw gfx counter: %f\n", this->jaw_render_timer);
	printf("mutations: (%i)\n", this->mutations.size());
	for (int i = 0; i < (int)this->mutations.size(); i++) {
		cout << this->mutations[i] << ",";
	}
	cout << endl;
	printf("damages:\n");
	for (int i=0; i < (int)this->damages.size(); i++) {
		cout << " " << this->damages[i].first << ": " << this->damages[i].second << endl;
	}
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

void Agent::traceBack(int outback)
{
	//given an output index, ask the brain to re-trace contributing inputs
/*	printf("beginning traceback of output # %d, ", outback);
	switch(outback){
		case Output::BLU :			printf("blue");				break;
		case Output::BOOST :		printf("boost");			break;
		case Output::CLOCKF3 :		printf("clock3 freq.");		break;
		case Output::GIVE :			printf("give");				break;
		case Output::GRAB :			printf("grab");				break;
		case Output::GRAB_ANGLE :	printf("grab angle");		break;
		case Output::GRE :			printf("green");			break;
		case Output::JAW :			printf("jaw");				break;
		case Output::JUMP :			printf("jump");				break;
		case Output::LEFT_WHEEL_B :	printf("left wheel (B)");	break;
		case Output::LEFT_WHEEL_F :	printf("left wheel (F)");	break;
		case Output::PROJECT :		printf("sexual proj.");		break;
		case Output::RED :			printf("red");				break;
		case Output::RIGHT_WHEEL_B : printf("right wheel (B)"); break;
		case Output::RIGHT_WHEEL_F : printf("right wheel (F)"); break;
		case Output::SPIKE :		printf("spike");			break;
		case Output::STIMULANT :	printf("stimulant");		break;
		case Output::TONE :			printf("voice tone");		break;
		case Output::VOLUME :		printf("voice volume");		break;
		case Output::WASTE_RATE :	printf("waste");			break;
	}
	printf("\n");

	std::vector<int> inputs= brain.traceBack(outback);
	std::sort (inputs.begin(), inputs.end());

	printf("This output is strongly connected to  _%d_  inputs:\n", inputs.size());

	for (int i=0; i<inputs.size(); i++) {
		switch(inputs[i]){
			case Input::BLOOD :			printf("blood");			break;
			case Input::BOT_SMELL :		printf("agent_smell");		break;
			case Input::CLOCK1 :		printf("clock1");			break;
			case Input::CLOCK2 :		printf("clock2");			break;
			case Input::CLOCK3 :		printf("clock3");			break;
			case Input::FRUIT_SMELL :	printf("fruit_smell");		break;
			case Input::HAZARD_SMELL :	printf("hazard_smell");		break;
			case Input::HEALTH :		printf("health");			break;
			case Input::REPCOUNT :		printf("reproduction_counter");	break;
			case Input::MEAT_SMELL :	printf("meat_smell");		break;
			case Input::PLAYER :		printf("PLAYER_INPUT");		break;
			case Input::RANDOM :		printf("RaNdOm");			break;
			case Input::TEMP : 			printf("temperature");		break;
			case Input::WATER_SMELL :	printf("water_smell");		break;
		}

		if(inputs[i]>=Input::EYES && inputs[i]<=Input::xEYES){
			printf("eye#%d_",(int)floor((float)(inputs[i])/3.0)-Input::EYES);
			if(inputs[i]%3==0)		printf("red"); //a->in[Input::EYES+i*3]= cap(r[i]);
			else if(inputs[i]%3==1)	printf("green"); //a->in[Input::EYES+i*3+1]= cap(g[i]);
			else if(inputs[i]%3==2)	printf("blue"); //a->in[Input::EYES+i*3+2]= cap(b[i]);
		} else if(inputs[i]>=Input::EARS && inputs[i]<=Input::xEARS){
			printf("ear#%d",inputs[i]-Input::EARS);
		}
		printf(" ");
	}
	printf("\n\n");*/
}


void Agent::initSplash(float size, float r, float g, float b)
{
	this->indicator = min(size,conf::RENDER_MAXSPLASHSIZE);
	this->ir = r;
	this->ig = g;
	this->ib = b;
}

float Agent::getActivity() const
{
	float brainact = this->brain.getActivityRatio();
	return brainact;
}

float Agent::getOutputSum() const
{
	float sum = 0;
	for(int op= 0; op < Output::OUTPUT_SIZE; op++){
		sum += this->out[op];
	}
	return sum;
}

float Agent::getWheelOutputSum() const
{
	return this->out[Output::RIGHT_WHEEL_F] + this->out[Output::RIGHT_WHEEL_B] 
		+ this->out[Output::LEFT_WHEEL_F] + this->out[Output::LEFT_WHEEL_B];
}

void Agent::resetRepCounter(float MEANRADIUS, float REP_PER_BABY)
{
	this->maxrepcounter= max(conf::REPCOUNTER_MIN, 
		REP_PER_BABY*this->traits[Trait::NUM_BABIES]*sqrt(this->traits[Trait::RADIUS]/MEANRADIUS));
	this->repcounter= this->maxrepcounter;
}

void Agent::setHerbivore()
{
	float temp_herbivore = randf(0.75, 1);
	float temp_carnivore = cap(randf(-0.3, 0.3));
	float temp_frugivore = cap(randf(-0.3, 0.3));

	for (int i = 0; i < genes.size(); i++) {
		if (this->genes[i].type == Trait::STOMACH + Stomach::PLANT) this->genes[i].value = temp_herbivore; // set all relevant genes to these forced values
		if (this->genes[i].type == Trait::STOMACH + Stomach::MEAT) this->genes[i].value = temp_carnivore;
		if (this->genes[i].type == Trait::STOMACH + Stomach::FRUIT) this->genes[i].value = temp_frugivore;
	}
	//expressGenes is handled by call sites
}

void Agent::setCarnivore()
{
	float temp_herbivore = cap(randf(-0.3, 0.3));
	float temp_carnivore = randf(0.75, 1);
	float temp_frugivore = cap(randf(-0.3, 0.3));

	for (int i = 0; i < genes.size(); i++) {
		if (this->genes[i].type == Trait::STOMACH + Stomach::PLANT) this->genes[i].value = temp_herbivore; // set all relevant genes to these forced values
		if (this->genes[i].type == Trait::STOMACH + Stomach::MEAT) this->genes[i].value = temp_carnivore;
		if (this->genes[i].type == Trait::STOMACH + Stomach::FRUIT) this->genes[i].value = temp_frugivore;
	}
	//expressGenes is handled by call sites
}

void Agent::setFrugivore()
{
	float temp_herbivore = cap(randf(-0.3, 0.3));
	float temp_carnivore = cap(randf(-0.3, 0.3));
	float temp_frugivore = randf(0.75, 1);

	for (int i = 0; i < genes.size(); i++) {
		if (this->genes[i].type == Trait::STOMACH + Stomach::PLANT) this->genes[i].value = temp_herbivore; // set all relevant genes to these forced values
		if (this->genes[i].type == Trait::STOMACH + Stomach::MEAT) this->genes[i].value = temp_carnivore;
		if (this->genes[i].type == Trait::STOMACH + Stomach::FRUIT) this->genes[i].value = temp_frugivore;
	}
	//expressGenes is handled by call sites
}

void Agent::setPos(float x, float y)
{
	this->pos.x= x;
	this->pos.y= y;
	this->borderRectify();
}

void Agent::setPos(float x, float y, float z)
{
	this->pos.x= x;
	this->pos.y= y;
	this->pos.z= z;
	this->borderRectify();
}

void Agent::setPosRandom(float maxx, float maxy)
{
	this->pos= Vector3f(randf(0,maxx), randf(0,maxy), 0);
	this->borderRectify();
} //currently I see no need for a maxz setting/overload; all agents are spawned on the ground for now

void Agent::borderRectify()
{
	//if this agent has fallen outside of the world borders, rectify and wrap him to the other side
	if (this->pos.x<0)			  this->pos.x= this->pos.x + conf::WIDTH*((int)((0-this->pos.x)/conf::WIDTH)+1);
	if (this->pos.x>=conf::WIDTH)  this->pos.x= this->pos.x - conf::WIDTH*((int)((this->pos.x-conf::WIDTH)/conf::WIDTH)+1);
	if (this->pos.y<0)			  this->pos.y= this->pos.y + conf::HEIGHT*((int)((0-this->pos.y)/conf::HEIGHT)+1);
	if (this->pos.y>=conf::HEIGHT) this->pos.y= this->pos.y - conf::HEIGHT*((int)((this->pos.y-conf::HEIGHT)/conf::HEIGHT)+1);
}

void Agent::setIdealTempPref(float temp)
{
	float ideal_temp;

	if (temp == -1) ideal_temp = cap(randn(1-2.0*abs(pos.y/conf::HEIGHT - 0.5),0.02));
	else ideal_temp = randn(temp, 0.02);

	for (int i = 0; i < genes.size(); i++) {
		if (this->genes[i].type == Trait::THERMAL_PREF) this->genes[i].value = ideal_temp; // set all relevant genes to these forced values
	}
	//expressGenes is handled by call sites
}

void Agent::setIdealLungs(float target)
{
	float temp_lungs = cap(randn(target,0.05));

	for (int i = 0; i < genes.size(); i++) {
		if (this->genes[i].type == Trait::LUNGS) this->genes[i].value = temp_lungs; // set all relevant genes to these forced values
	}
	//expressGenes is handled by call sites
}

bool Agent::isDead() const
{
	if (this->health <= 0) return true;
	return false;
}

bool Agent::isHerbivore() const
{
	if (this->traits[Trait::STOMACH + Stomach::PLANT] >= this->traits[Trait::STOMACH + Stomach::MEAT]
		&& this->traits[Trait::STOMACH + Stomach::PLANT] >= this->traits[Trait::STOMACH + Stomach::FRUIT]) return true;
	return false;
}

bool Agent::isCarnivore() const
{
	if (this->traits[Trait::STOMACH + Stomach::MEAT] >= this->traits[Trait::STOMACH + Stomach::PLANT] 
		&& this->traits[Trait::STOMACH + Stomach::MEAT] >= this->traits[Trait::STOMACH + Stomach::FRUIT]) return true;
	return false;
}

bool Agent::isFrugivore() const
{
	if (this->traits[Trait::STOMACH + Stomach::FRUIT] >= this->traits[Trait::STOMACH + Stomach::PLANT]
		&& this->traits[Trait::STOMACH + Stomach::FRUIT] >= this->traits[Trait::STOMACH + Stomach::MEAT]) return true;
	return false;
}

bool Agent::isTerrestrial() const
{
	if (this->traits[Trait::LUNGS] >= conf::AMPHIBIAN_THRESHOLD) return true;
	return false;
}

bool Agent::isAmphibious() const
{
	if (this->traits[Trait::LUNGS] > 0.5*(Elevation::BEACH_MID + Elevation::SHALLOWWATER)
		&& this->traits[Trait::LUNGS] < conf::AMPHIBIAN_THRESHOLD) return true;
	return false;
}

bool Agent::isAquatic() const
{
	if (this->traits[Trait::LUNGS] <= 0.5*(Elevation::BEACH_MID + Elevation::SHALLOWWATER)) return true;
	return false;
}

bool Agent::isSpiky(float MAX_SPIKE_LENGTH) const
{
	if(this->spikeLength*MAX_SPIKE_LENGTH >= this->traits[Trait::RADIUS]) return true;
	return false;
}

bool Agent::isSpikedDist(float MAX_SPIKE_LENGTH, float d) const
{
	if(d < (this->traits[Trait::RADIUS] + MAX_SPIKE_LENGTH*this->spikeLength)) return true;
	return false;
}

bool Agent::isBitey() const
{
	if(jaw_render_timer > 0) return true;
	return false;
}

bool Agent::isTiny() const
{
	if(traits[Trait::RADIUS] <= conf::TINY_RADIUS) return true;
	return false;
}

bool Agent::isTinyEye(int eyenumber) const
{
	if(eyenumber<NUMEYES/2) return true;
	return false;
}

bool Agent::isAsexual() const
{
	if(this->sexproject <= 0.5) return true;
	return false;
}

bool Agent::isMale() const
{
	if(this->sexproject>1.0) return true;
	return false;
}

int Agent::getRepType() const
{
	if(isAsexual()) return RepType::ASEXUAL;
	else if(isMale()) return RepType::MALE;
	return RepType::FEMALE;
}

bool Agent::isGrabbing() const
{
	if(this->grabbing > 0.5) return true;
	return false;
}

bool Agent::isGiving() const
{
	if(this->give > 0.5) return true;
	return false;
}

bool Agent::isSelfish(float MAXSELFISH) const
{
	if(this->give <= MAXSELFISH) return true;
	return false;
}

bool Agent::isAirborne() const
{
	if(this->pos.z > 0) return true;
	return false;
}


void Agent::addDamage(const char * sourcetext, float amount)
{
	std::string newtext= std::string(sourcetext);
	this->addDamage(newtext, amount);
}

void Agent::addDamage(std::string sourcetext, float amount)
{
	//this method now handles subtraction of health as well as checking if the agent died
	#pragma omp critical //protect us from ourselves... collapse any "for" threads 
	if(amount>0){
		this->health-= amount;

		//we are going to interate through the list of injury causes, and if we've been injured before, add to the amount
		bool added= false;
		for(int i=0; i<this->damages.size(); i++){
			if(this->damages[i].first.compare(sourcetext)==0){
				this->damages[i].second+= amount;
				added= true;
				break;
			}
		}

		if(!added){ //if we checked the list and didn't find the injury cause, add it as a new one
			std::pair<std::string, float> temppair= std::make_pair(sourcetext, amount);
			this->damages.push_back(temppair);
		}
	}

	//check if agent died. If it did, properly bury it
	if(this->health<0){
		if(age < conf::TENDERAGE && (sourcetext.compare(conf::DEATH_HAZARD)==0 || sourcetext.compare(conf::DEATH_NATURAL)==0) && gencount > 0) {
			sourcetext= std::string(conf::DEATH_TOOYOUNG);
			//before TENDERAGE, a death is suspected to be caused by mutations
			//only overwrite hazard and natural deaths however; I want to know when children die from murders, generocity, temperature, etc
		}
		this->death= "Killed by " + sourcetext;
		this->health= 0;
		this->carcasscount= 0;
	}
}

std::pair<std::string, float> Agent::getMostDamage() const
{
	float max= 0;
	std::string text= "";
	for(int i=0; i<this->damages.size(); i++){
		if(this->damages[i].second>max) {
			max= this->damages[i].second;
			text= this->damages[i].first;
		}
	}
	std::pair<std::string, float> temppair= std::make_pair(text, max);
	return temppair;
}

void Agent::addIntake(const char * sourcetext, float amount)
{
	std::string newtext= std::string(sourcetext);
	this->addIntake(newtext, amount);
}

void Agent::addIntake(std::string sourcetext, float amount)
{
	//this method ONLY adds the amount of food taken from world cells
	#pragma omp critical //protect us from ourselves... collapse any threads for 
	if(amount>0){
		//we are going to interate through the list of intake sources, and if we've taken it before, add to the amount
		bool added= false;
		for(int i=0; i<this->intakes.size(); i++){
			if(this->intakes[i].first.compare(sourcetext)==0){
				this->intakes[i].second+= amount;
				added= true;
				break;
			}
		}

		if(!added){ //if we checked the list and didn't find the injury cause, add it as a new one
			std::pair<std::string, float> temppair= std::make_pair(sourcetext, amount);
			this->intakes.push_back(temppair);
		}
	}
}