#ifndef WORLD_H
#define WORLD_H

#include "View.h"
#include "Agent.h"
#include "settings.h"
//#include "ReadWrite.h"
#include <vector>
class World
{
public:
    World();
	World(int width, int height);
    ~World();
    
	void init();
	void readConfig();
	void writeConfig();

    void reset();
	void sanitize();
	void spawn();
	void update();
	
    
    void draw(View* view, int layer);
    
    bool isClosed() const;
    void setClosed(bool close);

	//debug stuff
	bool isDebug() const;
	void setDebug(bool state);
	std::vector<Vector2f> linesA;
	std::vector<Vector2f> linesB;

	//following and selected agent stuff
	void positionOfInterest(int type, float &xi, float &yi);

	int getSelection() const;

	float pleft;
	float pright;
	bool pcontrol;
	void setControl(bool state);

	void setSelection(int type);
	bool setSelectionRelative(int posneg);
	void setSelectedAgent(int idx = -1);
	int getSelectedAgent() const;
	int getClosestRelative(int idx = -1) const;
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
    
    //mouse interaction
	bool processMouse(int button, int state, int x, int y, float scale);
    
	//graph stuff
    int numHerbivore[conf::RECORD_SIZE];
	int numCarnivore[conf::RECORD_SIZE];
	int numFrugivore[conf::RECORD_SIZE];
	int numTerrestrial[conf::RECORD_SIZE];
	int numAmphibious[conf::RECORD_SIZE];
	int numAquatic[conf::RECORD_SIZE];
	int numHybrid[conf::RECORD_SIZE];
	int numDead[conf::RECORD_SIZE];
	int numTotal[conf::RECORD_SIZE];
	int ptr;
	int lastptr;

	//counters
	int modcounter;
    int current_epoch;
    int idcounter;

	//the agents!
	std::vector<Agent> agents;

	void setInputs();
	void brainsTick();  //takes in[] to out[] for every agent
    void processOutputs(bool prefire= false); //prefire used to run post-load sim restoration before restart
	void processInteractions(); //does the final causes of death and interactions with cells and agents

	void healthTick();

	void addAgents(int num, int set_stomach=-1, float nx=-1, float ny=-1, bool set_lungs=true);

	std::vector<const char *> deaths; //record of all the causes of death this epoch

	std::vector<float> selectedSounds; //a list of all the sounds heard by the agent. the int-value /100 is the volume, the float-value is the tone. Visual only

	std::vector<std::pair <const char *,std::pair <int,int> > > events; //short-term record of events happening in the sim. includes text, type, and counter
	void addEvent(const char * text, int type= 0); //adds event to the event list. max of ~40 characters
	void dismissNextEvents(int count= 1); //dismisses next [count] events (starts their disappearing act, doesn't delete them!)

	//helper functions to give us counts of agents and cells
    int getHerbivores() const;
	int getCarnivores() const;
	int getFrugivores() const;
	int getLungLand() const;
	int getLungAmph() const;
	int getLungWater() const;
    int getAgents() const;
	int getHybrids() const;
	int getSpiked() const;
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


	//reloadable "constants"
	int MINFOOD;
	float INITFOODDENSITY;
	int INITFOOD;
	float INITFRUITDENSITY;
	int INITFRUIT;

	int NUMBOTS;
	int ENOUGHBOTS;
	int NOTENOUGHFREQ;
	int TOOMANYBOTS;

	int REPORTS_PER_EPOCH;
	int FRAMES_PER_EPOCH;
	int FRAMES_PER_DAY;

	int CONTINENTS;
	float OCEANPERCENT;
	bool DISABLE_LAND_SPAWN;
	bool MOONLIT;
	bool DROUGHTS;
	float DROUGHTMULT; //saved multiplier of the current epoch drought/overgrowth state
	float DROUGHT_MIN;
	float DROUGHT_MAX;
	bool MUTEVENTS;
	int MUTEVENTMULT; //saved multiplier of the current epoch mutation chance & count multiplier (min always 1)
	int MUTEVENT_MAX;
	float GRAVITYACCEL;
	float BUMP_PRESSURE;
	float GRAB_PRESSURE;
	int BRAINSIZE;
	float BOTSPEED;
	float BOOSTSIZEMULT;
	float SOUNDPITCHRANGE;

	float FOODTRANSFER;
	float BASEEXHAUSTION;
	float EXHAUSTION_MULT;
	float MEANRADIUS;
	float SPIKESPEED;
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
	float MAXDEVIATION;
	float MUTCHANCE;
	float MUTSIZE;
	float LIVE_MUTATE_CHANCE;
	int MAXAGE;

	float DIST;
	float SPIKELENGTH;
	float TOOCLOSE;
	float FOOD_SHARING_DISTANCE;
	float SEXTING_DISTANCE;
	float GRABBING_DISTANCE;

	float HEALTHLOSS_WHEELS;
	float HEALTHLOSS_BOOSTMULT;
	float HEALTHLOSS_BADTEMP;
	float HEALTHLOSS_AGING;
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
    
private:
    void writeReport();
	    
    void reproduce(int ai, int bi);

	void cellsRandomFill(int layer, float amount, int number);
	void cellsLandMasses();
	void findStats(); //finds the world's current stats (population counts, best gens, etc)

	std::vector<std::string> tips;//list of tips to display every once in a while (more frequently at epoch=0)

    bool CLOSED; //if environment is closed, then no random bots or food are added per time interval
	bool DEBUG; //if debugging, collect additional data, print more feedback, and draw extra info
	bool NOTIPS; //if the config value is set true, no tips will be displayed
	bool AUTOSELECT; //if autoselecting, the agent which we are newly following gets selected
	int SELECTION; //id of selected agent
	int EXTREMOPHILE; //type of extremophile currently being seeked (see settings.h)

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
	int STATspiked; //count of spikey agents
	int STAThybrids; //count of hybrid (sexually produced) babies
	int STATbestherbi; //best generation of the different stomach types and land types
	int STATbestfrugi;
	int STATbestcarni;
	int STATbestterran;
	int STATbestamphibious;
	int STATbestaquatic;
	int STATbesthybrid; //best gen hybrid
	int STATplants; //count of plant cells over 75%
	int STATfruits; //count of fruit cells over 50%
	int STATmeats; //count of meat cells over 50%
	int STAThazards; //count of hazard cells over 50%
	float STATlandratio; //saved ratio of land/water
	float STATallplant; //exact number sum of all plant matter
	float STATallfruit; //exact number sum of all fruit matter
	float STATallmeat; //exact number sum of all plant matter
	float STATallhazard; //exact number sum of all plant matter

	bool STATuseracted; //true if the user took control of an agent
	bool STATfirstspecies; //true if we have had at least one agent with a generation =5 during a report
	bool STATfirstpopulation; //true if we had a population count of > ENOUGHBOTS
	bool STATfirstglobal; //true if we had max gens >= 5 for terran and aquatic
	bool STATstrongspecies; //true if we had a max gen count of > 500
	bool STATstrongspecies2; //true if we had a max gen count of > 1000
	bool STATallachieved; //true if all the above are true
};

#endif // WORLD_H
