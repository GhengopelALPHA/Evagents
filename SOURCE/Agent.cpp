#include "Agent.h"

#include "helpers.h"
#include <stdio.h>

using namespace std;
Agent::Agent(
	int NUMBOXES, 
	int NUMINITCONNS, 
	bool SPAWN_MIRROR_EYES, 
	int OVERRIDE_KINRANGE, 
	float MEANRADIUS, 
	float REP_PER_BABY, 
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
	brain_mutation_chance= BRAIN_MUTATION_CHANCE; //abs(BRAIN_MUTATION_CHANCE+randf(-conf::META_MUTCHANCE,conf::META_MUTCHANCE));
	brain_mutation_size= BRAIN_MUTATION_SIZE; //abs(MUTARATE2+randf(-conf::META_MUTSIZE,conf::META_MUTSIZE)*10);
	gene_mutation_chance= GENE_MUTATION_CHANCE; //abs(BRAIN_MUTATION_CHANCE+randf(-conf::META_MUTCHANCE,conf::META_MUTCHANCE));
	gene_mutation_size= GENE_MUTATION_SIZE; //abs(MUTARATE2+randf(-conf::META_MUTSIZE,conf::META_MUTSIZE)*10);
	parentid= -1;
	radius= randf(MEANRADIUS*0.2,MEANRADIUS*2.2);
	strength= randf(0.01,1);
	numbabies= randi(1,7);
	gene_red= randf(0,1);
	gene_gre= randf(0,1);
	gene_blu= randf(0,1);
	chamovid= cap(randf(-0.25,0.5));
	lungs= randf(0,1);
	metabolism= randf(0.25,0.75);
	setIdealTempPref();
	species= randi(-conf::SPECIESID_RANGE,conf::SPECIESID_RANGE);
	kinrange= OVERRIDE_KINRANGE>=0 ? OVERRIDE_KINRANGE : randi(1,conf::SPECIESID_RANGE/10);
	sexprojectbias= -1; //purposefully excluding "male" and "female" biases for random spawn agents
	setRandomStomach();

	//senses and sense-ability
	//eyes
	eye_see_agent_mod= randf(0.3, 3);
	eye_see_cell_mod= randf(0.3, 3);
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
	hear_mod= randf(0.1, 3);
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

	clockf1= randf(5,100);
	clockf2= randf(5,100);
	clockf3= 5;
	blood_mod= randf(0.1, 3);
	smell_mod= randf(0.01, 1);

	//brain matters
	brain = CPBrain(NUMBOXES, NUMINITCONNS);
	in.resize(Input::INPUT_SIZE, 0);
	out.resize(Output::OUTPUT_SIZE, 0);
	brainmutations= 0;
	mutations.clear();
	damages.clear();


	//triggers, counters, and stats
	health = 1+randf(0.5,0.75);
	resetRepCounter(MEANRADIUS, REP_PER_BABY); //make sure numbabies is set before this!
	exhaustion = 0;
	age = 0;
	freshkill = 0;
	gencount = 0;
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
	jawOldOutput= 0;
	grabID= -1;
	grabbing= 0;
	grabangle= 0;
	sexproject= 0;

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
	jawrend= 0;
	centerrender= 0.5;
	dhealth= 0;
}

Agent::Agent(){
}

/*Agent& Agent::operator=(const Agent& other)
{
	if( this != &other )
		this
	return *this;
}*/

//void Agent::exhibitGenes()
//{
	//for every gene in our genelist, we calculate our agent's stats and traits
//	for(int i=0;i<geneType.size();i++) {

