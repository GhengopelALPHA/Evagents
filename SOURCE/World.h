#ifndef WORLD_H
#define WORLD_H

#include "Agent.h"
#include "settings.h"
#include <vector>
#include <sstream>

#include <irrKlang.h>
using namespace irrklang;

class World
{
public:
    World();
	World(int width, int height);
    ~World();

	ISoundEngine* audio; //audio engine, provided by irrKlang. See http://www.ambiera.com/irrklang for details
	bool dosounds; //are we currently allowed to start new sounds?
	bool domusic; //are we allowed to start new music?
	void setAudioEngine(ISoundEngine* a);
	void tryPlayAudio(const char* soundFileName, float x= 0, float y= 0, float pitch= 1.0, float volume= 1.0);
	int last5songs[5]; //tracker for last 5 songs based on index that we played, to prevent recent repeats
	ISound* currentsong;
	int timenewsong; //timer. Gets set when a song ends, and when it hits 0, a new song starts
    
	void init();
	void readConfig();
	void writeConfig();

	//setup functions
	void setSeed(unsigned int seed);
    void reset();
	void sanitize();
	void spawn();
	void update();
	void cellsRoundTerrain();
    
	//world status
    bool isClosed() const;
    void setClosed(bool close);
	bool isDrought() const;
	bool isOvergrowth() const;
	bool isIceAge() const;
	bool isHadean() const;
	bool isExtreme() const;
	bool isDemo() const;
	void setDemo(bool state);

	//debug stuff
	bool isDebug() const;
	void setDebug(bool state);
	std::vector<Vector2f> linesA;
	std::vector<Vector2f> linesB;

	//following and selected agent stuff
	int getSelectedID() const;

	float pleft;
	float pright;
	bool pcontrol;
	void setControl(bool state);

	void setSelection(int type);
	bool setSelectionRelative(int posneg);
	void setSelectedAgent(int idx = -1);
	int getSelectedAgentIndex() const;
	Agent* getSelectedAgent();
	int getClosestRelative(int idx = -1);
	void selectedHeal();
	void selectedKill();
	void selectedBabys();
	void selectedMutate();
	void selectedStomachMut();
	void selectedPrint();
	void selectedTrace(int mode);
	void selectedInput(bool state);
	void getFollowLocation(float &xi, float &yi);

	bool isAutoselecting() const;
	void setAutoselect(bool state);

	int getEpoch() const;
	int getDay() const;
	float getLowTemp();
	float getHighTemp();
    
    //mouse interaction
	bool processMouse(int button, int state, int x, int y, float scale);
    
	//graph stuff
	std::vector<int> graphGuides;
	std::vector<int> numTotal;
	std::vector<int> numDead;
	std::vector<int> numHerbivore;
	std::vector<int> numCarnivore;
	std::vector<int> numFrugivore;
	std::vector<int> numTerrestrial;
	std::vector<int> numAmphibious;
	std::vector<int> numAquatic;
	std::vector<int> numHybrid;

	//counters
	int modcounter;
    int current_epoch;
    int idcounter;

	//the agents!
	std::vector<Agent> agents;
	Agent loadedagent;

	void processWorldTick(); //handle all tick/day/report/epoch opperations
	void processNewEpoch();
	void processClimate();
	void processMutationEvent();
	void processReporting();
	void processCells(bool prefire= false);
	void tryPlayMusic(); //CONTROL.CPP!!!
	void setInputs();
	void brainsTick();  //takes in[] to out[] for every agent
	void processCounters(); //handle most agent counter variables and do other stuff before processOutputs and healthTick
    void processOutputs(bool prefire= false); //prefire used to run post-load sim restoration before restart
	void healthTick(); //process agent health
	void processReproduction(); //handle all agent's reproduction needs
	void processCellInteractions(); //does interactions of agents with cells
	void processAgentInteractions(); //does interactions of agents with agents
	void processDeath(); //manage the distribution of meat, alerts, and death system functions
	void processRandomSpawn(); //handle spawning of random agents; gotta keep the world filled!

	void addAgent(Agent &agent);
	bool addLoadedAgent(float x, float y);
	void addAgents(int num, int set_stomach=-1, bool set_lungs=true, bool set_temp_pref=true, float nx=-1, float ny=-1);

	float calcTempAtCoord(float y); //calculate the temperature at any y-coord
	float calcTempAtCoord(int worldcy); //calculate temp at coord, using cell coord

	std::vector<std::string> deaths; //record of all the causes of death this epoch

	std::vector<float> selectedSounds; //a list of all the sounds heard by the agent. the int-value /100 is the volume, the float-value is the tone. Visual only

