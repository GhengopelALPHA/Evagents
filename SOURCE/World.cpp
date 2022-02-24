#include "World.h"

#include <ctime>

#include "settings.h"
#include "helpers.h"
#include "vmath.h"
#ifdef _WIN32
#define _CRT_SECURE_NO_DEPRECATE
#endif
#include <stdio.h>
#include <iostream>

using namespace std;

World::World() :
		CLOSED(false),
		DEBUG(false),
		DEMO(true),
		SELECTION(-1),
		DROUGHTMULT(1.0),
		MUTEVENTMULT(1),
		CLIMATEBIAS(0.5),
		CLIMATEMULT(0.5),
		pcontrol(false),
		pright(0),
		pleft(0),
		dosounds(true),
		recordlifepath(false),
		timenewsong(10),
		currentsong(0)
{
	//inititalize happens only once when launching program (like right now!)
	init();	
	//reset happens any time we have to clear the world or change variables (now, or when loading a save or starting a new world)
	reset();
	//spawn happens whenever we wish to seed a world with random food, terrain, and bots (now, or after starting a new world)
	spawn();

	printf("WORLD MADE!\n");
	addEvent("Simulation Started!", EventColor::MINT);
}

World::~World()
{

}


void World::setAudioEngine(ISoundEngine* a)
//Control.cpp
{
	audio= a;
	//audio call example: if(dosounds) audio->play2D("sounds/agents/chirp1.ogg");
}


void World::tryPlayAudio(const char* soundFileName, float x, float y, float pitch, float volume)
//Control.cpp
{
	//try to play audio at location x,y if specified (if not, just play 2D audio), and with a specified pitch change multiple
	if(dosounds){ //dosounds is set by GLView and disables when rendering is disabled
		#if defined(_DEBUG)
			if(DEBUG) printf("Trying to play sound '%s' ... ", soundFileName);
		#endif
		#pragma omp critical
		if(soundFileName!=NULL){
			ISound* play = 0;
			if(x==0 && y==0) play= audio->play2D(soundFileName, false, false, true);
			else play= audio->play3D(soundFileName, vec3df(-x, -y, 0), false, false, true);
			//-x,-y because irrklang thinks we're in opposite world if I negate the translates... it's silly
			if(play){
				play->setPlaybackSpeed(pitch);
				play->setVolume(volume);
			}
		}
		#if defined(_DEBUG)
			if(DEBUG) printf("success!\n");
		#endif
	}
}


//void World::tryCullAudio(Controller* control)
////Control.cpp
//{
//	audio->
//	for(int a=0; a<audio->; a++) {
//		ISoundSource* check= audio->getSoundSource(a);
///		if(check){
//			vec3df pos= check->getPosition();
///			if(control->cullAtCoords((int)pos.X, (int)pos.Y)) check->drop();
//		}
//	}
//}


void World::setSeed(unsigned int seed)
//Control.cpp
{
	if(SEED!=seed) srand(seed);
	SEED= seed;
}


void World::reset()
{
	printf("...reticulating splines...\n");
	if(currentsong && !currentsong->getIsPaused()) currentsong->setIsPaused(true);
//	if(randf(0,1)>0.5) timenewsong= randi(100,8000);

	current_epoch= 0;
	modcounter= 0;
	idcounter= 0;

	//try loading constants config. Best do this first in case we change any "constants"!
	readConfig();

	sanitize(); //clear all agents 

	//handle layers
	for(int cx=0; cx<(int)CW; cx++){
		for(int cy=0; cy<(int)CH; cy++){
			for(int l=0;l<Layer::LAYERS;l++){
				cells[l][cx][cy]= 0;
			}
//			cells[TEMPLAYER][cx][cy]= 2.0*abs((float)cy/CH - 0.5); [old temperature indicating code]
		}
	}

	if(!isDemo()){ 
		//open report file; null it up if it exists. ONLY if we are not in demo mode. Mind you, demo mode could get turned off by config above
		FILE* fr = fopen("report.txt", "w");
		fclose(fr);

		STATuserseengenerosity= true;
		STATuserseenjumping= true;
	} else {
		//only reset generosity stat if in demo mode
		STATuserseengenerosity= false;
		STATuserseenjumping= false;
	}

	graphGuides.assign(REPORTS_PER_EPOCH, GuideLine::NONE);
	numTotal.assign(REPORTS_PER_EPOCH, 0);
	numDead.assign(REPORTS_PER_EPOCH, 0);
	numHerbivore.assign(REPORTS_PER_EPOCH, 0);
	numCarnivore.assign(REPORTS_PER_EPOCH, 0);
	numFrugivore.assign(REPORTS_PER_EPOCH, 0);
	numTerrestrial.assign(REPORTS_PER_EPOCH, 0);
	numAmphibious.assign(REPORTS_PER_EPOCH, 0);
	numAquatic.assign(REPORTS_PER_EPOCH, 0);
	numHybrid.assign(REPORTS_PER_EPOCH, 0);

	lifepath.clear();

	//reset achievements
	STATuseracted= false;
	STATfirstspecies= false;
	STATfirstpopulation= false;
	STATfirstglobal= false;
	STATstrongspecies= false;
	STATstrongspecies2= false;
	STATwildspecies= false;
	STATallachieved= false;

	//reset live mutation count
	STATlivemutations= 0;
}

void World::sanitize()
{
	printf("...sanitizing agents...\n");
	agents.clear();
	deaths.clear();
}

void World::spawn()
{
	findStats();//needs to be outside loop to start the loop
	printf("...growing food...\n");
	//handle init resource layers
	float targetplant= (float)INITPLANT/CW/CH;
	float targetfruit= (float)INITFRUIT/CW/CH;
	float targetmeat= (float)INITMEAT/CW/CH;
	float targethazard= (float)INITHAZARD/CW/CH;

	if(targetplant>0.25 || targetfruit>0.25 || targetmeat>0.25 || targethazard>0.25){ 
		//if desired amount of any resource is more than 0.25, let's help random seed by throwing all cells
		for(int cx=0; cx<(int)CW; cx++){
			for(int cy=0; cy<(int)CH; cy++){
				if(targetplant>=1) cells[Layer::PLANTS][cx][cy]= 1.0;
				else if(targetplant<=0) cells[Layer::PLANTS][cx][cy]= 0.0;
				else if(targetplant>0.25) cells[Layer::PLANTS][cx][cy]= cap(randn(targetplant,1-targetplant));

				if(targetfruit>=1) cells[Layer::FRUITS][cx][cy]= 1.0;
				else if(targetfruit<=0) cells[Layer::FRUITS][cx][cy]= 0.0;
				else if(targetfruit>0.25) cells[Layer::FRUITS][cx][cy]= cap(randn(targetfruit,1-targetfruit));

				if(targetmeat>=1) cells[Layer::MEATS][cx][cy]= 1.0;
				else if(targetmeat<=0) cells[Layer::MEATS][cx][cy]= 0.0;
				else if(targetmeat>0.25) cells[Layer::MEATS][cx][cy]= cap(randn(targetmeat,1-targetmeat));

				if(targethazard>=1) cells[Layer::HAZARDS][cx][cy]= 0.9;
				else if(targethazard<=0) cells[Layer::HAZARDS][cx][cy]= 0.0;
				else if(targethazard>0.25) cells[Layer::HAZARDS][cx][cy]= capm(randn(targethazard,1-targethazard), 0, 0.9);

	//			cells[TEMPLAYER][cx][cy]= 2.0*abs((float)cy/CH - 0.5); [old temperature indicating code]
			}
		}
	}

	while(getFood()<INITPLANT || getFruit()<INITFRUIT || getMeat()<INITMEAT || getHazards()<INITHAZARD) {
		//random seeding of cells with init resources
		int rx= randi(0,CW);
		int ry= randi(0,CH);
		if(getFood()<INITPLANT) {
			//germinate plants
			cells[Layer::PLANTS][rx][ry] += randf(0.5, 1.0);
			cells[Layer::PLANTS][rx][ry]= cap(cells[Layer::PLANTS][rx][ry]);
		}
		if (getFruit()<INITFRUIT) { 
			//pollinate fruit
			cells[Layer::FRUITS][rx][ry] = randf(0.5, 1.0);
		}
		if (getMeat()<INITMEAT) {
			//toss meat
			cells[Layer::MEATS][rx][ry] = randf(0.5, 1.0);
		}
		if (getHazards()<INITHAZARD) {
			//hurl waste
			cells[Layer::HAZARDS][rx][ry] = randf(0.5, 0.9);
		}
		findStats();
	}

	//spawn land masses
	cellsLandMasses();

	//init climate settings
	if(CLIMATE) {
		printf("...(un)freezing glaciers...\n");
		CLIMATEBIAS= randf(0.3,0.7);
		CLIMATEMULT= randf(0.4,0.8);
		if(DEBUG) printf("Set Climate Bias to %f, and Climate Multiplier to %f\n", CLIMATEBIAS, CLIMATEMULT);
		if(CLIMATEMULT<0.75) {
			if(CLIMATEMULT<0.25) addEvent("Uniform Temperature generated", EventColor::ORANGE);
			if(CLIMATEBIAS<0.35) addEvent("Cool world generated", EventColor::CYAN);
			else if(CLIMATEBIAS>0.65) addEvent("Warm world generated", EventColor::ORANGE);
		} else addEvent("Extreme Temperature generated", EventColor::ORANGE);
	}

	//add init agents
	printf("...programming agents...\n");
	addAgents(AGENTS_MIN_NOTCLOSED, -2); //-2 creates both herbivores and frugivores alternately
	findStats(); //without this, selecting "New World" in the GUI would infiniloop program somewhere... I'm too tired to figure out where
}


void World::cellsLandMasses()
{
	//creates land masses for the layer given
	int leftcount= CW*CH;

	printf("...clearing land...\n");
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			cells[Layer::ELEVATION][i][j]= -1; //"null" all cells
		}
	}

	if(OCEANPERCENT<1){
		int lastcx, lastcy;
		for (int i=0; i<CONTINENTS*CONTINENT_ROUGHNESS; i++) { //give ourselves more iterations to do extra stuff
			int lowcx= 0;
			int lowcy= 0;
			int highcx= CW;
			int highcy= CH;
			bool setcoast= false;

			if(i%3!=0) { //if we are NOT on one of the core continent iterations (where i divisible by 3, our multiple from above
				lowcx= max(lowcx, lastcx - conf::CONTINENT_SPREAD);
				lowcy= max(lowcy, lastcy - conf::CONTINENT_SPREAD);
				highcx= min(highcx, lastcx + conf::CONTINENT_SPREAD);
				highcy= min(highcy, lastcy + conf::CONTINENT_SPREAD);
			} else if (i>0) {
				//a bit cheeky: if we ARE on a core continent that is NOT the first one, take the midpoint of next cell and last. Put a coastal tile there later!
				setcoast= true;
			}

			int cx=randi(lowcx,highcx);
			int cy=randi(lowcy,highcy);

			cells[Layer::ELEVATION][cx][cy]= (i%2==0 && i<=10) ? Elevation::MOUNTAIN_HIGH : Elevation::HILL;
			//50% of the land spawns are type "Hill", = 0.7, UNLESS we spawned over 10 points already (5 mountains)

			//if told to, find midpoint between last cell and this one, and place water, dividing the CONTINENTS!
			if(setcoast) cells[Layer::ELEVATION][(int)((cx+lastcx)*0.5)][(int)((cy+lastcy)*0.5)]= randf(0,1)>0.5 ? Elevation::SHALLOWWATER : Elevation::DEEPWATER_LOW;

			lastcx= cx;
			lastcy= cy;
		}
	}

	if (OCEANPERCENT > 0) {
		for (int i=0;i<(0.8+CONTINENT_ROUGHNESS)*(powf((float)CW*CH,1/3)/10*pow((float)(CONTINENTS+2),(float)(OCEANPERCENT+1.2))) ;i++) {
			//spawn oceans (water= 0)
			int cx=randi(0,CW);
			int cy=randi(0,CH);
			cells[Layer::ELEVATION][cx][cy]= i<=1 ? Elevation::SHALLOWWATER : Elevation::DEEPWATER_LOW;
		}
	}

	printf("...moving tectonic plates...\n");
	int bumbler = 0;
	while(leftcount!=0){
		bumbler++;
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				float height= cells[Layer::ELEVATION][i][j];
				//land spread
				if (height > Elevation::BEACH_MID){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2); //+2 to correct for randi [x,y)
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[Layer::ELEVATION][ox][oy]==-1 && randf(0,1)>OCEANPERCENT/max(CONTINENT_ROUGHNESS-0.15,0.001)) {
						//chance to spread tied to desired ocean percentage
						if(height <= Elevation::PLAINS){
							//will not reduce level 0.6 and below, allowing us to create beaches (0.5) later
							cells[Layer::ELEVATION][ox][oy]= height;
						} else cells[Layer::ELEVATION][ox][oy]= randf(0,1)>conf::LOWER_ELEVATION_CHANCE ? height : height - 0.1;
						//there's a chance we reduce the next terrain cell by 0.1 in elevation, to produce variation!
					}
				}

				//water spread
				else if (height<Elevation::BEACH_MID && height>=Elevation::DEEPWATER_LOW){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2);
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[Layer::ELEVATION][ox][oy]==-1 && randf(0,1)<OCEANPERCENT*2*(1-2*height)) cells[Layer::ELEVATION][ox][oy]= height;
					//1-2*height comes in here because we want shallow ocean to spread slower than deep ocean so there's less of it and it will get rounded
				}
			}
		}
		
		//count the number of cells left to do (==-1), and do some beach-making
		leftcount= 0;
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				if (cells[Layer::ELEVATION][i][j]==-1){
					leftcount++;

				} else if (cells[Layer::ELEVATION][i][j]>Elevation::BEACH_MID){
					//if this is land, check nearby cells. If 2+ are water, turn into beach
					int watercount= 0;
					if(i==0 || j==0 || i==CW-1 || j==CH-1) watercount= 1; //edges get a helping hand (without this there are weird beaches)
					for(int oi=max(0,i-1);oi<min(CW,i+2);oi++) {
						for(int oj=max(0,j-1);oj<min(CH,j+2);oj++) {

							if(cells[Layer::ELEVATION][oi][oj]<Elevation::BEACH_MID && cells[Layer::ELEVATION][oi][oj]>=Elevation::DEEPWATER_LOW) 
								watercount++;
						}
					}

					if(watercount>=2) cells[Layer::ELEVATION][i][j]= Elevation::BEACH_MID;

				} else if (cells[Layer::ELEVATION][i][j]==Elevation::DEEPWATER_LOW){
					//if this is deep water, check nearby cells. If 1+ is land, turn into shallows
					for(int oi=max(0,i-1);oi<min(CW,i+2);oi++) {
						for(int oj=max(0,j-1);oj<min(CH,j+2);oj++) {

							if(cells[Layer::ELEVATION][oi][oj]>=Elevation::BEACH_MID || 
								(cells[Layer::ELEVATION][oi][oj]>=Elevation::SHALLOWWATER && randf(0,1)<0.008)) {
								cells[Layer::ELEVATION][i][j]= Elevation::SHALLOWWATER;
							}
						}
					}
				}
			}
		}

		//form islands/lakes if leftcount is low and we're missing our target
		float startphase = ISLANDNESS+0.05;
		float endphase = startphase*3/4;
		if(SPAWN_LAKES && OCEANPERCENT>0 && leftcount < startphase*CW*CH && leftcount > endphase*CW*CH) {
			setSTATLandRatio();

			for(int i=0;i<CW;i++) {
				for(int j=0;j<CH;j++) {
					if (cells[Layer::ELEVATION][i][j] == -1 && randf(0,1) < pow(ISLANDNESS, 2)/16 + 0.015) {
						if(STATlandratio - (float)leftcount/CW/CH*0.5 < (1-OCEANPERCENT) && randf(0,1) < (ISLANDNESS/4 + 0.3)) {
							if (randf(0,1) < 0.25) cells[Layer::ELEVATION][i][j] = Elevation::HILL;
							else cells[Layer::ELEVATION][i][j] = Elevation::PLAINS;
						} else cells[Layer::ELEVATION][i][j] = Elevation::SHALLOWWATER;
					}
				}
			}
		}
		if (bumbler > 5000) leftcount = 0;
	}

	for (int n=0; n<FEATURES_TO_SPAWN; n++) {
		//what's this? What's going on? Why is the sky all lit up?
		int cx=randi(20,CW-20);
		int cy=randi(20,CH-20);
		int radius= randi(7,16);
		bool peak = randf(0,1)<0.5;
		bool flatter = randf(0,1)<0.5;

		for(int i=0;i<2*radius;i++) {
			for(int j=0;j<2*radius;j++) {
				//my god, it's a falling star!
				Vector2f vector = Vector2f(i-radius, j-radius);
				float length = vector.length();
				float pression = cells[Layer::ELEVATION][i+cx][j+cy];

				if (length < radius*0.05 && peak) pression += 0.2;
				else if (length < radius*0.1 && peak) pression += 0.1;
				else if (length < radius*0.15 && peak) pression -= 0;
				else if (length < radius*0.25 && peak && !flatter) pression -= 0.1;
				//
				else if (length < radius*0.55 && !flatter) pression -= 0.2;
				else if (length < radius*0.65 && !flatter) pression -= 0.1;
				else if (length < radius*0.7) pression += 0;
				else if (length < radius*0.75) pression += 0.1;
				else if (length < radius*0.85) pression += 0.2;
				else if (length < radius*0.9 && !flatter) pression += 0.3;
				else if (length < radius*0.95) pression += 0.2;
				else if (length < radius) pression += 0.1;
				//N#!P3s%eg9 G9!O&mo7iM I2^E6Om sCVb()S@E\GM)(En!Mg

				if (length < radius) {
					if (pression > cells[Layer::ELEVATION][i+cx][j+cy]) pression -= (float)(randi(0,3))/10;
					if (pression > Elevation::BEACH_MID-0.05 && pression < Elevation::BEACH_MID+0.05) pression = Elevation::BEACH_MID;
				}

				//*WOOOOOOOOSH*
				if (pression > Elevation::DEEPWATER_LOW && pression < Elevation::SHALLOWWATER) pression = Elevation::DEEPWATER_LOW;
				else if (pression > Elevation::SHALLOWWATER && pression < Elevation::BEACH_MID) pression = Elevation::SHALLOWWATER;

				cells[Layer::ELEVATION][i+cx][j+cy] = cap(pression);
			}
		}
	}


	printf("...rolling hills...\n");
	for(int n=0;n<4;n++){
		cellsRoundTerrain();
	}

	setSTATLandRatio();

	#if defined(_DEBUG)
	printf("_DEBUG: Land %%: %.2f. The rest is water.\n",100*STATlandratio);
	#endif

	//briefly allow land spawn for Init ONLY! See update() for long-term
	if(DISABLE_LAND_SPAWN && STATlandratio>0.75){
		DISABLE_LAND_SPAWN= false;
		printf("ALERT: Enabled Land Init Spawns due to high land:water ratio.\n");
	}
}

void World::cellsRoundTerrain()
{
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			int mincounts= 5;
			if(cells[Layer::ELEVATION][i][j]<Elevation::BEACH_MID) mincounts= 4;

			int greatercount= 0, lowercount= 0;
			if(i==0 || j==0 || i==CW-1 || j==CH-1){ greatercount= 2; lowercount= 2;}//without this there are weird edges

			for(int oi=max(0,i-1);oi<min(CW,i+2);oi++) {
				for(int oj=max(0,j-1);oj<min(CH,j+2);oj++) {
					if(oi==i && oj==j) continue;
					if(abs(cells[Layer::ELEVATION][i][j] - cells[Layer::ELEVATION][oi][oj]) > 0.8) continue;
					//if either tile is too far away, skip (so we don't round beaches touching water or mountains, but we do round shallow water and hills+)
					if(cells[Layer::ELEVATION][oi][oj]<cells[Layer::ELEVATION][i][j]) lowercount++;
					if(cells[Layer::ELEVATION][oi][oj]>cells[Layer::ELEVATION][i][j]) greatercount++;
				}
			}

			if(greatercount>mincounts) {
				if(cells[Layer::ELEVATION][i][j]==Elevation::SHALLOWWATER) cells[Layer::ELEVATION][i][j]= Elevation::BEACH_MID;
				else cells[Layer::ELEVATION][i][j]= cap(cells[Layer::ELEVATION][i][j]+0.1);
			} else if(lowercount>mincounts) {
				if(cells[Layer::ELEVATION][i][j]==Elevation::SHALLOWWATER) cells[Layer::ELEVATION][i][j]= cap(cells[Layer::ELEVATION][i][j]-0.2);
				cells[Layer::ELEVATION][i][j]= cap(cells[Layer::ELEVATION][i][j]-0.1);
			}

			if(cells[Layer::ELEVATION][i][j]<Elevation::BEACH_MID && cells[Layer::ELEVATION][i][j]>Elevation::SHALLOWWATER)
				cells[Layer::ELEVATION][i][j]=Elevation::BEACH_MID;
			if(cells[Layer::ELEVATION][i][j]<Elevation::SHALLOWWATER && cells[Layer::ELEVATION][i][j]>Elevation::DEEPWATER_LOW)
				cells[Layer::ELEVATION][i][j]=Elevation::SHALLOWWATER;
		}
	}

}

void World::setSTATLandRatio()
{
	STATlandratio= 0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			if(cells[Layer::ELEVATION][i][j]>=Elevation::BEACH_MID) STATlandratio++;
		}
	}
	STATlandratio/= CW*CH;
}

void World::update()
{
	//Process periodic world events
	#if defined(_DEBUG)
	if(DEBUG) printf("W");
	#endif

	processWorldTick();

	//play music! move to CONTROL.CPP
	tryPlayMusic();

	#if defined(_DEBUG)
	if(DEBUG) printf("/S");
	#endif

	//add new agents, if environment isn't closed
	processRandomSpawn();

	#if defined(_DEBUG)
	if(DEBUG) printf("/C");
	#endif

	//Process cell changes
	processCells();

	#if defined(_DEBUG)
	if(DEBUG) printf("/I");
	#endif

	//give input to every agent. Sets in[] array
	setInputs();

	#if defined(_DEBUG)
	if(DEBUG) printf("/B");
	#endif

	//brains tick. computes in[] -> out[]
	brainsTick();

	#if defined(_DEBUG)
	if(DEBUG) printf("/N");
	#endif

	//reset any counter variables per agent and do other stuff before processOutputs and healthTick
	processCounters();

	#if defined(_DEBUG)
	if(DEBUG) printf("/O");
	#endif

	//read output and process consequences of bots on environment. requires out[]
	processOutputs();

	#if defined(_DEBUG)
	if(DEBUG) printf("/H");
	#endif

	//process health:
	healthTick();

	#if defined(_DEBUG)
	if(DEBUG) printf("/R");
	#endif
	
	//handle reproduction
	processReproduction();

	#if defined(_DEBUG)
	if(DEBUG) printf("/CA");
	#endif

	//process agent-cell interactions
	processCellInteractions();

	#if defined(_DEBUG)
	if(DEBUG) printf("/AA");
	#endif

	//process agent-agent interactions
	processAgentInteractions();
	//DO NOT add causes of death after this!!! Health should be exactly 0 when leaving this function

	#if defined(_DEBUG)
	if(DEBUG) printf("/X");
	#endif

	//process dead agents
	processDeath();

	#if defined(_DEBUG)
	if(DEBUG) printf("/");
	#endif

	//move to Control.cpp ... why is this even stored in world?...
	vector<std::pair <std::string ,std::pair <int,int> > >::iterator item= events.begin();
	while (item != events.end()) {
		if (item->second.second<-conf::EVENTS_HALFLIFE) {
			item= events.erase(item);
		} else {
			++item;
		}
	}

	#if defined(_DEBUG)
	if(DEBUG) printf("tick/"); //must occur at end of tick!
	#endif

}

void World::processWorldTick()
{
	//disable demo mode if epoch>0
	if(current_epoch>0) setDemo(false);

	//if this is very first tick,
	if(modcounter==0 && current_epoch==0){
		setInputs(); //take a snapshot...
		brainsTick(); //...push it through the brain...
		processOutputs(true);
		processOutputs(true);
		processOutputs(true);
		processOutputs(true);
		processOutputs(true); //...and equalize all agent's outputs

		if(DISABLE_LAND_SPAWN && STATlandratio>0.75) {
			//disable disable land spawning if the oceans are too small
			DISABLE_LAND_SPAWN= false;
			addEvent("Land spawning force-enabled!", EventColor::WHITE);
			addEvent("(you can disable again in menu)", EventColor::WHITE);
		}
	}

	processReporting(); 

	if (modcounter>=FRAMES_PER_EPOCH) {
		processNewEpoch(); //resets modcounter= 0 after running
	}

	processClimate();

	modcounter++;
}

void World::processNewEpoch()
{
	modcounter = 0;
	current_epoch++;

	//post event that a new epoch has started
	std::string eventstring= "Epoch #";
	std::ostringstream ss;
	ss << current_epoch;
	eventstring.append( ss.str() );
	eventstring.append(" Started");
	addEvent(eventstring, EventColor::BLACK);

	//adjust global mutation rate
	processMutationEvent();
}