//}

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
	float BMR= randf(min(this->brain_mutation_chance, that.brain_mutation_chance), max(this->brain_mutation_chance, that.brain_mutation_chance));
	float BMR2= randf(min(this->brain_mutation_size, that.brain_mutation_size), max(this->brain_mutation_size, that.brain_mutation_size));
	float GMR= randf(min(this->gene_mutation_chance, that.gene_mutation_chance), max(this->gene_mutation_chance, that.gene_mutation_chance));
	float GMR2= randf(min(this->gene_mutation_size, that.gene_mutation_size), max(this->gene_mutation_size, that.gene_mutation_size));

	//create baby. Note that if the bot selects itself to mate with, this function acts also as assexual reproduction
	//NOTES: Agent "this" is mother, Agent "that" is father, Agent "a2" is daughter
	//if a single parent's trait is required, use the mother's (this->)
	Agent a2(
		this->brain.boxes.size(),
		this->brain.conns.size(),
		PRESERVE_MIRROR_EYES,
		OVERRIDE_KINRANGE,
		MEANRADIUS,
		this->maxrepcounter,
		EYE_SENSE_MAX_FOV,
		BMR,
		BMR2,
		GMR,
		GMR2
		);
	//Agent::Agent(

	//spawn the baby somewhere closeby behind the mother
	//we want to spawn behind so that agents dont accidentally kill their young right away
	//note that this relies on bots actally driving forward, not backward. We'll let natural selection choose who lives and who dies
	Vector3f fb(this->radius*randf(1.9, 2.5), 0, 0);
	float floatrange = 0.4;
	fb.rotate(0, 0, this->angle + M_PI*(1 + floatrange - 2*floatrange*(baby+1)/(this->numbabies+1)));
	a2.pos= this->pos + fb;
	a2.dpos= a2.pos;
	a2.borderRectify();

	//basic trait inheritance
	a2.gencount= max(this->gencount+1,that.gencount+1);
	a2.numbabies= randf(0,1)<0.5 ? this->numbabies : that.numbabies;
	a2.metabolism= randf(0,1)<0.5 ? this->metabolism : that.metabolism;
	for(int i=0; i<Stomach::FOOD_TYPES; i++) a2.stomach[i]= randf(0,1)<0.5 ? this->stomach[i]: that.stomach[i];
	a2.species= randf(0,1)<0.5 ? this->species : that.species;
	a2.kinrange= randf(0,1)<0.5 ? this->kinrange : that.kinrange;
	a2.radius= randf(0,1)<0.5 ? this->radius : that.radius;
	a2.strength= randf(0,1)<0.5 ? this->strength : that.strength;
	a2.chamovid= randf(0,1)<0.5 ? this->chamovid : that.chamovid;
	a2.gene_red= randf(0,1)<0.5 ? this->gene_red : that.gene_red;
	a2.gene_gre= randf(0,1)<0.5 ? this->gene_gre : that.gene_gre;
	a2.gene_blu= randf(0,1)<0.5 ? this->gene_blu : that.gene_blu;
	a2.sexprojectbias= randf(0,1)<0.5 ? this->sexprojectbias : that.sexprojectbias;

	a2.brain_mutation_chance= randf(0,1)<0.5 ? this->brain_mutation_chance : that.brain_mutation_chance;
	a2.brain_mutation_size= randf(0,1)<0.5 ? this->brain_mutation_size : that.brain_mutation_size;
	a2.parentid= this->id; //parent ID is strictly inherited from mothers
	a2.clockf1= randf(0,1)<0.5 ? this->clockf1 : that.clockf1;
	a2.clockf2= randf(0,1)<0.5 ? this->clockf2 : that.clockf2;

	a2.smell_mod= randf(0,1)<0.5 ? this->smell_mod : that.smell_mod;
	a2.hear_mod= randf(0,1)<0.5 ? this->hear_mod : that.hear_mod;
	a2.eye_see_agent_mod= randf(0,1)<0.5 ? this->eye_see_agent_mod : that.eye_see_agent_mod;
	a2.eye_see_cell_mod= randf(0,1)<0.5 ? this->eye_see_cell_mod : that.eye_see_cell_mod;
	a2.blood_mod= randf(0,1)<0.5 ? this->blood_mod : that.blood_mod;

	a2.temperature_preference= randf(0,1)<0.5 ? this->temperature_preference : that.temperature_preference;
	a2.discomfort= this->discomfort;
	a2.lungs= randf(0,1)<0.5 ? this->lungs : that.lungs;
	
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
	if (randf(0,1)<GMR/2) a2.numbabies= (int) (randn(a2.numbabies, 0.1 + GMR2*50));
	if (a2.numbabies<1) a2.numbabies= 1; //technically, 0 babies is perfectly logical, but in this sim it is pointless, so it's disallowed
	a2.resetRepCounter(MEANRADIUS, REP_PER_BABY);
	if (randf(0,1)<GMR/2) a2.metabolism= cap(randn(a2.metabolism, GMR2*2));
	for(int i=0; i<Stomach::FOOD_TYPES; i++) if (randf(0,1)<GMR*6) a2.stomach[i]= cap(randn(a2.stomach[i], GMR2*3));
	if (randf(0,1)<GMR*10) a2.species+= (int) (randn(0, 0.5+GMR2*50));
	if (randf(0,1)<GMR*5 && OVERRIDE_KINRANGE<0) a2.kinrange= (int) abs(randn(a2.kinrange, 1+GMR2*5*(a2.kinrange+1)));
	if (randf(0,1)<GMR*6) a2.radius= randn(a2.radius, GMR2*15);
	if (a2.radius<1) a2.radius= 1;
	if (randf(0,1)<GMR) a2.strength= cap(randn(a2.strength, GMR2*4));
	if (randf(0,1)<GMR*2) a2.chamovid= cap(randn(a2.chamovid, GMR2));
	if (randf(0,1)<GMR*4) a2.gene_red= cap(randn(a2.gene_red, GMR2*2));
	if (randf(0,1)<GMR*4) a2.gene_gre= cap(randn(a2.gene_gre, GMR2*2));
	if (randf(0,1)<GMR*4) a2.gene_blu= cap(randn(a2.gene_blu, GMR2*2));
	if (randf(0,1)<GMR*4) a2.sexprojectbias= capm(randn(a2.sexprojectbias, GMR2*5), -1.0, 1.0);

	if (randf(0,1)<conf::META_MUTCHANCE*2) a2.brain_mutation_chance= abs(randn(a2.brain_mutation_chance, conf::META_MUTSIZE*3));
	if (randf(0,1)<conf::META_MUTCHANCE) a2.brain_mutation_size= abs(randn(a2.brain_mutation_size, conf::META_MUTSIZE));
	if (randf(0,1)<conf::META_MUTCHANCE*2) a2.gene_mutation_chance= abs(randn(a2.gene_mutation_chance, conf::META_MUTSIZE*3));
	if (randf(0,1)<conf::META_MUTCHANCE) a2.gene_mutation_size= abs(randn(a2.gene_mutation_size, conf::META_MUTSIZE));
	//we dont really want mutrates to get to zero; thats too stable, take the absolute randn instead

	if (randf(0,1)<GMR) a2.clockf1= randn(a2.clockf1, GMR2);
	if (a2.clockf1<2) a2.clockf1= 2;
	if (randf(0,1)<GMR) a2.clockf2= randn(a2.clockf2, GMR2);
	if (a2.clockf2<2) a2.clockf2= 2;

	if (randf(0,1)<GMR) a2.smell_mod= abs(randn(a2.smell_mod, GMR2*4));
	if (randf(0,1)<GMR) a2.hear_mod= abs(randn(a2.hear_mod, GMR2*4));
	if (randf(0,1)<GMR) a2.eye_see_agent_mod= abs(randn(a2.eye_see_agent_mod, GMR2*4));
	if (randf(0,1)<GMR) a2.eye_see_cell_mod= abs(randn(a2.eye_see_cell_mod, GMR2*4));
	if (randf(0,1)<GMR) a2.blood_mod= abs(randn(a2.blood_mod, GMR2*4));

	if (randf(0,1)<GMR*2) a2.temperature_preference= cap(randn(a2.temperature_preference, GMR2/2));
	if (randf(0,1)<GMR*4) a2.lungs= cap(randn(a2.lungs, GMR2/2));

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

