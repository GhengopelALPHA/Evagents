#include "Agent.h"

#include "settings.h"
#include "helpers.h"
#include <stdio.h>

using namespace std;
Agent::Agent(int NUMBOXES, float MEANRADIUS, float REP_PER_BABY, float MUTARATE1, float MUTARATE2) : 
		brain(NUMBOXES)
{
	//basics
	id= 0;
	setPosRandom(conf::WIDTH, conf::HEIGHT);
	angle= randf(-M_PI,M_PI);

	//genes
	MUTRATE1= abs(MUTARATE1+randf(-conf::METAMUTRATE1,conf::METAMUTRATE1)*50); //chance of mutations.
	MUTRATE2= abs(MUTARATE2+randf(-conf::METAMUTRATE2,conf::METAMUTRATE2)*50); //size of mutations
	radius= randf(MEANRADIUS*0.2,MEANRADIUS*2.2);
	numbabies= randi(1,6);
	gene_red= randf(0,1);
	gene_gre= randf(0,1);
	gene_blu= randf(0,1);
	chamovid= cap(randf(-0.25,0.5));
	lungs= randf(0,1);
	metabolism= randf(0.25,0.75);
	temperature_preference= cap(randn(2.0*abs(pos.y/conf::HEIGHT - 0.5),0.05));
	species= randi(-conf::NUMBOTS*200,conf::NUMBOTS*200); //related to numbots because it's a good relationship
	sexprojectbias= randf(-1,1);
	for(int i=0; i<Stomach::FOOD_TYPES; i++) stomach[i]= 0;
	float budget= 1.5; //
	int overtype;
	while(budget>0){
		overtype= randi(0,Stomach::FOOD_TYPES);
		float budget_minus= randf(0,budget*1.25);
		stomach[overtype]= cap(stomach[overtype] + budget_minus);
		budget-= budget_minus;
	}
	//senses and sense-ability
	//eyes
	eye_see_agent_mod= randf(0.6, 4);
	eyefov.resize(NUMEYES, 0);
	eyedir.resize(NUMEYES, 0);
	for(int i=0;i<NUMEYES;i++) {
		eyefov[i] = randf(0.01, 1.5);
		eyedir[i] = randf(0, 2*M_PI);
	}

	//ears
	hear_mod= randf(0.1, 3);
	eardir.resize(NUMEARS, 0);
	hearlow.resize(NUMEARS, 0);
	hearhigh.resize(NUMEARS, 0);
	for(int i=0;i<NUMEARS;i++) {
		eardir[i]= randf(0, 2*M_PI);
		float temp1= randf(0,1);
		float temp2= randf(0,1);
		if(temp1>temp2){
			hearlow[i]= temp2;
			hearhigh[i]= temp1;
		} else {
			hearlow[i]= temp1;
			hearhigh[i]= temp2;
		}
	}
	clockf1= randf(5,100);
	clockf2= randf(5,100);
	clockf3= 5;
	blood_mod= randf(0.1, 3);
	smell_mod= randf(0.1, 3);

	//brain matters
	in.resize(Input::INPUT_SIZE, 0);
	out.resize(Output::OUTPUT_SIZE, 0);
	brainmutations= 0;


	//triggers, counters, and stats
	health= 1+randf(0.5,0.75);
	age= 0;
	freshkill= 0;
	gencount= 0;
	near= false;
	resetRepCounter(MEANRADIUS, REP_PER_BABY); //make sure numbabies is set before this!
	discomfort= 0;
	encumbered= false;
	exhaustion= 0;
	carcasscount= -1;


	//output mechanical values
	w1= 0;
	w2= 0;
	strength= randf(0.01,1);
	boost= false;
	jump= 0;
	real_red= 0.5;
	real_gre= 0.5;
	real_blu= 0.5;
	volume= 1;
	tone= 0.5;
	give= 0;
	spikeLength= 0;
	jawPosition= 0;
	jawOldPos= 0;
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

//void Agent::exhibitGenes()
//{
	//for every gene in our genelist, we calculate our agent's stats and traits
//	for(int i=0;i<geneType.size();i++) {

//}

void Agent::printSelf()
{
	printf("\n~~~~~~~~~~~~~~~~~~~~~~~~~\n");
	//print all Variables
	//bot basics
	printf("AGENT, ID: %i\n", id);
	printf("pos & angle: (%f,%f), %f\n", pos.x, pos.y, angle);
	printf("health, age, & gencount: %f, %.1f, %i\n", health, (float)age/10, gencount);
	printf("MUTRATE1: %f, MUTRATE2: %f\n", MUTRATE1, MUTRATE2);
	printf("radius: %f\n", radius);
	printf("strength: %f\n", strength);
	printf("camo: %f\n", chamovid);
	printf("gene_red: %f\ngene_gre: %f\ngene_blu: %f\n", gene_red, gene_gre, gene_blu);

	//triggers, counters, and layer interaction
	if(near) printf("PRESENTLY NEAR\n");
	else printf("PRESENTLY NOT NEAR (not interaction-processed)\n");
	printf("freshkill: %i\n", freshkill);
	printf("species id: %i\n", species);
	printf("num_babies: %f\n", numbabies);
	printf("maxrepcounter: %f\n", maxrepcounter);
	printf("sexprojectbias: %f\n", sexprojectbias);
	printf("repcounter: %f\n", repcounter);
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
	printf("exhaustion: %f\n", exhaustion);
	if(encumbered) printf("encumbered\n");
	else printf("not encumbered\n");

	//senses
//	float eye_see_agent_mod;
//	std::vector<float> eyefov; //field of view for each eye
//	std::vector<float> eyedir; //direction of each eye
//	float hear_mod;
//	std::vector<float> eardir; //position of ears
//	std::vector<float> hearlow; //low values of hearing ranges
//	std::vector<float> hearhigh; //high values of hearing ranges
//	float clockf1, clockf2, clockf3; //the frequencies of the three clocks of this bot
//	float blood_mod;
//	float smell_mod;

	//the brain
	printf("brain mutations: %f\n", brainmutations);
	for(int i=0; i<Output::OUTPUT_SIZE; i++){
		switch(i){
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
		printf(" output: %f\n", out[i]);
	}
	
	//outputs
	printf("wheel 1 & 2: %f, %f\n", w1, w2);
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
	if(hybrid) printf("is hybrid\n");
	else printf("is budded\n");
//	const char * death; //the cause of death of this agent
	printf("children, killed, hits: %i, %i, %i\n", children, killed, hits);
	printf("indicator size, rgb: %f, %f, %f, %f\n", indicator, ir, ig, ib); 
	printf("give health gfx magnitude, pos: %f, (%f,%f)\n", dhealth, dhealthx, dhealthy);
	printf("grab gfx pos: (%f,%f)\n", grabx, graby);
	printf("jaw gfx counter: %f\n", jawrend); //render counter for jaw. Past ~10 ticks of no jaw action, it is "retracted" visually
	for (int i=0; i<(int)mutations.size(); i++) {
		cout << mutations[i];
	}
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

void Agent::traceBack(int outback)
{
	//given an output index, ask the brain to re-trace contributing inputs
	printf("beginning traceback of output # %d, ", outback);
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
			case Input::HEARING1 :		printf("ear1");				break;
			case Input::HEARING2 :		printf("ear2");				break;
			case Input::MEAT_SMELL :	printf("meat_smell");		break;
			case Input::PLAYER :		printf("PLAYER_INPUT");		break;
			case Input::RANDOM :		printf("RaNdOm");			break;
			case Input::TEMP : 			printf("temperature");		break;
			case Input::WATER_SMELL :	printf("water_smell");		break;
		}

		if(inputs[i]>=Input::EYES && inputs[i]<=Input::xEYES){
			printf("eye#%d_",(int)floor((float)(inputs[i])/3.0));
			if(inputs[i]%3==0)		printf("red"); //a->in[Input::EYES+i*3]= cap(r[i]);
			else if(inputs[i]%3==1)	printf("green"); //a->in[Input::EYES+i*3+1]= cap(g[i]);
			else if(inputs[i]%3==2)	printf("blue"); //a->in[Input::EYES+i*3+2]= cap(b[i]);
		}
		printf(" ");
	}
	printf("\n\n");
}


void Agent::initSplash(float size, float r, float g, float b)
{
	indicator=min(size,conf::MAXSPLASHSIZE);
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

void Agent::tick()
{
	brain.tick(in, out);
}
Agent Agent::reproduce(Agent that, float MEANRADIUS, float REP_PER_BABY)
{
	//moved muterate gets into reproduce because thats where its needed and used, no reason to go through world
	//choose a value of our agent's mutation and their saved mutation value, can be anywhere between the parents'
	float MR= randf(min(this->MUTRATE1,that.MUTRATE1),max(this->MUTRATE1,that.MUTRATE1));
	float MR2= randf(min(this->MUTRATE2,that.MUTRATE2),max(this->MUTRATE2,that.MUTRATE2));

	//create baby. Note that if the bot selects itself to mate with, this function acts also as assexual reproduction
	//NOTES: Agent "this" is mother, Agent "that" is father, Agent "a2" is daughter
	//if a single parent's trait is required, use the mother's (this->)
	Agent a2(this->brain.boxes.size(), MEANRADIUS, this->maxrepcounter, MR, MR2);

	//spawn the baby somewhere closeby behind the mother
	//we want to spawn behind so that agents dont accidentally kill their young right away
	//note that this relies on bots actally driving forward, not backward. We'll let natural selection choose who lives and who dies
	Vector2f fb(this->radius*randf(2, 3),0);
	fb.rotate(this->angle+M_PI+this->numbabies*randf(-0.4,0.4));
	a2.pos= this->pos + fb;
	a2.dpos= a2.pos;
	a2.borderRectify();

	//basic trait inheritance
	a2.gencount= max(this->gencount+1,that.gencount+1);
	a2.numbabies= randf(0,1)<0.5 ? this->numbabies : that.numbabies;
	a2.metabolism= randf(0,1)<0.5 ? this->metabolism : that.metabolism;
	for(int i=0; i<Stomach::FOOD_TYPES; i++) a2.stomach[i]= randf(0,1)<0.5 ? this->stomach[i]: that.stomach[i];
	a2.species= randf(0,1)<0.5 ? this->species : that.species;
	a2.radius= randf(0,1)<0.5 ? this->radius : that.radius;
	a2.strength= randf(0,1)<0.5 ? this->strength : that.strength;
	a2.chamovid= randf(0,1)<0.5 ? this->chamovid : that.chamovid;
	a2.gene_red= randf(0,1)<0.5 ? this->gene_red : that.gene_red;
	a2.gene_gre= randf(0,1)<0.5 ? this->gene_gre : that.gene_gre;
	a2.gene_blu= randf(0,1)<0.5 ? this->gene_blu : that.gene_blu;
	a2.sexprojectbias= randf(0,1)<0.5 ? this->sexprojectbias : that.sexprojectbias;

	a2.MUTRATE1= randf(0,1)<0.5 ? this->MUTRATE1 : that.MUTRATE1;
	a2.MUTRATE2= randf(0,1)<0.5 ? this->MUTRATE2 : that.MUTRATE2;
	a2.clockf1= randf(0,1)<0.5 ? this->clockf1 : that.clockf1;
	a2.clockf2= randf(0,1)<0.5 ? this->clockf2 : that.clockf2;

	a2.smell_mod= randf(0,1)<0.5 ? this->smell_mod : that.smell_mod;
	a2.hear_mod= randf(0,1)<0.5 ? this->hear_mod : that.hear_mod;
	a2.eye_see_agent_mod= randf(0,1)<0.5 ? this->eye_see_agent_mod : that.eye_see_agent_mod;
//	a2.eye_see_cell_mod= randf(0,1)<0.5 ? this->eye_see_cell_mod : that.eye_see_cell_mod;
	a2.blood_mod= randf(0,1)<0.5 ? this->blood_mod : that.blood_mod;

	a2.temperature_preference= randf(0,1)<0.5 ? this->temperature_preference : that.temperature_preference;
	a2.lungs= randf(0,1)<0.5 ? this->lungs : that.lungs;
	
	a2.eardir= randf(0,1)<0.5 ? this->eardir : that.eardir;
	a2.hearlow= randf(0,1)<0.5 ? this->hearlow : that.hearlow;
	a2.hearhigh= randf(0,1)<0.5 ? this->hearhigh : that.hearhigh;

	a2.eyefov= randf(0,1)<0.5 ? this->eyefov : that.eyefov;
	a2.eyedir= randf(0,1)<0.5 ? this->eyedir : that.eyedir;


	//---MUTATIONS---//
	if (randf(0,1)<MR/10) a2.numbabies= (int) (randn(a2.numbabies, 0.1 + MR2*50));
	if (a2.numbabies<1) a2.numbabies= 1; //technically, 0 babies is perfectly logical, but in this sim it is pointless, so it's disallowed
	a2.resetRepCounter(MEANRADIUS, REP_PER_BABY);
	if (randf(0,1)<MR) a2.metabolism= cap(randn(a2.metabolism, MR2*3));
	for(int i=0; i<Stomach::FOOD_TYPES; i++) if (randf(0,1)<MR*4) a2.stomach[i]= cap(randn(a2.stomach[i], MR2*7)); //*10 was a bit much
	if (randf(0,1)<MR*20) a2.species+= (int) (randn(0, MR2*120));
	if (randf(0,1)<MR*5) a2.radius= randn(a2.radius, MR2*10);
	if (a2.radius<1) a2.radius= 1;
	if (randf(0,1)<MR*2) a2.strength= cap(randn(a2.strength, MR2));
	if (randf(0,1)<MR) a2.chamovid= cap(randn(a2.chamovid, MR2/2));
	if (randf(0,1)<MR*2) a2.gene_red= cap(randn(a2.gene_red, MR2));
	if (randf(0,1)<MR*2) a2.gene_gre= cap(randn(a2.gene_gre, MR2));
	if (randf(0,1)<MR*2) a2.gene_blu= cap(randn(a2.gene_blu, MR2));
	if (randf(0,1)<MR/2) a2.sexprojectbias= capm(randn(a2.sexprojectbias, MR2/2), -1.0, 1.0);

	if (randf(0,1)<MR) a2.MUTRATE1= abs(randn(a2.MUTRATE1, conf::METAMUTRATE1));
	if (randf(0,1)<MR) a2.MUTRATE2= abs(randn(a2.MUTRATE2, conf::METAMUTRATE2));
	//we dont really want mutrates to get to zero; thats too stable. so take absolute randn instead.

	if (randf(0,1)<MR) a2.clockf1= randn(a2.clockf1, MR2);
	if (a2.clockf1<2) a2.clockf1= 2;
	if (randf(0,1)<MR) a2.clockf2= randn(a2.clockf2, MR2);
	if (a2.clockf2<2) a2.clockf2= 2;

	if (randf(0,1)<MR) a2.smell_mod= randn(a2.smell_mod, MR2);
	if (randf(0,1)<MR) a2.hear_mod= randn(a2.hear_mod, MR2);
	if (randf(0,1)<MR) a2.eye_see_agent_mod= randn(a2.eye_see_agent_mod, MR2);
//	if (randf(0,1)<MR) a2.eye_see_cell_mod= randn(a2.eye_see_cell_mod, MR2);
	if (randf(0,1)<MR) a2.blood_mod= randn(a2.blood_mod, MR2);

	if (randf(0,1)<MR*3) a2.temperature_preference= cap(randn(a2.temperature_preference, MR2));
	if (randf(0,1)<MR*4) a2.lungs= cap(randn(a2.lungs, MR2));

	for(int i=0;i<eyedir.size();i++){
		if (randf(0,1)<MR/100) {
			//LOW-CHANCE COPY EVENT (multiply by NUMEYES for real probability)
			int origeye= randi(0,eyedir.size());
			if(randf(0,1)<MR*5) a2.eyedir[i]= a2.eyedir[origeye];
			if(randf(0,1)<MR*5) a2.eyefov[i]= a2.eyefov[origeye];
		}

		if(randf(0,1)<MR) a2.eyefov[i] = abs(randn(a2.eyefov[i], MR2/2));
		if(a2.eyefov[i]>M_PI) a2.eyefov[i] = M_PI; //eyes cannot wrap around bot
		
		if(randf(0,1)<MR*2) a2.eyedir[i] = randn(a2.eyedir[i], MR2*5);
		if(a2.eyedir[i]<0) a2.eyedir[i] = 0;
		if(a2.eyedir[i]>2*M_PI) a2.eyedir[i] = 2*M_PI;
		//not going to loop coordinates; 0,2pi is bots front again, so it provides a good point to "bounce" off of
	}

	for(int i=0;i<NUMEARS;i++){
		if (randf(0,1)<MR/60) {
			//LOW-CHANCE COPY EVENT (multiply by NUMEARS for total probability)
			int origear= randi(0,NUMEARS);
//			if(randf(0,1)<MR*5) a2.eardir[i]= a2.eardir[origear];
			if(randf(0,1)<MR*5) a2.hearlow[i]= a2.hearlow[origear];
			if(randf(0,1)<MR*5) a2.hearhigh[i]= a2.hearhigh[origear];
		}
		
		if(randf(0,1)<MR*2) a2.eardir[i] = randn(a2.eardir[i], MR2*5);
		if(a2.eardir[i]<0) a2.eardir[i] = 0;
		if(a2.eardir[i]>2*M_PI) a2.eardir[i] = 2*M_PI;
		if(randf(0,1)<MR) a2.hearlow[i]= randn(a2.hearlow[i], MR2);
		if(randf(0,1)<MR) a2.hearhigh[i]= randn(a2.hearhigh[i], MR2);

		//restore order if lost
		if (a2.hearlow[i]>a2.hearhigh[i]) {
			float temp= a2.hearlow[i];
			a2.hearlow[i]= a2.hearhigh[i];
			a2.hearhigh[i]= temp;
		}
	}
	
	//create brain here
	a2.brain= this->brain.crossover(that.brain);
	a2.brain.initMutate(MR,MR2);

	a2.initSplash(conf::MAXSPLASHSIZE*0.5,0.8,0.8,0.8); //grey event means we were just born! Welcome!
	
	return a2;

}

void Agent::resetRepCounter(float MEANRADIUS, float REP_PER_BABY)
{
	this->maxrepcounter= max(conf::REPCOUNTER_MIN, REP_PER_BABY*this->numbabies*sqrt(this->radius/MEANRADIUS));
	this->repcounter= this->maxrepcounter;
}

void Agent::liveMutate()//float MR, float MR2)
{
	initSplash(conf::MAXSPLASHSIZE*0.5,0.5,0,1.0);
	
	float MR= this->MUTRATE1;
	float MR2= this->MUTRATE2;
	this->brain.liveMutate(MR, MR2, this->out);

	//change other mutable traits here
	if (randf(0,1)<MR) this->metabolism= cap(randn(this->metabolism, MR2/5));
	for(int i=0; i<Stomach::FOOD_TYPES; i++) if (randf(0,1)<MR*2) this->stomach[i]= cap(randn(this->stomach[i], MR2*2));
	//METAMUTERATE used for chance because this is supposed to represent background mutation chances
	if (randf(0,1)<conf::METAMUTRATE1/10) this->MUTRATE1= abs(randn(this->MUTRATE1, conf::METAMUTRATE1*25));
	if (randf(0,1)<conf::METAMUTRATE1/10) this->MUTRATE2= abs(randn(this->MUTRATE2, conf::METAMUTRATE2*100));
	if (randf(0,1)<MR) this->clockf1= randn(this->clockf1, MR2/2);
	if (this->clockf1<2) this->clockf1= 2;
	if (randf(0,1)<MR) this->clockf2= randn(this->clockf2, MR2/2);
	if (this->clockf2<2) this->clockf2= 2;
	if (randf(0,1)<MR) this->temperature_preference= cap(randn(this->temperature_preference, MR2/4));
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

void Agent::setPos(float x, float y)
{
	this->pos.x= x;
	this->pos.y= y;
	this->borderRectify();
}

void Agent::setPosRandom(float maxx, float maxy)
{
	this->pos= Vector2f(randf(0,maxx),randf(0,maxy));
	this->borderRectify();
}

void Agent::borderRectify()
{
	//if this agent has fallen outside of the world borders, rectify and wrap him to the other side
	if (this->pos.x<0)			  this->pos.x= this->pos.x + conf::WIDTH*((int)((0-this->pos.x)/conf::WIDTH)+1);
	if (this->pos.x>=conf::WIDTH)  this->pos.x= this->pos.x - conf::WIDTH*((int)((this->pos.x-conf::WIDTH)/conf::WIDTH)+1);
	if (this->pos.y<0)			  this->pos.y= this->pos.y + conf::HEIGHT*((int)((0-this->pos.y)/conf::HEIGHT)+1);
	if (this->pos.y>=conf::HEIGHT) this->pos.y= this->pos.y - conf::HEIGHT*((int)((this->pos.y-conf::HEIGHT)/conf::HEIGHT)+1);
}

bool Agent::isHerbivore() const
{
	if (health>0 && stomach[Stomach::PLANT]>=stomach[Stomach::MEAT] && stomach[Stomach::PLANT]>=stomach[Stomach::FRUIT]) return true;
	return false;
}

bool Agent::isCarnivore() const
{
	if (health>0 && stomach[Stomach::MEAT]>=stomach[Stomach::PLANT] && stomach[Stomach::MEAT]>=stomach[Stomach::FRUIT]) return true;
	return false;
}

bool Agent::isFrugivore() const
{
	if (health>0 && stomach[Stomach::FRUIT]>=stomach[Stomach::PLANT] && stomach[Stomach::FRUIT]>=stomach[Stomach::MEAT]) return true;
	return false;
}

bool Agent::isTerrestrial() const
{
	if (lungs>0.5) return true;
	return false;
}

bool Agent::isAmphibious() const
{
	if (lungs>0.25 && lungs<=0.5) return true;
	return false;
}

bool Agent::isAquatic() const
{
	if (lungs<=0.25) return true;
	return false;
}

bool Agent::isSpikey(float SPIKELENGTH) const
{
	if(spikeLength*SPIKELENGTH>=radius) return true;
	return false;
}

bool Agent::isTiny() const
{
	if(radius<=5) return true;
	return false;
}

bool Agent::isTinyEye(int eyenumber) const
{
	if(eyenumber<NUMEYES/2) return true;
	return false;
}

bool Agent::isAsexual() const
{
	if(sexproject>0.5) return false;
	return true;
}

bool Agent::isMale() const
{
	if(sexproject>1.0) return true;
	return false;
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

void Agent::addDamage(const char * sourcetext, float amount)
{
	std::string newtext= std::string(sourcetext);
	addDamage(newtext, amount);
}

void Agent::addDamage(std::string sourcetext, float amount)
{
	//this method now handles subtraction of health as well as checking if the agent died
	#pragma omp critical //protect us from ourselves... collapse any threads for 
	if(amount!=0){
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