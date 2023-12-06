#pragma once

#include "CPBrain.h"
#include "Gene.h"

#include "vmath.h"
#include <vector>
#include <string>
#include <algorithm>

struct Eye
{
    Eye(float maxfov) : dir(randf(0, 2 * M_PI)), fov(std::min(maxfov, static_cast<float>(std::abs(randn(0.0, conf::EYE_SENSE_FOV_STD_DEV))))), type(0) {}
    Eye(float dir, float fov, int type) : dir(dir), fov(fov), type(type) {}

    float dir;
    float fov;
    int type;
};

struct Ear
{
    Ear() : dir(randf(0, 2 * M_PI)), low(randf(0, 1)), high(randf(0, 1)) { order(); }
    Ear(float dir, float low, float high) : dir(dir), low(low), high(high) { order(); }

    void order() {
        if (low > high) std::swap(low, high);
    }

    float dir;
    float low;
    float high;
};

class Agent
{
//IMPORTANT: if ANY variables are added/removed, you MUST check ReadWrite.cpp to see how loading and saving will be effected!!!
public:
	Agent(
		int NUMBOXES, 
		int NUMINITCONNS, 
		bool SPAWN_MIRROR_EYES,
		float EYE_SENSE_MAX_FOV,
		float BRAIN_MUTATION_CHANCE, 
		float BRAIN_MUTATION_SIZE,
		float GENE_MUTATION_CHANCE,
		float GENE_MUTATION_SIZE
	);
	Agent();
		
	// --- Design Note: --- //
	// There are Stats, and there are Traits/Genes.
	// - Stats are initialized at spawn/reproduction, and OFTEN change. They are SOMETIMES saved, but since they 
	//    USUALLY are recalculated every tick, we decide case-by-case.
	// - Traits/Genes are calculated at spawn/reproduction, and aside from mutations, RARELY change. They are ALWAYS saved.
	//    The Brain is a kind of trait that is not controlled by genes but rather is it's own thing. It is DEFINITELY saved.

	//Traits/Genes:
	std::vector<Gene> genes; //list of Genes. Inherited, mutable
	int counts[Trait::TRAIT_TYPES]; //list of traits by number of counts they appear in list of Genes. Used to apply average and mutation deletions
	float traits[Trait::TRAIT_TYPES]; //the final list of trait values. These are added to by Genes and then divided by the number of genes

	std::vector<Eye> eyes; //collection of eye structs
	std::vector<Ear> ears; //collection of ear structs

	//the BRAIN!!!
	CPBrain brain;
	std::vector<float> in; //see Input in settings.h
	std::vector<float> out; //see Output in settings.h


	//Stats:
	// - simulation basics
	int id;  // unique identifier of this agent. NOTE: Load/Save does not NEED this; all agents are re-created trait-by-trait on new templates
			// some scenarios that it might want to be saved/loaded: 
			// - Grab uses id to determine who is targeted - fixed by running prefire updates when loading
			// - preserving selected agent - determined to be unimportant
	Vector3f pos; // physical position within the world x & y, z is used for jump height
	Vector3f dpos; // previous position of the agent. For rendering only! Important Note: this is NOT SAVED, but is LOADED, with the pos values
	float angle; // angle of the bot in the world, from the positive x-axis I think

	// - counters & triggers
	//   - saved:
	float health; //in range [0,HEALTH_CAP].
	float maxrepcounter; //value of maximum (initial and reset) repcounter
	float repcounter; //when repcounter gets to 0, this bot reproduces
	float exhaustion; //sum of this agent's outputs over time, reduced by a constant. If this gets too high, the agent suffers
	int age; //how old is the agent, in 1/10ths
	int gencount; //generation counter
	int parentid; //who's your momma? Note that like mitochondrial DNA, this is only passed from mother to children
	int freshkill; //were you just stabbed/collided with? if so, how long ago?
	int carcasscount; //counter for how long a dead agent stays on the world. Is reset as long as there is meat under the agent
	bool hybrid; //is this agent result of crossover?

	//   - unsaved, are recalculated at load
	float discomfort; //what level of temperature discomfort this agent is currently experiencing [0,1]
	int encumbered; //is this agent experiencing any encumbering effects (childbearing, difficult terrain), and if so, how many?
	int jaw_render_timer; //render counter for jaw. Past ~10 ticks of no jaw action, it is "retracted" visually
	float centerrender; //alpha of the agent's center. This is changed slowly by current reproduction mode: asexual= 0, clear; sexual (F)= 1.0, (M)= 2.0
	float indicator;
	float ir, ig, ib; //indicator colors
	bool near; //are we near any other bots? Used to "tree" the agent-agent code
	float dhealth; //what is change in health of this agent due to giving/receiving?
	float dhealthx; //x and y location of other bot - UNUSED CURRENTLY
	float dhealthy;
	float grabx; //x and y location of the grab target. Visuals-affecting only
	float graby;
	int children; //how many kids did you say you had again?
	int killed; //how many (secret) agents have you murdered???
	int hits; //you should see the other guy... counts attacks on other agents

