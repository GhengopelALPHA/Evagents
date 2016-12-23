#ifndef AGENT_H
#define AGENT_H

#include "DRAWSBrain.h"
#include "vmath.h"

#include <vector>
#include <string>

class Agent
{
//IMPORTANT: if ANY variables are added/removed, you MUST check ReadWrite.cpp to see how loading and saving will be effected!!!
public:
	Agent(int NUMBOXES, float MEANRADIUS, float REPRATE, float MUTARATE1, float MUTARATE2);

	void printSelf();
	//for drawing purposes
	void initEvent(float size, float r, float g, float b);
	
	void tick();
	void setActivity();
	void writeIfKilled(const char * cause);

	Agent reproduce(Agent that, float REPRATE);

	//random agent creation tweakers
	void setHerbivore();
	void setCarnivore();
	void setFrugivore();
	void setPos(float x, float y);
	void borderRectify();

	bool isHerbivore() const;
	bool isCarnivore() const;
	bool isFrugivore() const;
	bool isTerran() const;
	bool isAquatic() const;
	bool isSpikey(float SPIKELENGTH) const;
	
	//Variables
	//bot basics
	int id;
	Vector2f pos;
	float angle; //of the bot
	float health; //in range [0,2]. I cant remember why.
	int age; //how old is the agent
	float MUTRATE1; //how often do mutations occur?
	float MUTRATE2; //how significant are they?
	float radius; //radius of bot

	//triggers, counters, and layer interaction
	bool near; //are we near any other bots? Used to "tree" the agent-agent code
	int freshkill; //were you just stabbed/collided with? how long ago?
	int species; //if two bots are of significantly different species, then they can't crossover
	int gencount; //generation counter
	float repcounter; //when repcounter gets to 0, this bot reproduces
	int numbabies; //number of children this bot creates with every reproduction event
	float temperature_preference; //what temperature does this agent like? [0 to 1]
	float lungs; //what type of environment does this agent need? [0 for "water", 1 for "land"]
	float metabolism; //rate modifier for food to repcounter conversion, also, a factor of max bot speed
	float stomach[Stomach::FOOD_TYPES]; //stomach: #0 is herbivore, #1 is carnivore, #2 is frugivore

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

	//the brain
	DRAWSBrain brain;
	std::vector<float> in; //see Input in settings.h
	std::vector<float> out; //see Output in settings.h
	float brainact; //records the activity of the brain
	
	//outputs
	float w1; //wheel speeds. in range [-1,1]
	float w2;
	float strength; //how "strongly" the wheel speeds are pushed towards their outputs. Or their muscle strength, whatever. In range [0,2]
	bool boost; //is this agent boosting?
	float jump; //what "height" this bot is at after jumping
	float red, gre, blu; //colors of the
	float volume; //sound volume of this bot. It can scream, or be very sneaky.
	float tone; //sound tone of this bot. it can be low pitched (<0.5) or high (>0.5), where only bots with hearing in range of tone will hear
	float give;	//is this agent attempting to give food to other agent?
	float spikeLength; //"my, what a long spike you have!"
	float jawPosition; //what "position" the jaw is in. 0 for open, 1 for closed
	float jawOldPos; //the previous "position" of the jaw
	int grabID; //id of agent this agent is "glued" to. ==-1 if none selected
	float grabbing; //is this agent attempting to grab another? If already grabbed, how far are we willing to let them go?
	bool sexproject; //is this bot trying to give out its genetic data?
	
	//stats
	bool hybrid; //is this agent result of crossover?
	const char * death; //the cause of death of this agent
	int children; //how many kids did you say you had again?
	int killed; //how many bots have you murdered???
	int hits; //how much fighting have you done?
	float indicator;
	float ir, ig, ib; //indicator colors
	float dfood; //what is change in health of this agent due to giving/receiving?
	float dfoodx; //x and y location of other bot
	float dfoody;
	float grabx; //x and y location of the grab target
	float graby;
	int jawrend; //render counter for jaw. Past ~10 ticks of no jaw action, it is "retracted" visually
	std::vector<std::string> mutations;
};

#endif // AGENT_H