void Agent::liveMutate(int MUTMULT)
{
	initSplash(conf::RENDER_MAXSPLASHSIZE*0.75,0.5,0,1.0);
	
	float BMR= this->gene_mutation_chance*MUTMULT;
	float BMR2= this->gene_mutation_size;
	float GMR= this->gene_mutation_chance*MUTMULT;
	float GMR2= this->gene_mutation_size;
	for(int i= 0; i<randi(1,6); i++){ //mutate between 1 and 5 brain boxes
		this->brain.liveMutate(BMR, BMR2, this->out);
	}

	//change other live-mutable traits here
	if (randf(0,1)<GMR) this->metabolism= cap(randn(this->metabolism, GMR2/5));
	for(int i=0; i<Stomach::FOOD_TYPES; i++) if (randf(0,1)<GMR*2) this->stomach[i]= cap(randn(this->stomach[i], GMR2));
	if (randf(0,1)<conf::META_MUTCHANCE) this->brain_mutation_chance= abs(randn(this->brain_mutation_chance, conf::META_MUTSIZE*20)); 
	if (randf(0,1)<conf::META_MUTCHANCE) this->brain_mutation_size= abs(randn(this->brain_mutation_size, conf::META_MUTSIZE/2));
	if (randf(0,1)<conf::META_MUTCHANCE) this->gene_mutation_chance= abs(randn(this->gene_mutation_chance, conf::META_MUTSIZE*20)); 
	if (randf(0,1)<conf::META_MUTCHANCE) this->gene_mutation_size= abs(randn(this->gene_mutation_size, conf::META_MUTSIZE/2));
	//sensory failure: rare chance that the value of a sense gets cut down by roughly half, depending on mutation size
	if (randf(0,1)<GMR/30) this->smell_mod /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR/30) this->hear_mod /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR/30) this->eye_see_agent_mod /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR/30) this->eye_see_cell_mod /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR/30) this->blood_mod /= randf(1, 2+GMR2*50);
	if (randf(0,1)<GMR) this->clockf1= randn(this->clockf1, GMR2/2);
	if (this->clockf1<2) this->clockf1= 2;
	if (randf(0,1)<GMR) this->clockf2= randn(this->clockf2, GMR2/2);
	if (this->clockf2<2) this->clockf2= 2;
	if (randf(0,1)<GMR) this->temperature_preference= cap(randn(this->temperature_preference, GMR2/4));
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
	printf("SELECTED AGENT REPORT, ID: %i\n", id);
	printf("pos & angle: (%f, %f, %f), %f\n", pos.x, pos.y, pos.z, angle);
	printf("health, age, & gencount: %f, %.1f days, %i\n", health, (float)age/10, gencount);
	printf("repcounter: %f\n", repcounter);
	printf("exhaustion: %f\n", exhaustion);

	//traits
	printf("======================= traits =======================\n");
	printf("brain_mutation_chance: %f, brain_mutation_size: %f\n", brain_mutation_chance, brain_mutation_size);
	printf("gene_mutation_chance: %f, gene_mutation_size: %f\n", gene_mutation_chance, gene_mutation_size);
	printf("species id: %i, kin_range: +/-%i\n", species, kinrange);
	printf("radius: %f\n", radius);
	printf("strength: %f\n", strength);
	printf("camo: %f, gene_red: %f, gene_gre: %f, gene_blu: %f\n", chamovid, gene_red, gene_gre, gene_blu);
	printf("num_babies: %f\n", numbabies);
	printf("max_repcounter: %f\n", maxrepcounter);
	printf("sex_project_bias: %f\n", sexprojectbias);
	printf("temp_preference: %f\n", temperature_preference);
	printf("temp_discomfort: %f\n", discomfort);
	printf("lungs: %f\n", lungs);
	printf("metabolism: %f\n", metabolism);
	for(int i=0; i<Stomach::FOOD_TYPES; i++){
		printf("stomach %i value: %f\n", i, stomach[i]);
	}
	if (isHerbivore()) printf("Herbivore\n");
	if (isCarnivore()) printf("Carnivore\n");
	if (isFrugivore()) printf("Frugivore\n");
	
	//senses
	printf("======================= senses ========================\n");
	printf("eye_see_agent_mod: %f\n", eye_see_agent_mod);
	printf("eye_see_cell_mod: %f\n", eye_see_cell_mod);
