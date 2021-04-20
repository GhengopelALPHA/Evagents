#ifndef AGENT_H
#define AGENT_H

#include "DRAWSBrain.h"
#include "vmath.h"

#include <vector>
#include <string>
#include <algorithm>

class Agent
{
//IMPORTANT: if ANY variables are added/removed, you MUST check ReadWrite.cpp to see how loading and saving will be effected!!!
public:
	Agent(int NUMBOXES, float MEANRADIUS, float REP_PER_BABY, float MUTARATE1, float MUTARATE2);
	Agent();
		
	//Saved Variables
	//simulation basics
	int id;
	Vector2f pos;
	Vector2f dpos; //UNSAVED
	float angle; //of the bot

	//Genes! WIP
	std::vector<std::pair<int, float> > genes; //NEW genes. First is type, second is value. All Values of same Type get averaged or added

	float MUTCHANCE; //how often do mutations occur?
	float MUTSIZE; //how significant are they?
	int parentid; //who's your momma? Note that like mitochondrial DNA, this is only passed from mother to children
	float radius; //radius of bot
	float gene_red, gene_gre, gene_blu; //genetic color traits of the agent. can be hidden by chamovid= 1
	float chamovid; //how strongly the output colors are overriding the genetic colors. 0= full genes, 1= full output
	float strength; //how "strongly" the wheel speeds are pushed towards their outputs (their muscle strength). In range [0,2]
	int numbabies; //number of children this bot creates with every reproduction event
	float temperature_preference; //what temperature does this agent like? [0 to 1]
	float lungs; //what type of environment does this agent need? [0 for "water", 1 for "land"]
	float metabolism; //rate modifier for food to repcounter conversion, also, a factor of max bot speed
	float stomach[Stomach::FOOD_TYPES]; //stomach, see settings.h for descriptions and order
	float maxrepcounter; //value of maximum (initial and reset) repcounter
	float sexprojectbias; //a physical bias trait, making sexual reproduction easier for some species/members. in range [0,1]
	//senses
	float eye_see_agent_mod;
	std::vector<float> eyefov; //field of view for each eye
	std::vector<float> eyedir; //direction of each eye
	float hear_mod;
	std::vector<float> eardir; //position of ears
	std::vector<float> hearlow; //low values of hearing ranges
	std::vector<float> hearhigh; //high values of hearing ranges
	float clockf1, clockf2, clockf3; //the frequencies of the three clocks of this bot
	float blood_mod;
	float smell_mod;
	//the BRAIN!!!
	DRAWSBrain brain;
	std::vector<float> in; //see Input in settings.h
	std::vector<float> out; //see Output in settings.h


	//counters, triggers, and layer interaction
	//saved
	float health; //in range [0,2]. I cant remember why.
	int age; //how old is the agent, in 1/10ths
	int freshkill; //were you just stabbed/collided with? if so, how long ago?
	int species; //if two bots are of significantly different species, then they can't crossover
	int kinrange; //range from the species ID that this agent will actively reproduce with
	int gencount; //generation counter
	float repcounter; //when repcounter gets to 0, this bot reproduces
	float exhaustion; //sum of this agent's outputs over time, reduced by a constant. If this gets too high, the agent suffers
	int carcasscount; //counter for how long a dead agent stays on the world. Is reset as long as there is meat under the agent
	bool hybrid; //is this agent result of crossover?

	//unsaved, are recalculated as needed
	float discomfort; //what level of temperature discomfort this agent is currently experiencing [0,1]
	bool encumbered; //is this agent experiencing any encumbering effects (childbearing, difficult terrain)?
	int jawrend; //render counter for jaw. Past ~10 ticks of no jaw action, it is "retracted" visually
	float centerrender; //alpha of the agent's center. This is changed slowly by current reproduction mode: asexual= 0, clear; sexual (F)= 1.0, (M)= 2.0
	float indicator;
	float ir, ig, ib; //indicator colors
	bool near; //are we near any other bots? Used to "tree" the agent-agent code
	float dhealth; //what is change in health of this agent due to giving/receiving?
	float dhealthx; //x and y location of other bot
	float dhealthy;
	float grabx; //x and y location of the grab target
	float graby;
	int children; //how many kids did you say you had again?
	int killed; //how many (secret) agents have you murdered???
	int hits; //you should see the other guy... counts attacks on other agents
	float brainmutations; //records the number of mutations of the brain
	std::vector<std::string> mutations;
	std::vector<std::pair<std::string, float>> damages; //tracker for sources of injury
	std::vector<std::pair<std::string, float>> intakes; //tracker for sources of intake
	std::string death; //the cause of death of this agent
	
	
	//outputs
	//none are saved b/c they are recalculated at world load
	float w1; //wheel speeds. in range [-1,1]
	float w2;
	bool boost; //is this agent boosting?
	float jump; //what "height" this bot is at after jumping
	float real_red, real_gre, real_blu; //real colors of the agent
	float volume; //sound volume of this bot. It can scream, or be very sneaky.
	float tone; //sound tone of this bot. it can be low pitched (<0.5) or high (>0.5), where only bots with hearing in range of tone will hear
	float give;	//is this agent attempting to give food to other agent?
	float spikeLength; //"my, what a long spike you have!"
	float jawPosition; //what "position" the jaw is in. 0 for open, 1 for closed
	float jawOldOutput; //the previous output of the jaw
	int grabID; //id of agent this agent is "glued" to. =-1 if none selected
	float grabbing; //is this agent attempting to grab another? If already grabbed, how far are we willing to let them go?
	float grabangle; //the position of this bot's grab. Other agents can only be grabbed at this angle, and are drawn to this point specifically once grabbed
	float sexproject; //is this bot trying to give out its genetic data? if so, how strongly? in range [0,2] (bias+output, considered 'father' if >1.0

	//WIP
	void exhibitGenes(); //process genes list and set/update agent traits & abilities

	void printSelf();
	void traceBack(int outback=0);

	void initSplash(float size, float r, float g, float b);
	
	void tick();
	float getActivity() const;
	float getOutputSum() const;
	void addDamage(const char * sourcetext, float amount);
	void addDamage(std::string sourcetext, float amount);
	std::pair<std::string, float> getMostDamage() const;
	void addIntake(const char * sourcetext, float amount);
	void addIntake(std::string sourcetext, float amount);
	void writeIfKilled();

	Agent reproduce(Agent that, float MEANRADIUS, float REP_PER_BABY);
	void resetRepCounter(float MEANRADIUS, float REP_PER_BABY);

	void liveMutate(int MUTMULT= 1);

	//random agent creation tweakers
	void setHerbivore();
	void setCarnivore();
	void setFrugivore();
	void setRandomStomach();
	void setPos(float x, float y);
	void setPosRandom(float maxx, float maxy);
	void borderRectify();

	void setIdealTempPref(float temp= -1);
	void setIdealLungs(float target);

	bool isHerbivore() const;
	bool isCarnivore() const;
	bool isFrugivore() const;
	bool isTerrestrial() const;
	bool isAmphibious() const;
	bool isAquatic() const;
	bool isSpikey(float SPIKELENGTH) const;
	bool isTiny() const;
	bool isTinyEye(int eyenumber) const;
	bool isAsexual() const;
	bool isMale() const;
	bool isGrabbing() const;
	bool isGiving() const;
	bool isSelfish(float MAXSELFISH) const;
	bool isAirborne() const;
};

#endif // AGENT_H