void World::processClimate()
{
	if(modcounter%(5*FRAMES_PER_DAY)==0) { //every 5 days, we adjust climate
		if(CLIMATE){
			float epochmult= modcounter%FRAMES_PER_EPOCH == 0 ? conf::CLIMATE_INTENSITY_EPOCH_MULT : 1.0f;
			//simple cap of bias - we can get stuck at the extremes 
			CLIMATEBIAS= cap(randn(CLIMATEBIAS, CLIMATE_INTENSITY*epochmult));
			
			//more complicated behavior of the mult - first, average the climate mult towards 0.5 occasionally
			if(modcounter%(int)(FRAMES_PER_EPOCH/2)==0)
				CLIMATEMULT= (CLIMATEMULT*conf::CLIMATEMULT_WEIGHT + CLIMATEMULT_AVERAGE) / ((float)(conf::CLIMATEMULT_WEIGHT+1));
			//next, take abs randn and cap it
			CLIMATEMULT= cap(abs(randn(CLIMATEMULT, CLIMATE_INTENSITY*epochmult)));

			if(current_epoch == 2050 && agents.size()>9000) {
				CLIMATEBIAS = randf(0,1);
				CLIMATEMULT = 0.85;
				if(epochmult!=1.0) {
					addEvent("2050 Climate Target Status:", EventColor::BLACK);
					addEvent("     ???     ", EventColor::PURPLE);
				}
			}

			if(epochmult!=1.0) {
				if(isHadean()) addEvent("Global Hadean Epoch!", EventColor::ORANGE);
				else if(isIceAge()) addEvent("Global Ice Age Epoch!", EventColor::CYAN);
				if(isExtreme()) addEvent("Global Temp Extremes", EventColor::ORANGE);
				//else if(CLIMATEMULT<0.25) addEvent("Global Uniform Temp", EventColor::YELLOW);
			}

			if(DEBUG) printf("Set Climate Bias to %f, and Climate Multiplier to %f\n", CLIMATEBIAS, CLIMATEMULT);
		}

		if (DROUGHTS){
			bool beforedrought = isDrought();
			bool beforeovergrowth = isOvergrowth();

			//every 25 days, we pick a new drought mult
			if (modcounter%(25*FRAMES_PER_DAY)==0 && current_epoch > 0) {
				DROUGHTMULT= randn(DROUGHTMULT, conf::DROUGHT_STDDEV);
			} else if (modcounter%(10*FRAMES_PER_DAY)==0) {
				//try to average Droughts down to a random float every 10 days
				DROUGHTMULT= (DROUGHTMULT*conf::DROUGHT_WEIGHT_TO_RANDOM+randf(0,1)) / ((float)(conf::DROUGHT_WEIGHT_TO_RANDOM+1));
			}
			//average back toward 1.0 if out of range
			if (DROUGHTMULT>DROUGHT_MAX || DROUGHTMULT<DROUGHT_MIN) 
				DROUGHTMULT= (DROUGHTMULT*conf::DROUGHT_WEIGHT_TO_ONE+1.0) / ((float)(conf::DROUGHT_WEIGHT_TO_ONE+1));
				
			if (!beforedrought && isDrought())
				addEvent("Global Drought Started!", EventColor::PINK);
			else if (beforedrought && !isDrought())
				addEvent("Global Drought Ended!", EventColor::LIME);
			
			if (!beforeovergrowth && isOvergrowth())
				addEvent("Global Overgrowth Start!", EventColor::LIME);
			else if (beforeovergrowth && !isOvergrowth())
				addEvent("Global Overgrowth Ended!", EventColor::PINK);

			if (DEBUG)
				printf("Set Drought Multiplier to %f\n", DROUGHTMULT);
		} else {
			DROUGHTMULT= 1.0;
		}
	}
}

void World::processMutationEvent()
{
	if (MUTEVENTS) {
		MUTEVENTMULT= randf(0,1)<conf::MUTEVENT_CHANCE ? randi(min(1,MUTEVENT_MAX),max(1,MUTEVENT_MAX+1)) : 1;
		if (MUTEVENTMULT<0) 
			MUTEVENTMULT= 0;
		if (MUTEVENTMULT==0) 
			addEvent("No Mutation chance Epoch!?!", EventColor::LAVENDER);
		else if (MUTEVENTMULT==2) 
			addEvent("Double Mutation chance Epoch!", EventColor::LAVENDER);
		else if (MUTEVENTMULT==3) 
			addEvent("TRIPPLE Mutation chance Epoch!", EventColor::LAVENDER);
		else if (MUTEVENTMULT==4) 
			addEvent("QUADRUPLE Mutation chance Epoch!", EventColor::LAVENDER);
		else if (MUTEVENTMULT>4) 
			addEvent("Mutation chance > *4!!!", EventColor::LAVENDER);
		if(DEBUG) 
			printf("Set Mutation Multiplier to %i\n", MUTEVENTMULT);
	} else {
		MUTEVENTMULT = 1;
	}
}

void World::findStats()
{
	//clear old stats
	STATherbivores= 0;
	STATfrugivores= 0;
	STATcarnivores= 0;
	STATterrans= 0;
	STATamphibians= 0;
	STATaquatic= 0;
	STATalive= 0;
	STATdead= 0; 
	STATspiky= 0;
	STAThybrids= 0;
	STAThighestgen= 0;
	STATlowestgen= INT_MAX;
	STATbestherbi= 0;
	STATbestfrugi= 0;
	STATbestcarni= 0;
	STATbestterran= 0;
	STATbestamphibious= 0;
	STATbestaquatic= 0;
	STATbesthybrid= 0;
	STATplants= 0;
	STATfruits= 0;
	STATmeats= 0;
	STAThazards= 0;
	STATallplant= 0;
	STATallfruit= 0;
	STATallmeat= 0;
	STATallhazard= 0;

	//agents
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].health>0) {
			STATalive++;
			
			if (agents[i].isHerbivore()) {
				STATherbivores++;
				if (agents[i].gencount>STATbestherbi) STATbestherbi= agents[i].gencount;
			} else if (agents[i].isFrugivore()) {
				STATfrugivores++;
				if (agents[i].gencount>STATbestfrugi) STATbestfrugi= agents[i].gencount;
			} else if (agents[i].isCarnivore()) {
				STATcarnivores++;
				if (agents[i].gencount>STATbestcarni) STATbestcarni= agents[i].gencount;
			}

			if (agents[i].isTerrestrial()) {
				STATterrans++;
				if (agents[i].gencount>STATbestterran) STATbestterran= agents[i].gencount;
			} else if (agents[i].isAquatic()) {
				STATaquatic++;
				if (agents[i].gencount>STATbestaquatic) STATbestaquatic= agents[i].gencount;
			} else if (agents[i].isAmphibious()) {
				STATamphibians++;
				if (agents[i].gencount>STATbestamphibious) STATbestamphibious= agents[i].gencount;
			}

			if (agents[i].gencount>STAThighestgen) STAThighestgen= agents[i].gencount;
			if ((agents[i].gencount<STATlowestgen && agents[i].gencount!=0) //set lowestgen to lowest gen, unless that gen == 0...
				|| (STATlowestgen==INT_MAX && i==agents.size()-1)){ //...UNLESS there were no agents that had any other gen value
					STATlowestgen= agents[i].gencount;
			}

			if (agents[i].isSpikey(SPIKE_LENGTH)) STATspiky++;

			if (agents[i].hybrid) {
				STAThybrids++;
				if (agents[i].gencount>STATbesthybrid) STATbesthybrid= agents[i].gencount;
			}
		}
		else STATdead++;
	}

	if (STAThighestgen>0) STATinvgenrange= 1/(1.01*STAThighestgen - STATlowestgen);
	else STATinvgenrange= 1;

	//agent population graph min/max
	MINPOP = 10000000;
	MAXPOP = 0; //GLView uses this value to scale the graph vertically
	//something-something, intermediate value theorem FTW!
	for (int i=0; i<numTotal.size(); i++) {
		if (numTotal[i] < MINPOP) {
			MINPOP= numTotal[i];
			MAXAFTERMIN= false;
		}
		if (numTotal[i] > MAXPOP) {
			MAXPOP= numTotal[i];
			MAXAFTERMIN= true;
		}
	}

	//oh wait, don't forget to check right now too, maybe something happened!
	int alivenow = getAlive();
	if (alivenow < MINPOP) {
		MINPOP= alivenow;
		MAXAFTERMIN= false;
	}
	if (alivenow > MAXPOP) {
		MAXPOP= alivenow;
		MAXAFTERMIN= true;
	}

	//cell layers
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			if(cells[Layer::PLANTS][i][j]>=0.25) STATplants++;
			if(cells[Layer::FRUITS][i][j]>=0.25) STATfruits++;
			if(cells[Layer::MEATS][i][j]>=0.25) STATmeats++;
			if(cells[Layer::HAZARDS][i][j]>=0.25) STAThazards++;
			STATallplant+= cells[Layer::PLANTS][i][j];
			STATallfruit+= cells[Layer::FRUITS][i][j];
			STATallmeat+= cells[Layer::MEATS][i][j];
			STATallhazard+= cells[Layer::HAZARDS][i][j];
		}
	}
}

void World::processReporting()
{
	//update achievements, write report, record counts, display population events
	if (!STATuseracted && pcontrol) { //outside of the report if statement b/c it can happen anytime
		addEvent("User, Take the Wheel!", EventColor::MULTICOLOR);
		STATuseracted= true;
	}

	//start reporting process
	if (REPORTS_PER_EPOCH>0 && modcounter%(FRAMES_PER_EPOCH/REPORTS_PER_EPOCH)==0) {
		//first, collect all stats
		findStats();

		//pop the front of all the vectors
		pop_front(graphGuides);
		pop_front(numTotal);
		pop_front(numDead);
		pop_front(numHerbivore);
		pop_front(numCarnivore);
		pop_front(numFrugivore);
		pop_front(numTerrestrial);
		pop_front(numAmphibious);
		pop_front(numAquatic);
		pop_front(numHybrid);

		//push back all new values
		int guideline;
		if(modcounter%FRAMES_PER_EPOCH==0) guideline = GuideLine::EPOCH;
		else if (modcounter%FRAMES_PER_DAY==0) guideline = GuideLine::DAY;
		else guideline = GuideLine::NONE;
		graphGuides.push_back(guideline);

		numTotal.push_back(getAlive());
		numDead.push_back(getDead());
		numHerbivore.push_back(getHerbivores());
		numCarnivore.push_back(getCarnivores());
		numFrugivore.push_back(getFrugivores());
		numTerrestrial.push_back(getLungLand());
		numAmphibious.push_back(getLungAmph());
		numAquatic.push_back(getLungWater());
		numHybrid.push_back(getHybrids());

		//events for achievements and stats every epoch
		triggerStatEvents();

		//once-epoch events tags (skipping if the 0th slot is totally zero, indicating a load or program start)
		if(modcounter%FRAMES_PER_EPOCH == 0 && numTotal[0] != 0 && current_epoch > 0){
			//int preeventcount= events.size(); //count of events before checks. Will cause next cycle to skip, reducing repeat event flags
			int ptr = REPORTS_PER_EPOCH-1;

			//generic stats between start of epoch and current
			if(!MAXAFTERMIN && MINPOP <= AGENTS_MIN_NOTCLOSED && STATfirstspecies) {
				//population fell and the minimum is at or below min_agents after the First Species alert was fired
				addEvent("Population Crash Epoch!", EventColor::RED);
				STATfirstspecies= false; //reset first species alert

			} else if (!MAXAFTERMIN && MINPOP <= MAXPOP*0.5) {
				//population fell and the minimum is half the maximum or lower
				addEvent("Population Bust Epoch!", EventColor::RED);

			} else if (MAXAFTERMIN && MAXPOP >= MINPOP*1.3) {
				//population rose and the maximum exceeds the minimum by a large amount (130%)
				if (MAXPOP >= MINPOP*2 && MINPOP > AGENTS_MIN_NOTCLOSED+5) {
					//check for a huge population boom (doubling) as long as the min was greater than the base state
					addEvent("Population *2 (or more) Epoch!", EventColor::BLUE);
				}

				//check major phenotypes for blooms or branches:
				//CONSIDER: phenotype min & max trackers (not public values, just local)
				bool anyphenotype = false;
				//stomach phenotype
				if(numHerbivore[ptr] >= numHerbivore[0]*1.3 && numHerbivore[ptr] > AGENTS_MIN_NOTCLOSED) {
					if(numCarnivore[0] + numFrugivore[0] > numHerbivore[0] && numTotal[0] > AGENTS_MIN_NOTCLOSED+5) 
						addEvent("Herbivore Branch Epoch", EventColor::GREEN); //if the other phenotypes were >, we branched
					else addEvent("Herbivore Bloom Epoch", EventColor::GREEN); //otherwise, we bloomed
					anyphenotype = true;
				}
				if(numCarnivore[ptr] >= numCarnivore[0]*1.3 && numCarnivore[ptr] > AGENTS_MIN_NOTCLOSED) {
					if(numHerbivore[0] + numFrugivore[0] > numCarnivore[0] && numTotal[0] > AGENTS_MIN_NOTCLOSED+5) 
						addEvent("Carnivore Branch Epoch", EventColor::GREEN);
					else addEvent("Carnivore Bloom Epoch", EventColor::GREEN);
					anyphenotype = true;
				}
				if(numFrugivore[ptr] >= numFrugivore[0]*1.3 && numFrugivore[ptr] > AGENTS_MIN_NOTCLOSED) {
					if(numHerbivore[0] + numCarnivore[0] > numFrugivore[0] && numTotal[0] > AGENTS_MIN_NOTCLOSED+5) 
						addEvent("Frugivore Branch Epoch", EventColor::GREEN);
					else addEvent("Frugivore Bloom Epoch", EventColor::GREEN);
					anyphenotype = true;
				}
				//lung phenotype
				if(numAmphibious[ptr] >= numAmphibious[0]*1.3 && numAmphibious[ptr] > AGENTS_MIN_NOTCLOSED) {
					if(numAquatic[0] + numTerrestrial[0] > numAmphibious[0] && numTotal[0] > AGENTS_MIN_NOTCLOSED+5) 
						addEvent("Amphibian Branch Epoch", EventColor::GREEN);
					else addEvent("Amphibian Bloom Epoch", EventColor::GREEN);
					anyphenotype = true;
				}
				if(numTerrestrial[ptr] >= numTerrestrial[0]*1.3 && numTerrestrial[ptr] > AGENTS_MIN_NOTCLOSED) {
					if(numAquatic[0] + numAmphibious[0] > numTerrestrial[0] && numTotal[0] > AGENTS_MIN_NOTCLOSED+5) 
						addEvent("Terrestrial Branch Epoch", EventColor::GREEN);
					else addEvent("Terrestrial Bloom Epoch", EventColor::GREEN);
					anyphenotype = true;
				}
				if(numAquatic[ptr] >= numAquatic[0]*1.3 && numAquatic[ptr] > AGENTS_MIN_NOTCLOSED) {
					if(numTerrestrial[0] + numAmphibious[0] > numAquatic[0] && numTotal[0] > AGENTS_MIN_NOTCLOSED+5) 
						addEvent("Aquatic Branch Epoch", EventColor::GREEN);
					else addEvent("Aquatic Bloom Epoch", EventColor::GREEN);
					anyphenotype = true;
				}

				if(!anyphenotype) {
					//if none of the major phenotypes triggered, we just had a generic population bloom
					addEvent("Population Bloom Epoch", EventColor::GREEN);
				}
			} else {
				//if not having a major population boom or bust, check slower rates of growth/death
				//if(numTotal[ptr] > numTotal[0]*1.2) addEvent("Slight Increase Population Epoch", EventColor::BLUE);
				//else if(numTotal[ptr] < numTotal[0]*0.8) addEvent("Slight Decrease Population Epoch", EventColor::BLUE);
				//else if(numTotal[ptr] < AGENTS_MIN_NOTCLOSED*1.2) addEvent("Minimum Population Epoch", EventColor::BLUE);
				//else 
				addEvent("Steady Population Epoch", EventColor::BLUE);
			}
		}

		//please note: DEMO mode is checked inside this func
		writeReport();

		deaths.clear();
	} else if (modcounter%12==0) findStats(); //ocasionally collect stats regardless
}

void World::triggerStatEvents(bool showevents)
{
	if(!STATfirstspecies && max(STATbestherbi,max(STATbestfrugi,STATbestcarni))==5) {
		if(showevents) addEvent("First Species Suriving!", EventColor::MULTICOLOR);
		STATfirstspecies= true;}
	if(!STATfirstpopulation && getAlive()>AGENTS_MAX_SPAWN) {
		if(showevents) addEvent("First Large Population!", EventColor::MULTICOLOR);
		STATfirstpopulation= true;}
	if(!STATfirstglobal && STATbestaquatic>=5 && STATbestterran>=5) {
		if(showevents) addEvent("First Global Population(s)!", EventColor::MULTICOLOR);
		STATfirstglobal= true;}
	if(!STATstrongspecies && max(STATbestherbi,max(STATbestfrugi,STATbestcarni))>=500) {
		if(showevents) addEvent("First to Generation 500!", EventColor::MULTICOLOR);
		STATstrongspecies= true;}
	if(!STATstrongspecies2 && max(STATbestherbi,max(STATbestfrugi,STATbestcarni))>=1000) {
		if(showevents) addEvent("First to Generation 1000!", EventColor::MULTICOLOR);
		STATstrongspecies2= true;}
	if(!STATwildspecies && STATstrongspecies) {
		int wildest= 0;
		for(int i=0; i<agents.size(); i++){
			if(abs(agents[i].species)>abs(wildest)) wildest= agents[i].species;
		}
		if(abs(wildest)>conf::SPECIESID_RANGE){
			if(showevents) addEvent("It\'s over 9000!", EventColor::MULTICOLOR);
			STATwildspecies= true;
		}
	}
	if(STATallachieved && FUN==true) FUN= false;
	if(!STATallachieved && STATuseracted && STATfirstspecies && STATfirstpopulation && STATfirstglobal && STATstrongspecies && STATstrongspecies2 && STATwildspecies){
		if(showevents) addEvent("All Achievements Achieved!", EventColor::MULTICOLOR);
		STATallachieved= true;
		FUN= true;
	}

}

void World::processCells(bool prefire)
{
	if(conf::CELL_TICK_RATE!=0) {
		//random seeds/spawns
		if ((modcounter%PLANT_ADD_FREQ==0 && !CLOSED) || getFood()<MIN_PLANT) {
			int cx=randi(0,CW);
			int cy=randi(0,CH);
			cells[Layer::PLANTS][cx][cy]= 1.0;
		}
		if (modcounter%HAZARD_EVENT_FREQ==0) {
			int cx=randi(0,CW);
			int cy=randi(0,CH);
			cells[Layer::HAZARDS][cx][cy]= cap((cells[Layer::HAZARDS][cx][cy]/90+0.99));
		}
		if (modcounter%FRUIT_ADD_FREQ==0 && randf(0,1)<abs(DROUGHTMULT)) {
			while (true) {
				int cx=randi(0,CW);
				int cy=randi(0,CH);
				if(DROUGHTMULT>0){
					if (cells[Layer::PLANTS][cx][cy]>FRUIT_PLANT_REQUIRE) {
						cells[Layer::FRUITS][cx][cy]= 1.0;
						break;
					}
				} else { //also handle negative Drought multipliers by reducing random fruit cells
					cells[Layer::FRUITS][cx][cy]= cap(cells[Layer::FRUITS][cx][cy]-0.1);
					break;
				}
			}
		}

		float invCW= 1/(float)CW;
		float daytime= (modcounter+current_epoch*FRAMES_PER_EPOCH)*2*M_PI/FRAMES_PER_DAY;

		#pragma omp parallel for schedule(dynamic)
		for(int cy=0; cy<(int)CH;cy++){
			float tempzone= CLIMATE_AFFECT_FLORA ? calcTempAtCoord(cy) : 0.5; //calculate the y-coord's temp once per row

			for(int cx=0; cx<(int)CW;cx++){
				//basic simulation spread function; calculate every other cell every other tick, except modcounter= first frame
				if((cx+cy)%conf::CELL_TICK_RATE!=modcounter%conf::CELL_TICK_RATE && modcounter!=1 && !prefire) continue;

				float plant = cells[Layer::PLANTS][cx][cy];
				float fruit = cells[Layer::FRUITS][cx][cy];
				float meat = cells[Layer::MEATS][cx][cy];
				float hazard = cells[Layer::HAZARDS][cx][cy];

				//plant ops
				if (plant>0 && !prefire) {
					float tempmult= CLIMATE_AFFECT_FLORA ? 2-cap(2*min(tempzone+0.5-conf::CLIMATE_KILL_FLORA_ZONE,1.5-conf::CLIMATE_KILL_FLORA_ZONE-tempzone)) : 1.0;
					plant-= PLANT_DECAY*conf::CELL_TICK_RATE*tempmult; //food quantity is changed by PLANT_DECAY, which is doubled in bad temperature regions (arctic and hadean)
					if (hazard>0) {
						plant+= PLANT_GROWTH*conf::CELL_TICK_RATE*max(0.0f,DROUGHTMULT)*hazard; //food grows out of waste/hazard, limited by low DROUGHTMULT
					}

					if(CLIMATE_AFFECT_FLORA) tempmult= 2*cap(2*min(tempzone,1-tempzone)-conf::CLIMATE_KILL_FLORA_ZONE); //adjust the temp mult for plant spread
					if (randf(0,1)<PLANT_SPREAD*conf::CELL_TICK_RATE*DROUGHTMULT*tempmult && plant>=0.5) { //plant grows from itself
						//food seeding
						int ox= randi(cx-1-PLANT_RANGE,cx+2+PLANT_RANGE);
						int oy= randi(cy-1-PLANT_RANGE,cy+2+PLANT_RANGE);
						if (ox<0) ox+= CW;
						if (ox>CW-1) ox-= CW;
						if (oy<0) oy+= CH;
						if (oy>CH-1) oy-= CH; //code up to this point ensures world edges are crossed and not skipped
						if (cells[Layer::PLANTS][ox][oy]<=0.75) cells[Layer::PLANTS][ox][oy]+= 0.25;
					}
				}
				cells[Layer::PLANTS][cx][cy]= cap(plant);

				//meat ops
				if (meat>0 && !prefire) {
					meat -= MEAT_DECAY*conf::CELL_TICK_RATE; //consider: meat decay effected by direct tempzone?
				}
				cells[Layer::MEATS][cx][cy]= cap(meat);

				//fruit ops
				if (fruit>0 && !prefire) {
					if (plant<=FRUIT_PLANT_REQUIRE || DROUGHTMULT<0){
						fruit-= FRUIT_DECAY*conf::CELL_TICK_RATE; //fruit decays, double if lack of plant life or DROUGHTMULT is negative
					}
					fruit-= FRUIT_DECAY*conf::CELL_TICK_RATE;
				}
				cells[Layer::FRUITS][cx][cy]= cap(fruit);

				//hazard = cells[Layer::HAZARDS]...
				if (hazard>0 && hazard<=0.9 && !prefire){
					hazard-= HAZARD_DECAY*conf::CELL_TICK_RATE; //hazard decays
				} else if (hazard>0.9 && randf(0,1)<0.0625 && !prefire){
					hazard= 90*(hazard-0.99); //instant hazards will be reset to proportionate value
				}
				cells[Layer::HAZARDS][cx][cy]= cap(hazard);

				//light ops
				//if we are moonlit and moonlight happens to be 1.0, then the light layer is useless. Set all values to 1 for rendering
				cells[Layer::LIGHT][cx][cy]= (MOONLIT && MOONLIGHTMULT==1.0) ? 1.0 :
					cap(0.6+sin((cx*2*M_PI)*invCW - daytime));
				//cap(0.6+sin((cx*2*M_PI)/CW-(modcounter+current_epoch*FRAMES_PER_EPOCH)*2*M_PI/FRAMES_PER_DAY));
			}
		}
	}
}

void World::tryPlayMusic()
//CONTROL.CPP!!!
{
	if(dosounds && domusic){
		//unpause music if it's paused
		if(currentsong) {
			if(currentsong->getIsPaused()) currentsong->setIsPaused(false);
			if(currentsong->getVolume()<0.01) currentsong->stop();
		}

		//if we are ready for a new song, pick a new song
		if(timenewsong == 0 && !currentsong) {
			int songindex = randi(0,8);
			if(current_epoch == 0 && isDemo() && last5songs[0] == -1)
				songindex = 0; //always play the first song first in demo mode; it's our theme song!

			//check our list of recently played track indexes to get a new index if needed
			int tries = 0;
			while(tries < 25) {
				bool songplayed = false;
				for(int i=0; i<5; i++){
					if(songindex == last5songs[i]) {
						songplayed = true;
						songindex = randi(0,conf::NUM_SONGS);
						tries++;
						break; //if the song was in last 5 songs, pick a new one, increment the tries counter, and try again
					}
				}
				if (!songplayed) break;
			}
			
			std::string songfile = conf::SONGS[songindex];

			printf("Now playing track: %s", songfile.c_str());
			#if defined(_DEBUG) 
				printf(", index: %i", songindex);
			#endif

			currentsong= audio->play2D(songfile.c_str(), false, false, true, ESM_NO_STREAMING);

			if(currentsong){
				currentsong->setVolume(0.7);

				//finally, add our index to the list of recently played songs
				for(int i=4; i>=0; i--){
					if(last5songs[i]==-1) {
						last5songs[(i+4)%5]= -1; // this little do-dad will set the next index to -1, allowing us to cycle!
						last5songs[i]= songindex;
						break;
					}
				}
			} else {
				#if defined(_DEBUG)
				printf("...failed!\n");
				#endif
			}
			printf("\n");
		}

	} else if (currentsong) {
		currentsong->setVolume(currentsong->getVolume()*0.92); //fade music if we aren't allowed to play it
		if(currentsong->getVolume()<0.001) currentsong->setIsPaused(true);
	}

	if(domusic){
		if(timenewsong<-300 || //time-out if it's been a long while since we started a song
		(currentsong && currentsong->isFinished())) { //or if the song is finished now
			if(currentsong) currentsong->drop();
			currentsong= 0;
			timenewsong= randi(15+ceil(modcounter*0.001),50+ceil(modcounter*0.001));
			#if defined(_DEBUG)
				printf("setting seconds to next song: %i\n", timenewsong);
			#endif
		}
	} else timenewsong= 10;
}