//	std::vector<float> eyefov; //field of view for each eye
//	std::vector<float> eyedir; //direction of each eye
	printf("hear_mod: %f\n", hear_mod);
//	std::vector<float> eardir; //position of ears
//	std::vector<float> hearlow; //low values of hearing ranges
//	std::vector<float> hearhigh; //high values of hearing ranges
	printf("clockf1: %f, clockf2: %f, clockf3: %f\n", clockf1, clockf2, clockf3);
	printf("blood_mod: %f\n", blood_mod);
	printf("smell_mod: %f\n", smell_mod);

	//the brain
	printf("====================== the brain ======================\n");
	printf("brain mutations: %f\n", brainmutations);
	for(int i=0; i<Output::OUTPUT_SIZE; i++){
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
			case Output::RIGHT_WHEEL_B : printf("- right wheel (B)");	break;
			case Output::RIGHT_WHEEL_F : printf("- right wheel (F)");	break;
			case Output::SPIKE :		printf("- spike");				break;
			case Output::STIMULANT :	printf("- stimulant");			break;
			case Output::TONE :			printf("- voice tone");			break;
			case Output::VOLUME :		printf("- voice volume");		break;
			case Output::WASTE_RATE :	printf("- waste");				break;
		}
		printf(" output: %f\n", out[i]);
	}
	
	//outputs
	printf("wheel 1 & 2 sums: %f, %f\n", w1, w2);