	std::vector<std::pair <std::string ,std::pair <int,int> > > events; //short-term record of events happening in the sim. includes text, type, and counter
	void addEvent(std::string text, int type= 0); //adds event to the event list. max of ~40 characters
	void addDemoEvent(std::string text, int type, int agentIndex); //post an event only if we: A: are in demo mode, and B: have the current agent selected that matches the index passed
	void dismissNextEvents(int count= 1); //dismisses next [count] events (starts their disappearing act, doesn't delete them!)

	void setStatsAfterLoad();

	//helper functions to give us counts of agents and cells
    int getHerbivores() const;
	int getCarnivores() const;
	int getFrugivores() const;
	int getLungLand() const;
	int getLungAmph() const;
	int getLungWater() const;
    int getAgents() const;
	int getHybrids() const;
	int getSpiky() const;
	int getAlive() const;
	int getDead() const;
	int getMutations();
	int getFood() const;
	int getFruit() const;
	int getMeat() const;
	int getHazards() const;
	float getLandRatio() const;
	float getFoodSupp() const;
	float getFruitSupp() const;
	float getMeatSupp() const;
	float getHazardSupp() const;

	//cells; replaces food layer, can be expanded (4 layers currently)
	//[LAYER] represents current layer, see settings.h for ordering
	int CW;
	int CH;
	float cells[Layer::LAYERS][conf::WIDTH/conf::CZ][conf::HEIGHT/conf::CZ]; //[LAYER][CELL_X][CELL_Y]
	std::vector<std::vector<float> > Tcells[Layer::LAYERS]; //test cell layer: array of 2-d vectors. Array portion is immutable layer data. 2-d size is adjustable

	//public stats
	int STAThighestgen; //highest and lowest generation (excluding gen 0 unless that's all there is)
	int STATlowestgen;
	float STATinvgenrange; //range of generation values, with high-gen forcast, inverted (1/this)

	//reloadable "constants"
	int MIN_PLANT;
	float INITPLANTDENSITY;
	int INITPLANT;
	float INITFRUITDENSITY;
	int INITFRUIT;
	float INITMEATDENSITY;
	int INITMEAT;
	float INITHAZARDDENSITY;
	int INITHAZARD;

	int AGENTS_MIN_NOTCLOSED;
	int AGENTS_MAX_SPAWN;
	int AGENTSPAWN_FREQ;
	int AGENTS_MAX_NOOXYGEN;

	int REPORTS_PER_EPOCH;
	int FRAMES_PER_EPOCH;
	int FRAMES_PER_DAY;

	bool NO_TIPS; //if the config value is set true, no tips will be displayed
	int CONTINENTS;
	float OCEANPERCENT;
	bool SPAWN_LAKES;
	bool DISABLE_LAND_SPAWN;
	bool MOONLIT;
	float MOONLIGHTMULT; //saved multiplier of the desired moonlight mult
	bool DROUGHTS;
	float DROUGHTMULT; //saved multiplier of the current epoch drought/overgrowth state
	float DROUGHT_MIN;
	float DROUGHT_MAX;
	bool MUTEVENTS;
	int MUTEVENTMULT; //saved multiplier of the current epoch mutation chance & count multiplier (min always 1)
	int MUTEVENT_MAX;
	bool CLIMATE;
	bool CLIMATE_AFFECT_FLORA;
	float CLIMATEBIAS; //saved bias of the current epoch climate state. This can push the entire planet towards 0 or 1 temperature
	float CLIMATEMULT; //saved multiplier of the current epoch climate state. This can blend the poles and equator closer to CLIMATEBIAS
	float CLIMATEMULT_AVERAGE;
	float CLIMATE_INTENSITY;
	float GRAVITYACCEL;
	float BUMP_PRESSURE;
	float GRAB_PRESSURE;
	int BRAINBOXES;
	int BRAINCONNS;
	float WHEEL_SPEED;
	float BOOSTSIZEMULT;
	float BOOSTEXAUSTMULT;
	int CORPSE_FRAMES;
	float CORPSE_MEAT_MIN;
	float SOUNDPITCHRANGE;

	float MEANRADIUS;
	float FOODTRANSFER;
	float BASEEXHAUSTION;
	float EXHAUSTION_MULT_PER_OUTPUT;
	float EXHAUSTION_MULT_PER_CONN;
	float SPIKESPEED;
	bool SPAWN_MIRROR_EYES;
	bool PRESERVE_MIRROR_EYES;

	int FRESHKILLTIME;
	int TENDERAGE;
	float MINMOMHEALTH;
	float MIN_INTAKE_HEALTH_RATIO;
	bool FUN;
	float SUN_RED;
	float SUN_GRE;
	float SUN_BLU;
	float REP_PER_BABY;
	float OVERHEAL_REPFILL;
//	float LEARNRATE;
	int OVERRIDE_KINRANGE;
	float DEFAULT_MUTCHANCE;
	float DEFAULT_MUTSIZE;
	float LIVE_MUTATE_CHANCE;
	int MAXAGE;
	int MAXWASTEFREQ;