void World::setInputs()
{
	float PI8=M_PI/8/2; //pi/8/2
	float PI38= 3*PI8; //3pi/8/2
	float PI4= M_PI/4;
	Vector3f v(1,0,0); //unit vector for +x axis

	selectedSounds.clear(); //clear selection's heard sounds
   
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].health<=0) continue;
		Agent* a= &agents[i];

		//our cell position
		int scx= (int)(a->pos.x/conf::CZ);
		int scy= (int)(a->pos.y/conf::CZ);

		float light = (MOONLIT) ? max(MOONLIGHTMULT, cells[Layer::LIGHT][scx][scy]) : cells[Layer::LIGHT][scx][scy]; //grab min light level for conditions
		float light_cell_mult = light*a->eye_see_cell_mod;
		float light_agent_mult = light*a->eye_see_agent_mod;

		vector<float> r(NUMEYES, 0);
		vector<float> g(NUMEYES, 0);
		vector<float> b(NUMEYES, 0);
		if (!AGENTS_SEE_CELLS) {
			r.assign(NUMEYES, AMBIENT_LIGHT_PERCENT*light);
			g.assign(NUMEYES, AMBIENT_LIGHT_PERCENT*light);
			b.assign(NUMEYES, AMBIENT_LIGHT_PERCENT*light);
		}			   
		float smellsum=0;
		vector<float> hearsum(NUMEARS,0);
		float blood= 0;
		float fruit= 0, meat= 0, hazard= 0, water= 0;

		//cell sense min-max's
		int celldist= ceil(MAX_SENSORY_DISTANCE*conf::SMELL_DIST_MULT/(float)conf::CZ/2);
		int minx= max((scx - celldist), 0);
		int maxx= min((scx+1 + celldist), CW);
		int miny= max((scy - celldist), 0);
		int maxy= min((scy+1 + celldist), CH);

		//---AGENT-CELL SENSES---//
		for(int tcx= minx; tcx<maxx; tcx++){
			for(int tcy= miny; tcy<maxy; tcy++){

				//---AGENT-CELL SMELL---//
				//a faster/better food- & hazard-revealing solution than eyesight
				fruit+= cells[Layer::FRUITS][tcx][tcy];
				meat+= cells[Layer::MEATS][tcx][tcy];
				hazard+= cells[Layer::HAZARDS][tcx][tcy];
				water+= cells[Layer::ELEVATION][tcx][tcy]<=Elevation::BEACH_MID ? 1 : 0; // all water (and beach) smells the same

				//---AGENT-CELL EYESIGHT---//
				//slow, but realistic, and applies selection pressure to agents of certain colors of various reasons. can be disabled
				if(AGENTS_SEE_CELLS && light != 0) {
					Vector3f cellpos = Vector3f((float)(tcx*conf::CZ+conf::CZ/2), (float)(tcy*conf::CZ+conf::CZ/2), 0);
					Vector3f diffpos = cellpos - a->pos;
					//find midpoint of the cell, relative to agent z-axis
					float d = max((diffpos).length2d(), a->radius); // can't see cells as being inside of us; they're around us

					float ang = v.angle_between2d(diffpos);

					for (int q=0;q<NUMEYES;q++) {
						if (a->isTiny() && !a->isTinyEye(q)){
							//small agents have half-count of eyes, the rest just keep the ambient light values they got
							r[q] = AMBIENT_LIGHT_PERCENT*light;
							g[q] = AMBIENT_LIGHT_PERCENT*light;
							b[q] = AMBIENT_LIGHT_PERCENT*light;
							continue;
						}

						Eye* e = &a->eyes[q];

						float eye_absolute_angle = a->angle + e->dir; //get eye's absolute (world) angle
						// a->angle is in [-pi,pi], a->eyedir[] is in [0,2pi]. it's possible to be > pi but not > 3*pi or < -pi
						if (eye_absolute_angle >= M_PI) eye_absolute_angle -= 2*M_PI; //correct if > pi
						
						//remember, eye_absolute_angle is in [-pi,pi], and ang is in [-pi,pi]
						float eye_target_angle = eye_absolute_angle - ang; //get the difference in absolute angles between the eye and the cell
						if (eye_target_angle >= M_PI) eye_target_angle -= 2*M_PI; //correct to within [-pi,pi] again, because a difference of 2pi is no difference at all
						else if (eye_target_angle < -M_PI) eye_target_angle += 2*M_PI;
						eye_target_angle = fabs(eye_target_angle);
						
						float fov = e->fov + conf::CZ/d;

						if (eye_target_angle < fov) {
							float invDIST = INV_MAX_SENSORY_DISTANCE/conf::SMELL_DIST_MULT;
							//we see the cell with this eye. Accumulate stats
							float sight_mult= AMBIENT_LIGHT_PERCENT*cap(light_cell_mult*((fov - eye_target_angle)/fov)*(1-d*d*invDIST*invDIST));

							for (int l=0; l < Layer::LAYERS; l++) {
								float seen_r = 0, seen_g = 0, seen_b = 0;

								if (l == Layer::PLANTS) { 
									//plants are green
									seen_g = sight_mult*cells[Layer::PLANTS][tcx][tcy];
								} else if (l == Layer::FRUITS) { 
									//fruit looks yellow
									seen_r = sight_mult*cells[Layer::FRUITS][tcx][tcy]; 
									seen_g = sight_mult*cells[Layer::FRUITS][tcx][tcy];
								} else if (l == Layer::MEATS) { 
									//meat looks red
									seen_r = sight_mult*cells[Layer::MEATS][tcx][tcy]; 
								} else if (l == Layer::HAZARDS) { 
									//hazards are purple
									seen_r = sight_mult*cells[Layer::HAZARDS][tcx][tcy]; 
									seen_b = sight_mult*cells[Layer::HAZARDS][tcx][tcy];
								} else if (l == Layer::ELEVATION) { 
									//water is blue
									seen_b = sight_mult*(cells[Layer::ELEVATION][tcx][tcy]<Elevation::BEACH_MID ? 1.0 : 0.0); 
								}

								if(r[q]<seen_r) r[q]= seen_r;
								if(g[q]<seen_g) g[q]= seen_g;
								if(b[q]<seen_b) b[q]= seen_b;
							}

							if(isAgentSelected(a->id) && isDebug()){
								linesA.push_back(a->pos);
								linesB.push_back(cellpos);
							}
						}
					}
				}
			}
		}

		//---FINALIZE AGENT-CELL SMELL---//
		float smellmult= a->smell_mod/((float)(maxx-minx)*(maxy-miny));
		fruit*= smellmult;
		meat*= smellmult;
		hazard*= smellmult;
		water*= smellmult;
				
		//---AGENT-AGENT SENSES---//
		for (int j=0; j<(int)agents.size(); j++) {
			//can still see and smell dead agents, so no health check here
			if (i==j) continue;
			Agent* a2= &agents[j];

			if (a->pos.x<a2->pos.x-MAX_SENSORY_DISTANCE || a->pos.x>a2->pos.x+MAX_SENSORY_DISTANCE
				|| a->pos.y>a2->pos.y+MAX_SENSORY_DISTANCE || a->pos.y<a2->pos.y-MAX_SENSORY_DISTANCE) continue;

			float d= (a->pos - a2->pos).length2d();

			if (d<MAX_SENSORY_DISTANCE) {
				float invDIST = INV_MAX_SENSORY_DISTANCE;
				float d_center_inv_DIST = d*invDIST;

				//---AGENT-AGENT SMELL---//
				//adds up all agents inside MAX_SENSORY_DISTANCE (without range mult)
				if(d<MAX_SENSORY_DISTANCE*conf::SMELL_DIST_MULT) smellsum+= a->smell_mod;		

				//---HEARING---//
				//adds up vocalization and other emissions from agents inside MAX_SENSORY_DISTANCE
				if(!a2->isDead() || a2->volume > 0) { //can't hear dead unless they're still making a sound (grace period stuff, not ghostly... no ghosts here)
					for (int q=0; q < NUMEARS; q++){
						Ear* e = &(a->ears[q]);

						Vector3f earvect(a->radius, 0, 0);
						earvect.rotate(0, 0, a->angle + e->dir);

						Vector3f earpos = a->pos + earvect;

						float eardist = (earpos - a2->pos).length2d();

						for(int n=0; n < Hearing::TYPES; n++){
							float otone = 0, ovolume = 0;

							if (n == Hearing::VOICE){
								otone = a2->tone;
								ovolume = a2->volume;
							} else if (n == Hearing::MOVEMENT){ 
								otone = conf::WHEEL_TONE;
								ovolume = conf::WHEEL_VOLUME*(max(fabs(a2->w1),fabs(a2->w2)));
							} //future: do agent intake sound

							float heardvolume = a->hear_mod*(1-eardist*invDIST)*ovolume;

							//package up this sound source if user is watching
							if(isAgentSelected(a->id)){
								int volfact = (int)(heardvolume*100);
								float finalfact = otone + volfact;
								selectedSounds.push_back(finalfact);
							}

							if(otone <= e->low || otone >= e->high) continue;

							if(isAgentSelected(a->id)){
								//modify the selected sound listing if its actually heard so that we can change the visual
								selectedSounds[selectedSounds.size()-1] += 100;
							}

							float tonemult = cap( min((e->high - otone)*INV_SOUND_PITCH_RANGE, (otone - e->low)*INV_SOUND_PITCH_RANGE) );
							//sounds within SOUND_PITCH_RANGE of a low or high hearing range edge get scaled down, by the distance to that edge

							hearsum[q] += heardvolume*tonemult;
						}

						if(a->isTiny()) break; //small agents only get one ear input, the rest are 0
					}
				}

				//current angle between bots, in the interval [-pi,pi] radians (a-> is at origin). Used for both Blood and Eyesight
				float ang = v.angle_between2d(a2->pos - a->pos);

				//---AGENT-AGENT EYESIGHT---//
				if(light!=0){//we will skip all eyesight if our agent is in the dark (light==0)
					for(int q=0;q<NUMEYES;q++){
						if(a->isTiny() && !a->isTinyEye(q)){ //small agents have half-count of eyes, the rest just keep the ambient light values they got
							continue;
						}

						Eye* e = &a->eyes[q];

						float eye_absolute_angle = a->angle + e->dir; //get eye's absolute (world) angle
						// a->angle is in [-pi,pi], a->eyedir[] is in [0,2pi]. it's possible to be > pi but not > 3*pi or < -pi
						if (eye_absolute_angle >= M_PI) eye_absolute_angle -= 2*M_PI; //correct if > pi
						
						//remember, eye_absolute_angle is in [-pi,pi], and ang is in [-pi,pi]
						float eye_target_angle = eye_absolute_angle - ang; //get the difference in angles between the eye and the other agent
						if (eye_target_angle >= M_PI) eye_target_angle -= 2*M_PI; //correct to within [-pi,pi] again, because a difference of 2pi is no difference at all
						else if (eye_target_angle < -M_PI) eye_target_angle += 2*M_PI;
						eye_target_angle = fabs(eye_target_angle);
						
						float fov = e->fov + a2->radius/d; //add in radius/d to allowed fov, which for large distances is the same as the angle
						//"fov" here is a bit of a misnomer, as it's taking into account the seen agent's radius. This is used later in the calc again
						//but this is the idea:
						//											|						 _
						//											|             d    _ _-- _|_	
						//           d     _ _ _					|	        _ _ --     /  | a2.radius
						//     _ _ _ - - -      | a2.radius        vs	  _ _ --  $      /    |    \
						//   a______$________ ( a2 )				|	a_______________(    a2     )
						// d=h, a2.radius = o, sin($)= o/h

						if (eye_target_angle < fov) {
							//we see a2 with this eye. Accumulate stats
							float sight_mult= cap(light_agent_mult*((fov - eye_target_angle)/fov)*(1-d_center_inv_DIST*d_center_inv_DIST));

							float seen_r = sight_mult*a2->real_red;
							float seen_g = sight_mult*a2->real_gre;
							float seen_b = sight_mult*a2->real_blu;

							if(r[q]<seen_r) r[q]= seen_r;
							if(g[q]<seen_g) g[q]= seen_g;
							if(b[q]<seen_b) b[q]= seen_b;

							if(isAgentSelected(a->id) && isDebug()){ //debug sight lines, get coords
								linesA.push_back(a->pos);
								linesB.push_back(a2->pos);
							}
						}
					}
				}
				
				//---BLOOD SENSE---//
				float bloodangle = a->angle - ang; //remember, a->angle is in [-pi,pi], and ang is in [-pi,pi]
				if (bloodangle >= M_PI) bloodangle -= 2*M_PI; //correct to within [-pi,pi] again, because a difference of 2pi is no difference at all
				else if (bloodangle < -M_PI) bloodangle += 2*M_PI;
				bloodangle= fabs(bloodangle);

				if (bloodangle < PI38) {
					float newblood= a->blood_mod*((PI38 - bloodangle)/PI38)*(1-d_center_inv_DIST)*(1-agents[j].health/conf::HEALTH_CAP);
					if(newblood>blood) blood= newblood;
					//agents with high life dont bleed. low life makes them bleed more. dead agents bleed the maximum
				}
			}
		}

		//---FINALIZE EYES---//
		for(int i=0;i<NUMEYES;i++){
			a->in[Input::EYES+i*3]= cap(r[i]);
			a->in[Input::EYES+i*3+1]= cap(g[i]);
			a->in[Input::EYES+i*3+2]= cap(b[i]);
		}

		//---HEALTH---//
		a->in[Input::HEALTH]= cap(a->health/conf::HEALTH_CAP); //if we start using mutable health maximums, use that instead of HEALTH_CAP

		//---REPRODUCTION COUNTER---//
		a->in[Input::REPCOUNT]= cap((a->maxrepcounter-a->repcounter)/a->maxrepcounter); //inverted repcounter, babytime is input val of 1

		// = = = = = END SENSES COLLECTION = = = = = //

		//---PACKAGE UP INPUTS---//
		float t= (modcounter+current_epoch*FRAMES_PER_EPOCH);
		a->in[Input::CLOCK1]= abs(sinf(t/a->clockf1));
		if(!a->isTiny()) a->in[Input::CLOCK2]= abs(sinf(t/a->clockf2));
		if(!a->isTiny()) a->in[Input::CLOCK3]= abs(sinf(t/a->clockf3));
		for(int i=0;i<NUMEARS;i++){
			a->in[Input::EARS+i]= cap(hearsum[i]);
		}
		a->in[Input::BOT_SMELL]= cap(smellsum);
		a->in[Input::BLOOD]= cap(blood);
		a->in[Input::TEMP]= calcTempAtCoord(a->pos.y);
		if (!isAgentSelected(a->id)) {
			a->in[Input::PLAYER]= 0.0;
		}
		a->in[Input::FRUIT_SMELL]= cap(fruit);
		a->in[Input::MEAT_SMELL]= cap(meat);
		a->in[Input::HAZARD_SMELL]= cap(hazard);
		a->in[Input::WATER_SMELL]= cap(water);
		a->in[Input::RANDOM]= cap(randn(a->in[Input::RANDOM],0.08));
	}
}

void World::brainsTick()
{
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].isDead()) continue;
		agents[i].tick();
	}
}

void World::processCounters()
{
	#pragma omp parallel for
	for (int i = 0; i < (int)agents.size(); i++) {
		Agent* a = &agents[i];

		//process indicator, used in drawing
		if(a->indicator > 0) a->indicator -= 0.5;
		if(a->indicator < 0) a->indicator -= 1;

		//process jaw renderer
		if(a->jawrend > 0 && a->jawPosition == 0 || a->jawrend == conf::JAWRENDERTIME) a->jawrend -= 1;

		//process carcass counter, which keeps dead agents on the world while meat is under them
		if(a->carcasscount >= 0) a->carcasscount++;

		//if alive...
		if(!a->isDead()){
			//reduce fresh kill flag
			if(a->freshkill > 0) a->freshkill -= 1;

			//Live mutations
			for(int m = 0; m < MUTEVENTMULT; m++) {
				if(randf(0,1) < LIVE_MUTATE_CHANCE){
					a->liveMutate(MUTEVENTMULT);
					addTipEvent("Selected Agent Was Mutated!", EventColor::PURPLE, a->id); //control.cpp - want this to be supressed when in fast mode
					STATlivemutations++;
					if(DEBUG) printf("Live Mutation Event\n");
				}
			}

			//Age goes up!
			if (modcounter%(FRAMES_PER_DAY/10) == 0) a->age += 1;

			//update jaw
			if(a->jawPosition > 0) {
				a->jawPosition *= -1; //jaw is an instantaneous source of damage. Reset to a negative number if positive
				a->jawrend = conf::JAWRENDERTIME; //set render timer

			//once negative, jaw moves toward to 0, slowly if near -1, faster once closer to 0
			} else if (a->jawPosition < 0) a->jawPosition = min(0.0, a->jawPosition + 0.01*(2 + a->jawPosition));

			//update center render mode. Asexual pulls toward 0, female sexual pulls toward 1, male sexual towards 2
			if(a->isAsexual()) a->centerrender -= 0.01*(a->centerrender);
			else if(a->isMale()) a->centerrender -= 0.01*(a->centerrender-2);
			else a->centerrender -= 0.01*(a->centerrender-1);
			a->centerrender = cap(a->centerrender*0.5)*2; //duck the counter under a cap to allow range [0,2]

			if (recordlifepath && isAgentSelected(a->id)) {
				if (modcounter%conf::RENDER_LIFEPATH_INTERVAL==0) lifepath.push_back(a->pos);
			}
		}
	}
}

void World::processOutputs(bool prefire)
// A prefire is needed when loading, as many values need refreshing (for example, from save loading), but the agents shouldn't move yet
{
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a = &agents[i];

		if (a->isDead()) {
			//dead agents continue to exist, slow down, and loose their voice, but skip everything else
			float sp = conf::DEADSLOWDOWN*(a->w1+a->w2)*0.5;
			a->w1 = sp;
			a->w2 = sp;

			a->volume *= 0.2;
			if(a->volume < 0.0001) a->volume = 0;

			a->sexproject = 0; //no necrophilia
			a->boost = false; //no boosting
			continue;
		}

		//set exhaustion
		float boostmult = a->boost ? BOOST_EXAUSTION_MULT : 1;
		float exhaustion_wheels = a->getWheelOutputSum()*boostmult;
		float exhaustion_outputs = a->getOutputSum()*EXHAUSTION_MULT_PER_OUTPUT;
		float exhaustion_conns = a->brain.conns.size()*EXHAUSTION_MULT_PER_CONN;
		float exhaustion_total = exhaustion_wheels + exhaustion_outputs + exhaustion_conns;

		a->exhaustion = max((float)0, (a->exhaustion + exhaustion_total) + BASEEXHAUSTION)*0.333333; // /3 to average the value

		//temp discomfort gets re-calculated intermittently based on size, to simulate heat absorption/release
		if (modcounter%(int)ceil(10+2*a->radius) == 0 || prefire){
			//calculate temperature at the agents spot. (based on distance from horizontal equator)
			float currenttemp = calcTempAtCoord(a->pos.y);
			a->discomfort += 0.1*( (currenttemp-a->temperature_preference) - a->discomfort ); //if agent is too cold, value is negative
			if (abs(a->discomfort) < conf::MIN_TEMP_DISCOMFORT_THRESHOLD) a->discomfort = 0; //below a minimum, agent can deal with it, no problem
		}

		//return to processing outputs now
		float exh = 1.0/(1.0+a->exhaustion); //*exh reduces agent output->physical action rates for high exhaustion values

		//jump sets the zvelocity value instantaneously if it was zero, otherwise we increase the height (pos.z) and decrease the zvelocity by GRAVITY
		if(GRAVITY_ACCELERATION > 0 && a->zvelocity == 0 && a->age > 0){
			a->zvelocity = cap(a->out[Output::JUMP]*exh - 0.5)*2*conf::JUMP_VELOCITY_MULT; //jump output needs to be > 0.5 to activate
			if (a->zvelocity > 0) {
				a->pos.z = a->zvelocity; //give us some initial height
				if(!STATuserseenjumping) {
					addTipEvent("Selected Agent jumped!", EventColor::YELLOW, a->id);
					if (randf(0,1) < 0.05) STATuserseenjumping = true;
				}
			}
		} //do this before setting wheels

		if (!a->isAirborne()) { //if not jumping, then change wheel speeds. otherwise, we want to keep wheel speeds constant
			if (pcontrol && isAgentSelected(a->id)) {
				a->w1= pright;
				a->w2= pleft;
			} else {
				a->w1+= a->strength*(a->out[Output::LEFT_WHEEL_F] - a->out[Output::LEFT_WHEEL_B] - a->w1);
				a->w2+= a->strength*(a->out[Output::RIGHT_WHEEL_F] - a->out[Output::RIGHT_WHEEL_B] - a->w2);
			}
		}
		a->real_red+= 0.2*((1-a->chamovid)*a->gene_red + a->chamovid*a->out[Output::RED]-a->real_red);
		a->real_gre+= 0.2*((1-a->chamovid)*a->gene_gre + a->chamovid*a->out[Output::GRE]-a->real_gre);
		a->real_blu+= 0.2*((1-a->chamovid)*a->gene_blu + a->chamovid*a->out[Output::BLU]-a->real_blu);
		if (!a->isAirborne()) a->boost= (a->out[Output::BOOST])>0.5; //if jump height is zero, boost can change. Also, exhaustion doesn't effect the boost toggle
		a->volume= a->out[Output::VOLUME]*exh;
		a->tone+= (a->out[Output::TONE]-a->tone)*exh; //exhaustion effects the change rate of tone
		a->give= a->out[Output::GIVE]*exh;
		a->sexproject= a->sexprojectbias + a->out[Output::PROJECT]; //exhaustion does not effect the physical type of sexual rep; should it somehow?

		//spike length should slowly tend towards spike output
		float g= a->out[Output::SPIKE];
		if (a->spikeLength<g && !a->isTiny()) { //its easy to retract spike, just hard to put it up. Also, Tiny agents can't wield spikes (can still bite)
			a->spikeLength+=SPIKESPEED*exh; //exhaustion does not effect spike length, but rather spike extension speed
			if(HEALTHLOSS_SPIKE_EXT!=0) a->addDamage(conf::DEATH_SPIKERAISE, HEALTHLOSS_SPIKE_EXT);
		} else if (a->spikeLength>g) a->spikeLength= g;

		//grab gets set
		a->grabbing= a->out[Output::GRAB]*exh;
		a->grabangle+= 0.2*exh*(a->out[Output::GRAB_ANGLE]*2*M_PI-a->grabangle); //exhaustion effects angle move speed

		//jaw *snap* mechanic
		float newjaw= cap(a->out[Output::JAW] - a->jawOldOutput)*exh; //new output has to be at least twice the old output to really trigger

		if (BITE_DISTANCE > 0 && a->jawPosition==0 && a->age>TENDERAGE && newjaw>0.01) a->jawPosition= newjaw;
		a->jawOldOutput= a->out[Output::JAW];

		//clock 3 gets frequency set by output, but not immediately
		a->clockf3+= 0.05*(95*(1-a->out[Output::CLOCKF3])+5 - a->clockf3); //exhaustion doesn't effect clock freq
		if(a->clockf3>100) a->clockf3= 100;
		if(a->clockf3<2) a->clockf3= 2;

		//play some audio for agents making sounds!
		if(a->volume>0.05 && (modcounter+a->id*8)%min(300,(int)ceil(40.0/(a->tone+0.0000001)))==0){
			//math explains: modcounter+id*8 to give some variety between agents
			//min(300, ensures no more than 300 ticks between sounds
			//ceil(40.0/(a->tone+0.0000001)) makes it so that low tones make less often sounds, high tones make more often, fastest possible is near 40 ticks

			float playx= a->pos.x, playy= a->pos.y;
			if(isAgentSelected(a->id)) { playx= 0; playy= 0; } //if we're selected, forget position, set us right at the listener position

			//Finally, these tones and volumes have been pre-adjusted for the sounds used in game:
			if(a->isTiny()) tryPlayAudio(conf::SFX_CHIRP1, playx, playy, a->tone*0.8+0.3, (a->volume+0.2)*0.5-0.1); //tiny agents make a high-pitched chirp, like crickets
			else tryPlayAudio(conf::SFX_CHIRP2, playx, playy, a->tone*0.8+0.2, a->volume); //large agents make a lower, cat-like "churr"
		}

		//backup previous x & y pos's This is done as a prefire on purpose (it's not saved)
		a->dpos.x = a->pos.x;
		a->dpos.y = a->pos.y;
	}

	//move bots if this is not a prefire
	if(!prefire){
		float invMEANRADIUS= 1/MEANRADIUS;

		#pragma omp parallel for schedule(dynamic)
		for (int i = 0; i < (int)agents.size(); i++) {
			Agent* a = &agents[i];

			if(a->pos.z > 0) {
				a->zvelocity -= GRAVITY_ACCELERATION;
				a->pos.z += a->zvelocity;
			} else {
				a->zvelocity = 0;
				a->pos.z = 0;
			}

			//first calculate the exact wheel scalar values
			float basewheel = 1;
			if (a->encumbered > 0) {
				basewheel *= powf(ENCUMBERED_MOVE_MULT, a->encumbered);
				//reset the encumberment
				a->encumbered = 0;
			} else if (a->boost) basewheel *= BOOST_MOVE_MULT;
			else if (a->isAirborne()) basewheel *= JUMP_MOVE_BONUS_MULT; //only if boost isn't active.
			basewheel *= sqrt(a->radius*invMEANRADIUS); //wheel speed depends on the size of the agent: smaller agents move slower

			basewheel /= 2*(1.0+a->exhaustion); //apply exhaustion. Wheels are very sensitive
			float BW1 = basewheel*a->w1;
			float BW2 = basewheel*a->w2;

			//next, generate left direction vector
			Vector3f vleft(0, 1, 0);
			vleft.rotate(0, 0, a->angle);

			//right and left wheel vectors get assigned
			Vector3f vwheelleft= vleft;
			Vector3f vwheelright= -vleft;

			//the vectors get rotated by the amount (???) and then merged into position (???)
			//I believe this was done to make large values have less impact than smaller ones, perhaps encouraging fine-tuning behavior
			//based on max basewheel calculations, we could be looking at +/- 1.73*PI, which is more than half a circle. Having boost or jump is harmful at large radii,
			//while advantageous at smaller and smaller sizes. This is intentional!
			vwheelleft.rotate(0, 0, -BW1*M_PI);
			vwheelright.rotate(0, 0, BW2*M_PI);

			a->pos += (vwheelleft*WHEEL_SPEED - vleft) + (vwheelright*WHEEL_SPEED + vleft);

			//angle bots
			if (!a->isAirborne()) {
				a->angle += BW2-BW1;
			}
			if (a->angle<-M_PI) a->angle= a->angle + 2*M_PI;
			if (a->angle>M_PI) a->angle= a->angle - 2*M_PI;

			//wrap around the map
			a->borderRectify();
		}
	}
}