//	bool boost; //is this agent boosting?
//	float jump; //what "height" this bot is at after jumping
//	float red, gre, blu; //colors of the
//	float volume; //sound volume of this bot. It can scream, or be very sneaky.
//	float tone; //sound tone of this bot. it can be low pitched (<0.5) or high (>0.5), where only bots with hearing in range of tone will hear
//	float give;	//is this agent attempting to give food to other agent?
//	float spikeLength; //"my, what a long spike you have!"
//	float jawPosition; //what "position" the jaw is in. 0 for open, 1 for closed
//	float jawOldPos; //the previous "position" of the jaw
//	int grabID; //id of agent this agent is "glued" to. ==-1 if none selected
//	float grabbing; //is this agent attempting to grab another? If already grabbed, how far are we willing to let them go?
//	float grabangle; //the position of this bot's grab. Other agents can only be grabbed at this angle, and are drawn to this point specifically once grabbed
//	bool sexproject; //is this bot trying to give out its genetic data?
	
	//stats
	printf("======================== stats ========================\n");
	if(near) printf("PRESENTLY NEAR\n");
	else printf("PRESENTLY NOT NEAR (not interaction-processed)\n");
	printf("freshkill: %i\n", freshkill);
	printf("carcasscount: %i\n", carcasscount);
	printf("encumbered x%i\n", encumbered);
	if(hybrid) printf("is hybrid\n");
	else printf("is budded\n");
	if(isDead()) printf("Killed by %s\n", death);
	else printf("STILL ALIVE (o)\n");
	printf("parent ID: %i\n", parentid);
	printf("children, killed, hits: %i, %i, %i\n", children, killed, hits);
	printf("indicator size, rgb: %f, %f, %f, %f\n", indicator, ir, ig, ib); 
	printf("give health gfx magnitude, pos: %f, (%f,%f)\n", dhealth, dhealthx, dhealthy);
	printf("grab gfx pos: (%f,%f)\n", grabx, graby);
	printf("jaw gfx counter: %f\n", jawrend);
	printf("mutations:\n");
		for (int i=0; i<(int)mutations.size(); i++) {
		cout << mutations[i] << endl;
	}
	printf("damages:\n");
	for (int i=0; i<(int)damages.size(); i++) {
		cout << damages[i].first << ": " << damages[i].second << endl;
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
	indicator=min(size,conf::RENDER_MAXSPLASHSIZE);
	ir=r;
	ig=g;
	ib=b;
}

float Agent::getActivity() const
{
	float brainact= brain.getActivityRatio();
	return brainact;
}

float Agent::getOutputSum() const
{
	float sum= 0;
	for(int op= 0; op<Output::OUTPUT_SIZE; op++){
		sum+= out[op];
	}
	return sum;
}

float Agent::getWheelOutputSum() const
{
	return out[Output::RIGHT_WHEEL_F] + out[Output::RIGHT_WHEEL_B] + out[Output::LEFT_WHEEL_F] + out[Output::LEFT_WHEEL_B];
}

void Agent::resetRepCounter(float MEANRADIUS, float REP_PER_BABY)
{
	this->maxrepcounter= max(conf::REPCOUNTER_MIN, REP_PER_BABY*this->numbabies*sqrt(this->radius/MEANRADIUS));
	this->repcounter= this->maxrepcounter;
}