	// - lists & death cause - unsaved
	std::vector<std::string> mutations;
	std::vector<std::pair<std::string, float>> damages; //tracker for sources of injury
	std::vector<std::pair<std::string, float>> intakes; //tracker for sources of intake
	std::string death; //the cause of death of this agent. Do not load-save without handling spaces
	
	// - outputs - unsaved
	float w1; //wheel speeds. in range [-1,1]
	float w2;
	bool boost; //is this agent boosting?
	float zvelocity; //velocity of the vertical (z-axis) movement produced by jumping
	float real_red, real_gre, real_blu; //real colors of the agent
	float volume; //sound volume of this bot. It can scream, or be very sneaky.
	float tone; //sound tone of this bot. it can be low pitched (<0.5) or high (>0.5), where only bots with hearing in range of tone will hear
	float give;	//is this agent attempting to give food to other agent?
	float spikeLength; //"my, what a long spike you have!"
	float jawPosition; //what "position" the jaw is in. 0 for open, positive for activated (apply damage), negative for opening (going back to 0)
	float jawMaxRecent; //what jaw value was recently applied? Used for display and for chewing feature
	float jawOldOutput; //the previous output of the jaw
	int grabID; //id of agent this agent is "glued" to. =-1 if none selected
	float grabbing; //is this agent attempting to grab another? If already grabbed, how far are we willing to let them go?
	float grabangle; //the position of this bot's grab. Other agents can only be grabbed at this angle, and are drawn to this point specifically once grabbed
	float sexproject; //is this bot trying to give out its genetic data? if so, how strongly? in range [0,2] (bias+output), considered 'father' if >1.0
	float clockf3; //frequency of the third clock, which can change via an output

	//Functions
	void populateGenes(
		float MEANRADIUS,
		float REP_PER_BABY,
		float BRAIN_MUTATION_CHANCE,
		float BRAIN_MUTATION_SIZE,
		float GENE_MUTATION_CHANCE,
		float GENE_MUTATION_SIZE,
		int OVERRIDE_KINRANGE
	); //give the agent some initial random genes. Run this at init only
	void populateStomach();
	void countGenes(); // counts up the number of each gene type and adds the value they have to traits (use before mutation and expression)
	void expressGenes(); //process genes list and set/update agent traits & abilities. Run this to get the trait list updated
	 //forces all traits to be specific default values used in situations where no genes encode for it
	void mutateGenesBirth(float basechance, float basesize, int OVERRIDE_KINRANGE); //mutate all genes with a given base chance and size at births
	void mutateGenesLive(float basechance, float basesize, int OVERRIDE_KINRANGE); //mutate some genes with a given base chance and size for live agents

	void printSelf(); // print agent details
	void initSplash(float size, float r, float g, float b); // start an indicator
	void traceBack(int outback=0); // trace back important contributing conns for a given output. UNUSED CURRENTLY
	
	void tick();
	float getActivity() const;
	float getOutputSum() const;
	float getWheelOutputSum() const;

	void addDamage(const char * sourcetext, float amount);
	void addDamage(std::string sourcetext, float amount);
	std::pair<std::string, float> getMostDamage() const;
	void addIntake(const char * sourcetext, float amount);
	void addIntake(std::string sourcetext, float amount);
	void writeIfKilled();

	Agent reproduce(
		Agent that,
		bool PRESERVE_MIRROR_EYES,
		int OVERRIDE_KINRANGE,
		float MEANRADIUS,
		float REP_PER_BABY,
		int baby,
		float EYE_SENSE_MAX_FOV
	);
	void resetRepCounter(float MEANRADIUS, float REP_PER_BABY);

	void liveMutate(int MUTMULT = 1, int OVERRIDE_KINRANGE = -1);

	//random agent creation tweakers
	void setHerbivore();
	void setCarnivore();
	void setFrugivore();
	void setPos(float x, float y);
	void setPos(float x, float y, float z);
	void setPosRandom(float maxx, float maxy);
	void borderRectify();

	void setIdealTempPref(float temp= -1);
	void setIdealLungs(float target);

	bool isDead() const;
	bool isHerbivore() const;
	bool isCarnivore() const;
	bool isFrugivore() const;
	bool isTerrestrial() const;
	bool isAmphibious() const;
	bool isAquatic() const;
	bool isSpiky(float MAX_SPIKE_LENGTH) const;
	bool isSpikedDist(float MAX_SPIKE_LENGTH, float d) const;
	bool isBitey() const;
	bool isTiny() const;
	bool isTinyEye(int eyenumber) const;
	bool isAsexual() const;
	bool isMale() const;
	int getRepType() const;
	bool isGrabbing() const;
	bool isGiving() const;
	bool isSelfish(float MAXSELFISH) const;
	bool isAirborne() const;
};
