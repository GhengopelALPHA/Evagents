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
    ~World();
    
    void update();
    void reset();
	void spawn();
	void readConfig();
	void writeConfig();
    
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

	int pinput1;
	float pleft;
	float pright;
	bool pcontrol;
	void setControl(bool state);

	void setSelection(int type);
	void setSelectedAgent(int idx = -1);
	int getSelectedAgent() const;
	int getClosestRelative(int idx = -1) const;
	void selectedHeal();
	void selectedKill();
	void selectedBabys();
	void selectedMutate();
	void getFollowLocation(float &xi, float &yi);

	bool isAutoselecting() const;
	void setAutoselect(bool state);

	int epoch() const;
    
    //mouse interaction
	bool processMouse(int button, int state, int x, int y, float scale);

	void addAgents(int num, int set_stomach=-1, float nx=-1, float ny=-1, bool set_lungs=true);
    
	//graph stuff
    int numHerbivore[conf::RECORD_SIZE];
	int numCarnivore[conf::RECORD_SIZE];
	int numFrugivore[conf::RECORD_SIZE]; 
	int numHybrid[conf::RECORD_SIZE];
	int numDead[conf::RECORD_SIZE];
	int numTotal[conf::RECORD_SIZE];
	int ptr;

	//counters
	int modcounter;
    int current_epoch;
    int idcounter;

	//the agents!
	std::vector<Agent> agents;

	void setInputs();
	void brainsTick();  //takes in[] to out[] for every agent
    void processOutputs();
	void processInteractions(); //does the final causes of death and interactions with cells and agents

	void healthTick();

	std::vector<const char *> deaths; //record of all the causes of death this epoch

	std::vector<float> selectedSounds; //a list of all the sounds heard by the agent. the int-value /100 is the volume, the float-value is the tone. Visual only

	std::vector<std::pair <const char *,int> > events; //short-term record of events happening in the sim. counters for events. all start at 100, finish at -100
	void addEvent(const char * text); //adds event to the event list. max of ~40 characters

	//helper functions to give us counts of agents and cells
    int getHerbivores() const;
	int getCarnivores() const;
	int getFrugivores() const;
	int getLungLand() const;
	int getLungWater() const;
    int getAgents() const;
	int getHybrids() const;
	int getSpiked() const;
	int getAlive() const;
	int getDead() const;
	int getFood() const;
	int getFruit() const;
	int getMeat() const;
	int getHazards() const;
	float getFoodSupp() const;
	float getFruitSupp() const;
	float getMeatSupp() const;
	float getHazardSupp() const;

	//cells; replaces food layer, can be expanded (4 layers currently)
	//[LAYER] represents current layer, see settings.h for ordering
	int CW;
	int CH;
	float cells[Layer::LAYERS][conf::WIDTH/conf::CZ][conf::HEIGHT/conf::CZ]; //[LAYER][CELL_X][CELL_Y]

	//reloadable "constants"
	int MINFOOD;
	float INITFOODDENSITY;
	int INITFOOD;
	float INITFRUITDENSITY;
	int INITFRUIT;

	int NUMBOTS;
	int ENOUGHBOTS;
	int TOOMANYBOTS;

	int REPORTS_PER_EPOCH;
	int FRAMES_PER_EPOCH;
	int FRAMES_PER_DAY;

	int CONTINENTS;
	float OCEANPERCENT;
	bool MOONLIT;
	float GRAVITYACCEL;
	float REACTPOWER;
	float SPIKEMULT;
	int BRAINSIZE;
	float BOTSPEED;
	float BOOSTSIZEMULT;
	float SOUNDPITCHRANGE;
	float FOODTRANSFER;
	float MEANRADIUS;
	float SPIKESPEED;
	int FRESHKILLTIME;
	int TENDERAGE;
	float MINMOMHEALTH;
	bool FUN;
	float REPRATE;
	bool OVERHEAL_REPFILL;
//	float LEARNRATE;
	float MAXDEVIATION;
	float MUTCHANCE;
	float MUTSIZE;
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
	float HEALTHLOSS_BUMP;
	float HEALTHLOSS_SPIKE_EXT;
	float HEALTHLOSS_BADAIR;
	float HEALTHLOSS_NOOXYGEN;
	float HEALTHLOSS_ASSEX;
	float HEALTHLOSS_JAWSNAP;

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
	float HAZARDDECAY;
	float HAZARDDEPOSIT;
	float HAZARDDAMAGE;
    
private:
    void writeReport();
	    
    void reproduce(int ai, int bi);

	void cellsRandomFill(int layer, float amount, int number);
	void cellsLandMasses();
	void findStats(); //finds the world's current stats (population counts, best gens, etc)

	std::vector<std::string> tips;//list of tips to display every once in a while (more frequently at epoch=0)

    bool CLOSED; //if environment is closed, then no random bots or food are added per time interval
	bool DEBUG; //if debugging, collect additional data, print more feedback, and draw extra info
	bool AUTOSELECT; //if autoselecting, the agent which we are newly following gets selected
	int SELECTION; //id of selected agent

	int STATherbivores; //count of the different stomach types
	int STATfrugivores;
	int STATcarnivores;
	int STATterrans; //count of land- and sea-dwelling agents
	int STATaquatic;
	int STATalive; //count of alive and dead, not total
	int STATdead; 
	int STATspiked; //count of spikey agents
	int STAThybrids; //count of hybrid (sexually produced) babies
	int STATbestherbi; //best generation of the different stomach types and land types
	int STATbestfrugi;
	int STATbestcarni;
	int STATbestterran;
	int STATbestaquatic;
	int STATbesthybrid; //best gen hybrid
	int STATplants; //count of plant cells over 75%
	int STATfruits; //count of fruit cells over 50%
	int STATmeats; //count of meat cells over 50%
	int STAThazards; //count of hazard cells over 50%
	float STATallplant; //exact number sum of all plant matter
	float STATallfruit; //exact number sum of all fruit matter
	float STATallmeat; //exact number sum of all plant matter
	float STATallhazard; //exact number sum of all plant matter


};

#endif // WORLD_H