void Agent::setHerbivore()
{
	this->stomach[Stomach::PLANT]= randf(0.5, 1);
	this->stomach[Stomach::MEAT]= cap(randf(-0.3, 0.3));
	this->stomach[Stomach::FRUIT]= cap(randf(-0.3, 0.3));
}

void Agent::setCarnivore()
{
	this->stomach[Stomach::PLANT]= cap(randf(-0.3, 0.3));
	this->stomach[Stomach::MEAT]= randf(0.5, 1);
	this->stomach[Stomach::FRUIT]= cap(randf(-0.3, 0.3));
}

void Agent::setFrugivore()
{
	this->stomach[Stomach::PLANT]= cap(randf(-0.3, 0.3));
	this->stomach[Stomach::MEAT]= cap(randf(-0.3, 0.3));
	this->stomach[Stomach::FRUIT]= randf(0.5, 1);
}

void Agent::setRandomStomach()
{
	for(int i=0; i<Stomach::FOOD_TYPES; i++) stomach[i]= 0;
	float budget= 1.5; //give ourselves a budget that's over 100%
	int overtype;
	while(budget>0){ 
		overtype= randi(0,Stomach::FOOD_TYPES); //pick a random food type
		float budget_minus= randf(0,budget*1.25); //take a value that could be more than our budget (but will be capped)
		stomach[overtype]= cap(stomach[overtype] + budget_minus);
		budget-= budget_minus; //set stomach of type selected and reduce budget, allowing both variety and specialized stomachs
	}
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
	if(temp==-1) temperature_preference= cap(randn(1-2.0*abs(pos.y/conf::HEIGHT - 0.5),0.02)); //logic may need to move when global climate comes in
	else temperature_preference= randn(temp, 0.02);
}

void Agent::setIdealLungs(float target)
{
	lungs= cap(randn(target,0.05));
}

bool Agent::isDead() const
{
	if (health<=0) return true;
	return false;
}

bool Agent::isHerbivore() const
{
	if (stomach[Stomach::PLANT]>=stomach[Stomach::MEAT] && stomach[Stomach::PLANT]>=stomach[Stomach::FRUIT]) return true;
	return false;
}

bool Agent::isCarnivore() const
{
	if (stomach[Stomach::MEAT]>=stomach[Stomach::PLANT] && stomach[Stomach::MEAT]>=stomach[Stomach::FRUIT]) return true;
	return false;
}

bool Agent::isFrugivore() const
{
	if (stomach[Stomach::FRUIT]>=stomach[Stomach::PLANT] && stomach[Stomach::FRUIT]>=stomach[Stomach::MEAT]) return true;
	return false;
}

bool Agent::isTerrestrial() const
{
	if (lungs >= conf::AMPHIBIAN_THRESHOLD) return true;
	return false;
}

bool Agent::isAmphibious() const
{
	if (lungs > 0.5*(Elevation::BEACH_MID + Elevation::SHALLOWWATER) && lungs < conf::AMPHIBIAN_THRESHOLD) return true;
	return false;
}

bool Agent::isAquatic() const
{
	if (lungs <= 0.5*(Elevation::BEACH_MID + Elevation::SHALLOWWATER)) return true;
	return false;
}

bool Agent::isSpikey(float SPIKE_LENGTH) const
{
	if(spikeLength*SPIKE_LENGTH>=radius) return true;
	return false;
}

bool Agent::isTiny() const
{
	if(radius<=conf::TINY_RADIUS) return true;
	return false;
}

bool Agent::isTinyEye(int eyenumber) const
{
	if(eyenumber<NUMEYES/2) return true;
	return false;
}

bool Agent::isAsexual() const
{
	if(sexproject<=0.5) return true;
	return false;
}

bool Agent::isMale() const
{
	if(sexproject>1.0) return true;
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
	if(grabbing>0.5) return true;
	return false;
}

bool Agent::isGiving() const
{
	if(give>0.5) return true;
	return false;
}

bool Agent::isSelfish(float MAXSELFISH) const
{
	if(give<=MAXSELFISH) return true;
	return false;
}

bool Agent::isAirborne() const
{
	if(pos.z>0) return true;
	return false;
}


void Agent::addDamage(const char * sourcetext, float amount)
{
	std::string newtext= std::string(sourcetext);
	addDamage(newtext, amount);
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
	addIntake(newtext, amount);
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