void World::processCellInteractions()
{
	//process interaction with cells
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];

		if(a->isDead() && modcounter%25>0) continue; // okay, so what we're doing here is complicated
		//if the agent is alive, we process them. If they are dead and the tick count mod 25 is 0, we process them
		//we want to process dead agents because we want to check if they have meat under them, and if so, reset the corpse counter

		float randr= min(a->radius, (float)abs(randn(0.0,a->radius/3)));
		Vector2f source(randr, 0);
		source.rotate(randf(-M_PI,M_PI));

		int scx= (int) capm(a->pos.x + source.x, 0, conf::WIDTH-1)/conf::CZ;
		int scy= (int) capm(a->pos.y + source.y, 0, conf::HEIGHT-1)/conf::CZ;

		#if defined(_DEBUG)
		if(isDebug() && isAgentSelected(agents[i].id)) {
			printf("scx: %i, scy: %i\n", scx, scy);
		}
		#endif

		if(!a->isDead()){
			if (!a->isAirborne()){ //no interaction if jumping
				if (!a->boost) { //no intake if boosting
					float intake= 0;
					float speedmult= pow(1 - max(abs(a->w1), abs(a->w2)),3); //penalty for moving
					speedmult/= (1.0+a->exhaustion); //exhaustion reduces agent physical actions including all intake

					float invmult= 1-STOMACH_EFFICIENCY;
					float invplant=1-a->stomach[Stomach::PLANT]*invmult;
					float invmeat= 1-a->stomach[Stomach::MEAT]*invmult;
					float invfruit=1-a->stomach[Stomach::FRUIT]*invmult;
					//inverted stomach vals, with the efficiency mult applied

					//---START FOOD---//
					//plant food
					float food= cells[Layer::PLANTS][scx][scy];
					if (food>0) { //agent eats the food
						//Plant intake is proportional to plant stomach, inverse to speed & exhaustion
						//min rate is the actual amount of food we can take. Otherwise, apply wasterate
						float planttake= min(food, PLANT_WASTE*a->stomach[Stomach::PLANT]*invmeat*invfruit*speedmult);
						//unique for plant food: the less there is, the harder it is to eat, but not impossible
						planttake*= max(food,PLANT_TENACITY);
						//decrease cell content
						cells[Layer::PLANTS][scx][scy]-= planttake;
						//now convert the taken food into intake for the agent, applying inverted stomach mults
						intake+= PLANT_INTAKE*planttake/PLANT_WASTE;
						//this way, it's possible to eat a lot of something from the world, but if stomach isn't efficient, it's wasted
						a->addIntake(conf::PLANT_TEXT, planttake);
					}

					//meat food
					float meat= cells[Layer::MEATS][scx][scy];
					if (meat>0) { //agent eats meat
						float meattake= min(meat,MEAT_WASTE*a->stomach[Stomach::MEAT]*invplant*invfruit*speedmult);
						cells[Layer::MEATS][scx][scy]-= meattake;
						intake+= MEAT_INTAKE*meattake/MEAT_WASTE;
						a->addIntake(conf::MEAT_TEXT, meattake);
					}

					//Fruit food
					float fruit= cells[Layer::FRUITS][scx][scy];
					if (fruit>0) { //agent eats fruit
						float fruittake= min(fruit,FRUIT_WASTE*a->stomach[Stomach::FRUIT]*invmeat*invplant*cap(speedmult-0.5)*2);
						//unique for fruit - speed penalty is more extreme, being completely 0 until agents slow down <0.25
						cells[Layer::FRUITS][scx][scy]-= fruittake;
						intake+= FRUIT_INTAKE*fruittake/FRUIT_WASTE;
						a->addIntake(conf::FRUIT_TEXT, fruittake);
					}

					// proportion intake via metabolism
					float metabmult = getMetabolismRatio(a->metabolism);
					a->repcounter -= metabmult*intake;
					if (a->repcounter < 0) a->encumbered += 1;

					a->health += (1 - metabmult)*intake;
					//for default settings, metabolism splits intake this way
					// M=0		M=0.5	 M=1
					//H: 1		0.75	 0.5
					//R: 0		0.25	 0.5
					// Using a MIN_INTAKE_HEALTH_RATIO of 0, the above table becomes
					// M=0		M=0.5	 M=1
					//H: 1		0.5		 0
					//R: 0		0.5		 1
					// Using a ratio of 1, the table is always H: 1 and R: 0, no babies ever will be born, because all agents take 100% intake for health

					//---END FOOD---//

					//if (isAgentSelected(a->id)) printf("intake/sum(rates)= %f\n", 6*intake/(PLANT_INTAKE + FRUIT_INTAKE + MEAT_INTAKE));
				} //end if boosting

				//hazards
				float hazard= cells[Layer::HAZARDS][scx][scy];
				if (hazard>0){
					//the young are spry, and don't take much damage from hazard
					float agemult= 1.0;
					if(a->age<TENDERAGE) agemult= 0.5+0.5*agents[i].age*INV_TENDERAGE;

					float damage= HAZARD_DAMAGE*agemult*pow(hazard, HAZARD_POWER);
					if(hazard>0.9) {
						damage*= HAZARD_EVENT_MULT; //if a hazard event, apply multiplier
						if(modcounter%5==0) addTipEvent("Agent struck by lightning!", EventColor::PURPLE, a->id);
					}

					a->addDamage(conf::DEATH_HAZARD, damage);
				}
			}

			//waste deposit; done regardless of boost or jump
			int freqwaste= (int)max(1.0f, a->out[Output::WASTE_RATE]*MAXWASTEFREQ);
			//agents can control the rate of deposit, [1,MAXWASTEFREQ] frames
			if (modcounter%freqwaste==0){
				float hazard= cells[Layer::HAZARDS][scx][scy];
				//agents fill up hazard cells only up to 9/10, because any greater is a special event
				if(hazard<0.9) hazard+= HAZARD_DEPOSIT*freqwaste*a->health;
				
				cells[Layer::HAZARDS][scx][scy]= capm(hazard, 0, 0.9);
			}

			//land/water (always interacted with)
			float land= cells[Layer::ELEVATION][scx][scy];
			if(a->isAirborne()){
				//jumping while underwater allows one to breathe at their desired lung value if aquatic and too deep, or 0.5 if terrestrial,
				//meaning land creatures can "float" when they jump. Amphibians have best of both worlds
				land= max(land, min(Elevation::BEACH_MID, a->lungs));
			}
			float dd= pow(land - a->lungs,2);
			if (dd>=0.01){ //a difference of 0.1 or less between lung and land type lets us skip taking damage
				if(dd>=0.1) a->encumbered += (int)(dd*10); //a diff of 0.4 -> +1 encumberment. diff of 0.5 -> +2, 0.6 -> +3, 0.7 -> +4, 0.8 -> +6, 0.9+ -> +10
				//this should reeeeeally be some sort of "leg type" mechanic, not lungs
				if(HEALTHLOSS_BADTERRAIN!=0) a->addDamage(conf::DEATH_BADTERRAIN, HEALTHLOSS_BADTERRAIN*dd);
			}

			if (a->health>conf::HEALTH_CAP){ //if health has been increased over the cap, limit
				if(OVERHEAL_REPFILL!=0) a->repcounter -= (a->health-conf::HEALTH_CAP)*OVERHEAL_REPFILL; //if enabled, allow overflow to first fill the repcounter
				a->health= conf::HEALTH_CAP;
			}

		} else {
			//agent is dead. Just check for meat and refresh corpse counter if meat amount is high enough
			float meat= cells[Layer::MEATS][scx][scy];
			if(meat>CORPSE_MEAT_MIN) a->carcasscount= 1; //1 because 0 triggers meat dropping... it's a mess, I'm sorry
		}

	}
}

float World::getMetabolismRatio(float metabolism)
{
	return metabolism*(1 - MIN_INTAKE_HEALTH_RATIO);
}

void World::processAgentInteractions()
{
	//process agent-agent dynamics
	if (modcounter%2==0) { //we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents

		//phase 1: determine for all agents if they are near enough to another agent to warrant processing them
		#pragma omp parallel for schedule(dynamic)
		for (int i=0; i<(int)agents.size(); i++) {
			Agent* a1= &agents[i];

			a1->near= false;
			a1->dhealth= 0; //no better place for this now, since we use it in phase 2
			if (a1->isDead()) continue; //skip dead agents

			for (int j=0; j<i; j++) {
				Agent* a2= &agents[j];
				//note: NEAREST is calculated upon config load/reload
				if (a1->pos.x < a2->pos.x - NEAREST
					|| a1->pos.x > a2->pos.x + NEAREST
					|| a1->pos.y > a2->pos.y + NEAREST
					|| a1->pos.y < a2->pos.y - NEAREST) continue;

				if (a2->isDead()) {
					if(a1->isGrabbing() && a1->grabID==a2->id) a1->grabID= -1;
					continue;
				}

				a1->near= true;
				a2->near= true;
			}
		}

		//phase 2: actually process agents which were flagged in phase 1
		#pragma omp parallel for schedule(dynamic)
		for (int i=0; i<(int)agents.size(); i++) {
			if(agents[i].isDead()) continue;
			if(!agents[i].near){
				//no one is near enough this agent to process interactions. break grabbing and skip
				agents[i].grabID= -1;
				continue;
			}

			Agent* a = &agents[i];

			float ainvrad = 1/a->radius;

			for (int j=0; j<(int)agents.size(); j++) {
				if (i==j || !agents[j].near) continue;
				if(agents[j].health == 0) continue; //health == because we want to weed out bots who died already via other causes, but not ones in process of dying

				Agent* a2 = &agents[j];

				float d = (a->pos - a2->pos).length2d();
				float sumrad = a->radius + a2->radius;
				float a2invrad = 1/a2->radius;

				//---HEALTH GIVING---//

				if (FOOD_SHARING_DISTANCE>0 && GENEROSITY_RATE!=0 && !a->isSelfish(conf::MAXSELFISH) && a2->health+GENEROSITY_RATE<2) {
					//all non-selfish agents allow health trading to anyone within their kin range
					float deviation = abs(a->species - a2->species);

					float rd = a->isGiving() ? FOOD_SHARING_DISTANCE : sumrad+1;
					//rd is the max range allowed to agent j. If generous, range allowed, otherwise bots must basically touch

					if (d <= rd && deviation <= a->kinrange) {
						float healthrate = a->isGiving() ? GENEROSITY_RATE*a->give : GENEROSITY_RATE*2*a->give;
						//healthrate goes from 0->1 for give [0,0.5], and from 0.5->1 for give (0.5,1]...
						if(d <= sumrad + 1 && a->isGiving()) healthrate = GENEROSITY_RATE;
						//...it is maxxed when touching and generous...
						if(a->give != 1 && AGENTS_DONT_OVERDONATE) healthrate = capm(healthrate, 0, cap(a->health - a2->health)/2);
						//...and can't give more than half the diff in health if agent is max giving (if they are, it is possible to give till they die)

						//initiate transfer
						a2->health += healthrate;
						a->addDamage(conf::DEATH_GENEROSITY, healthrate);
						a2->dhealth += healthrate; //only for drawing
						a->dhealth -= healthrate;

						//play a soft purring sound for the agent being given to
						float purrx= a2->pos.x, purry= a2->pos.y;
						if(isAgentSelected(a2->id)) { purrx= 0; purry= 0; }
						if((modcounter + a2->id + (int)a2->pos.x)%60==0) tryPlayAudio(conf::SFX_PURR1, purrx, purry, randf(0.8,1.1));

						if(!STATuserseengenerosity && healthrate != 0){
							addTipEvent("Agent donated health!", EventColor::GREEN, a->id);
							addTipEvent("Agent received health donation!", EventColor::GREEN, a2->id);
							if (randf(0,1)<0.05) STATuserseengenerosity = true;
						}
					}
				}

				//---COLLISIONS---///

				if (BUMP_PRESSURE > 0 && d < sumrad && !a->isAirborne() && !a2->isAirborne()) {
					//if inside each others radii and neither are jumping, fix physics
					float ov= (sumrad-d);
					if (ov > 0 && d > 0.00001) {
						if (BUMP_DAMAGE_OVERLAP > 0 && ov > BUMP_DAMAGE_OVERLAP) {//if bots are too close, they get injured before being pushed away
							float invsumrad = 1/sumrad;
							float aagemult = 1;
							float a2agemult = 1;
							if(a->age < TENDERAGE) aagemult = (float)(a->age+1)*INV_TENDERAGE;
							if(a2->age < TENDERAGE) a2agemult = (float)(a2->age+1)*INV_TENDERAGE;
							float damagemult = ov*DAMAGE_COLLIDE*MEANRADIUS/sumrad;

							float DMG1 = capm(damagemult*ainvrad*aagemult, 0, conf::HEALTH_CAP); //larger, younger bots take less damage, bounce less
							float DMG2 = capm(damagemult*a2invrad*a2agemult, 0, conf::HEALTH_CAP);

							if(DEBUG) printf("\na collision occured. overlap: %.4f, aagemult: %.2f, a2agemult: %.2f, Damage on a: %f. Damage on a2: %f\n", ov, aagemult, a2agemult, DMG1, DMG2);
							a->addDamage(conf::DEATH_COLLIDE, DMG1);
							a2->addDamage(conf::DEATH_COLLIDE, DMG2);

							if(a->health==0){
								continue;
							}
							if(a2->health==0){
								a->killed++;
								continue;
							}

							//only trigger collision splash if another splash not being displayed
							if(a->indicator <= 0) a->initSplash(conf::RENDER_MAXSPLASHSIZE*0.5,0,0.5,1);
							if(a2->indicator <= 0) a2->initSplash(conf::RENDER_MAXSPLASHSIZE*0.5,0,0.5,1);
							addTipEvent("Agent collided hard", EventColor::CYAN, a->id);
							addTipEvent("Agent collided hard", EventColor::CYAN, a2->id);
							
							a->freshkill = FRESHKILLTIME; //this agent was hit this turn, giving full meat value
							a2->freshkill = FRESHKILLTIME;
						}

						//now anti-kith...
						float bumpmult = ov*BUMP_PRESSURE/d;
						float ff1 = capm(bumpmult*a2->radius*ainvrad, 0, 2); //the radii come in here for inertia-like effect
						float ff2 = capm(bumpmult*a->radius*a2invrad, 0, 2);
						float diffx = (a2->pos.x-a->pos.x);
						float diffy = (a2->pos.y-a->pos.y);

						a->pos.x -= diffx*ff1;
						a->pos.y -= diffy*ff1;
						a2->pos.x += diffx*ff2;
						a2->pos.y += diffy*ff2;

						a->borderRectify();
						a2->borderRectify();

//						printf("%f, %f, %f, %f, and %f\n", a->pos.x, a->pos.y, a2->pos.x, a2->pos.y, ov);
					}
				} //end collision mechanics

				//---SPIKING---//

				//small spike doesn't count. If the target is jumping in midair, can't attack them either
				if(a->isSpikey(SPIKE_LENGTH) && d<=(a2->radius + SPIKE_LENGTH*a->spikeLength) && !a2->isAirborne()){
					Vector3f v(1,0,0);
					v.rotate(0, 0, a->angle);
					float diff= v.angle_between2d(a2->pos - a->pos);
					if (fabs(diff)<M_PI/2){ //need to be in front
						float spikerange= d*abs(sin(diff)); //get range to agent from spike, closest approach
						if (a2->radius>spikerange) { //other agent is properly aligned and in range!!! getting close now...
							//need to calculate the velocity differential of the agents
							Vector3f velocitya(a->pos.x-a->dpos.x, a->pos.y-a->dpos.y, 0);
							Vector3f velocitya2(a2->pos.x-a2->dpos.x, a2->pos.y-a2->dpos.y, 0);

							velocitya-= velocitya2;
							float diff2= velocitya.angle_between2d(a2->pos - a->pos);
							float veldiff= velocitya.length2d()*cap(cos(diff2));

							if(veldiff>conf::VELOCITYSPIKEMIN){
								//these two are in collision and agent a has extended spike and is going decently fast relatively! That's a hit!
							
								float DMG= DAMAGE_FULLSPIKE*a->spikeLength*veldiff*(1-1.0/(1+expf(-(a2->radius/2-MEANRADIUS))));
								//tiny, small, & average agents take full damage, scaled by the difference in velocity. 
								//large (2*MEANRADIUS) agents take half damage, and huge agents (2*MEANRADIUS+10) take no damage from spikes at all

								if(DEBUG) printf("\nan agent received spike damage: %.4f\n", DMG);
								a2->addDamage(conf::DEATH_SPIKE, DMG); 
								a2->freshkill= FRESHKILLTIME; //this agent was hit this turn, giving full meat value
								addTipEvent("Agent was Stabbed!", EventColor::BLOOD, a2->id);

								a->hits++;
								a->spikeLength= 0; //retract spike down
								a->initSplash(conf::RENDER_MAXSPLASHSIZE*0.5*DMG,1,0.5,0); //orange splash means bot has spiked the other bot. nice!
								tryPlayAudio(conf::SFX_STAB1, a2->pos.x, a2->pos.y, randn(1.0,0.2));

								if (a2->health==0){ 
									//red splash means bot has killed the other bot. Murderer!
									a->initSplash(conf::RENDER_MAXSPLASHSIZE,1,0,0);
									addTipEvent("Agent Killed Another!", EventColor::RED, a->id);
									a->killed++;
									continue;
								} else addTipEvent("Agent Stabbed Another!", EventColor::ORANGE, a->id);

								/*Vector2f v2(1,0);
								v2.rotate(a2->angle);
								float adiff= v.angle_between(v2);
								if (fabs(adiff)<M_PI/2) {
									//this was attack from the back. Retract spike of the other agent (startle!)
									//this is done so that the other agent cant right away "by accident" attack this agent
									a2->spikeLength= 0;
								}*/
							}
						}
					}
				} //end spike mechanics

				//---JAWS---//

				if(BITE_DISTANCE > 0 && a->jawPosition>0 && d <= (sumrad+BITE_DISTANCE)) { //only bots that are almost touching may chomp
					Vector3f v(1,0,0);
					v.rotate(0, 0, a->angle);
					float diff= v.angle_between2d(a2->pos - a->pos);
					if (fabs(diff) < M_PI/6) { //advantage over spike: wide AOE
						float DMG= DAMAGE_JAWSNAP*a->jawPosition*(a->radius*a2invrad); //advantage over spike: large agents do more damage to smaller agents

						if(DEBUG) printf("\nan agent received bite damage: %.4f\n", DMG);
						a2->addDamage(conf::DEATH_BITE, DMG);
						a2->freshkill = FRESHKILLTIME;
						addTipEvent("Agent was Bitten!", EventColor::BLOOD, a2->id);

						a->hits++;
						a->initSplash(conf::RENDER_MAXSPLASHSIZE*0.5*DMG,1,1,0); //yellow splash means bot has chomped the other bot. ouch!

						float chompvolume = 0.5;
						if (a2->health == 0){ 
							chompvolume = 1;
							//red splash means bot has killed the other bot. Murderer!
							a->initSplash(conf::RENDER_MAXSPLASHSIZE,1,0,0);
							addTipEvent("Agent Killed Another!", EventColor::RED, a->id);
							a->killed++;
							if(a->grabID == a2->id) a->grabID= -1;
							continue;
						} else addTipEvent("Agent Bit Another!", EventColor::YELLOW, a->id);

						if(randf(0,1) > 0.5) tryPlayAudio(conf::SFX_CHOMP1, a->pos.x, a->pos.y, randn(1.0,0.2), chompvolume);
						else tryPlayAudio(conf::SFX_CHOMP2, a->pos.x, a->pos.y, randn(1.0,0.2), chompvolume);
					}
				} //end jaw mechanics

				//---GRABBING---//

				//doing this last because agent deaths may occur up till now, and we don't want to worry about holding onto things that die while holding them
				if(GRAB_PRESSURE!=0 && GRABBING_DISTANCE>0 && a->isGrabbing() && !a2->isDead()) {
					if(d<=GRABBING_DISTANCE+a->radius){
						Vector3f v(1,0,0);
						v.rotate(0, 0, a->angle + a->grabangle);
						float diff= v.angle_between2d(a2->pos - a->pos);

						//check init grab
						if(a->grabID==-1){
							if (fabs(diff)<M_PI/4 && randf(0,1)<0.3) { //very wide AOE centered on a's grabangle, 30% chance any one bot is picked
								a->grabID= a2->id;
								addTipEvent("Agent grabbed another", EventColor::CYAN, a->id);
								addTipEvent("Agent was grabbed", EventColor::CYAN, a2->id);
							}
						} 

						if (a->grabID==a2->id) {
							//we have a grab target, and it is this other agent. 
							//"grab" (hehe) the coords of the other agent, relative to current agent
							a->grabx= a2->pos.x-a->pos.x; a->graby= a2->pos.y-a->pos.y;

							if(d>(sumrad+1.0)){
								//Pull us together!
								//find the target point for a2
								float dpref= sumrad + 1.5;
								Vector2f tpos(a->pos.x+dpref*cos(a->angle+a->grabangle),a->pos.y+dpref*sin(a->angle+a->grabangle));

								float ff1= a2->radius*ainvrad*a->grabbing*GRAB_PRESSURE; //the radii come in here for inertia-like effect
								float ff2= a->radius*a2invrad*a->grabbing*GRAB_PRESSURE;
								a->pos.x-= (tpos.x-a2->pos.x)*ff1;
								a->pos.y-= (tpos.y-a2->pos.y)*ff1;
								a2->pos.x+= (tpos.x-a2->pos.x)*ff2;
								a2->pos.y+= (tpos.y-a2->pos.y)*ff2;

								a->borderRectify();
								a2->borderRectify();
							}

							//the graber gets rotated toward the grabbed agent by a ratio of their radii, limited to 0.08 radian
							a->angle+= capm(a->grabbing*diff*a2->radius*ainvrad, -0.08, 0.08);
						}
					} else if (a->grabID==a2->id) {
						//grab distance exceeded, break the bond
						a->grabID= -1;
					}
				} else {
					//if we can't grab, or the other agent died, clear the grab target
					a->grabID= -1;
				} //end grab mechanics. DO NOT ADD DEATH CAUSES AFTER THIS
			}
		}
	}
}

void World::healthTick()
{
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];
		if (!a->isDead()){
			//"natural" causes of death: age, wheel activity, excesive brain activity
			//baseloss starts by penalizing high average wheel speed
			float baseloss= HEALTHLOSS_WHEELS * (fabs(a->w1) + fabs(a->w2))/2;

			//getting older reduces health.
			baseloss +=  HEALTHLOSS_AGING * (float)a->age / MAXAGE;

			//being exhausted reduces health (both indirectly through it's effects on output, and directly here so we can increase agent turnover)
			baseloss += HEALTHLOSS_EXHAUSTION * a->exhaustion * a->exhaustion;

			//if boosting, loss from age, wheels, and exhaustion is multiplied
			if (a->boost) {
				baseloss *= HEALTHLOSS_BOOSTMULT;
			}

			//brain activity reduces health (note: this doesn't make sense now that exhaustion is in the picture. default mult has been 0)
			baseloss += HEALTHLOSS_BRAINUSE*a->getActivity();

			a->addDamage(conf::DEATH_NATURAL, baseloss);
			if (a->health==0) continue; //agent died, we must move on.

			//remove health from lack of "air", a straight up penalty to all bots for large population
			a->addDamage(conf::DEATH_TOOMANYAGENTS, HEALTHLOSS_NOOXYGEN*(float)max((int)agents.size()-AGENTS_MIN_NOTCLOSED, 0)/AGENTS_MAX_NOOXYGEN);
			if (a->health==0) continue; //agent died, we must move on!

			//apply temperature discomfort
			a->addDamage(conf::DEATH_BADTEMP, HEALTHLOSS_BADTEMP*abs(a->discomfort));
		}
	}
}

void World::processReproduction()
{
	//the reasoning for this seems to be that if we process every tick, it opens up weird memory reference inaccessible issues, so we mod it
	if(modcounter%2==0) {
		for (int mother=0; mother < (int)agents.size(); mother++) {
			if (agents[mother].repcounter<0 && agents[mother].health>=MINMOMHEALTH) { 
				//agent is healthy and is ready to reproduce. Now to decide how...

				if(SEXTING_DISTANCE>0 && !agents[mother].isAsexual()){
					if(agents[mother].isMale()) continue;; //'fathers' cannot themselves reproduce, only be reproduced with

					for (int father=0; father < (int)agents.size(); father++) {
						if (mother == father) continue;

						if (agents[father].isAsexual()) continue;

						float deviation= abs(agents[mother].species - agents[father].species); //species deviation check
						if (deviation>=agents[mother].kinrange) continue; //uses mother's kinrange; if outside, skip

						float distance= (agents[mother].pos - agents[father].pos).length2d();
						if(distance>SEXTING_DISTANCE) continue;

						if(agents[father].repcounter>0){
							//prepare to add agents[i].numbabies to world, with two parents
							if(DEBUG) printf("Attempting to have children...");

							reproduce(mother, father);

							addTipEvent("Agent Sexually Reproduced", EventColor::BLUE, agents[mother].id);
							addTipEvent("Agent Sexually Reproduced", EventColor::BLUE, agents[father].id);
							tryPlayAudio(conf::SFX_SMOOCH1, agents[mother].pos.x, agents[mother].pos.y, randn(1.0,0.15));

							if(DEBUG) printf(" Success!\n");
							break;
						}
					}
				} else {
					//this adds mother's numbabies to world, but with just one parent (budding)
					if(DEBUG) printf("Attempting budding...");

					reproduce(mother, mother);

					addTipEvent("Agent Assexually Budded", EventColor::NEON, agents[mother].id);
					tryPlayAudio(conf::SFX_PLOP1, agents[mother].pos.x, agents[mother].pos.y, randn(1.0,0.15));

					if(DEBUG) printf(" Success!\n");
					break;
				}
			}
			if(FUN && randf(0,1)<0.3){
				if(agents[mother].indicator<=0) agents[mother].indicator= 60;
				agents[mother].ir= randf(0,1);
				agents[mother].ig= randf(0,1);
				agents[mother].ib= randf(0,1);
			}
		}
	}
}

void World::processDeath()
{
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a = &agents[i];
		if(a->health < 0) {
			printf("Please check the code: an agent unexpectedly had negative health when it should have had exactly zero\n");
			a->health = 0;
		}
		if (a->health == 0 && a->carcasscount == 0) { 
			//world is only now picking up the fact that the agent died from the agent itself
			if (isAgentSelected(a->id)){
				printf("The Selected Agent was %s!\n", a->death.c_str());
				addTipEvent("The Selected Agent Died!", 0, a->id);

				//play audio clip of death. At position 0,0 (no position) if we had the agent selected, otherwise, use world coords
				tryPlayAudio(conf::SFX_DEATH1, 0, 0, randn(1.0,0.05));
			} else tryPlayAudio(conf::SFX_DEATH1, a->pos.x, a->pos.y, randn(1.0,0.15));

			//next, distribute meat
			int cx = (int) a->pos.x/conf::CZ;
			int cy = (int) a->pos.y/conf::CZ;

			float meat = cells[Layer::MEATS][cx][cy];
			float agemult =  a->age<TENDERAGE ? ((float)a->age+1)*INV_TENDERAGE : 1.0; //young killed agents should give very little resources until age 9
			float freshmult = a->freshkill>0 ? 1.0 : MEAT_NON_FRESHKILL_MULT; //agents which were spiked recently will give full meat, otherwise give MEAT_NON_FRESHKILL_MULT
			float stomachmult = 1+(conf::CARNIVORE_MEAT_EFF-1)*sqrt(a->stomach[Stomach::MEAT]); //carnivores give less, specified by CARNIVORE_MEAT_EFF

			meat += MEAT_VALUE*agemult*freshmult*stomachmult;
			cells[Layer::MEATS][cx][cy] = cap(meat);

			//collect all the death causes from all dead agents
			deaths.push_back(a->death);
		}
	}

	vector<Agent>::iterator iter= agents.begin();
	while (iter != agents.end()) {
		if (iter->isDead() && iter->carcasscount >= conf::CORPSE_FRAMES) {
			if(isAgentSelected(iter->id)) {
				lifepath.clear();
				if (isDemo()) addEvent("The Selected Agent decayed", EventColor::BROWN);
			}
			iter= agents.erase(iter);
		} else {
			++iter;
		}
	}
}