	float DIST;
	float SPIKELENGTH;
	float TOOCLOSE;
	float FOOD_SHARING_DISTANCE;
	float SEXTING_DISTANCE;
	float GRABBING_DISTANCE;
	float NEAREST;

	float HEALTHLOSS_WHEELS;
	float HEALTHLOSS_BOOSTMULT;
	float HEALTHLOSS_BADTEMP;
	float HEALTHLOSS_AGING;
	float HEALTHLOSS_EXHAUSTION;
	float HEALTHLOSS_BRAINUSE;
	float HEALTHLOSS_SPIKE_EXT;
	float HEALTHLOSS_BADTERRAIN;
	float HEALTHLOSS_NOOXYGEN;
	float HEALTHLOSS_ASSEX;

	float DAMAGE_FULLSPIKE;
	float DAMAGE_COLLIDE;
	float DAMAGE_JAWSNAP;

	float STOMACH_EFF;

	float FOODINTAKE;
	float FOODDECAY;
	float FOODGROWTH;
	float FOODWASTE;
	int FOODADDFREQ;
	float FOODSPREAD;
	int FOODRANGE;

	float FRUITINTAKE;
	float FRUITDECAY;
	float FRUITWASTE;
	int FRUITADDFREQ;
	float FRUITREQUIRE;

	float MEATINTAKE;
	float MEATDECAY;
	float MEATWASTE;
	float MEATVALUE;

	int HAZARDFREQ;
	float HAZARDEVENT_MULT;
	float HAZARDDECAY;
	float HAZARDDEPOSIT;
	float HAZARDDAMAGE;
	float HAZARDPOWER;

	std::vector<std::string> tips;//list of tips to display every once in a while (more frequently at epoch=0)
    
private:
    void writeReport();
	    
	void reproduce(int ai, int bi);

	void cellsRandomFill(int layer, float amount, int number);
	void cellsLandMasses();
	void findStats(); //finds the world's current stats (population counts, best gens, etc)

	unsigned int SEED; //THE random seed for the entire sim.
    bool CLOSED; //if environment is closed, then no random bots or food are added per time interval
	bool DEMO; //if demo mode active, we don't save report until it gets turned off, which happens automatically at Epoch 1. Also settings.cfg-controllable
	bool DEBUG; //if debugging, collect additional data, print more feedback, and draw extra info
	bool AUTOSELECT; //if autoselecting, the agent which we are newly following gets selected
	int SELECTION; //id of selected agent

	//Stats and acheivements
	int STATherbivores; //count of the different stomach types
	int STATfrugivores;
	int STATcarnivores;
	int STATterrans; //count of land- and sea-dwelling agents
	int STATamphibians;
	int STATaquatic;
	int STATalive; //count of alive, not total
	int STATdead; //count of dead agents
	int STATlivemutations; //count of live mutations occured
	int STATspiky; //count of spikey agents
	int STAThybrids; //count of hybrid (sexually produced) babies
	int STATbestherbi; //best generation of the different stomach types and land types
	int STATbestfrugi;
	int STATbestcarni;
	int STATbestterran;
	int STATbestamphibious;
	int STATbestaquatic;
	int STATbesthybrid; //best gen hybrid
	int STATplants; //count of plant cells over 50%
	int STATfruits; //count of fruit cells over 50%
	int STATmeats; //count of meat cells over 50%
	int STAThazards; //count of hazard cells over 50%
	float STATlandratio; //saved ratio of land/water
	float STATallplant; //exact number sum of all plant matter
	float STATallfruit; //exact number sum of all fruit matter
	float STATallmeat; //exact number sum of all plant matter
	float STATallhazard; //exact number sum of all plant matter

	bool STATuserseengenerosity; //true if the user was shown an event when an agent sent health via generosity
	bool STATuserseenjumping; //true if the user has seen jumping
	bool STATuseracted; //true if the user took control of an agent
	bool STATfirstspecies; //true if we have had at least one agent with a generation =5 during a report
	bool STATfirstpopulation; //true if we had a population count of > AGENTS_MAX_SPAWN
	bool STATfirstglobal; //true if we had max gens >= 5 for terran and aquatic
	bool STATstrongspecies; //true if we had a max gen count of > 500
	bool STATstrongspecies2; //true if we had a max gen count of > 1000
	bool STATwildspecies; //true if we had a speciesID > the default range value
	bool STATallachieved; //true if all the above are true
};

#endif // WORLD_H