void World::processRandomSpawn()
{
	if (!CLOSED) {
		int alive= agents.size()-getDead();
		//make sure environment is always populated with at least AGENTS_MIN_NOTCLOSED bots
		if (alive<AGENTS_MIN_NOTCLOSED) {
			if(DEBUG) printf("Attempting agent conservation program...");
			addAgents(AGENTS_MIN_NOTCLOSED-alive,-1);
			if(DEBUG) printf(" Success!\n");
		}
		if (alive<AGENTS_MAX_SPAWN && modcounter%AGENTSPAWN_FREQ==0 && randf(0,1)<0.5) {
			if(DEBUG) printf("Attempting random spawn...");
			addAgents(1,-1); //every now and then add random bot in if population too low
			if(DEBUG) printf(" Success!\n");
		}
	}
}


//====================================================END MAIN LOOP FUNCTIONS==========================================================//



void World::setSelection(int type) 
//Control.cpp?
{
	int maxi= -1;
	//to find the desired selection, we must process all agents. Lets do this sparingly unless no-one is selected
	if (SELECTION==-1 || modcounter%15==0){
		if (type==Select::NONE) {
			//easy enough: if none desired, set selection ID to negative 
			SELECTION= -1;
		} else if (type==Select::OLDEST) {
			//oldest: simply compare all agents age's and when we find the highest one, save its index
			int maxage= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				if (!agents[i].isDead() && agents[i].age>maxage) { 
				   maxage= agents[i].age; 
				   maxi= i;
			   }
			}
		} else if(type==Select::BEST_GEN){
			//highest generation: same as with Oldest, but this time the generation counter
			int maxgen= 0;
			for (int i=0; i<(int)agents.size(); i++) {
			   if (!agents[i].isDead() && agents[i].gencount>maxgen) { 
				   maxgen= agents[i].gencount; 
				   maxi= i;
			   }
			}
		} else if(type==Select::HEALTHY){
			//Healthiest: this one's tricky, and part of the reason this is on mod-5, otherwise it bounces constantly
			float maxhealth= 0;
			for (int i=0; i<(int)agents.size(); i++) {
				if (agents[i].health>maxhealth) {
					maxhealth= agents[i].health;
					maxi= i;
				}
			}
		} else if(type==Select::ENERGETIC){
			//Energetic: finds agent with most energy (lowest exhaustion value)
			float minexhaust= 1000;
			for (int i=0; i<(int)agents.size(); i++) {
				if (!agents[i].isDead() && agents[i].age>0 && agents[i].exhaustion<minexhaust) {
					minexhaust= agents[i].exhaustion;
					maxi= i;
				}
			}
		} else if(type==Select::PRODUCTIVE){
			float maxprog= 0;
			//Productivity: find the agent who's been most successful at having children
			for (int i=0; i<(int)agents.size(); i++) {
				if(!agents[i].isDead() && agents[i].age!=0){
					float prog= (float)agents[i].children/sqrt((float)agents[i].age);
					//note: productivity needs to be / age b/c otherwise the oldest will almost always be the most productive
						//and we're using sqrt because pow^1 it would be inverse (youngest who just had babies is always selected)
						//we want a middle-ground: select the middle-aged agent who's had the most babies
					if (prog>maxprog){
						maxprog= prog;
						maxi= i;
					}
				}
			}
		} else if (type==Select::AGGRESSIVE){
			//Aggressiveness: this mode calculates a score via (# of hits)*0.25 + (# killed), and whoever has the highest score wins
			float maxindex= 0;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= 0.25*agents[i].hits + agents[i].killed;
				if (!agents[i].isDead() && index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		} else if (type==Select::RELATIVE){
			//Relative: this special mode will calculate the closest relative to the selected if the selected died. Good mode to leave on fast mode
			int select= getSelectedAgentIndex();
			if(select>=0){
				if(agents[select].isDead()){
					int index= getClosestRelative(select);
					if(index>=0) {
						maxi= index;
					}
				} else maxi= select;
			}
		} else if (type==Select::BEST_HERBIVORE){
			//best of a given stomach type, not counting gen 0. calculates stomach score and then compares; unlike types subtract at half value
			float maxindex= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= agents[i].stomach[Stomach::PLANT] - 0.5*agents[i].stomach[Stomach::MEAT] - 0.5*agents[i].stomach[Stomach::FRUIT];
				if (!agents[i].isDead() && agents[i].gencount>0 && index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		} else if (type==Select::BEST_FRUGIVORE){
			//best of a given stomach type, not counting gen 0. calculates stomach score and then compares; unlike types subtract at half value
			float maxindex= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= agents[i].stomach[Stomach::FRUIT] - 0.5*agents[i].stomach[Stomach::MEAT] - 0.5*agents[i].stomach[Stomach::PLANT];
				if (!agents[i].isDead() && agents[i].gencount>0 && index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		} else if (type==Select::BEST_CARNIVORE){
			//best of a given stomach type, not counting gen 0. calculates stomach score and then compares; unlike types subtract at half value
			float maxindex= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= agents[i].stomach[Stomach::MEAT] - 0.5*agents[i].stomach[Stomach::FRUIT] - 0.5*agents[i].stomach[Stomach::PLANT];
				if (!agents[i].isDead() && agents[i].gencount>0 && index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		}
			
		if (maxi!=-1) {
			if(!isAgentSelected(agents[maxi].id)) {
				setSelectedAgent(maxi);
				if(type==Select::RELATIVE) tryPlayAudio(conf::SFX_UI_RELATIVESELECT);
				//else PLAY SIMPLE WOOSH SFX
			}
		} else if (type!=Select::MANUAL) {
			SELECTION= -1;
		}
	}
}

bool World::setSelectionRelative(int posneg) 
//Control.cpp???
{
	//returns bool because GLView would like to know if successful or not
	int sid= getSelectedAgentIndex();
	if(sid>=0){
		//get selected species id
		int species= agents[sid].species;
		int bestidx= sid;
		int bestdd= agents[sid].kinrange + conf::VISUALIZE_RELATED_RANGE;

		//for each agent, check if its alive, not exactly like us, within kin range, and a positive difference in speciesID
		for(int i=0; i<agents.size(); i++){
			if(agents[i].isDead() || agents[i].species==species) continue;

			int dd= posneg*(agents[i].species - species);
			if(dd>=bestdd || dd<0) continue;

			if(dd<bestdd){ //if its the best so far, pick it
				bestdd= dd;
				bestidx= i;
			}
			if(bestdd==1) break; //if its the best possible, finish up!
		}

		if(bestidx!=sid){
			setSelectedAgent(bestidx);
			return true;
		}
		return false;
	}
	return false;
}

void World::setSelectedAgent(int idx)
//Control.cpp
{
	if (isAgentSelected(agents[idx].id)) SELECTION= -1; //toggle selection if already selected
	else SELECTION= agents[idx].id; //otherwise, select as normal
	lifepath.clear();

	if(!agents[idx].isDead()) agents[idx].initSplash(10,1.0,1.0,1.0);
	setControl(false);
}

Agent* World::getSelectedAgent()
//Control.cpp
{
	int idx= -1;
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		if(isAgentSelected(agents[i].id)) idx= i;
	}
	if (idx>=0) {
		return &agents[idx];
	}
	return NULL;
}

int World::getSelectedAgentIndex()
//Control.cpp???
{
	//retrieve array index of selected agent, returns -1 if none
	int idx= -1;
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		if(isAgentSelected(agents[i].id)) idx= i;
	}
	return idx;
}

int World::getClosestRelative(int idx) 
//Control.cpp???
{
	//retrieve the index of the given agent's closest living relative. Takes age, gencount, stomach, lungs, and species id into account, returns -1 if none
	int nidx= -1;
	int meta, metamax= 1;
	std::string bestrelative= "";

	if(idx>=0){
		for (int i=0; i<(int)agents.size(); i++) {
			if(idx==i) continue;

			if(!agents[i].isDead() && abs(agents[idx].species - agents[i].species) <= agents[idx].kinrange){
				//choose an alive agent that is within kin range; closer the better
				meta= 1 - abs(agents[idx].species - agents[i].species);
			} else continue; //if you aren't related, or dead, you're off to a very bad start if we're trying to find relatives... skip!

			if(agents[i].age<TENDERAGE) meta+= 1; //choose young agents at least

			std::string temprelative= "";

  			if ((agents[idx].gencount+1) == agents[i].gencount) { //choose children or nephews and nieces
				meta+= 10;
				temprelative.assign("youngling");
			} else if (agents[idx].gencount == agents[i].gencount) { //at least choose cousins/siblings... 
				meta+= 6;
				temprelative.assign("cousin");
			} else if (agents[idx].gencount == (agents[i].gencount+1)) { //...or parents/aunts/uncles
				meta+= 4;
				temprelative.assign("elder");
			} 

			if (agents[idx].parentid == agents[i].parentid) { 
				meta+= 100; //...note this gets +103 b/c of cousin above
				temprelative.assign("sibling");
			}
			if (agents[idx].parentid == agents[i].id) {
				meta+= 100; //...note this gets +102 b/c of elder above
				temprelative.assign("mother");
			}
			if (agents[idx].id == agents[i].parentid) {
				meta+= 100; //...note this gets +105 b/c of youngling above
				temprelative.assign("daughter");
			}

			for(int j=0; j<Stomach::FOOD_TYPES; j++){ //penalize mis-matching stomach types harshly
				meta-= ceilf(5*abs(agents[idx].stomach[j]-agents[i].stomach[j])); //diff of 0.2 scales to a -1 penalty, max possible= -15
			}
			meta-= ceilf(10*abs(agents[idx].lungs-agents[i].lungs)); //penalize mis-matching lung types, max possible= -10
			meta-= ceilf(5*abs(agents[idx].temperature_preference-agents[i].temperature_preference)); //penalize mis-matching temp preference, max possible: -5
			
			//if the meta counter is better than the best selection so far*, return our new target
			//*note that it has to do better than MAXDEVIATION as a baseline because we want ONLY relatives, and if no matches, we return -1
			if(meta>metamax){
				nidx= i;
				metamax= meta;
				bestrelative = temprelative;
			}
		}
	}

	if(bestrelative!="") {
		bestrelative= "Autoselected living "+bestrelative;
		addEvent(bestrelative, EventColor::BLACK);
		printf("---> New relative selected: %s\n", bestrelative.c_str());
	} else {
		addEvent("No More Living Relatives!", EventColor::BLACK);
		printf("---> No More Living Relatives.\n");
	}

	//IMPORTANT: we may return -1 here; DO NOT USE FOR ARRAYS DIRECTLY
	return nidx;
}

int World::getSelectedID() const 
//Control.cpp
{
	//retrieve world->SELECTION
	return SELECTION;
}

bool World::isAgentSelected(int id)
{
	return id == SELECTION;
}

void World::selectedHeal() 
//Control.cpp
{
	//heal selected agent
	int sidx= getSelectedAgentIndex();
	if(sidx>=0){
		agents[sidx].health= conf::HEALTH_CAP;
		agents[sidx].carcasscount= -1;
	}
}


void World::selectedKill() {
	//kill (delete) selected agent
	int sidx= getSelectedAgentIndex();
	if(sidx>=0){
		//if we press the button on an agent already dead, mark them for clearing them from the world
		if(agents[sidx].isDead()) agents[sidx].carcasscount= conf::CORPSE_FRAMES;
		else agents[sidx].addDamage(conf::DEATH_USER, 9001);
	}
}

void World::selectedBabys() {
	//force selected agent to assexually reproduce
	int sidx= getSelectedAgentIndex();
	if(sidx>=0){
		reproduce(sidx, sidx);
	}
}

void World::selectedMutate() {
	//mutate selected agent
	int sidx= getSelectedAgentIndex();
	if(sidx>=0){
		agents[sidx].liveMutate();
		addTipEvent("Selected Agent was Mutated!", EventColor::PURPLE, agents[sidx].id);
	}
}

void World::selectedStomachMut() {
	//rotate selected agent's stomach
	int sidx= getSelectedAgentIndex();
	if(sidx>=0){
		float temp= agents[sidx].stomach[0];
		for(int i=0; i<Stomach::FOOD_TYPES-1; i++){
			agents[sidx].stomach[i]= agents[sidx].stomach[i+1];
		}
		agents[sidx].stomach[Stomach::FOOD_TYPES-1]= temp;
	}
}

void World::selectedPrint() {
	//print verbose stats of selected agent
	int sidx= getSelectedAgentIndex();
	if(sidx>=0){
		agents[sidx].printSelf();
	}
}

void World::selectedTrace(int mode) {
	//perform a traceback on the selected agent's outputs as called by mode
	int sidx= getSelectedAgentIndex();
/*	if(sidx>=0){
		if(mode==1){
			//get all the wheel output inputs
			printf("==========TRACEBACK START===========\n");
			agents[sidx].traceBack(Output::LEFT_WHEEL_F);
			agents[sidx].traceBack(Output::LEFT_WHEEL_B);
			agents[sidx].traceBack(Output::RIGHT_WHEEL_F);
			agents[sidx].traceBack(Output::RIGHT_WHEEL_B);
			printf("====================================\n");
		}
	} CPBrain NOT IMPLEMENTED*/
}

void World::selectedInput(bool state) {
	//set the selected agent's user input
	int sidx= getSelectedAgentIndex();
	if(sidx>=0){
		agents[sidx].in[Input::PLAYER]= (float)state;
	}
}

void World::getFollowLocation(float &xi, float &yi) 
//Control.cpp
{
	int sidx= getSelectedAgentIndex();
	if (sidx>=0){
		xi= agents[sidx].pos.x;
		yi= agents[sidx].pos.y;
	}
}

bool World::processMouse(int button, int state, int x, int y, float scale)
//Control.cpp
{
	//now returns true if an agent was selected manually successfully
	if(button==0 && state==1){
		float mind= 1e10;
		int mini= -1;
		float d;

		for (int i=0; i<(int)agents.size(); i++) {
			d= pow(x-agents[i].pos.x,2)+pow(y-agents[i].pos.y,2);
				if (d<mind) {
					mind= d;
					mini= i;
				}
			}
		if (mind<3500/scale) {
			//toggle selection of this agent
			setSelectedAgent(mini);
			return true;
		} else return false;
	} else return false;
}


void World::addAgent(Agent &agent)
//Control.cpp???
{
	agent.id= idcounter;
	idcounter++;
	agents.push_back(agent);
}

bool World::addLoadedAgent(float x, float y)
{
	if(loadedagent.brain.boxes.size()==BRAINBOXES){ //todo: soon this won't matter
		Agent a= loadedagent;
		a.pos.x= x; a.pos.y= y;
		a.angle= randf(-M_PI,M_PI);
		addAgent(a);
	} else return false;
	return true;
}

void World::addAgents(int num, int set_stomach, bool set_lungs, bool set_temp_pref, float nx, float ny)
{
	for (int i=0;i<num;i++) {
		int scx= 0,scy= 0;
		Agent a(
			BRAINBOXES,
			BRAINCONNS,
			SPAWN_MIRROR_EYES,
			OVERRIDE_KINRANGE,
			MEANRADIUS,
			REP_PER_BABY,
			DEFAULT_BRAIN_MUTCHANCE,
			DEFAULT_BRAIN_MUTSIZE,
			DEFAULT_GENE_MUTCHANCE,
			DEFAULT_GENE_MUTSIZE
			);
		//Agent::Agent
		scx= (int) a.pos.x/conf::CZ;
		scy= (int) a.pos.y/conf::CZ;

		if (set_stomach==Stomach::PLANT) a.setHerbivore(); //if told to predetermine stomach type
		else if (set_stomach==Stomach::MEAT) a.setCarnivore();
		else if (set_stomach==Stomach::FRUIT) a.setFrugivore();
		else if (set_stomach==-2) i%2==0 ? a.setHerbivore() : a.setFrugivore();
		else if (getEpoch()==0 && a.isCarnivore()) randf(0,1)<0.5 ? a.setHerbivore() : a.setFrugivore(); //epoch 0 has no carnivores forced
		
		if (nx!=-1 && ny!=-1){ //if custom location given
			a.pos.x= nx;
			a.pos.y= ny;

		} else {
			for(int count= 0; count<100000; count++){
				if(count>0){
					a.setPosRandom(conf::WIDTH, conf::HEIGHT);
					scx= (int) a.pos.x/conf::CZ;
					scy= (int) a.pos.y/conf::CZ;
				}

				//add various spawn-blocking conditions here
				if(DISABLE_LAND_SPAWN && cells[Layer::ELEVATION][scx][scy]>=Elevation::BEACH_MID) continue;
				else break;

				if(STATlandratio==1.0) {
					printf("ALERT: spawn failed to find a suitable water cell. Enabling land spawn!");
					DISABLE_LAND_SPAWN= false;
					break;
				}
			}
		}

		//these flags's functions depend on location
		if (set_lungs){ //if told to fix lungs for agent's position
			a.setIdealLungs(cells[Layer::ELEVATION][scx][scy]);
		}

		if (set_temp_pref) {
			a.setIdealTempPref(calcTempAtCoord(a.pos.y));
		}

		addAgent(a);
	}
}


void World::reproduce(int mother, int father)
{
	float healthloss= 0; //health lost by mother for assexual reproduction
	bool hybridoffspring = false; //if parents are not the same agent (sexual reproduction), then mark the child
	int numbabies = agents[mother].numbabies;

	if (mother==father){ //if assexual rep, father is just the mother again
		healthloss= agents[mother].radius/MEANRADIUS*HEALTHLOSS_ASSEX;

		agents[mother].initSplash(conf::RENDER_MAXSPLASHSIZE,0,0.8,0); //green splash means agent asexually reproduced

	}else{ //otherwise, it's sexual
		hybridoffspring= true;
		agents[mother].initSplash(conf::RENDER_MAXSPLASHSIZE,0,0,0.8);
		agents[father].initSplash(conf::RENDER_MAXSPLASHSIZE,0,0,0.8); //blue splashes mean agents sexually reproduced.
	}

	float newhealth= agents[mother].health/(numbabies + 1) + cap(-agents[mother].repcounter/agents[mother].maxrepcounter);

	agents[mother].health= newhealth;
	agents[mother].addDamage(conf::DEATH_ASEXUAL, healthloss);

	//Reset the MOTHER's repcounter ONLY (added bonus of sexual rep and allows possible dichotomy of sexes)
	agents[mother].resetRepCounter(MEANRADIUS, REP_PER_BABY);

	//do not omp
	for (int i=0;i<numbabies;i++) {
		Agent daughter= agents[mother].reproduce(agents[father], PRESERVE_MIRROR_EYES, OVERRIDE_KINRANGE, MEANRADIUS, REP_PER_BABY, i);

		daughter.health= newhealth;
		daughter.hybrid= hybridoffspring;

		if (hybridoffspring) agents[father].children++;
		agents[mother].children++;

		addAgent(daughter);
	}
	if (isAgentSelected(agents[mother].id)) printf("The Selected Agent has Reproduced and had %i Babies!\n", numbabies);
}

void World::writeReport()
//Control.cpp???
{
	if(DEMO) return;
	printf("Writing Report, Epoch: %i, Day: %i\n", getEpoch(), getDay());
	//save all kinds of nice data stuff
	int randspawned;
	int randgen = 0;
	int randspecies = 0;
	int randkinrange = 0;
	int randseed = 0;
	float randradius = 0;
	float randmetab = 0;
	float randsex = 0;
	float randbrainmutchance = 0;
	float randbrainmutsize = 0;
	float randgenemutchance = 0;
	float randgenemutsize = 0;
	float randtemppref = 0;
	int randchildren = 0;
	int randbrainsize = 0;

	int randagent= randi(0,agents.size());
	randspawned= agents[randagent].gencount==0 ? 1 : 0;
	randgen= agents[randagent].gencount;
	randspecies= agents[randagent].species;
	randkinrange= agents[randagent].kinrange;
	for (int i= 0; i<(int)agents[randagent].brain.boxes.size(); i++){
		if (agents[randagent].brain.boxes[i].seed>randseed) randseed=agents[randagent].brain.boxes[i].seed;
	}
	randbrainsize = agents[randagent].brain.conns.size();
	randradius= agents[randagent].radius;
	randmetab= agents[randagent].metabolism;
	randsex = agents[randagent].sexproject;
	randbrainmutchance= agents[randagent].brain_mutation_chance;
	randbrainmutsize= agents[randagent].brain_mutation_size;
	randgenemutchance= agents[randagent].gene_mutation_chance;
	randgenemutsize= agents[randagent].gene_mutation_size;
	randtemppref= agents[randagent].temperature_preference;
	randchildren= agents[randagent].children;

/*	for(int s=0;s<(int)species.size();s++){
		if(members[s]>=3) happyspecies++; //3 agents with the same species id counts as a species
	}*/

	//death cause reporting
	std::vector<std::string> deathlog;
	std::vector<int> deathcounts;
	for (int d=0; d<(int)deaths.size(); d++) {
		bool added= false;
		for (int e=0; e<(int)deathlog.size(); e++) {
			if (deaths[d].compare(deathlog[e]) == 0) {
				deathcounts[e]++;
				added= true;
				break;
			}
		}
		if (!added) {
			deathlog.push_back(deaths[d]);
			deathcounts.push_back(1);
		}
	}

	FILE* fr = fopen("report.txt", "a");
	//print basics: Epoch and Agent counts
	fprintf(fr, "Epoch:\t%i\tDay:\t%i\t#Agents:\t%i\t",
		getEpoch(), getDay(), getAlive());
	//print world stats: world settings
	fprintf(fr, "Growth*:\t%f\tMutation*:\t%i\tLowTemp:\t%f\tHighTemp:\t%f\t",
		DROUGHTMULT, MUTEVENTMULT, getLowTemp(), getHighTemp());
	//print world stats: agent specifics
	fprintf(fr, "#Herbi:\t%i\t#Frugi:\t%i\t#Carni:\t%i\t#Terra:\t%i\t#Amphib:\t%i\t#Aqua:\t%i\t#Hybrids:\t%i\t#Spikes:\t%i\t",
		getHerbivores(), getFrugivores(), getCarnivores(), getLungLand(), getLungAmph(), getLungWater(), getHybrids(), getSpiky());
	//print world stats: cell counts
	fprintf(fr, "#Plant:\t%i\t#Meat:\t%i\t#Hazard:\t%i\t#Fruit:\t%i\t",
		getFood(), getMeat(), getHazards(), getFruit());
	//print random selections: Genome, brain seeds, [[[generation]]]
	fprintf(fr, "RGenome:\t%i\tRKinRange:\t%i\tRBrainSize:\t%i\tRSeed:\t%i\tRGen:\t%i\tRRadius:\t%f\tRMetab:\t%f\tRSexProj:\t%f\tRTempP:\t%f\tRBrainMutChance:\t%f\tRBrainMutSize:\t%f\tRGeneMutChance:\t%f\tRGeneMutSize:\t%f\tRChildren:\t%i\t",
		randspecies, randkinrange, randbrainsize, randseed, randgen, randradius, randmetab, randsex, randtemppref, randbrainmutchance, randbrainmutsize, randgenemutchance, randgenemutsize, randchildren);
	//print generations: Top Gen counts
	fprintf(fr, "TopHGen:\t%i\tTopFGen:\t%i\tTopCGen:\t%i\tTopLGen:\t%i\tTopAGen:\t%i\tTopWGen:\t%i\t",
		STATbestherbi, STATbestfrugi, STATbestcarni, STATbestterran, STATbestamphibious, STATbestaquatic);
	//print mutations and deaths: Number and Causes
	fprintf(fr, "#Mutations:\t%i\t#Deaths:\t%i\tDEATHLOG:\t",
		getMutations(),deaths.size());
	for (int e=0; e<(int)deathlog.size(); e++) {
		fprintf(fr, "%s:\t%i\t", deathlog[e].c_str(), deathcounts[e]);
	}
	fprintf(fr, "\n");
	fclose(fr);
}


bool World::isClosed() const
{
	return CLOSED;
}

void World::setClosed(bool close)
{
	CLOSED = close;
}

bool World::isDrought() const
{
	if(DROUGHTMULT<1.0-conf::DROUGHT_NOTIFY) return true;
	return false;
}

bool World::isOvergrowth() const
{
	if(DROUGHTMULT>1.0+conf::DROUGHT_NOTIFY) return true;
	return false;
}

bool World::isIceAge() const
{
	if(CLIMATEMULT<0.6 && CLIMATEBIAS<0.3) return true;
	return false;
}

bool World::isHadean() const
{
	if(CLIMATEMULT<0.6 && CLIMATEBIAS>0.7) return true;
	return false;
}

bool World::isExtreme() const
{
	if(CLIMATEMULT>0.7) return true;
	return false;
}

bool World::isDemo() const
{
	return DEMO;
}

void World::setDemo(bool state)
{
	if(DEMO && !state) { //first, if DEMO is on and we're turning it off, null up report.txt
		FILE* fr = fopen("report.txt", "w");
		fclose(fr);
	}

	DEMO= state;
}

void World::setControl(bool state)
{
	//reset left and right wheel controls 
	pleft= 0;
	pright= 0;

	pcontrol= state;
}

bool World::isAutoselecting() const
{
	return AUTOSELECT;
}

void World::setAutoselect(bool state)
{
	AUTOSELECT= state;
}

void World::setDebug(bool state)
{
	DEBUG = state;
}

bool World::isDebug() const
{
	return DEBUG;
}

int World::getHerbivores() const
{
	return STATherbivores;
}

int World::getFrugivores() const
{
	return STATfrugivores;
}

int World::getCarnivores() const
{
	return STATcarnivores;
}

int World::getLungLand() const
{
	return STATterrans;
}

int World::getLungAmph() const
{
	return STATamphibians;
}

int World::getLungWater() const
{
	return STATaquatic;
}

int World::getAgents() const
{
	return agents.size();
}

int World::getHybrids() const
{
	return STAThybrids;
}

int World::getSpiky() const
{
	return STATspiky;
}

int World::getDead() const
{
	return STATdead;
}

int World::getMutations()
{
	int val= STATlivemutations;
	STATlivemutations= 0;
	return val;
}

int World::getAlive() const
{
	return STATalive;
}

int World::getFood() const //return plant cells with 50% max or more
{
	return STATplants;
}

int World::getFruit() const //return fruit cells with 50% max or more
{
	return STATfruits;
}

int World::getMeat() const //return meat cells with 50% max or more
{
	return STATmeats;
}

int World::getHazards() const //return hazard cells with 50% max or more
{
	return STAThazards;
}

float World::getLandRatio() const //count land cells and report as a ratio
{
	return STATlandratio;
}

float World::getFoodSupp() const
{
	return STATallplant/PLANT_WASTE*PLANT_INTAKE/2/100; //#food * 1/PLANT_WASTE tick/food * PLANT_INTAKE/1 health/tick * 1/2 agent/health = #agents
	// /100 because... reasons
}

float World::getFruitSupp() const
{
	return STATallfruit/FRUIT_WASTE*FRUIT_INTAKE/2/100;
}

float World::getMeatSupp() const
{
	return STATallmeat/MEAT_WASTE*MEAT_INTAKE/2/100;
}

float World::getHazardSupp() const
{
	return STATallhazard*HAZARD_DAMAGE/2*50; //BUG? #hazard * HAZARD_DAMAGE/1/1 health/hazard/tick * 1/2 agent/health != #agents killed per tick
}

/*
int World::getSelected() const //returns the ID of the selected agent
{
	int id= -1;
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].selectflag) id= i;
	}
	return id;
}*/

int World::getEpoch() const
{
	return current_epoch;
}

int World::getDay() const
{
	return (int)ceil((float)modcounter/(float)FRAMES_PER_DAY);
}

float World::getLowTemp()
{
	return calcTempAtCoord(0);
}

float World::getHighTemp()
{
	return calcTempAtCoord((float)(conf::HEIGHT/2));
}

float World::calcTempAtCoord(float y)
{
	//calculate the temperature at a given world coordinate
	//currently the function is 0 @ equator and 1 @ poles
	return cap((1 - 2.0*abs(y/conf::HEIGHT - 0.5))*CLIMATEMULT + CLIMATEBIAS*(1-CLIMATEMULT));
}


float World::calcTempAtCoord(int worldcy)
{
	//calculate the temperature at a given world cell y-index
	return calcTempAtCoord((float)worldcy*conf::CZ);
}


void World::addEvent(std::string text, int type)
//Control.cpp
{
	//use EventColor:: namespace to access available colors for the "type" input
	std::pair <std::string ,std::pair <int,int> > data;
	data.first= text;
	data.second.first= type;
	data.second.second= conf::EVENTS_HALFLIFE;
	events.push_back(data);
}

void World::addTipEvent(std::string text, int type, int agentid)
//Control.cpp
{
	//use EventColor:: namespace to access available colors for the "type" input
	//post an event only if we: A: tips are enabled, and B: have the current agent selected
	if (isAgentSelected(agentid) && !NO_TIPS) {
		std::pair <std::string ,std::pair <int,int> > data;
		data.first= text;
		data.second.first= type;
		data.second.second= conf::EVENTS_HALFLIFE;
		events.push_back(data);
	}
}

void World::dismissNextEvents(int count)
//Control.cpp
{
	int disevents= 0;
	for(int e=0; e<events.size(); e++){
		if(events[e].second.second>-conf::EVENTS_HALFLIFE+20) {
			events[e].second.second= -conf::EVENTS_HALFLIFE+20;
			disevents++;
		}
		if(disevents==count) break;
	}
}

void World::setStatsAfterLoad()
{
	//reset some stats that often trigger
	findStats();
	triggerStatEvents(false);
}

void World::init()
{
	printf("...starting up...\n");
	last5songs[0]= -1;

	//ALL .cfg constants must be initially declared in world.h and defined here.
	NO_TIPS= false;
	CONTINENTS= conf::CONTINENTS;
	CONTINENT_ROUGHNESS= conf::CONTINENT_ROUGHNESS;
    OCEANPERCENT= conf::OCEANPERCENT;
	SPAWN_LAKES= conf::SPAWN_LAKES;
	ISLANDNESS= conf::ISLANDNESS;
	FEATURES_TO_SPAWN= conf::FEATURES_TO_SPAWN;
	DISABLE_LAND_SPAWN= conf::DISABLE_LAND_SPAWN;
	AMBIENT_LIGHT_PERCENT = conf::AMBIENT_LIGHT_PERCENT;
	AGENTS_SEE_CELLS = conf::AGENTS_SEE_CELLS;
	AGENTS_DONT_OVERDONATE = conf::AGENTS_DONT_OVERDONATE;
	MOONLIT= conf::MOONLIT;
	MOONLIGHTMULT= conf::MOONLIGHTMULT;
	DROUGHTS= conf::DROUGHTS;
	DROUGHT_MIN= conf::DROUGHT_MIN;
	DROUGHT_MAX= conf::DROUGHT_MAX;
	MUTEVENTS= conf::MUTEVENTS;
	MUTEVENT_MAX= conf::MUTEVENT_MAX;
	CLIMATE= conf::CLIMATE;
	CLIMATE_INTENSITY= conf::CLIMATE_INTENSITY;
	CLIMATEMULT_AVERAGE= conf::CLIMATEMULT_AVERAGE;
	CLIMATE_AFFECT_FLORA= conf::CLIMATE_AFFECT_FLORA;

    MIN_PLANT= conf::MIN_PLANT;
    INITPLANTDENSITY= conf::INITPLANTDENSITY;
    INITFRUITDENSITY= conf::INITFRUITDENSITY;
	INITMEATDENSITY= conf::INITMEATDENSITY;
    INITHAZARDDENSITY= conf::INITHAZARDDENSITY;
    AGENTS_MIN_NOTCLOSED= conf::AGENTS_MIN_NOTCLOSED;
    AGENTS_MAX_SPAWN= conf::AGENTS_MAX_SPAWN;
	AGENTSPAWN_FREQ= conf::AGENTSPAWN_FREQ;
    AGENTS_MAX_NOOXYGEN= conf::AGENTS_MAX_NOOXYGEN;

	REPORTS_PER_EPOCH= conf::REPORTS_PER_EPOCH;
    FRAMES_PER_EPOCH= conf::FRAMES_PER_EPOCH;
    FRAMES_PER_DAY= conf::FRAMES_PER_DAY;

	GRAVITY_ACCELERATION= conf::GRAVITY_ACCELERATION;
    BUMP_PRESSURE= conf::BUMP_PRESSURE;
	GRAB_PRESSURE= conf::GRAB_PRESSURE;
	BRAINBOXES= conf::BRAINBOXES;
	BRAINCONNS= conf::BRAINCONNS;
    WHEEL_SPEED= conf::WHEEL_SPEED;
	JUMP_MOVE_BONUS_MULT= conf::JUMP_MOVE_BONUS_MULT;
    BOOST_MOVE_MULT= conf::BOOST_MOVE_MULT;
	BOOST_EXAUSTION_MULT= conf::BOOST_EXAUSTION_MULT;
	CORPSE_FRAMES= conf::CORPSE_FRAMES;
	CORPSE_MEAT_MIN= conf::CORPSE_MEAT_MIN;
	SOUND_PITCH_RANGE= conf::SOUND_PITCH_RANGE;
	INV_SOUND_PITCH_RANGE = 1/SOUND_PITCH_RANGE;

    GENEROSITY_RATE= conf::GENEROSITY_RATE;
	BASEEXHAUSTION= conf::BASEEXHAUSTION;
	EXHAUSTION_MULT_PER_CONN= conf::EXHAUSTION_MULT_PER_CONN;
	EXHAUSTION_MULT_PER_OUTPUT= conf::EXHAUSTION_MULT_PER_OUTPUT;
	ENCUMBERED_MOVE_MULT= conf::ENCUMBERED_MOVE_MULT;
	MEANRADIUS= conf::MEANRADIUS;
    SPIKESPEED= conf::SPIKESPEED;
	SPAWN_MIRROR_EYES= conf::SPAWN_MIRROR_EYES;
	PRESERVE_MIRROR_EYES= conf::PRESERVE_MIRROR_EYES;
    FRESHKILLTIME= conf::FRESHKILLTIME;
	TENDERAGE= conf::TENDERAGE;
	INV_TENDERAGE= 1/(float)conf::TENDERAGE;
    MINMOMHEALTH= conf::MINMOMHEALTH;
	MIN_INTAKE_HEALTH_RATIO= conf::MIN_INTAKE_HEALTH_RATIO;
	FUN= 0;
	SUN_RED= 1.0;
	SUN_GRE= 1.0;
	SUN_BLU= 0.8;
    REP_PER_BABY= conf::REP_PER_BABY;
	OVERHEAL_REPFILL= conf::OVERHEAL_REPFILL;
//    LEARNRATE= conf::LEARNRATE;
    OVERRIDE_KINRANGE= conf::OVERRIDE_KINRANGE;
	DEFAULT_BRAIN_MUTCHANCE= conf::DEFAULT_BRAIN_MUTCHANCE;
	DEFAULT_BRAIN_MUTSIZE= conf::DEFAULT_BRAIN_MUTSIZE;
	DEFAULT_GENE_MUTCHANCE= conf::DEFAULT_GENE_MUTCHANCE;
	DEFAULT_GENE_MUTSIZE= conf::DEFAULT_GENE_MUTSIZE;
	LIVE_MUTATE_CHANCE= conf::LIVE_MUTATE_CHANCE;
    MAXAGE= conf::MAXAGE;
	MAXWASTEFREQ= conf::MAXWASTEFREQ;

    MAX_SENSORY_DISTANCE= conf::MAX_SENSORY_DISTANCE;
	INV_MAX_SENSORY_DISTANCE= 1/MAX_SENSORY_DISTANCE;
    SPIKE_LENGTH= conf::SPIKE_LENGTH;
	BITE_DISTANCE = conf::BITE_DISTANCE;
    BUMP_DAMAGE_OVERLAP= conf::BUMP_DAMAGE_OVERLAP;
    FOOD_SHARING_DISTANCE= conf::FOOD_SHARING_DISTANCE;
    SEXTING_DISTANCE= conf::SEXTING_DISTANCE;
    GRABBING_DISTANCE= conf::GRABBING_DISTANCE;

    HEALTHLOSS_WHEELS= conf::HEALTHLOSS_WHEELS;
    HEALTHLOSS_BOOSTMULT= conf::HEALTHLOSS_BOOSTMULT;
    HEALTHLOSS_BADTEMP= conf::HEALTHLOSS_BADTEMP;
    HEALTHLOSS_AGING= conf::HEALTHLOSS_AGING;
	HEALTHLOSS_EXHAUSTION= conf::HEALTHLOSS_EXHAUSTION;
    HEALTHLOSS_BRAINUSE= conf::HEALTHLOSS_BRAINUSE;
    HEALTHLOSS_SPIKE_EXT= conf::HEALTHLOSS_SPIKE_EXT;
    HEALTHLOSS_BADTERRAIN= conf::HEALTHLOSS_BADTERRAIN;
    HEALTHLOSS_NOOXYGEN= conf::HEALTHLOSS_NOOXYGEN;
    HEALTHLOSS_ASSEX= conf::HEALTHLOSS_ASSEX;

	DAMAGE_FULLSPIKE= conf::DAMAGE_FULLSPIKE;
	DAMAGE_COLLIDE= conf::DAMAGE_COLLIDE;
    DAMAGE_JAWSNAP= conf::DAMAGE_JAWSNAP;

	STOMACH_EFFICIENCY= conf::STOMACH_EFFICIENCY;

    PLANT_INTAKE= conf::PLANT_INTAKE;
    PLANT_DECAY= conf::PLANT_DECAY;
    PLANT_GROWTH= conf::PLANT_GROWTH;
    PLANT_WASTE= conf::PLANT_WASTE;
    PLANT_ADD_FREQ= conf::PLANT_ADD_FREQ;
    PLANT_SPREAD= conf::PLANT_SPREAD;
    PLANT_RANGE= conf::PLANT_RANGE;
	PLANT_TENACITY= conf::PLANT_TENACITY;

    FRUIT_INTAKE= conf::FRUIT_INTAKE;
    FRUIT_DECAY= conf::FRUIT_DECAY;
    FRUIT_WASTE= conf::FRUIT_WASTE;
    FRUIT_ADD_FREQ= conf::FRUIT_ADD_FREQ;
    FRUIT_PLANT_REQUIRE= conf::FRUIT_PLANT_REQUIRE;

    MEAT_INTAKE= conf::MEAT_INTAKE;
    MEAT_DECAY= conf::MEAT_DECAY;
    MEAT_WASTE= conf::MEAT_WASTE;
    MEAT_VALUE= conf::MEAT_VALUE;
	MEAT_NON_FRESHKILL_MULT = conf::MEAT_NON_FRESHKILL_MULT;

    HAZARD_EVENT_FREQ= conf::HAZARD_EVENT_FREQ;
	HAZARD_EVENT_MULT= conf::HAZARD_EVENT_MULT;
    HAZARD_DECAY= conf::HAZARD_DECAY;
    HAZARD_DEPOSIT= conf::HAZARD_DEPOSIT;
    HAZARD_DAMAGE= conf::HAZARD_DAMAGE;
	HAZARD_POWER= conf::HAZARD_POWER;

	//tip popups. typically the first one is guarenteed to display the moment the sim starts, then each one after has a decreasing
	//chance of being seen second, third, etc. The first 16 or so are very likely to be seen at least once durring epoch 0
	#if defined(_DEBUG)
	printf("DEBUG ENVIRONMENT DETECTED, SKIPPING TIPS, SETTING MOONLIT=false\n");
	MOONLIT= false;
	NO_TIPS= true;
	#endif

//	if(!NO_TIPS){
	tips.push_back("Left-click an agent to select it");
	tips.push_back("Press 'f' to follow selected agent");
	tips.push_back("Zoom with middle mouse click&drag");
	tips.push_back("Pan around with left click&drag");
	tips.push_back("Press 'q' to recenter map & graphs");
	tips.push_back("Press 'spacebar' to pause");
	tips.push_back("Dead agents make 8bit death sound");
	tips.push_back("Early agents die from exhaustion a lot");
	tips.push_back("Press '1' to select oldest agent");
	tips.push_back("Oldest agent has good chances");
	tips.push_back("Demo prevents report.txt changes");
	tips.push_back("Autosaves happen 10 sim days");
	tips.push_back("Cycle layer views with 'k' & 'l'");
	tips.push_back("View all layers again with 'o'");
	tips.push_back("Green bar next to agents is Health");
	tips.push_back("Yellow bar next to agents is Energy");
	tips.push_back("Blue bar is Reproduction Counter");
	tips.push_back("Agent 'whiskers' are eye orientations");
	tips.push_back("Press number keys for auto-selections");
	tips.push_back("These tips may now repeat");
	//20 tips. After this, they may start repeating. I suggest not adding or taking away from above without replacement
	tips.push_back("Night and day are simulated");
	tips.push_back("Also zoom with '>' & '<'");
	tips.push_back("Press 'h' for detailed interface help");
	tips.push_back("Press 'm' to toggle Fast Mode");
	tips.push_back("Agents display splashes for events");
	tips.push_back("Yellow spash = agent bit another");
	tips.push_back("Orange spash= agent stabbed other");
	tips.push_back("Red spash = agent killed another");
	tips.push_back("Green spash= asexually reproduced");
	tips.push_back("Blue spash = sexually reproduced");
	tips.push_back("Purple spash = mutation occured");
	tips.push_back("Grey spash = agent was just born");
	tips.push_back("Press 'm' to simulate at max speed");
	tips.push_back("Select 'New World' to end Demo");
	tips.push_back("Use settings.cfg to change sim rules");
	tips.push_back("Hide dead with a UI option or 'j'");
	tips.push_back("Agents can hear each other moving");
	tips.push_back("Agents can hear agents with volume");
	tips.push_back("Agents can see each other's RGB");
	tips.push_back("Agents can smell (count nearby)");
	tips.push_back("Agents can sense blood (1-health)");
	tips.push_back("Agents can boost and double speed");
	tips.push_back("Agents have clock inputs");
	tips.push_back("Agents have a rAnDoM input");
	tips.push_back("Agents have a self-health input");
	tips.push_back("Press 'e' to activate a unique input");
	tips.push_back("Agents seeking grab have cyan limbs");
	tips.push_back("Agents grabbing shows a cyan pulse");
	tips.push_back("Grabbed agents get pushed around");
	tips.push_back("Grab has an angle agents can control");
	tips.push_back("Jumping agents appear closer");
	tips.push_back("Jumping agents can't rotate mid-jump");
	tips.push_back("Jumping agents can't be attacked");
	tips.push_back("Jumping agents can still be seen");
	//start with general world tips. These can be duplicated because it's more random chance once we get more than halfway
	tips.push_back("Overgrowth = more plants, fruit");
	tips.push_back("Overgrowth = more plants, fruit");
	tips.push_back("Droughts = less plant growth");
	tips.push_back("Droughts = less plant growth");
	tips.push_back("Moonlight lets agents see at night");
	tips.push_back("Moonlight lets agents see at night");
	tips.push_back("Moonlight lets agents see at night");
	tips.push_back("Hadean Epochs are HOT!");
	tips.push_back("Hadean Epochs are HOT!");
	tips.push_back("Ice Age Epochs are COOL!");
	tips.push_back("Ice Age Epochs are COOL!");
	tips.push_back("Lots of dead on Epoch 0? It's normal");
	tips.push_back("Early agents die a lot");
	tips.push_back("Lots of dead on Epoch 0 is normal");
	tips.push_back("Mutation x# Epochs= more mutations");
	tips.push_back("Mutation x2 = double mutations");
	tips.push_back("Mutation x3 = tripple mutations");
	tips.push_back("'report.txt' logs useful data");
	tips.push_back("Dizzy? Spawned agents like to spin");
	tips.push_back("Dizzy? Spawned agents like to spin");
	tips.push_back("Dizzy? Spawned agents like to spin");
	tips.push_back("Dizzy? Spawned agents like to spin");
	tips.push_back("Dizzy? Spawned agents like to spin");
	tips.push_back("Tiny agents have radius < 5");
	tips.push_back("Tiny agents have radius < 5");
	tips.push_back("Tiny agents have radius < 5");
	tips.push_back("Tiny agents have fewer eyes, ears");
	tips.push_back("Tiny agents have only one clock");
	tips.push_back("Tiny agents can't wield spikes");
	tips.push_back("Tiny agents can't have spikes");
	tips.push_back("Tiny agents can only bite attack");
	tips.push_back("The settings.cfg is FUN, isnt it?!");
	tips.push_back("Settings.cfg file can change constants");
	tips.push_back("Use settings.cfg to change constants");
	tips.push_back("Use settings.cfg to change constants");
	tips.push_back("Use settings.cfg to change constants");
	tips.push_back("Use settings.cfg to change constants");
	tips.push_back("You can disable sounds in the UI");
	tips.push_back("You can disable sounds in the UI");
	tips.push_back("You can disable sounds in the UI");
	tips.push_back("You can disable music in the UI");
	tips.push_back("You can disable music in the UI");
	tips.push_back("You can disable music in the UI");
	tips.push_back("See more options with right-click");
	tips.push_back("See more options with right-click");
	tips.push_back("See more options with right-click");
	tips.push_back("Contribute on Github!");
	tips.push_back("Press 'h' for interface help");
	tips.push_back("Tips will be disabled; see settings.cfg");
//	}

	for(int i=0; i<5; i++) last5songs[i]= -1;
	
//	#pragma omp critical
//	tryPlayAudio(conf::SFX_PLOP1); //finished setting up! make a sound
}

void World::readConfig()
{
	INITPLANT= -1; //set these to a default value before loading config, so after loading, we read if we changed them! magic
	INITFRUIT= -1;
	INITMEAT= -1;
	INITHAZARD= -1;

	//get world constants from config file, settings.cfg
	char line[256], *pos;
	char var[32];
	char dataval[32];
	int i; //integer buffer
	float f; //float buffer

	//first check version number
	printf("...looking for settings.cfg...\n");

	FILE* cf = fopen("settings.cfg", "r");
	if(cf){
		addEvent("settings.cfg detected & loaded", EventColor::WHITE);
		printf("...tweaking the following constants: ");
		while(!feof(cf)){
			fgets(line, sizeof(line), cf);
			pos= strtok(line,"\n");
			sscanf(line, "%s%s", var, dataval);

			if(strcmp(var, "V=")==0){ //strcmp= 0 when the arguements equal
				sscanf(dataval, "%f", &f);
				if(f!=conf::VERSION) {
					fclose(cf);
					printf("CONFIG FILE VERSION MISMATCH!\n EXPECTED 'V= %3.2f', found 'V= %3.2f'\n", conf::VERSION, f);
					printf("Need to overwrite config to avoid potentially serious errors!\n");
					printf("Exit now to keep your custom settings file; otherwise, hit enter to overwrite. . .");
					cin.get();
					writeConfig();
					readConfig();
					break;
				}
			}else if(strcmp(var, "CONTINENTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=CONTINENTS) printf("CONTINENTS, ");
				CONTINENTS= i;
			}else if(strcmp(var, "CONTINENT_ROUGHNESS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=CONTINENT_ROUGHNESS) printf("CONTINENT_ROUGHNESS, ");
				CONTINENT_ROUGHNESS= i;
			}else if(strcmp(var, "OCEANPERCENT=")==0 && agents.size() == 0){ //ok so what am I doing here: need to ONLY load this value
				//if this is the first load of the config. Since agents should be non-empty and we haven't sanitized yet, it doubles as a flag
				sscanf(dataval, "%f", &f);
				if(f!=OCEANPERCENT) printf("OCEANPERCENT, ");
				OCEANPERCENT= f;
			}else if(strcmp(var, "SPAWN_LAKES=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)SPAWN_LAKES) printf("SPAWN_LAKES, ");
				if(i==1) SPAWN_LAKES= true;
				else SPAWN_LAKES= false;
			}else if(strcmp(var, "ISLANDNESS=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=ISLANDNESS) printf("ISLANDNESS, ");
				ISLANDNESS= f;
			}else if(strcmp(var, "FEATURES_TO_SPAWN=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=FEATURES_TO_SPAWN) printf("FEATURES_TO_SPAWN, ");
				FEATURES_TO_SPAWN= i;
			}else if(strcmp(var, "DISABLE_LAND_SPAWN=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)DISABLE_LAND_SPAWN) printf("DISABLE_LAND_SPAWN, ");
				if(i==1) DISABLE_LAND_SPAWN= true;
				else DISABLE_LAND_SPAWN= false;
			}else if(strcmp(var, "NOTIPS=")==0 || strcmp(var, "NO_TIPS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)NO_TIPS) printf("NO_TIPS, ");
				if(i==1) NO_TIPS= true;
				else NO_TIPS= false;
			}else if(strcmp(var, "NO_DEMO=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(1-(int)DEMO)) printf("DEMO (NO_DEMO), ");
				if(i==1) DEMO= false; //DEMO vs NO_DEMO: setting is reversed, and we IGNORE it if it's 0 (otherwise we would get weird unintended behavior when loading/saving)
				//else DEMO= true;
			}else if(strcmp(var, "AMBIENT_LIGHT_PERCENT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=AMBIENT_LIGHT_PERCENT) printf("AMBIENT_LIGHT_PERCENT, ");
				AMBIENT_LIGHT_PERCENT= f;
			}else if(strcmp(var, "AGENTS_SEE_CELLS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)AGENTS_SEE_CELLS) printf("AGENTS_SEE_CELLS, ");
				if(i==1) AGENTS_SEE_CELLS= true;
				else AGENTS_SEE_CELLS= false;
			}else if(strcmp(var, "AGENTS_DONT_OVERDONATE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)AGENTS_DONT_OVERDONATE) printf("AGENTS_DONT_OVERDONATE, ");
				if(i==1) AGENTS_DONT_OVERDONATE= true;
				else AGENTS_DONT_OVERDONATE= false;
			}else if(strcmp(var, "MOONLIT=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)MOONLIT) printf("MOONLIT, ");
				if(i==1) MOONLIT= true;
				else MOONLIT= false;
			}else if(strcmp(var, "MOONLIGHTMULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MOONLIGHTMULT) printf("MOONLIGHTMULT, ");
				MOONLIGHTMULT= f;
			}else if(strcmp(var, "DROUGHTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)DROUGHTS) printf("DROUGHTS, ");
				if(i==1) DROUGHTS= true;
				else DROUGHTS= false;
			}else if(strcmp(var, "DROUGHT_MIN=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DROUGHT_MIN) printf("DROUGHT_MIN, ");
				DROUGHT_MIN= f;
			}else if(strcmp(var, "DROUGHT_MAX=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DROUGHT_MAX) printf("DROUGHT_MAX, ");
				DROUGHT_MAX= f;
			}else if(strcmp(var, "MUTEVENTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)MUTEVENTS) printf("MUTEVENTS, ");
				if(i==1) MUTEVENTS= true;
				else MUTEVENTS= true;
			}else if(strcmp(var, "MUTEVENT_MAX=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=MUTEVENT_MAX) printf("MUTEVENT_MAX, ");
				MUTEVENT_MAX= i;
			}else if(strcmp(var, "CLIMATE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)CLIMATE) printf("CLIMATE, ");
				if(i==1) CLIMATE= true;
				else CLIMATE= false;
			}else if(strcmp(var, "CLIMATE_INTENSITY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=CLIMATE_INTENSITY) printf("CLIMATE_INTENSITY, ");
				CLIMATE_INTENSITY= f;
			}else if(strcmp(var, "CLIMATEMULT_AVERAGE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=CLIMATEMULT_AVERAGE) printf("CLIMATEMULT_AVERAGE, ");
				CLIMATEMULT_AVERAGE= f;
			}else if(strcmp(var, "CLIMATE_AFFECT_FLORA=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)CLIMATE_AFFECT_FLORA) printf("CLIMATE_AFFECT_FLORA, ");
				if(i==1) CLIMATE_AFFECT_FLORA= true;
				else CLIMATE_AFFECT_FLORA= false;
			}else if(strcmp(var, "MIN_FOOD=")==0 || strcmp(var, "MINFOOD=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=MIN_PLANT) printf("MIN_FOOD, ");
				MIN_PLANT= i;
			}else if(strcmp(var, "INITPLANTDENSITY=")==0 || strcmp(var, "INITFOODDENSITY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=INITPLANTDENSITY) printf("INITPLANTDENSITY, ");
				INITPLANTDENSITY= f;
			}else if(strcmp(var, "INITPLANT=")==0 || strcmp(var, "INITFOOD=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=INITPLANT) printf("INITPLANT, ");
				INITPLANT= i;
			}else if(strcmp(var, "INITFRUITDENSITY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=INITFRUITDENSITY) printf("INITFRUITDENSITY, ");
				INITFRUITDENSITY= f;
			}else if(strcmp(var, "INITFRUIT=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=INITFRUIT) printf("INITFRUIT, ");
				INITFRUIT= i;
			}else if(strcmp(var, "INITMEATDENSITY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=INITMEATDENSITY) printf("INITMEATDENSITY, ");
				INITMEATDENSITY= f;
			}else if(strcmp(var, "INITMEAT=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=INITMEAT) printf("INITMEAT, ");
				INITMEAT= i;
			}else if(strcmp(var, "INITHAZARDDENSITY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=INITHAZARDDENSITY) printf("INITHAZARDDENSITY, ");
				INITHAZARDDENSITY= f;
			}else if(strcmp(var, "INITHAZARD=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=INITHAZARD) printf("INITHAZARD, ");
				INITHAZARD= i;
			}else if(strcmp(var, "AGENTS_MIN_NOTCLOSED=")==0 || strcmp(var, "NUMBOTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=AGENTS_MIN_NOTCLOSED) printf("AGENTS_MIN_NOTCLOSED, ");
				AGENTS_MIN_NOTCLOSED= i;
			}else if(strcmp(var, "AGENTS_MAX_SPAWN=")==0 || strcmp(var, "ENOUGHBOTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=AGENTS_MAX_SPAWN) printf("AGENTS_MAX_SPAWN, ");
				AGENTS_MAX_SPAWN= i;
			}else if(strcmp(var, "AGENTSPAWN_FREQ=")==0 || strcmp(var, "NOTENOUGHFREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=AGENTSPAWN_FREQ) printf("AGENTSPAWN_FREQ, ");
				AGENTSPAWN_FREQ= i;
			}else if(strcmp(var, "AGENTS_MAX_NOOXYGEN=")==0 || strcmp(var, "TOOMANYBOTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=AGENTS_MAX_NOOXYGEN) printf("AGENTS_MAX_NOOXYGEN, ");
				AGENTS_MAX_NOOXYGEN= i;
			}else if(strcmp(var, "REPORTS_PER_EPOCH=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=REPORTS_PER_EPOCH) printf("REPORTS_PER_EPOCH, ");
				REPORTS_PER_EPOCH= i;
			}else if(strcmp(var, "FRAMES_PER_EPOCH=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=FRAMES_PER_EPOCH) printf("FRAMES_PER_EPOCH, ");
				FRAMES_PER_EPOCH= i;
			}else if(strcmp(var, "FRAMES_PER_DAY=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=FRAMES_PER_DAY) printf("FRAMES_PER_DAY, ");
				FRAMES_PER_DAY= i;
			}else if(strcmp(var, "GRAVITY_ACCELERATION=")==0 || strcmp(var, "GRAVITYACCEL=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=GRAVITY_ACCELERATION) printf("GRAVITY_ACCELERATION, ");
				GRAVITY_ACCELERATION= f;
			}else if(strcmp(var, "BUMP_PRESSURE=")==0 || strcmp(var, "REACTPOWER=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BUMP_PRESSURE) printf("BUMP_PRESSURE, ");
				BUMP_PRESSURE= f;
			}else if(strcmp(var, "GRAB_PRESSURE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=GRAB_PRESSURE) printf("GRAB_PRESSURE, ");
				GRAB_PRESSURE= f;
			}else if(strcmp(var, "BRAINBOXES=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=BRAINBOXES) {
					if(modcounter!=0) printf("\nALERT: changing brain size is not allowed when the simulation is in progress!\n");
					else {
						printf("BRAINBOXES, ");
						BRAINBOXES= i;
					}
				}
			}else if(strcmp(var, "BRAINCONNS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=BRAINCONNS) printf("BRAINCONNS, ");
				BRAINCONNS= i;
			}else if(strcmp(var, "WHEEL_SPEED=")==0 || strcmp(var, "BOTSPEED=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=WHEEL_SPEED) printf("WHEEL_SPEED, ");
				WHEEL_SPEED= f;
			}else if(strcmp(var, "JUMP_MOVE_BONUS_MULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=JUMP_MOVE_BONUS_MULT) printf("JUMP_MOVE_BONUS_MULT, ");
				JUMP_MOVE_BONUS_MULT= f;
			}else if(strcmp(var, "BOOST_MOVE_MULT=")==0 || strcmp(var, "BOOSTSIZEMULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BOOST_MOVE_MULT) printf("BOOST_MOVE_MULT, ");
				BOOST_MOVE_MULT= f;
			}else if(strcmp(var, "BOOST_EXAUSTION_MULT=")==0 || strcmp(var, "BOOSTEXAUSTMULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BOOST_EXAUSTION_MULT) printf("BOOST_EXAUSTION_MULT, ");
				BOOST_EXAUSTION_MULT= f;
			}else if(strcmp(var, "CORPSE_FRAMES=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=CORPSE_FRAMES) printf("CORPSE_FRAMES, ");
				CORPSE_FRAMES= i;
			}else if(strcmp(var, "CORPSE_MEAT_MIN=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=CORPSE_MEAT_MIN) printf("CORPSE_MEAT_MIN, ");
				CORPSE_MEAT_MIN= f;
			}else if(strcmp(var, "SOUND_PITCH_RANGE=")==0 || strcmp(var, "SOUNDPITCHRANGE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SOUND_PITCH_RANGE) printf("SOUND_PITCH_RANGE, ");
				SOUND_PITCH_RANGE= f;
			}else if(strcmp(var, "GENEROSITY_RATE=")==0 || strcmp(var, "GENEROCITY_RATE=")==0 || strcmp(var, "FOODTRANSFER=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=GENEROSITY_RATE) printf("GENEROSITY_RATE, ");
				GENEROSITY_RATE= f;
			}else if(strcmp(var, "BASEEXHAUSTION=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BASEEXHAUSTION) printf("BASEEXHAUSTION, ");
				BASEEXHAUSTION= f;
			}else if(strcmp(var, "EXHAUSTION_MULT_PER_OUTPUT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=EXHAUSTION_MULT_PER_OUTPUT) printf("EXHAUSTION_MULT_PER_OUTPUT, ");
				EXHAUSTION_MULT_PER_OUTPUT= f;
			}else if(strcmp(var, "EXHAUSTION_MULT_PER_CONN=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=EXHAUSTION_MULT_PER_CONN) printf("EXHAUSTION_MULT_PER_CONN, ");
				EXHAUSTION_MULT_PER_CONN= f;
			}else if(strcmp(var, "ENCUMBERED_MOVE_MULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=ENCUMBERED_MOVE_MULT) printf("ENCUMBERED_MOVE_MULT, ");
				ENCUMBERED_MOVE_MULT= f;
			}else if(strcmp(var, "MEANRADIUS=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEANRADIUS) printf("MEANRADIUS, ");
				MEANRADIUS= f;
			}else if(strcmp(var, "SPIKESPEED=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SPIKESPEED) printf("SPIKESPEED, ");
				SPIKESPEED= f;
			}else if(strcmp(var, "SPAWN_MIRROR_EYES=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)SPAWN_MIRROR_EYES) printf("SPAWN_MIRROR_EYES, ");
				if(i==1) SPAWN_MIRROR_EYES= true;
				else SPAWN_MIRROR_EYES= false;
			}else if(strcmp(var, "PRESERVE_MIRROR_EYES=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)PRESERVE_MIRROR_EYES) printf("PRESERVE_MIRROR_EYES, ");
				if(i==1) PRESERVE_MIRROR_EYES= true;
				else PRESERVE_MIRROR_EYES= false;
			}else if(strcmp(var, "FRESHKILLTIME=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=FRESHKILLTIME) printf("FRESHKILLTIME, ");
				FRESHKILLTIME= i;
			}else if(strcmp(var, "TENDERAGE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=TENDERAGE) printf("TENDERAGE, ");
				TENDERAGE= i;
			}else if(strcmp(var, "MINMOMHEALTH=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MINMOMHEALTH) printf("MINMOMHEALTH, ");
				MINMOMHEALTH= f;
			}else if(strcmp(var, "MIN_INTAKE_HEALTH_RATIO=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MIN_INTAKE_HEALTH_RATIO) printf("MIN_INTAKE_HEALTH_RATIO, ");
				MIN_INTAKE_HEALTH_RATIO= f;
			}else if(strcmp(var, "FUN=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)FUN) for(int x=0; x<420; x++) printf("OHMYGODWHATHAVEYOUDONE");
				FUN= true;
			}else if(strcmp(var, "SUN_RED=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SUN_RED) printf("SUN_RED, ");
				SUN_RED= f;
			}else if(strcmp(var, "SUN_GRE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SUN_GRE) printf("SUN_GRE, ");
				SUN_GRE= f;
			}
			//hit a compiler limit; resetting the if blocks
			if(strcmp(var, "SUN_BLU=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SUN_BLU) printf("SUN_BLU, ");
				SUN_BLU= f;
			}else if(strcmp(var, "REP_PER_BABY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=REP_PER_BABY) printf("REP_PER_BABY, ");
				REP_PER_BABY= f;
			}else if(strcmp(var, "OVERHEAL_REPFILL=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=OVERHEAL_REPFILL) printf("OVERHEAL_REPFILL, ");
				OVERHEAL_REPFILL= f;
//			}else if(strcmp(var, "LEARNRATE=")==0){
//				sscanf(dataval, "%f", &f);
//				if(f!=LEARNRATE) printf("LEARNRATE, ");
//				LEARNRATE= f;
			}else if(strcmp(var, "OVERRIDE_KINRANGE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=OVERRIDE_KINRANGE) printf("OVERRIDE_KINRANGE, ");
				OVERRIDE_KINRANGE= i;
			}else if(strcmp(var, "MAXDEVIATION=")==0){ //MAXDEVIATION is an old config value for OVERRIDE_KINRANGE that was saved in float form
				sscanf(dataval, "%f", &f);
				if((int)f!=OVERRIDE_KINRANGE) printf("OVERRIDE_KINRANGE (MAXDEVIATION), ");
				OVERRIDE_KINRANGE= (int)f;
			}else if(strcmp(var, "DEFAULT_BRAIN_MUTCHANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DEFAULT_BRAIN_MUTCHANCE) printf("DEFAULT_BRAIN_MUTCHANCE, ");
				DEFAULT_BRAIN_MUTCHANCE= f;
			}else if(strcmp(var, "DEFAULT_BRAIN_MUTSIZE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DEFAULT_BRAIN_MUTSIZE) printf("DEFAULT_BRAIN_MUTSIZE, ");
				DEFAULT_BRAIN_MUTSIZE= f;
			}else if(strcmp(var, "DEFAULT_GENE_MUTCHANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DEFAULT_GENE_MUTCHANCE) printf("DEFAULT_GENE_MUTCHANCE, ");
				DEFAULT_GENE_MUTCHANCE= f;
			}else if(strcmp(var, "DEFAULT_GENE_MUTSIZE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DEFAULT_GENE_MUTSIZE) printf("DEFAULT_GENE_MUTSIZE, ");
				DEFAULT_GENE_MUTSIZE= f;
			}else if(strcmp(var, "MUTCHANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DEFAULT_BRAIN_MUTCHANCE) printf("DEFAULT_BRAIN_MUTCHANCE, ");
				if(f!=DEFAULT_GENE_MUTCHANCE) printf("DEFAULT_GENE_MUTCHANCE, ");
				DEFAULT_BRAIN_MUTCHANCE= f;
				DEFAULT_GENE_MUTCHANCE= f;
			}else if(strcmp(var, "MUTSIZE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DEFAULT_BRAIN_MUTSIZE) printf("DEFAULT_BRAIN_MUTSIZE, ");
				if(f!=DEFAULT_GENE_MUTSIZE) printf("DEFAULT_GENE_MUTSIZE, ");
				DEFAULT_BRAIN_MUTSIZE= f;
				DEFAULT_GENE_MUTSIZE= f;
			}else if(strcmp(var, "LIVE_MUTATE_CHANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=LIVE_MUTATE_CHANCE) printf("LIVE_MUTATE_CHANCE, ");
				LIVE_MUTATE_CHANCE= f;
			}else if(strcmp(var, "MAXAGE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=MAXAGE) printf("MAXAGE, ");
				MAXAGE= i;
			}else if(strcmp(var, "MAXWASTEFREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=MAXWASTEFREQ) printf("MAXWASTEFREQ, ");
				MAXWASTEFREQ= i;
			}else if(strcmp(var, "MAX_SENSORY_DISTANCE=")==0 || strcmp(var, "DIST=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MAX_SENSORY_DISTANCE) printf("MAX_SENSORY_DISTANCE, ");
				MAX_SENSORY_DISTANCE= f;
			}else if(strcmp(var, "SPIKE_LENGTH=")==0 || strcmp(var, "SPIKELENGTH=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SPIKE_LENGTH) printf("SPIKE_LENGTH, ");
				SPIKE_LENGTH= f;
			}else if(strcmp(var, "BITE_DISTANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BITE_DISTANCE) printf("BITE_DISTANCE, ");
				BITE_DISTANCE= f;
			}else if(strcmp(var, "BUMP_DAMAGE_OVERLAP=")==0 || strcmp(var, "TOOCLOSE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BUMP_DAMAGE_OVERLAP) printf("BUMP_DAMAGE_OVERLAP, ");
				BUMP_DAMAGE_OVERLAP= f;
			}else if(strcmp(var, "FOOD_SHARING_DISTANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FOOD_SHARING_DISTANCE) printf("FOOD_SHARING_DISTANCE, ");
				FOOD_SHARING_DISTANCE= f;
			}else if(strcmp(var, "SEXTING_DISTANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SEXTING_DISTANCE) printf("SEXTING_DISTANCE, ");
				SEXTING_DISTANCE= f;
			}else if(strcmp(var, "GRABBING_DISTANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=GRABBING_DISTANCE) printf("GRABBING_DISTANCE, ");
				GRABBING_DISTANCE= f;
			}else if(strcmp(var, "HEALTHLOSS_WHEELS=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_WHEELS) printf("HEALTHLOSS_WHEELS, ");
				HEALTHLOSS_WHEELS= f;
			}else if(strcmp(var, "HEALTHLOSS_BOOSTMULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_BOOSTMULT) printf("HEALTHLOSS_BOOSTMULT, ");
				HEALTHLOSS_BOOSTMULT= f;
			}else if(strcmp(var, "HEALTHLOSS_BADTEMP=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_BADTEMP) printf("HEALTHLOSS_BADTEMP, ");
				HEALTHLOSS_BADTEMP= f;
			}else if(strcmp(var, "HEALTHLOSS_AGING=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_AGING) printf("HEALTHLOSS_AGING, ");
				HEALTHLOSS_AGING= f;
			}else if(strcmp(var, "HEALTHLOSS_EXHAUSTION=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_EXHAUSTION) printf("HEALTHLOSS_EXHAUSTION, ");
				HEALTHLOSS_EXHAUSTION= f;
			}else if(strcmp(var, "HEALTHLOSS_BRAINUSE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_BRAINUSE) printf("HEALTHLOSS_BRAINUSE, ");
				HEALTHLOSS_BRAINUSE= f;
			}else if(strcmp(var, "HEALTHLOSS_SPIKE_EXT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_SPIKE_EXT) printf("HEALTHLOSS_SPIKE_EXT, ");
				HEALTHLOSS_SPIKE_EXT= f;
			}else if(strcmp(var, "HEALTHLOSS_BADTERRAIN=")==0 || strcmp(var, "HEALTHLOSS_BADAIR=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_BADTERRAIN) printf("HEALTHLOSS_BADTERRAIN, ");
				HEALTHLOSS_BADTERRAIN= f;
			}else if(strcmp(var, "HEALTHLOSS_NOOXYGEN=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_NOOXYGEN) printf("HEALTHLOSS_NOOXYGEN, ");
				HEALTHLOSS_NOOXYGEN= f;
			}else if(strcmp(var, "HEALTHLOSS_ASSEX=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HEALTHLOSS_ASSEX) printf("HEALTHLOSS_ASSEX, ");
				HEALTHLOSS_ASSEX= f;
			}else if(strcmp(var, "DAMAGE_FULLSPIKE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DAMAGE_FULLSPIKE) printf("DAMAGE_FULLSPIKE, ");
				DAMAGE_FULLSPIKE= f;
			}else if(strcmp(var, "DAMAGE_COLLIDE=")==0 || strcmp(var, "HEALTHLOSS_BUMP=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DAMAGE_COLLIDE) printf("DAMAGE_COLLIDE, ");
				DAMAGE_COLLIDE= f;
			}else if(strcmp(var, "DAMAGE_JAWSNAP=")==0 || strcmp(var, "HEALTHLOSS_JAWSNAP=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DAMAGE_JAWSNAP) printf("DAMAGE_JAWSNAP, ");
				DAMAGE_JAWSNAP= f;
			}else if(strcmp(var, "STOMACH_EFFICIENCY=")==0 || strcmp(var, "STOMACH_EFF=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=STOMACH_EFFICIENCY) printf("STOMACH_EFFICIENCY, ");
				STOMACH_EFFICIENCY= f;
			}else if(strcmp(var, "PLANT_INTAKE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=PLANT_INTAKE) printf("PLANT_INTAKE, ");
				PLANT_INTAKE= f;
			}else if(strcmp(var, "PLANT_DECAY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=PLANT_DECAY) printf("PLANT_DECAY, ");
				PLANT_DECAY= f;
			}else if(strcmp(var, "PLANT_GROWTH=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=PLANT_GROWTH) printf("PLANT_GROWTH, ");
				PLANT_GROWTH= f;
			}else if(strcmp(var, "PLANT_WASTE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=PLANT_WASTE) printf("PLANT_WASTE, ");
				PLANT_WASTE= f;
			}else if(strcmp(var, "PLANT_ADD_FREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=PLANT_ADD_FREQ) printf("PLANT_ADD_FREQ, ");
				PLANT_ADD_FREQ= i;
			}else if(strcmp(var, "PLANT_SPREAD=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=PLANT_SPREAD) printf("PLANT_SPREAD, ");
				PLANT_SPREAD= f;
			}else if(strcmp(var, "PLANT_RANGE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=PLANT_RANGE) printf("PLANT_RANGE, ");
				PLANT_RANGE= i;
			}else if(strcmp(var, "PLANT_TENACITY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=PLANT_TENACITY) printf("PLANT_TENACITY, ");
				PLANT_TENACITY= f;
			}else if(strcmp(var, "FRUIT_INTAKE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FRUIT_INTAKE) printf("FRUIT_INTAKE, ");
				FRUIT_INTAKE= f;
			}else if(strcmp(var, "FRUIT_DECAY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FRUIT_DECAY) printf("FRUIT_DECAY, ");
				FRUIT_DECAY= f;
			}else if(strcmp(var, "FRUIT_WASTE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FRUIT_WASTE) printf("FRUIT_WASTE, ");
				FRUIT_WASTE= f;
			}else if(strcmp(var, "FRUIT_ADD_FREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=FRUIT_ADD_FREQ) printf("FRUIT_ADD_FREQ, ");
				FRUIT_ADD_FREQ= i;
			}else if(strcmp(var, "FRUIT_PLANT_REQUIRE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FRUIT_PLANT_REQUIRE) printf("FRUIT_PLANT_REQUIRE, ");
				FRUIT_PLANT_REQUIRE= f;
			}else if(strcmp(var, "MEAT_INTAKE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEAT_INTAKE) printf("MEAT_INTAKE, ");
				MEAT_INTAKE= f;
			}else if(strcmp(var, "MEAT_DECAY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEAT_DECAY) printf("MEAT_DECAY, ");
				MEAT_DECAY= f;
			}else if(strcmp(var, "MEAT_WASTE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEAT_WASTE) printf("MEAT_WASTE, ");
				MEAT_WASTE= f;
			}else if(strcmp(var, "MEAT_VALUE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEAT_VALUE) printf("MEAT_VALUE, ");
				MEAT_VALUE= f;
			}else if(strcmp(var, "MEAT_NON_FRESHKILL_MULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEAT_NON_FRESHKILL_MULT) printf("MEAT_NON_FRESHKILL_MULT, ");
				MEAT_NON_FRESHKILL_MULT= f;
			}else if(strcmp(var, "HAZARD_EVENT_FREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=HAZARD_EVENT_FREQ) printf("HAZARD_EVENT_FREQ, ");
				HAZARD_EVENT_FREQ= i;
			}else if(strcmp(var, "HAZARD_EVENT_MULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARD_EVENT_MULT) printf("HAZARD_EVENT_MULT, ");
				HAZARD_EVENT_MULT= f;
			}else if(strcmp(var, "HAZARD_DECAY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARD_DECAY) printf("HAZARD_DECAY, ");
				HAZARD_DECAY= f;
			}else if(strcmp(var, "HAZARD_DEPOSIT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARD_DEPOSIT) printf("HAZARD_DEPOSIT, ");
				HAZARD_DEPOSIT= f;
			}else if(strcmp(var, "HAZARD_DAMAGE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARD_DAMAGE) printf("HAZARD_DAMAGE, ");
				HAZARD_DAMAGE= f;
			}else if(strcmp(var, "HAZARD_POWER=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARD_POWER) printf("HAZARD_POWER, ");
				HAZARD_POWER= f;
			}
		}
		if(cf){
			printf("and done!...\n");
			fclose(cf);
		}

	} else {
		#if defined(_DEBUG)
		printf("_DEBUG: settings.cfg did not exist and suppressing creation in debug.\n");
		#else
		writeConfig();
		#endif
	}

	//set some other variables that are based on the possibly loaded config values
	CW= conf::WIDTH/conf::CZ;
	CH= conf::HEIGHT/conf::CZ; //note: may add custom variables from loaded saves/config here eventually
	int CS= CW*CH;

	if(INITPLANT==-1) INITPLANT = (int)(INITPLANTDENSITY*CS);
	if(INITFRUIT==-1) INITFRUIT = (int)(INITFRUITDENSITY*CS);
	if(INITMEAT==-1) INITMEAT = (int)(INITMEATDENSITY*CS);
	if(INITHAZARD==-1) INITHAZARD = (int)(INITHAZARDDENSITY*CS);
	if(INITPLANT>CS) INITPLANT= CS;
	if(INITFRUIT>CS) INITFRUIT= CS;
	if(INITMEAT>CS) INITMEAT= CS;
	if(INITHAZARD>CS) INITHAZARD= CS;

	if(DEBUG){
		printf("INITPLANT: %i\n", INITPLANT);
		printf("INITFRUIT: %i\n", INITFRUIT);
		printf("INITMEAT: %i\n", INITMEAT);
		printf("INITHAZARD: %i\n", INITHAZARD);
	}

	if(BRAINBOXES < Output::OUTPUT_SIZE) {
		printf("BRAINBOXES config value was too small. It has been reset to min allowed (number of Outputs)\n");
		BRAINBOXES = Output::OUTPUT_SIZE;
	}

	//set the highest distance config value as the distance within which we check if an agent is "near" another, for tree processing
	NEAREST= max(max(max(max(FOOD_SHARING_DISTANCE, SEXTING_DISTANCE), GRABBING_DISTANCE), SPIKE_LENGTH + MEANRADIUS*3), BITE_DISTANCE + MEANRADIUS*3);
	INV_MAX_SENSORY_DISTANCE = 1/MAX_SENSORY_DISTANCE;
	INV_TENDERAGE = 1/(float)TENDERAGE;
	INV_SOUND_PITCH_RANGE = 1/SOUND_PITCH_RANGE;
}

void World::writeConfig()
{
	//called if settings.cfg is missing or version number of program is greater
	//happens after initializing and skips loading of config
	printf("...regenerating settings.cfg...\n");

	FILE* cf= fopen("settings.cfg", "w");

	fprintf(cf, "settings.cfg\nThis file will be regenerated with default values if missing (regeneration is not necessary for the program to work)\nModify any value in this file to change constants in Evagents.\nAll changes will take effect after either a world reset, config reload, or app re-launch.\n");
	fprintf(cf, "\n");
	fprintf(cf, "Any lines that the program can't recognize are silently ignored, and anything that is changed is reported by the program.\nPlease note the sim only takes the last entry of a flag's value if there are duplicates. Also, can you find the hidden config flags?...\n");
	fprintf(cf, "\n");
	fprintf(cf, "Remember to RIGHT-CLICK and select LOAD CONFIG, or relaunch the program after modification!!!");
	fprintf(cf, "\n\n");
	fprintf(cf, "V= %f \t\t\t//Version number this file was created in. If different than program's, will ask user to exit, or it will overwrite.\n", conf::VERSION);
	fprintf(cf, "\n");
	fprintf(cf, "NO_TIPS= %i \t\t\t//if true (=1), prevents tips from being displayed. Default= 1 when writing a new config\n", 1);
	fprintf(cf, "NO_DEMO= %i \t\t\t//if true (=1), this will start Evagents normally. Demo Mode prevents the report.txt from being overwritten during Epoch 0, and always disables itself on Epoch 1. Meant to prevent deletion of previous report.txts if you forgot to make a copy. Set to 0 to allow Demo Mode to start when you relaunch the program.\n", 1);
	fprintf(cf, "\n");
	fprintf(cf, "CONTINENTS= %i \t\t\t//number of 'continents' generated on the land layer. Not guaranteed to be accurate\n", conf::CONTINENTS);
	fprintf(cf, "CONTINENT_ROUGHNESS= %i \t\t//multiplier for number of 'seed' points for BOTH continents and ocean. Higher values will result in more 'granular' continent shapes and possibly more 'continents' than the above value indicates. 0 will make no continents at all, only ocean.\n", conf::CONTINENT_ROUGHNESS);
	fprintf(cf, "OCEANPERCENT= %f \t\t//decimal ratio of terrain layer which will be ocean. Approximately\n", conf::OCEANPERCENT);
	fprintf(cf, "SPAWN_LAKES= %i \t\t\t//if true (=1), and if terrain generation forms too much or too little land, the generator takes a moment to put in lakes (or islands)\n", conf::SPAWN_LAKES);
	fprintf(cf, "ISLANDNESS= %f \t\t//how much of the terrain, roughly, is going to be lakes/islands. 0 means lakes and islands will not generate, 0.5 means 50%% of the world will be islands/lakes.\n", conf::ISLANDNESS);
	fprintf(cf, "FEATURES_TO_SPAWN= %i \t\t//integer number of terrain features to try and generate (not all are guarenteed to appear). Set to 0 to disable\n", conf::FEATURES_TO_SPAWN);
	fprintf(cf, "\n");
	fprintf(cf, "DISABLE_LAND_SPAWN= %i \t\t//true-false flag for disabling agents from spawning on land. 0= land spawn allowed, 1= not allowed. Is GUI-controllable. This value is whatever was set in program when this file was saved\n", DISABLE_LAND_SPAWN);
	fprintf(cf, "AMBIENT_LIGHT_PERCENT= %f //multiplier of the natural light level that eyes will see. Be cautious with this; too high (>0.5) and cells will wash out agents. Note, if AGENTS_SEE_CELLS is enabled, then this gets applied to the cell colors instead\n", conf::AMBIENT_LIGHT_PERCENT);
	fprintf(cf, "AGENTS_SEE_CELLS= %i \t\t//true-false flag for letting agents see cells. 0= agents only see other agents and an ambient day/night brightness, 1= agents see agents and cells with day/night brightness applied, and is considerably laggier. Is saved/loaded.\n", conf::AGENTS_SEE_CELLS);
	fprintf(cf, "AGENTS_DONT_OVERDONATE= %i \t//true-false flag for letting agents over-donate heath to others. 0= agents will give freely as long as they want to, even to death, 1= agents will only give up to half the difference in health, meaning everyone gets equal amounts of health.\n", conf::AGENTS_DONT_OVERDONATE);
	fprintf(cf, "MOONLIT= %i \t\t\t//true-false flag for letting agents see other agents at night. 0= no eyesight, 1= see agents at half light. Is GUI-controllable and saved/loaded. This value is whatever was set in program when this file was saved\n", MOONLIT);
	fprintf(cf, "MOONLIGHTMULT= %f \t//the amount of minimum light MOONLIT provides. If set to 1, daylight cycles are not processed at all and the world becomes bathed in eternal sunlight.\n", conf::MOONLIGHTMULT);
	fprintf(cf, "DROUGHTS= %i \t\t\t//true-false flag for if the drought/overgrowth mechanic is enabled. 0= constant normal growth, 1= variable. Is GUI-controllable and saved/loaded. This value is whatever was set in program when this file was saved\n", DROUGHTS);
	fprintf(cf, "DROUGHT_MIN= %f \t\t//minimum multiplier value of droughts, below which it gets pushed back toward 1.0\n", conf::DROUGHT_MIN);
	fprintf(cf, "DROUGHT_MAX= %f \t\t//maximum multiplier value of droughts (overgrowth), above which it gets pushed back toward 1.0\n", conf::DROUGHT_MAX);
	fprintf(cf, "MUTEVENTS= %i \t\t\t//true-false flag for if the mutation event mechanic is enabled. 0= constant mutation rates, 1= variable. Is GUI-controllable and saved/loaded. This value is whatever was set in program when this file was saved\n", MUTEVENTS);
	fprintf(cf, "MUTEVENT_MAX= %i \t\t//integer max possible multiplier for mutation event mechanic. =1 effectively disables. =0 enables 50%% chance of no live mutations epoch. Negative values increase this chance (-1 => 66%%, -2 => 75%%, etc), thereby decreasing the chance of any live mutations at all. Positive values are useful if you want to increase base mutation rates.\n", conf::MUTEVENT_MAX);
	fprintf(cf, "CLIMATE= %i \t\t\t//true-false flag for if the global climate mechanic is enabled. 0= temperature ranges are locked in at spawn, 1= variable temp over time. Is GUI-controllable and saved/loaded. This value is whatever was set in program when this file was saved\n", CLIMATE);
	fprintf(cf, "CLIMATE_INTENSITY= %f \t//intensity multiplier of climate changes (effects both bias and mult). If this is too large (>0.01) climate will swing wildly. GUI-controlable, NOT saved with worlds. This value is whatever was set in program when this file was saved.\n", CLIMATE_INTENSITY);
	fprintf(cf, "CLIMATEMULT_AVERAGE= %f \t//the value that the climate multiplier gets pushed back towards every 0.5 epochs. Reminder: A CLIMATEMULT of 0 means uniform temp everywhere, =1 and no matter what the CLIMATEBIAS is, the full possible range of temperature exists. This value is whatever was set in program when this file was saved.\n", CLIMATEMULT_AVERAGE);
	fprintf(cf, "CLIMATE_AFFECT_FLORA= %i \t//true-false flag for if the global climate destroys plant life at the extremes. 0= temperature has no affect, 1= plant life dies at extreme temperatures (V0.06- behavior).\n", 1);
	fprintf(cf, "\n");
	fprintf(cf, "MINFOOD= %i \t\t\t//Minimum number of food cells which must have food during simulation. 0= off\n", conf::MIN_PLANT);
	fprintf(cf, "INITFOODDENSITY= %f \t//initial density of full food cells. Is a decimal percentage of the world cells. Use 'INITFOOD= #' to set a number\n", conf::INITPLANTDENSITY);
	fprintf(cf, "//INITFOOD= %i \t\t\t//remove '//' from before the flag to enable, overrides INITFOODDENSITY, this is a raw # of cells which must have food at init\n", 0);
	fprintf(cf, "INITFRUITDENSITY= %f \t//initial density of full fruit cells. Is a decimal percentage of the world cells.\n", conf::INITFRUITDENSITY);
	fprintf(cf, "//INITFRUIT= %i \t\t\t//remove '//' from before the flag to enable, overrides INITFRUITDENSITY, this is a raw # of cells which must have fruit at init\n", 0);
	fprintf(cf, "INITMEATDENSITY= %f \t//initial density of full meat cells. Is a decimal percentage of the world cells. Use 'INITMEAT= #' to set a number\n", conf::INITMEATDENSITY);
	fprintf(cf, "//INITMEAT= %i \t\t\t//remove '//' from before the flag to enable, overrides INITMEATDENSITY, this is a raw # of cells which must have meat at init\n", 0);
	fprintf(cf, "INITHAZARDDENSITY= %f \t//initial density of full hazard cells. Is a decimal percentage of the world cells.\n", conf::INITHAZARDDENSITY);
	fprintf(cf, "//INITHAZARD= %i \t\t//remove '//' from before the flag to enable, overrides INITHAZARDDENSITY, this is a raw # of cells which must have hazard at init\n", 0);
	fprintf(cf, "AGENTS_MIN_NOTCLOSED= %i \t//minimum number of agents. Random agents will get spawned in until this number is reached while the world is Open (as opposed to Closed, or all random spawns disabled).\n", conf::AGENTS_MIN_NOTCLOSED);
	fprintf(cf, "AGENTS_MAX_SPAWN= %i \t\t//if more agents than this number exist (live agents), random spawns no longer occur\n", conf::AGENTS_MAX_SPAWN);
	fprintf(cf, "AGENTSPAWN_FREQ= %i \t\t//how often does a random agent get spawned while below ENOUGHAGENTS? Higher values are less frequent\n", conf::AGENTSPAWN_FREQ);
	fprintf(cf, "AGENTS_MAX_NOOXYGEN= %i \t//number of agents at which the full HEALTHLOSS_NOOXYGEN is applied\n", conf::AGENTS_MAX_NOOXYGEN);
	fprintf(cf, "\n");
	fprintf(cf, "REPORTS_PER_EPOCH= %i \t\t//number of times to record data per epoch. 0 for off\n", conf::REPORTS_PER_EPOCH);
	fprintf(cf, "FRAMES_PER_EPOCH= %i \t//number of frames before epoch is incremented by 1\n", conf::FRAMES_PER_EPOCH);
	fprintf(cf, "FRAMES_PER_DAY= %i \t\t//number of frames it takes for the daylight cycle to go completely around the map\n", conf::FRAMES_PER_DAY);
	fprintf(cf, "\n");
	fprintf(cf, "GRAVITY_ACCELERATION= %f \t//how fast an agent will 'fall' after jumping. 0= jump disabled, 0.1+ = super-gravity\n", conf::GRAVITY_ACCELERATION);
	fprintf(cf, "BUMP_PRESSURE= %f \t//the restoring force between two colliding agents. 0= no reaction (disables all collisions). I'd avoid negative values if I were you...\n", conf::BUMP_PRESSURE);
	fprintf(cf, "GRAB_PRESSURE= %f \t//the restoring force between and agent and its grab target. 0= no reaction (disables grab function), negative values make grabbing agents push others away instead\n", conf::GRAB_PRESSURE);
	fprintf(cf, "SOUND_PITCH_RANGE= %f \t//range below hearhigh and above hearlow within which external sounds fade in. Would not recommend extreme values near or beyond [0,0.5]\n", conf::SOUND_PITCH_RANGE);
	fprintf(cf, "\n");
	fprintf(cf, "BRAINBOXES= %i \t\t\t//number boxes in every agent brain. Note: the sim will NEVER make brains smaller than # of Outputs. Saved per world, loaded worlds will override this value. You cannot change this value for worlds already in progress; either use New World options or restart\n", conf::BRAINBOXES);
	fprintf(cf, "BRAINCONNS= %i \t\t//number connections attempted for every init agent brain. Sim may make brains with less than this number, due to trimming. Changing this value will only effect newly spawned agents.\n", conf::BRAINCONNS);
	fprintf(cf, "WHEEL_SPEED= %f \t\t//fastest possible speed of agents. This effects so much of the sim I dont advise changing it\n", conf::WHEEL_SPEED);
	fprintf(cf, "ENCUMBERED_MOVE_MULT= %f \t//speed penalty multiplier for being encumbered from each level of encumberment. So for an agent with level 3 encumberment (perhaps from trying to walk in a terrain that's 0.6 away from their preferred), they get a mult equal to this value raised to the 3rd power.\n", conf::ENCUMBERED_MOVE_MULT);
	fprintf(cf, "MEANRADIUS= %f \t\t//\"average\" agent radius, range [0.2*this,2.2*this) (only applies to random agents, no limits on mutations). This effects SOOOO much stuff, and I would not recommend setting negative unless you like crashing programs.\n", conf::MEANRADIUS);
	fprintf(cf, "JUMP_MOVE_BONUS_MULT= %f \t//how much speed boost do agents get when jump is active? This is not the main feature of jumpping. Value of 1 gives no bonus move speed.\n", conf::JUMP_MOVE_BONUS_MULT);
	fprintf(cf, "BOOST_MOVE_MULT= %f \t//how much speed boost do agents get when boost is active? Value of 1 gives no bonus move speed.\n", conf::BOOST_MOVE_MULT);
	fprintf(cf, "BOOST_EXAUSTION_MULT= %f \t//how much exhaustion from brain is multiplied by when boost is active?\n", conf::BOOST_EXAUSTION_MULT);
	fprintf(cf, "BASEEXHAUSTION= %f \t//base value of exhaustion. When negative, is essentially the sum amount of output allowed before healthloss. Would not recommend positive values\n", conf::BASEEXHAUSTION);
	fprintf(cf, "EXHAUSTION_MULT_PER_OUTPUT= %f //multiplier applied to the sum value of all outputs; effectively, the cost multiplier of performing actions. Increase to pressure agents to chose their output more carefully\n", conf::EXHAUSTION_MULT_PER_OUTPUT);
	fprintf(cf, "EXHAUSTION_MULT_PER_CONN= %f //multiplier applied to the count of brain connections; effectively, the cost multiplier of maintaining each synapse in the brain. Increase to pressure agents to make more efficient brains.\n", conf::EXHAUSTION_MULT_PER_CONN);
	fprintf(cf, "MAXWASTEFREQ= %i \t\t//max waste frequency allowed for agents. Agents can select [1,this]. 1= no agent control allowed, 0= program crash\n", conf::MAXWASTEFREQ);
	fprintf(cf, "GENEROSITY_RATE= %f \t//how much health is transferred between two agents trading food per tick? =0 disables all generosity\n", conf::GENEROSITY_RATE);
	fprintf(cf, "SPIKESPEED= %f \t\t//how quickly can the spike be extended? Does not apply to retraction, which is instant\n", conf::SPIKESPEED);
	fprintf(cf, "SPAWN_MIRROR_EYES= %i \t\t//true-false flag for if random spawn agents have mirrored eyes. 0= asymmetry, 1= symmetry\n", (int)conf::SPAWN_MIRROR_EYES);
	fprintf(cf, "PRESERVE_MIRROR_EYES= %i \t//true-false flag for if mutations can break mirrored eye symmetry. 0= mutations do not preserve symmetry, 1= agents always have mirrored eyes\n", (int)conf::PRESERVE_MIRROR_EYES);
	fprintf(cf, "\n");
	fprintf(cf, "TENDERAGE= %i \t\t\t//age (in 1/10ths) of agents where full meat, hazard, and collision damage is finally given. These multipliers are reduced via *age/TENDERAGE. 0= off\n", conf::TENDERAGE);
	fprintf(cf, "MINMOMHEALTH= %f \t\t//minimum amount of health required for an agent to have a child\n", conf::MINMOMHEALTH);
	fprintf(cf, "MIN_INTAKE_HEALTH_RATIO= %f //minimum metabolism ratio of intake always sent to health. 0= no restrictions (agent metabolism has full control), 1= 100%% health, no babies ever. default= 0.5\n", conf::MIN_INTAKE_HEALTH_RATIO);
	fprintf(cf, "REP_PER_BABY= %f \t\t//amount of food required to be consumed for an agent to reproduce, per baby\n", conf::REP_PER_BABY);
	fprintf(cf, "OVERHEAL_REPFILL= %f \t//decimal value flag for letting agents redirect overfill health (>2) to repcounter. 0= extra intake is destroyed, punishing overeating, 1= conserves matter. Can be set to a decimal value to mult the conversion factor\n", conf::OVERHEAL_REPFILL);
//	fprintf(cf,	"LEARNRATE= %f\n", conf::LEARNRATE);
	fprintf(cf, "\n");
	fprintf(cf, "OVERRIDE_KINRANGE= %d \t\t//for the purposes of sexual reproduction and generosity, the kinrange of agents can be overridden to this value. set to -1 to disable forcing any value.\n", conf::OVERRIDE_KINRANGE);
	fprintf(cf, "DEFAULT_BRAIN_MUTCHANCE= %f //the default chance of BRAIN mutations occurring (note that various mutations modify this value up or down and this only effects initial agents)\n", conf::DEFAULT_BRAIN_MUTCHANCE);
	fprintf(cf, "DEFAULT_BRAIN_MUTSIZE= %f //the default magnitude of BRAIN mutations (note that various mutations modify this value up or down and this only effects initial agents)\n", conf::DEFAULT_BRAIN_MUTSIZE);
	fprintf(cf, "DEFAULT_GENE_MUTCHANCE= %f //the default chance of GENE mutations occurring (note that various mutations modify this value up or down and this only effects initial agents)\n", conf::DEFAULT_GENE_MUTCHANCE);
	fprintf(cf, "DEFAULT_GENE_MUTSIZE= %f \t//the default magnitude of GENE mutations (note that various mutations modify this value up or down and this only effects initial agents)\n", conf::DEFAULT_GENE_MUTSIZE);
	fprintf(cf, "LIVE_MUTATE_CHANCE= %f \t//chance, per tick, that a given agent will be mutated alive. Not typically harmful. Can be increased by mutation events if enabled.\n", conf::LIVE_MUTATE_CHANCE);
	fprintf(cf, "\n");
	fprintf(cf, "MAX_SENSORY_DISTANCE= %f //old DIST, how far the senses can detect other agents or cells\n", conf::MAX_SENSORY_DISTANCE);
	fprintf(cf, "SPIKE_LENGTH= %f \t//full spike length. Should not be more than MAX_SENSORY_DISTANCE. 0 disables interaction\n", conf::SPIKE_LENGTH);
	fprintf(cf, "BITE_DISTANCE= %f \t//distance that the bite feature ranges out beyond agent radius. Should not be more than MAX_SENSORY_DISTANCE. 0 disables interaction & rendering\n", conf::BITE_DISTANCE);
	fprintf(cf, "BUMP_DAMAGE_OVERLAP= %f \t//how much two agents can be overlapping before they take damage. 0 disables interaction\n", conf::BUMP_DAMAGE_OVERLAP);
	fprintf(cf, "FOOD_SHARING_DISTANCE= %f //how far away can food be shared between agents? Should not be more than MAX_SENSORY_DISTANCE. 0 disables interaction\n", conf::FOOD_SHARING_DISTANCE);
	fprintf(cf, "SEXTING_DISTANCE= %f \t//how far away can two agents sexually reproduce? Should not be more than MAX_SENSORY_DISTANCE. 0 disables interaction\n", conf::SEXTING_DISTANCE);
	fprintf(cf, "GRABBING_DISTANCE= %f \t//how far away can an agent grab another? Should not be more than MAX_SENSORY_DISTANCE. 0 disables interaction\n", conf::GRABBING_DISTANCE);
	fprintf(cf, "\n");
	fprintf(cf, "FRESHKILLTIME= %i \t\t//number of ticks after a spike, collision, or bite that an agent will still drop full meat\n", conf::FRESHKILLTIME);
	fprintf(cf, "CORPSE_FRAMES= %i \t\t//number of frames before dead agents are removed after meat= CORPSE_MEAT_MIN\n", conf::CORPSE_FRAMES);
	fprintf(cf, "CORPSE_MEAT_MIN= %f \t//minimum amount of meat on cell under dead agent before the agent starts counting down from CORPSE_FRAMES\n", conf::CORPSE_MEAT_MIN);
	fprintf(cf, "\n");
	fprintf(cf, "MAXAGE= %i \t\t\t//Age at which the full HEALTHLOSS_AGING amount is applied to an agent\n", conf::MAXAGE);
	fprintf(cf, "HEALTHLOSS_AGING= %f \t//health lost at MAXAGE. Note that this damage is applied to all agents in proportion to their age. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_AGING, conf::DEATH_NATURAL);
	fprintf(cf, "HEALTHLOSS_EXHAUSTION= %f //health lost from exhaustion squared. As agents get more exhausted, they loose more health. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_AGING, conf::DEATH_NATURAL);
	fprintf(cf, "HEALTHLOSS_WHEELS= %f \t//How much health is lost for an agent driving at full speed. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_WHEELS, conf::DEATH_NATURAL);
	fprintf(cf, "HEALTHLOSS_BOOSTMULT= %f \t//how much boost costs (its a multiplier; set to 1 to nullify boost cost). Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_BOOSTMULT, conf::DEATH_NATURAL);
	fprintf(cf, "HEALTHLOSS_BRAINUSE= %f \t//how much health is reduced for each box in the brain being active. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_BRAINUSE, conf::DEATH_NATURAL);
	fprintf(cf, "HEALTHLOSS_BADTEMP= %f \t//how quickly health drains in non-preferred temperatures. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_BADTEMP, conf::DEATH_BADTEMP);
	fprintf(cf, "HEALTHLOSS_SPIKE_EXT= %f \t//how much health an agent looses for extending spike. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_SPIKE_EXT, conf::DEATH_SPIKERAISE);
	fprintf(cf, "HEALTHLOSS_BADTERRAIN= %f //how much health is lost if in totally opposite environment. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_BADTERRAIN, conf::DEATH_BADTERRAIN);
	fprintf(cf, "HEALTHLOSS_NOOXYGEN= %f \t//how much agents are penalized when total agents = AGENTS_MAX_NOOXYGEN. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_NOOXYGEN, conf::DEATH_TOOMANYAGENTS);
	fprintf(cf, "HEALTHLOSS_ASSEX= %f \t//multiplier for radius/MEANRADIUS penalty on mother for asexual reproduction. Contributes to the death cause '%s'.\n", conf::HEALTHLOSS_ASSEX, conf::DEATH_ASEXUAL);
	fprintf(cf, "\n");
	fprintf(cf, "DAMAGE_FULLSPIKE= %f \t//health multiplier of spike injury. Note: it is effected by spike length and relative velocity of the agents. When used against another agent, it causes the death cause '%s'.\n", conf::DAMAGE_FULLSPIKE, conf::DEATH_SPIKE);
	fprintf(cf, "DAMAGE_COLLIDE= %f \t//how much health is lost upon collision. Note that =0 does not disable the event (see BUMP_DAMAGE_OVERLAP above). When used against another agent, it causes the death cause '%s'.\n", conf::DAMAGE_COLLIDE, conf::DEATH_COLLIDE);
	fprintf(cf, "DAMAGE_JAWSNAP= %f \t//when jaws snap fully (0->1), this is the damage applied to agents in front. When used against another agent, it causes the death cause '%s'.\n", conf::DAMAGE_JAWSNAP, conf::DEATH_BITE);
	fprintf(cf, "\n");
	fprintf(cf, "STOMACH_EFFICIENCY= %f \t//the worst possible multiplier produced from having at least two stomach types at 1. =0.1 is harsh. =1 disables (always full efficiency)\n", conf::STOMACH_EFFICIENCY);
	fprintf(cf, "\n");
	fprintf(cf, "PLANT_INTAKE= %f \t\t//how much plant food can feed an agent per tick?\n", conf::PLANT_INTAKE);
	fprintf(cf, "PLANT_DECAY= %f \t\t//how much does food decay per tick? (negative values make it grow everywhere instead)\n", conf::PLANT_DECAY);
	fprintf(cf, "PLANT_GROWTH= %f \t\t//how much does food increase by on a cell with both plant and hazard? (fertilizer effect). =0 disables\n", conf::PLANT_GROWTH);
	fprintf(cf, "PLANT_WASTE= %f \t\t//how much food disappears when an agent eats it?\n", conf::PLANT_WASTE);
	fprintf(cf, "PLANT_ADD_FREQ= %i \t\t//how often does a random cell get set to full food randomly? Lower values are more frequent\n", conf::PLANT_ADD_FREQ);
	fprintf(cf, "PLANT_SPREAD= %f \t\t//probability of a plant cell being seeded to a nearby cell. 0.0002= VERY fast food growth\n", conf::PLANT_SPREAD);
	fprintf(cf, "PLANT_RANGE= %i \t\t\t//distance that a single cell of food can seed. Units in cells.\n", conf::PLANT_RANGE);
	fprintf(cf, "PLANT_TENACITY= %f \t//Plant Tenacity is an effect where the amount of plant food taken is multiplied by the amount of plant food itself, where this value is the minimum allowed mult. Helps prevent plant cells from completely dieing. Set to 1 to disable tenacity effect, set to 0 to make herbivores never finish meals.\n", conf::PLANT_TENACITY);
	fprintf(cf, "\n");
	fprintf(cf, "FRUIT_INTAKE= %f \t\t//how much fruit can feed an agent per tick?\n", conf::FRUIT_INTAKE);
	fprintf(cf, "FRUIT_DECAY= %f \t\t//how much fruit decays per tick? This is applied *2 if less than FRUIT_PLANT_REQUIRE plant is in the cell\n", conf::FRUIT_DECAY);
	fprintf(cf, "FRUIT_WASTE= %f \t\t//how much fruit disappears when an agent eats it?\n", conf::FRUIT_WASTE);
	fprintf(cf, "FRUIT_ADD_FREQ= %i \t\t//how often does a high-plant-food cell get set to full fruit? Higher values are less frequent (must have at least FRUIT_PLANT_REQUIRE plant)\n", conf::FRUIT_ADD_FREQ);
	fprintf(cf, "FRUIT_PLANT_REQUIRE= %f \t//minimum plant food on same cell required for fruit to persist or populate\n", conf::FRUIT_PLANT_REQUIRE);
	fprintf(cf, "\n");
	fprintf(cf, "MEAT_INTAKE= %f \t\t//how much meat can feed an agent per tick?\n", conf::MEAT_INTAKE);
	fprintf(cf, "MEAT_DECAY= %f \t\t//how much meat decays on a cell per tick? (negative values make it grow everywhere instead)\n", conf::MEAT_DECAY);
	fprintf(cf, "MEAT_WASTE= %f \t\t//how much meat disappears when an agent eats it?\n", conf::MEAT_WASTE);
	fprintf(cf, "MEAT_VALUE= %f \t\t//how much meat an agent's body is worth? Typically, and agent > TENDERAGE, with no carnivore stomach, and was freshly killed, will fill a meat cell. Not being a fresh kill multiplies the amount by MEAT_NON_FRESHKILL_MULT. Being a full carnivore, dramatic reduction. And < TENDERAGE will multiply the amount by the proportion until they are TENDERAGE. This multiplies in after all that.\n", conf::MEAT_VALUE);
	fprintf(cf, "MEAT_NON_FRESHKILL_MULT= %f //mult for when the agent's death was not due to an attack (died by attrition means). 0= agents killed by other means never drop any meat, 1= no reduction from meat dropped.\n", conf::MEAT_NON_FRESHKILL_MULT);
	fprintf(cf, "\n");
	fprintf(cf, "HAZARD_EVENT_FREQ= %i \t\t//how often an instant hazard appears?\n", conf::HAZARD_EVENT_FREQ);
	fprintf(cf, "HAZARD_EVENT_MULT= %f \t//multiplier for agents standing in a cell with a hazard event ongoing. multipied after HAZARD_DAMAGE\n", conf::HAZARD_EVENT_MULT);
	fprintf(cf, "HAZARD_DECAY= %f \t\t//how much non-event hazard decays on a cell per tick? (negative values make it grow everywhere instead)\n", conf::HAZARD_DECAY);
	fprintf(cf, "HAZARD_DEPOSIT= %f \t//how much hazard is placed by an agent per tick? (Ideally. Agents can control the frequency and amount that they deposit, but in sum it should equal this per tick)\n", conf::HAZARD_DEPOSIT);
	fprintf(cf, "HAZARD_POWER= %f \t\t//power of the hazard layer value, applied before HAZARD_DAMAGE. default= 0.5 (remember, hazard in range [0,1])\n", conf::HAZARD_POWER);
	fprintf(cf, "HAZARD_DAMAGE= %f \t//how much health an agent looses while on a filled hazard cell per tick? (note that 9/10 of this is max waste damage). Contributes to the death cause '%s'.\n", conf::HAZARD_DAMAGE, conf::DEATH_HAZARD);
	fprintf(cf, "\n");
	fprintf(cf, "SUN_RED= %f \t\t//???\n", SUN_RED);
	fprintf(cf, "SUN_GRE= %f \t\t//???\n", SUN_GRE);
	fprintf(cf, "SUN_BLU= %f \t\t//???\n", SUN_BLU);
	if(STATallachieved) fprintf(cf, "FUN= 1\n");
	fclose(cf);
}