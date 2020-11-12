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
		pcontrol(false),
		pright(0),
		pleft(0),
		dosounds(true),
		timenewsong(350),
		currentsong(0)
{
	//inititalize happens only once when launching program (like right now!)
	init();	
	//reset happens any time we have to clear the world or change variables (now, or when loading a save or starting a new world)
	reset();
	//spawn happens whenever we wish to seed a world with random food, terrain, and bots (now, or after starting a new world)
	spawn();

	printf("WORLD MADE!\n");
}

World::~World()
{

}


void World::setAudioEngine(ISoundEngine* a)
{
	audio= a;
	//audio call example: if(dosounds) audio->play2D("sounds/agents/chirp1.ogg");
}


void World::tryPlayAudio(const char* soundFileName, float x, float y, float pitch, float volume)
{
	//try to play audio at location x,y if specified (if not, just play 2D audio), and with a specified pitch change multiple
	if(DEBUG) printf("Trying to play sound '%s' ... ", soundFileName);
	#pragma omp critical
	if(soundFileName!=NULL){
		if(dosounds){// && Cull::atCoords(x,y)){ //dosounds is set by GLView and disables when rendering is disabled
			ISound* play = 0;
			if(x==0 && y==0) play= audio->play2D(soundFileName, false, false, true);
			else play= audio->play3D(soundFileName, vec3df(-x, -y, 0), false, false, true);
			//-x,-y because irrklang thinks we're in opposite world if I negate the translates... it's silly
			if(play){
				play->setPlaybackSpeed(pitch);
				play->setVolume(volume);
			}
		}
	}
	if(DEBUG) printf("success!\n");
}


//void World::tryCullAudio(Controller* control)
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


void World::reset()
{
	printf("...reticulating splines...\n");
	if(currentsong && !currentsong->getIsPaused()) currentsong->setIsPaused(true);
//	if(randf(0,1)>0.5) timenewsong= randi(100,8000);

	current_epoch= 0;
	modcounter= 0;
	idcounter= 0;
	INITPLANT= -1; //set these to a crazy value before loading config, so after loading, we read if we changed them! magic
	INITFRUIT= -1;
	INITMEAT= -1;
	INITHAZARD= -1;

	//try loading constants config. Best do this first in case we change any "constants"!
	readConfig();

	//tidy up constants
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

	if(BRAINSIZE<(Input::INPUT_SIZE+Output::OUTPUT_SIZE)) {
		printf("BRAINSIZE config value was too small.\n It has been reset to min allowed (Inputs+Outputs)\n");
		BRAINSIZE= Input::INPUT_SIZE+Output::OUTPUT_SIZE;
	}

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

	//open report file; null it up if it exists. ONLY if we are not in demo mode. Mind you, demo mode could get turned off by config above
	if(!isDemo()){ 
		FILE* fr = fopen("report.txt", "w");
		fclose(fr);
	}

	ptr= 0; //bug: when world is loaded, this causes the ingame graph to be "desynced" relative to when it was saved
	for(int q=0;q<conf::RECORD_SIZE;q++) {
		numHerbivore[q]= 0;
		numCarnivore[q]= 0;
		numFrugivore[q]= 0;
		numTerrestrial[q]= 0;
		numAmphibious[q]= 0;
		numAquatic[q]= 0;
		numHybrid[q]= 0;
		numDead[q]= 0;
		numTotal[q]= 0;
	}
	lastptr= 0;

	//reset achievements
	STATuseracted= false;
	STATfirstspecies= false;
	STATfirstpopulation= false;
	STATfirstglobal= false;
	STATstrongspecies= false;
	STATstrongspecies2= false;
	STATallachieved= false;

	//reset live mutation count
	STATlivemutations= 0;

	addEvent("World Started!", EventColor::MINT);
}

void World::sanitize()
{
	printf("...sanitizing agents...\n");
	agents.clear();
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

	//add init agents
	printf("...programming agents...\n");
	addAgents(100, -2, true); //-2 creates both herbivores and frugivores alternately
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

	if(OCEANPERCENT!=1){
		int lastcx, lastcy;
		for (int i=0; i<CONTINENTS*3; i++) { //give ourselves more iterations to do extra stuff
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

			cells[Layer::ELEVATION][cx][cy]= i%2==0 ? Elevation::MOUNTAIN_HIGH : Elevation::HILL;
			//50% of the land spawns are type "Hill", = 0.7

			//if told to, find midpoint between last cell and this one, and place water, dividing the CONTINENTS!
			if(setcoast) cells[Layer::ELEVATION][(int)((cx+lastcx)/2)][(int)((cy+lastcy)/2)]= randf(0,1)>0.5 ? Elevation::SHALLOWWATER : Elevation::DEEPWATER_LOW;

			lastcx= cx;
			lastcy= cy;
		}
	}

	for (int i=0;i<1.85*(sqrt((float)CW*CH)/1000*pow((float)2.5,3*OCEANPERCENT)*(CONTINENTS+1)) ;i++) {
		//spawn oceans (water= 0)
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::ELEVATION][cx][cy]= i==0 ? Elevation::SHALLOWWATER : Elevation::DEEPWATER_LOW;
	}

	printf("...moving tectonic plates...\n");
	while(leftcount!=0){
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				float height= cells[Layer::ELEVATION][i][j];
				//land spread
				if (height>Elevation::BEACH_MID){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2); //+2 to correct for randi [x,y)
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[Layer::ELEVATION][ox][oy]==-1 && randf(0,1)>OCEANPERCENT*0.5-0.25) {//chance to spread tied to desired ocean percentage
						if(height<=Elevation::PLAINS){ //will not reduce level 0.6 and below, allowing us to create beaches (0.5) later
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
					//ok so, 1-3*height comes in here because we want shallow ocean to spread slower than deep ocean so there's less of it
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
							//if(oi==i && oj==j) continue; //this works better commented out (looks smoother)
							if(cells[Layer::ELEVATION][oi][oj]<Elevation::BEACH_MID && cells[Layer::ELEVATION][oi][oj]>=Elevation::DEEPWATER_LOW) 
								watercount++;
						}
					}

					if(watercount>=2) cells[Layer::ELEVATION][i][j]=Elevation::BEACH_MID;
				}
			}
		}
	}

	printf("...rolling hills...\n");
	for(int n=0;n<4;n++){
		cellsRoundTerrain();
	}

	STATlandratio= 0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			if(cells[Layer::ELEVATION][i][j]>=Elevation::BEACH_MID) STATlandratio++;
		}
	}
	STATlandratio/= CW*CH;

	/*if(SPAWN_LAKES){
		if((1-STATlandratio)<OCEANPERCENT) { //if ocean percentage is less than expected, form lakes!
			bool dolakes= true;
			
			while(dolakes){
				int cx=randi(0,CW);
				int cy=randi(0,CH);
				if(cells[Layer::ELEVATION][cx][cy]<Elevation::BEACH_MID) continue;
				
				float lakeheight= cells[Layer::ELEVATION][cx][cy];
				int makelake= true;
				
				while(makelake){
					int ox= randi(cx-1,cx+2);
					int oy= randi(cy-1,cy+2); //+2 to correct for randi [x,y)
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[Layer::ELEVATION][ox][oy]<=height && randf(0,1)>OCEANPERCENT*0.5-0.25) {//chance to spread tied to desired ocean percentage
						if(height<=Elevation::PLAINS){ //will not reduce level 0.6 and below, allowing us to create beaches (0.5) later
							cells[Layer::ELEVATION][ox][oy]= height;
						} else cells[Layer::ELEVATION][ox][oy]= randf(0,1)>conf::LOWER_ELEVATION_CHANCE ? height : height - 0.1;
						//there's a chance we reduce the next terrain cell by 0.1 in elevation, to produce variation!
					}
		}
	}*/

	#if defined(_DEBUG)
	printf("_DEBUG: Land %%: %.2f. The rest is water.\n",100*STATlandratio);
	#endif

	//briefly allow land spawn for Init ONLY! See update() for long-term
	if(DISABLE_LAND_SPAWN && STATlandratio>0.75){
		DISABLE_LAND_SPAWN= false;
		printf("Enabled Land Init Spawns due to high land:water ratio.\n");
	}
}

void World::cellsRoundTerrain()
{
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			int mincounts= 5;
			if(cells[Layer::ELEVATION][i][j]<=Elevation::BEACH_MID) mincounts= 4;

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

			if(greatercount>mincounts) cells[Layer::ELEVATION][i][j]= cap(cells[Layer::ELEVATION][i][j]+0.1);
			else if(lowercount>mincounts) cells[Layer::ELEVATION][i][j]= cap(cells[Layer::ELEVATION][i][j]-0.1);
			if(cells[Layer::ELEVATION][i][j]<Elevation::BEACH_MID && cells[Layer::ELEVATION][i][j]>Elevation::SHALLOWWATER)
				cells[Layer::ELEVATION][i][j]=Elevation::BEACH_MID;
			if(cells[Layer::ELEVATION][i][j]<Elevation::SHALLOWWATER)
				cells[Layer::ELEVATION][i][j]=Elevation::DEEPWATER_LOW;
		}
	}

}

void World::update()
{
	modcounter++; //mod at init== 1, this is intentional
	vector<int> dt;

	//Process periodic events
	#if defined(_DEBUG)
	if(DEBUG) printf("P");
	#endif

	//display tips
	if(!NO_TIPS || DEBUG){
		if(modcounter%conf::TIPS_PERIOD==conf::TIPS_DELAY && getEpoch()==0) {
			if(getDay()<4) addEvent(tips[min((int)(modcounter/800),randi(0,tips.size()))].c_str(), EventColor::YELLOW);
			else addEvent(tips[randi(0,tips.size())].c_str(), EventColor::YELLOW);
		}
	} else if (modcounter==conf::TIPS_DELAY && getEpoch()==0) addEvent("To re-enable tips, see settings.cfg", EventColor::YELLOW);

	//update achievements, write report, record counts, display population events
	if(!STATuseracted && pcontrol) { //outside of the report if statement b/c it can happen anytime
		addEvent("User, Take the Wheel!", EventColor::MULTICOLOR);
		STATuseracted= true;}

	//disable demo mode if epoch>0
	if(current_epoch>0) setDemo(false);

	//disable disable land spawning if the oceans are too small
	if(DISABLE_LAND_SPAWN && modcounter==1 && current_epoch==0 && STATlandratio>0.75){
		DISABLE_LAND_SPAWN= false;
		addEvent("Land spawning now enabled!", EventColor::CYAN);
		addEvent("(you can disable again in menu)", EventColor::CYAN);
	}

	//start reporting process
	if (REPORTS_PER_EPOCH>0 && modcounter%(FRAMES_PER_EPOCH/REPORTS_PER_EPOCH)==0) {
		//first, collect all stats
		findStats();

		numHerbivore[ptr]= getHerbivores();
		numCarnivore[ptr]= getCarnivores();
		numFrugivore[ptr]= getFrugivores();
		numTerrestrial[ptr]= getLungLand();
		numAmphibious[ptr]= getLungAmph();
		numAquatic[ptr]= getLungWater();
		numHybrid[ptr]= getHybrids();
		numDead[ptr]= getDead();
		numTotal[ptr]= getAlive();

		//events for achievements and stats every report cycle, every other if one was recorded 
		if(lastptr>=0){
			if(!STATfirstspecies && max(STATbestherbi,max(STATbestfrugi,STATbestcarni))==5) {
				addEvent("First Species Suriving!", EventColor::MULTICOLOR);
				STATfirstspecies= true;}
			if(!STATfirstpopulation && getAlive()>AGENTS_MAX_SPAWN) {
				addEvent("First Large Population!", EventColor::MULTICOLOR);
				STATfirstpopulation= true;}
			if(!STATfirstglobal && STATbestaquatic>=5 && STATbestterran>=5) {
				addEvent("First Global Population(s)!", EventColor::MULTICOLOR);
				STATfirstglobal= true;}
			if(!STATstrongspecies && max(STATbestherbi,max(STATbestfrugi,STATbestcarni))>=500) {
				addEvent("First to Generation 500!", EventColor::MULTICOLOR);
				STATstrongspecies= true;}
			if(!STATstrongspecies2 && max(STATbestherbi,max(STATbestfrugi,STATbestcarni))>=1000) {
				addEvent("First to Generation 1000!", EventColor::MULTICOLOR);
				STATstrongspecies2= true;}
			if(STATallachieved && FUN==true) FUN= false;
			if(!STATallachieved && STATuseracted && STATfirstspecies && STATfirstpopulation && STATfirstglobal && STATstrongspecies && STATstrongspecies2){
				addEvent("All Achievements Achieved!", EventColor::MULTICOLOR);
				STATallachieved= true;
				FUN= true;}

			int preeventcount= events.size(); //count of events before checks. Will cause next cycle to skip, reducing repeat event flags

			//generic stats between last report and current
			if(current_epoch>0 && numTotal[ptr]<=numTotal[lastptr]*0.9-20) {
				addEvent("Population Bust!", EventColor::RED);}
			if(current_epoch>0 && numTotal[ptr]==AGENTS_MIN_NOTCLOSED && STATfirstspecies && numTotal[lastptr]>AGENTS_MIN_NOTCLOSED*1.1+5) {
				addEvent("Population Crash!", EventColor::RED);}
			if(numTotal[ptr]>=numTotal[lastptr]*1.1+20) {
				//if having a major population boom, check major phenotypes
				if(numHerbivore[ptr]>=numHerbivore[lastptr]*1.1+20 && numHerbivore[ptr]>AGENTS_MIN_NOTCLOSED) {
					addEvent("Herbivore Bloom", EventColor::GREEN);}
				if(numCarnivore[ptr]>=numCarnivore[lastptr]*1.1+20 && numCarnivore[ptr]>AGENTS_MIN_NOTCLOSED) {
					addEvent("Carnivore Bloom", EventColor::GREEN);}
				if(numFrugivore[ptr]>=numFrugivore[lastptr]*1.1+20 && numFrugivore[ptr]>AGENTS_MIN_NOTCLOSED) {
					addEvent("Frugivore Bloom", EventColor::GREEN);}
				if(numAmphibious[ptr]>=numAmphibious[lastptr]*1.1+20 && numAmphibious[ptr]>AGENTS_MIN_NOTCLOSED && (numTerrestrial[ptr]>AGENTS_MIN_NOTCLOSED || numAquatic[ptr]>AGENTS_MIN_NOTCLOSED)) {
					addEvent("Amphibian Bloom", EventColor::GREEN);}
				if(numTerrestrial[ptr]>=numTerrestrial[lastptr]*1.1+20 && numTerrestrial[ptr]>AGENTS_MIN_NOTCLOSED && (numAmphibious[ptr]>AGENTS_MIN_NOTCLOSED || numAquatic[ptr]>AGENTS_MIN_NOTCLOSED)) {
					addEvent("Terrestrial Bloom", EventColor::GREEN);}
				if(numAquatic[ptr]>=numAquatic[lastptr]*1.1+20 && numAquatic[ptr]>AGENTS_MIN_NOTCLOSED && (numAmphibious[ptr]>AGENTS_MIN_NOTCLOSED || numTerrestrial[ptr]>AGENTS_MIN_NOTCLOSED)) {
					addEvent("Aquatic Bloom", EventColor::GREEN);}
			} else {
				//if not having a major population boom or bust, check slower rates of growth/death
				if(ptr>10 && numTotal[ptr]>numTotal[ptr-1] && numTotal[ptr-1]>numTotal[ptr-2] && numTotal[ptr-2]>numTotal[ptr-3]
					&& numTotal[ptr-3]>numTotal[ptr-4] && numTotal[ptr-4]>numTotal[ptr-5] && numTotal[ptr-5]>numTotal[ptr-6]
					&& numTotal[ptr-6]>numTotal[ptr-7] && numTotal[ptr-7]>numTotal[ptr-8] && numTotal[ptr-8]>numTotal[ptr-9]
					&& numTotal[ptr-9]>numTotal[ptr-10]) {
						addEvent("Steady Population Bloom", EventColor::GREEN);}
				else if(ptr>10 && numTotal[ptr]<numTotal[ptr-1] && numTotal[ptr-1]<numTotal[ptr-2] && numTotal[ptr-2]<numTotal[ptr-3]
					&& numTotal[ptr-3]<numTotal[ptr-4] && numTotal[ptr-4]<numTotal[ptr-5] && numTotal[ptr-5]<numTotal[ptr-6]
					&& numTotal[ptr-6]<numTotal[ptr-7] && numTotal[ptr-7]<numTotal[ptr-8] && numTotal[ptr-8]<numTotal[ptr-9]
					&& numTotal[ptr-9]<numTotal[ptr-10]) {
						addEvent("Steady Population Bust", EventColor::RED);}
			}

			if(events.size()>preeventcount) lastptr= -ptr; //new event? skip next cycle of checks; otherwise, update
			else lastptr= ptr;
		} else lastptr*= -1;
		
		ptr++;
		if(ptr == conf::RECORD_SIZE) ptr = 0;

		//please note: DEMO mode is checked inside this func
		writeReport();

		deaths.clear();
	} else if (modcounter%12==0) findStats(); //ocasionally collect stats regardless


	if (modcounter>=FRAMES_PER_EPOCH) {
		modcounter=0;
		current_epoch++;

		//adjust global drought multiplier at end of epoch
		if(DROUGHTS){
			DROUGHTMULT= randn(DROUGHTMULT, conf::DROUGHT_STDDEV);
			if(DROUGHTMULT>DROUGHT_MAX || DROUGHTMULT<DROUGHT_MIN) 
				DROUGHTMULT= (DROUGHTMULT*conf::DROUGHT_WEIGHT+1.0)/((float)(conf::DROUGHT_WEIGHT+1)); //try to average back wandering Droughts
			if(isDrought()) addEvent("Global Drought Epoch!", EventColor::PINK);
			if(isOvergrowth()) addEvent("Global Overgrowth Epoch!", EventColor::LIME);
			if(DEBUG) printf("Set Drought Multiplier to %f\n", DROUGHTMULT);
		} else DROUGHTMULT= 1.0;

		//adjust global mutation rate
		if(MUTEVENTS){
			MUTEVENTMULT= randf(0,1)<0.5 ? randi(min(1,MUTEVENT_MAX),max(1,MUTEVENT_MAX+1)) : 1;
			if(MUTEVENTMULT<0) MUTEVENTMULT= 0;
			if(MUTEVENTMULT==0) addEvent("No Mutation chance Epoch!?!", EventColor::LAVENDER);
			else if(MUTEVENTMULT==2) addEvent("Double Mutation chance Epoch!", EventColor::LAVENDER);
			else if(MUTEVENTMULT==3) addEvent("TRIPPLE Mutation chance Epoch!", EventColor::LAVENDER);
			else if(MUTEVENTMULT==4) addEvent("QUADRUPLE Mutation chance Epoch!", EventColor::LAVENDER);
			else if(MUTEVENTMULT>4) addEvent("Mutation chance > *4!!!", EventColor::LAVENDER);
			if(DEBUG) printf("Set Mutation Multiplier to %i\n", MUTEVENTMULT);
		}
	}

	//play music!
	timenewsong--; //reduce our song delay timer

	if(dosounds){
		if(currentsong && currentsong->getIsPaused()) currentsong->setIsPaused(false);

		if(timenewsong==0 && !currentsong) {
			int songindex= randi(0,8);
			if(current_epoch==0 && isDemo() && modcounter<1000) songindex= 0; //always play the first song first in demo mode; it's our theme song!

			//check our list of recently played song indexes to get a new index if needed
			for(int i=0; i<5; i++){
				if(songindex==last5songs[i]) { songindex= randi(0,8); i= 0; }
			}
			
			std::string songfile= conf::SONGS[songindex];
			#if defined(_DEBUG)
			printf("trying to play song: %s, index %i ", songfile.c_str(), songindex);
			#endif
			currentsong= audio->play2D(songfile.c_str(), false, false, true, ESM_NO_STREAMING);

			if(currentsong){
				//finally, add our index to the list of recently played songs
				for(int i=4; i>=0; i--){
					if(last5songs[i]==-1) {
						last5songs[(i+4)%5]= -1; // this little do-dad will set the next index to -1, allowing us to cycle!
						last5songs[i]= songindex;
						#if defined(_DEBUG)
						printf("added to list of recent songs\n");
						#endif
						break;
					}
				}
			} else {
				#if defined(_DEBUG)
				printf("...failed!\n");
				#endif
			}
		}
		if(currentsong) currentsong->setVolume(0.7);

	} else if (currentsong) {
		currentsong->setVolume(currentsong->getVolume()*0.95); //fade music if we aren't allowed to play it
		if(currentsong->getVolume()<0.001) currentsong->setIsPaused(true);
	}

	if(timenewsong<-30000 || //time-out if it's been a long while since we started a song
	(currentsong && (currentsong->isFinished() || //or if the song is finished now
	(timenewsong<-10000 && !dosounds)))) { //or if we've been in fast mode & not doing sounds for a while
		if(currentsong) currentsong->drop();
		currentsong= 0;
		timenewsong= randi(1000+ceil(modcounter*0.1),5000+ceil(modcounter*0.1));
		#if defined(_DEBUG)
		printf("setting ticks to next song: %i\n", timenewsong);
		#endif
	}

	//random seeds/spawns
	if ((modcounter%FOODADDFREQ==0 && !CLOSED) || getFood()<MIN_PLANT) {
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::PLANTS][cx][cy]= 1.0;
	}
	if (modcounter%HAZARDFREQ==0) {
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::HAZARDS][cx][cy]= cap((cells[Layer::HAZARDS][cx][cy]/90+0.99));
	}
	if (modcounter%FRUITADDFREQ==0 && randf(0,1)<abs(DROUGHTMULT)) {
		while (true) {
			int cx=randi(0,CW);
			int cy=randi(0,CH);
			if(DROUGHTMULT>0){
				if (cells[Layer::PLANTS][cx][cy]>FRUITREQUIRE) {
					cells[Layer::FRUITS][cx][cy]= 1.0;
					break;
				}
			} else { //also handle negative Drought multipliers by reducing random fruit cells
				cells[Layer::FRUITS][cx][cy]= cap(cells[Layer::FRUITS][cx][cy]-0.1);
				break;
			}
		}
	}

	#pragma omp parallel for schedule(dynamic)
	for(int cx=0; cx<(int)CW;cx++){
		for(int cy=0; cy<(int)CH;cy++){
			float plant = cells[Layer::PLANTS][cx][cy];
			float fruit = cells[Layer::FRUITS][cx][cy];
			float meat = cells[Layer::MEATS][cx][cy];
			float hazard = cells[Layer::HAZARDS][cx][cy];

			//plant ops
			if (plant>0) {
				plant-= FOODDECAY; //food quantity is changed by FOODDECAY
				if (hazard>0) {
					plant+= FOODGROWTH*max(0.0f,DROUGHTMULT)*hazard; //food grows out of waste/hazard, limited by low DROUGHTMULT
				}
			}
			if (randf(0,1)<FOODSPREAD*DROUGHTMULT && plant>=0.5) { //plant grows from itself
				//food seeding
				int ox= randi(cx-1-FOODRANGE,cx+2+FOODRANGE);
				int oy= randi(cy-1-FOODRANGE,cy+2+FOODRANGE);
				if (ox<0) ox+= CW;
				if (ox>CW-1) ox-= CW;
				if (oy<0) oy+= CH;
				if (oy>CH-1) oy-= CH; //code up to this point ensures world edges are crossed and not skipped
				if (cells[Layer::PLANTS][ox][oy]<=0.75) cells[Layer::PLANTS][ox][oy]+= 0.25;
			}
			cells[Layer::PLANTS][cx][cy]= cap(plant);
			//end plant

			//meat ops
			if (meat>0) {
				meat -= MEATDECAY;
			}
			cells[Layer::MEATS][cx][cy]= cap(meat);
			//end meat

			//fruit ops
			if (fruit>0) {
				if (plant<=FRUITREQUIRE || DROUGHTMULT<0){
					fruit-= FRUITDECAY; //fruit decays, double if lack of plant life or DROUGHTMULT is negative
				}
				fruit-= FRUITDECAY;
			}
			cells[Layer::FRUITS][cx][cy]= cap(fruit);
			//end fruit

			//hazard = cells[Layer::HAZARDS]...
			if (hazard>0 && hazard<=0.9){
				hazard-= HAZARDDECAY; //hazard decays
			} else if (hazard>0.9 && randf(0,1)<0.0625){
				hazard= 90*(hazard-0.99); //instant hazards will be reset to proportionate value
			}
			cells[Layer::HAZARDS][cx][cy]= cap(hazard);
			//end hazard

			//light ops
			//if we are moonlit and moonlight happens to be 1.0, then the light layer is useless. Set all values to 1 for rendering
			cells[Layer::LIGHT][cx][cy]= (MOONLIT && MOONLIGHTMULT==1.0) ? 1.0 :
				cap(0.5+sin((cx*2*M_PI)/CW-(modcounter+current_epoch*FRAMES_PER_EPOCH)*2*M_PI/FRAMES_PER_DAY));
			//end light
		}
	}

	#if defined(_DEBUG)
	if(DEBUG) printf("/");
	#endif

	//give input to every agent. Sets in[] array
	setInputs();

	//brains tick. computes in[] -> out[]
	brainsTick();

	//reset any counter variables per agent and do other stuff before processOutputs and healthTick
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];

		//process indicator, used in drawing
		if(a->indicator>0) a->indicator-= 0.5;
		if(a->indicator<0) a->indicator-= 1;

		//process carcass counter, which keeps dead agents on the world while meat is under them
		if(a->carcasscount>=0) a->carcasscount++;

		//process jaw renderer
		if(a->jawrend>0) a->jawrend-=1;

		//if alive...
		if(a->health>0){
			//reduce fresh kill flag
			if(a->freshkill>0) a->freshkill-= 1;
			
			//Live mutations
			for(int m=0; m<MUTEVENTMULT; m++) {
				if(randf(0,1)<LIVE_MUTATE_CHANCE){
					a->liveMutate();
					if(a->id==SELECTION) addEvent("The Selected Agent Was Mutated!", EventColor::PURPLE);
					STATlivemutations++;
					if(DEBUG) printf("Live Mutation Event\n");
				}
			}

			//Age goes up!
			if (modcounter%(FRAMES_PER_DAY/10)==0) a->age+= 1;

			//update jaw
			if(a->jawPosition>0) {
				a->jawPosition*= -1; //jaw is an instantaneous source of damage. Reset to a negative number if positive
				a->jawrend= 15; //set render timer

			//once negative, jaw resets to 0, slowly if near -1, faster once closer to 0
			} else if (a->jawPosition<0) a->jawPosition= min(0.0, a->jawPosition + 0.01*(2 + a->jawPosition));

			//update center render mode. Asexual pulls toward 0, female sexual pulls toward 1, male sexual towards 2
			if(a->isAsexual()) a->centerrender-= 0.01*(a->centerrender);
			else if(a->isMale()) a->centerrender-= 0.01*(a->centerrender-2);
			else a->centerrender-= 0.01*(a->centerrender-1);
			a->centerrender= cap(a->centerrender/2)*2; //duck the counter under a cap to allow range [0,2]

			//exhaustion gets increased
			float boostmult= a->boost ? BOOSTEXAUSTMULT : 1;
			a->exhaustion= max((float)0,a->exhaustion + a->getOutputSum()*boostmult + BASEEXHAUSTION)*EXHAUSTION_MULT;

			//temp discomfort gets re-calculated mod%10, to simulate heat absorption/release
			if (modcounter%10==0){
				//calculate temperature at the agents spot. (based on distance from horizontal equator)
				float dd= 2.0*abs(a->pos.y/conf::HEIGHT - 0.5);
				a->discomfort= pow(abs(dd-a->temperature_preference),2);
				if (a->discomfort<0.001) a->discomfort=0;
			}
		}
	}

	//read output and process consequences of bots on environment. requires out[]
	processOutputs();

	//process health:
	healthTick();
	
	//handle reproduction
	//do not omp any of this!
	if (modcounter%5==0){
		for (int i=0; i<(int)agents.size(); i++) {
			if (agents[i].repcounter<0 && agents[i].health>=MINMOMHEALTH) { 
				//agent is healthy and is ready to reproduce. Now to decide how...

				if(SEXTING_DISTANCE>0 && !agents[i].isAsexual()){
					if(agents[i].isMale()) continue;; //'fathers' cannot themselves reproduce

					for (int j=0; j<(int)agents.size(); j++) {
						if(i==j || agents[j].isAsexual() || agents[j].repcounter>0) continue;
						float d= (agents[i].pos-agents[j].pos).length();
						float deviation= abs(agents[i].species - agents[j].species); //species deviation check
						if (d<=SEXTING_DISTANCE && deviation<=MAXDEVIATION) {
							//this adds agent[i].numbabies to world, with two parents
							if(DEBUG) printf("Attempting to have children...");
							reproduce(i, j); //reproduce resets mother's rep counter, not father's
							if(agents[i].id==SELECTION) addEvent("Selected Agent Sexually Reproduced", EventColor::BLUE);
							tryPlayAudio(conf::SFX_SMOOCH1, agents[i].pos.x, agents[i].pos.y, randn(1.0,0.15));
							if(DEBUG) printf(" Success!\n");
							break;
							continue;
						}
					}
				} else {
					//this adds agents[i].numbabies to world, but with just one parent (budding)
					if(DEBUG) printf("Attempting budding...");
					reproduce(i, i);
					if(agents[i].id==SELECTION) addEvent("Selected Agent Assexually Budded", EventColor::NEON);
					tryPlayAudio(conf::SFX_PLOP1, agents[i].pos.x, agents[i].pos.y, randn(1.0,0.15));
					if(DEBUG) printf(" Success!\n");
					continue;
				}
			}
			if(FUN && randf(0,1)<0.3){
				if(agents[i].indicator<=0) agents[i].indicator= 60;
				agents[i].ir= randf(0,1);
				agents[i].ig= randf(0,1);
				agents[i].ib= randf(0,1);
			}
		}
	}

	processInteractions();
	//DO NOT add causes of death after this!!! Health should be exactly 0 when leaving this function

	for (int i=0; i<(int)agents.size(); i++) {
		//process dead agents. first distribute meat
		if(agents[i].health<0) printf("Please check the code: an agent unexpectedly had negative health when it should have had exactly zero\n");
		if (agents[i].health==0 && agents[i].carcasscount==0) { 
			//world is only now picking up the fact that the agent died from the agent itself
			if (SELECTION==agents[i].id){
				printf("The Selected Agent was %s!\n", agents[i].death.c_str());
				addEvent("The Selected Agent Died!");

				tryPlayAudio(conf::SFX_DEATH1, 0, 0, randn(1.0,0.05), 0.8);
			} else tryPlayAudio(conf::SFX_DEATH1, agents[i].pos.x, agents[i].pos.y, randn(1.0,0.15));
		
			int cx= (int) agents[i].pos.x/conf::CZ;
			int cy= (int) agents[i].pos.y/conf::CZ;

			float meat= cells[Layer::MEATS][cx][cy];
			float agemult= 1.0;
			float freshmult= 0.5; //if this agent wasnt freshly killed, default to 50%
			float stomachmult= (4.0-3*agents[i].stomach[Stomach::MEAT])/4; //carnivores give 25%
			if(agents[i].age<TENDERAGE) agemult= (float)agents[i].age/TENDERAGE; //young killed agents should give very little resources until age 10
			if(agents[i].freshkill>0) freshmult= 1.0; //agents which were spiked recently will give full meat

			meat+= MEATVALUE*agemult*freshmult*stomachmult;
			cells[Layer::MEATS][cx][cy]= cap(meat);

			//collect all the death causes from all dead agents
			deaths.push_back(agents[i].death);
		}
	}

	vector<Agent>::iterator iter= agents.begin();
	while (iter != agents.end()) {
		if (iter->health <=0 && iter->carcasscount >= conf::CARCASSFRAMES) {
			if(iter->id == SELECTION) addEvent("The Selected Agent has decayed", EventColor::BROWN);
			iter= agents.erase(iter);
		} else {
			++iter;
		}
	}

	//add new agents, if environment isn't closed
	if (!CLOSED) {
		int alive= agents.size()-getDead();
		//make sure environment is always populated with at least AGENTS_MIN_NOTCLOSED bots
		if (alive<AGENTS_MIN_NOTCLOSED) {
			if(DEBUG) printf("Attempting agent conservation program...");
			addAgents(AGENTS_MIN_NOTCLOSED-alive);
			if(DEBUG) printf(" Success!\n");
		}
		if (alive<AGENTS_MAX_SPAWN && modcounter%AGENTSPAWN_FREQ==0 && randf(0,1)<0.5) {
			if(DEBUG) printf("Attempting random spawn...");
			addAgents(1,-1,true); //every now and then add random bot in if population too low
			if(DEBUG) printf(" Success!\n");
		}
	}

	vector<std::pair <const char *,std::pair <int,int> > >::iterator item= events.begin();
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

void World::setInputs()
{
	#if defined(_DEBUG)
	if(DEBUG) printf("I");
	#endif

	float PI8=M_PI/8/2; //pi/8/2
	float PI38= 3*PI8; //3pi/8/2
	float PI4= M_PI/4;

	selectedSounds.clear(); //clear selection's heard sounds
   
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].health<=0) continue;
		Agent* a= &agents[i];

		//our cell position
		int scx= (int)(a->pos.x/conf::CZ);//, 0, conf::WIDTH/conf::CZ);
		int scy= (int)(a->pos.y/conf::CZ);//, 0, conf::HEIGHT/conf::CZ);
		
		//HEALTH
		a->in[Input::HEALTH]= cap(a->health/2); //divide by 2 since health is in [0,2]

		//SOUND SMELL EYES
		float light= (MOONLIT) ? max(MOONLIGHTMULT, cells[Layer::LIGHT][scx][scy]) : cells[Layer::LIGHT][scx][scy]; //grab min light level for conditions

		vector<float> r(NUMEYES,0.25*light);
		vector<float> g(NUMEYES,0.25*light);
		vector<float> b(NUMEYES,0.25*light);
					   
		float smellsum=0;

		vector<float> hearsum(NUMEARS,0);

		//BLOOD ESTIMATOR
		float blood= 0;

		//cell sense
		int minx= max((scx-DIST/conf::CZ/2),(float)0);
		int maxx= min((scx+1+DIST/conf::CZ/2),(float)CW);
		int miny= max((scy-DIST/conf::CZ/2),(float)0);
		int maxy= min((scy+1+DIST/conf::CZ/2),(float)CH);

		float fruit= 0, meat= 0, hazard= 0, water= 0;

		//cell "smelling": a faster/better food- & hazard-revealing solution than adding to eyesight
		for(scx=minx;scx<maxx;scx++){
			for(scy=miny;scy<maxy;scy++){
				fruit+= cells[Layer::FRUITS][scx][scy];
				meat+= cells[Layer::MEATS][scx][scy];
				hazard+= cells[Layer::HAZARDS][scx][scy];
				water+= cells[Layer::ELEVATION][scx][scy]<=Elevation::BEACH_MID ? 1 : 0; // all water smells the same
			}
		}
		float dimmensions= (maxx-minx)*(maxy-miny);
		fruit*= a->smell_mod/dimmensions;
		meat*= a->smell_mod/dimmensions;
		hazard*= a->smell_mod/dimmensions;
		water*= a->smell_mod/dimmensions;


				/* CELL EYESIGHT CODE
				if (cells[Layer::PLANTS][scx][scy]==0 && cells[Layer::MEATS][scx][scy]==0 && cells[Layer::HAZARDS][scx][scy]==0) continue;
				Vector2f cellpos= Vector2f((float)(scx*conf::CZ+conf::CZ/2),(float)(scy*conf::CZ+conf::CZ/2)); //find midpoint of the cell
				float d= (a->pos-cellpos).length();

				if (d<DIST) {
					float angle= (cellpos-a->pos).get_angle();

					for(int q=0;q<NUMEYES;q++){
						float aa = a->angle + a->eyedir[q];
						if (aa<-M_PI) aa += 2*M_PI;
						if (aa>M_PI) aa -= 2*M_PI;
						
						float diff1= aa- angle;
						if (fabs(diff1)>M_PI) diff1= 2*M_PI- fabs(diff1);
						diff1= fabs(diff1);
						
						float fov = a->eyefov[q];
						if (diff1<fov) {
							//we see this cell with this eye. Accumulate stats
							float mul1= a->eye_see_cell_mod*(fabs(fov-diff1)/fov)*((DIST-d)/DIST)*(d/DIST)*2;
							r[q] += mul1*0.25*cells[Layer::MEATS][scx][scy]; //meat looks red
							g[q] += mul1*0.25*cells[Layer::PLANTS][scx][scy]; //plants are green
							r[q] += mul1*0.25*cells[Layer::FRUITS][scx][scy]; //fruit looks yellow
							g[q] += mul1*0.25*cells[Layer::FRUITS][scx][scy];
							b[q] += mul1*0.25*cells[Layer::HAZARDS][scx][scy]; //hazards are blue???
							if(a->selectflag && isDebug()){
								linesA.push_back(a->pos);
								linesB.push_back(cellpos);
							}
						}
					}

					float forwangle= a->angle;
					float diff4= forwangle- angle;
					if (fabs(forwangle)>M_PI) diff4= 2*M_PI- fabs(forwangle);
					diff4= fabs(diff4);
					if (diff4<PI38) {
						float mul4= ((PI38-diff4)/PI38)*(1-d/DIST);
						//meat can also be sensed with blood sensor
						blood+= mul4*0.3*cells[Layer::MEATS][scx][scy];
					}
				}*/
					
		for (int j=0; j<(int)agents.size(); j++) {
			//can still see and smell dead agents, so no health check here
			if (i==j) continue;
			Agent* a2= &agents[j];

			if (a->pos.x<a2->pos.x-DIST || a->pos.x>a2->pos.x+DIST
				|| a->pos.y>a2->pos.y+DIST || a->pos.y<a2->pos.y-DIST) continue;

			float d= (a->pos-a2->pos).length();

			if (d<DIST) {

				//smell: adds up all agents inside DIST
				smellsum+= a->smell_mod;

				//sound and hearing: adds up vocalization and other emissions from agents inside DIST
				for (int q=0;q<NUMEARS;q++){

					Vector2f v(a->radius, 0);
					v.rotate(a->angle + a->eardir[q]);

					Vector2f earpos= a->pos+ v;

					float eardist= (earpos-a2->pos).length();

					for(int n=0;n<2;n++){
						float otone= 0, ovolume= 0;
						if(n==0){ //if n=0, do agent vocals.
							otone= a2->tone;
							ovolume= a2->volume;
						}else if(n==1){ //if n=1, do agent wheels
							otone= 0.25;
							ovolume= (max(fabs(a2->w1),fabs(a2->w2)));
						} //future: if n=2, do agent intake sound

						//package up this sound source if user is watching
						if(getSelectedID()==a->id){
							int volfact= (int)(a->hear_mod*(1-eardist/DIST)*ovolume*100);
							float finalfact= otone + volfact;
							selectedSounds.push_back(finalfact);
						}

						if(otone>=a->hearhigh[q]) continue;
						if(otone<=a->hearlow[q]) continue;
						if(getSelectedID()==a->id){
							//modify the selected sound listing if its actually heard so that we can change the visual
							selectedSounds[selectedSounds.size()-1]+= 100;
						}
						float tonemult= cap(min((a->hearhigh[q] - otone)/SOUNDPITCHRANGE,(otone - a->hearlow[q])/SOUNDPITCHRANGE));
						hearsum[q]+= a->hear_mod*(1-eardist/DIST)*ovolume*tonemult;

						if(a->isTiny()) break; //small agents only get one ear input
					}
				}

				float ang= (a2->pos- a->pos).get_angle(); //current angle between bots, with origin at agent a

				//eyes: bot sight
				//we will skip all eyesight if our agent is in the dark (light==0)
				if(light!=0){
					for(int q=0;q<NUMEYES;q++){
						if(a->isTiny() && !a->isTinyEye(q)){ //small agents have half-count of eyes, the rest get set to constant 1 input
							r[q]= 1.0;
							g[q]= 1.0;
							b[q]= 1.0;
							continue;
						}

						float aa = a->angle + a->eyedir[q];//get eye's absolute (world) angle, with origin at agent a
						if (aa<-M_PI) aa += 2*M_PI;
						if (aa>M_PI) aa -= 2*M_PI;//not only loop aa angle, but throw it a whole circle around (we're allowing negative)
						
						float diff1= aa- ang; //get the difference in absolute angles between the eye and the other agent
						if (fabs(diff1)>M_PI) diff1= 2*M_PI- fabs(diff1); //now take the absolute value to get a raw angle between
						diff1= fabs(diff1);
						
						float fov = a->eyefov[q] + a2->radius/d; //add in radius/d to allowed fov, which for large distances is the same as the angle
						//"fov" here is a bit of a misnomer, as it's taking into account the seen agent's radius. This is used later in the calc again

						if (diff1<fov) {
							//we see a2 with this eye. Accumulate stats
							float mul1= light*a->eye_see_agent_mod*(fabs(fov-diff1)/fov)*(1-d*d/DIST/DIST);/*(1-d/DIST)*/;

							if(r[q]<mul1*a2->real_red) r[q]= mul1*a2->real_red;
							if(g[q]<mul1*a2->real_gre) g[q]= mul1*a2->real_gre;
							if(b[q]<mul1*a2->real_blu) b[q]= mul1*a2->real_blu;
							if(a->id==SELECTION && isDebug()){ //debug sight lines, get coords
								linesA.push_back(a->pos);
								linesB.push_back(a2->pos);
							}
						}
					}
				}
				
				//blood sensor
				float forwangle= a->angle;
				float diff4= forwangle- ang;
				if (fabs(forwangle)>M_PI) diff4= 2*M_PI- fabs(forwangle);
				diff4= fabs(diff4);
				if (diff4<PI38) {
					float newblood= a->blood_mod*((PI38-diff4)/PI38)*(1-d/DIST)*(1-agents[j].health/2); //remember: health is in [0,2]
					if(newblood>blood) blood= newblood;
					//agents with high life dont bleed. low life makes them bleed more. dead agents bleed the maximum
				}
			}
		}

		//temperature varies from 0 to 1 across screen.
		//it is 0 at equator (in middle), and 1 on edges. Agents can sense this range
		float temp= (float)2.0*abs(a->pos.y/conf::HEIGHT - 0.5);

		for(int i=0;i<NUMEYES;i++){
			a->in[Input::EYES+i*3]= cap(r[i]);
			a->in[Input::EYES+i*3+1]= cap(g[i]);
			a->in[Input::EYES+i*3+2]= cap(b[i]);
		}

		float t= (modcounter+current_epoch*FRAMES_PER_EPOCH);
		a->in[Input::CLOCK1]= abs(sinf(t/a->clockf1));
		if(!a->isTiny()) a->in[Input::CLOCK2]= abs(sinf(t/a->clockf2));
		if(!a->isTiny()) a->in[Input::CLOCK3]= abs(sinf(t/a->clockf3));
		a->in[Input::HEARING1]= cap(hearsum[0]);
		a->in[Input::HEARING2]= cap(hearsum[1]);
		a->in[Input::BOT_SMELL]= cap(smellsum);
		a->in[Input::BLOOD]= cap(blood);
		a->in[Input::TEMP]= temp;
		if (a->id!=SELECTION) {
			a->in[Input::PLAYER]= 0.0;
		}
		a->in[Input::FRUIT_SMELL]= cap(fruit);
		a->in[Input::MEAT_SMELL]= cap(meat);
		a->in[Input::HAZARD_SMELL]= cap(hazard);
		a->in[Input::WATER_SMELL]= cap(water);
		a->in[Input::RANDOM]= cap(randn(a->in[Input::RANDOM],0.08));
	}

	#if defined(_DEBUG)
	if(DEBUG) printf("/");
	#endif
}

void World::brainsTick()
{
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].health<=0) continue;
		agents[i].tick();
	}
}

void World::processOutputs(bool prefire)
{
	#if defined(_DEBUG)
	if(DEBUG) printf("O");
	#endif

	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];
		float exh= 1.0/(1.0+a->exhaustion); //exhaustion reduces agent output->physical action rates

		if (a->health<=0) {
			//dead agents continue to exist, come to a stop, and loose their voice, but skip everything else
			float sp= (a->w1+a->w2)/2;
			a->w1= 0.75*sp;
			a->w2= 0.75*sp;

			a->volume*= 0.2;
			if(a->volume<0.0001) a->volume= 0;

			a->sexproject= 0; //no necrophilia
			a->boost= false; //no boosting
			continue;
		}

		if (a->jump<=0) { //if not jumping, then change wheel speeds. otherwise, we want to keep wheel speeds constant
			if (pcontrol && a->id==SELECTION) {
				a->w1= pright;
				a->w2= pleft;
			} else {
				a->w1+= a->strength*(a->out[Output::LEFT_WHEEL_F] - a->out[Output::LEFT_WHEEL_B] - a->w1);
				a->w2+= a->strength*(a->out[Output::RIGHT_WHEEL_F] - a->out[Output::RIGHT_WHEEL_B] - a->w2);
			}
			a->w1*= exh;
			a->w2*= exh;
		}
		a->real_red+= 0.2*((1-a->chamovid)*a->gene_red + a->chamovid*a->out[Output::RED]-a->real_red);
		a->real_gre+= 0.2*((1-a->chamovid)*a->gene_gre + a->chamovid*a->out[Output::GRE]-a->real_gre);
		a->real_blu+= 0.2*((1-a->chamovid)*a->gene_blu + a->chamovid*a->out[Output::BLU]-a->real_blu);
		if (a->jump<=0) a->boost= (a->out[Output::BOOST])>0.5; //if jump height is zero, boost can change. Also, exhaustion doesn't effect the boost toggle
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

		//jump gets set to 2*((jump output) - 0.5) if itself is zero (the bot is on the ground) and if jump output is greater than 0.5
		if(GRAVITYACCEL>0 && a->jump==0){
			float height= (a->out[Output::JUMP]*exh - 0.5)*2;
			if (height>0 && a->age>0) a->jump= height;
		}

		//jaw *snap* mechanic
		float newjaw= cap(a->out[Output::JAW] - a->jawOldOutput)*exh; //new output has to be at least twice the old output to really trigger

		if (a->jawPosition==0 && a->age>0 && newjaw>0.01) a->jawPosition= newjaw;
		a->jawOldOutput= a->out[Output::JAW];

		//clock 3 gets frequency set by output, but not immediately
		a->clockf3+= 0.3*(95*(1-a->out[Output::CLOCKF3])+5 - a->clockf3); //exhaustion doesn't effect clock freq
		if(a->clockf3>100) a->clockf3= 100;
		if(a->clockf3<2) a->clockf3= 2;

		//play some audio for agents making sounds!
		if(a->volume>0.05 && (modcounter+a->id*8)%min(300,(int)ceil(40.0/(a->tone+0.0000001)))==0){
			//math explains: modcounter+id*8 to give some variety between agents
			//min(300, ensures no more than 300 ticks between sounds
			//ceil(40.0/(a->tone+0.0000001)) makes it so that low tones make less often sounds, high tones make more often, fastest possible is near 40 ticks

			float playx= a->pos.x, playy= a->pos.y;
			if(a->id==SELECTION) { playx= 0; playy= 0; } //if we're selected, forget position, set us right at the listener position

			//Finally, these tones and volumes have been pre-adjusted for the sounds used in game:
			if(a->isTiny()) tryPlayAudio(conf::SFX_CHIRP1, playx, playy, a->tone*0.8+0.3, a->volume*0.2); //tiny agents make a high-pitched chirp, like crickets
			else tryPlayAudio(conf::SFX_CHIRP2, playx, playy, a->tone*0.8+0.2, a->volume*0.7); //large agents make a lower, cat-like "churr"
		}

		//backup previous x & y pos's This is done as a prefire on purpose (it's not saved)
		a->dpos.x= a->pos.x;
		a->dpos.y= a->pos.y;
	}

	//move bots if this is not a prefire. A prefire is needed when loading, as all the values above need refreshing, but we shouldn't move yet
	if(!prefire){
		#pragma omp parallel for schedule(dynamic)
		for (int i=0; i<(int)agents.size(); i++) {
			Agent* a= &agents[i];

			a->jump-= GRAVITYACCEL;
			if(a->jump<-1) a->jump= 0; //-1 because we will be nice and give a "recharge" time between jumps

			float basewheel= powf(a->radius/MEANRADIUS,0.25);
			if (a->encumbered) basewheel*= conf::ENCUMBEREDMULT;
			if (a->boost) basewheel*= BOOSTSIZEMULT;

			float BW1= basewheel*a->w1;
			float BW2= basewheel*a->w2;

			//move bots
			Vector2f v1(BOTSPEED, 0);
			v1.rotate(a->angle + M_PI/2);

			Vector2f vv1= v1;
			Vector2f vv2= -v1;
			vv1.rotate(-BW1);
			vv2.rotate(BW2);
			a->pos= a->pos+(vv1 - v1)+(vv2 + v1);

			//angle bots
			if (a->jump<=0) {
				a->angle += BW2-BW1;
			}
			if (a->angle<-M_PI) a->angle= M_PI - (-M_PI-a->angle);
			if (a->angle>M_PI) a->angle= -M_PI + (a->angle-M_PI);

			//wrap around the map
			a->borderRectify();
		}
	}

	#if defined(_DEBUG)
	if(DEBUG) printf("/");
	#endif
}

void World::processInteractions()
{
	//process interaction with cells
	#if defined(_DEBUG)
	if(DEBUG) printf("CA");
	#endif

	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].health<=0 && modcounter%25>0) continue; // okay, so what we're doing here is complicated
		//if the agent is alive, we process them. If they are dead and the tick count mod 25 is not 0, we process them
		//we want to process dead agents because we want to check if they have meat under them, and if so, reset the corpse counter
		Agent* a= &agents[i];

		float randr= min(a->radius, (float)abs(randn(0.0,a->radius/3)));
		Vector2f source(randr, 0);
		source.rotate(randf(-M_PI,M_PI));

		int scx= (int) (a->pos.x + source.x)/conf::CZ;
		int scy= (int) (a->pos.y + source.y)/conf::CZ;

		if(agents[i].health>0){
			if (a->jump<=0){ //no interaction with these cells if jumping
				float intake= 0;
				float speedmult= pow(1-max(abs(a->w1), abs(a->w2)),3) / (1.0+a->exhaustion);
				//exhaustion reduces agent physical actions including all intake, applied to speed for code efficiency

				float invmult= 1-STOMACH_EFF;
				float invplant=1-a->stomach[Stomach::PLANT]*invmult;
				float invmeat= 1-a->stomach[Stomach::MEAT]*invmult;
				float invfruit=1-a->stomach[Stomach::FRUIT]*invmult;
				//inverted stomach vals, with the efficiency mult applied

				//---START FOOD---//
				//plant food
				float food= cells[Layer::PLANTS][scx][scy];
				if (food>0) { //agent eats the food
					//Plant intake is proportional to plant stomach, inverse to meat & fruit stomachs, and speed/exhaustion
					//min rate is the actual amount of food we can take. Otherwise, apply wasterate
					float planttake= min(food,FOODWASTE*a->stomach[Stomach::PLANT]*invmeat*invfruit*speedmult);
					//unique for plant food: the less there is, the harder it is to eat, but not impossible
					planttake*= max(food,0.25f);
					//decrease cell content
					cells[Layer::PLANTS][scx][scy]-= planttake;
					//now convert the taken food into intake for the agent
					intake+= FOODINTAKE*planttake/FOODWASTE;
					a->addIntake(conf::FOOD_TEXT, planttake);
				}

				//meat food
				float meat= cells[Layer::MEATS][scx][scy];
				if (meat>0) { //agent eats meat
					float meattake= min(meat,MEATWASTE*a->stomach[Stomach::MEAT]*invplant*invfruit*speedmult);
					cells[Layer::MEATS][scx][scy]-= meattake;
					intake+= MEATINTAKE*meattake/MEATWASTE;
					a->addIntake(conf::MEAT_TEXT, meattake);
				}

				//Fruit food
				float fruit= cells[Layer::FRUITS][scx][scy];
				if (fruit>0) { //agent eats fruit
					float fruittake= min(fruit,FRUITWASTE*a->stomach[Stomach::FRUIT]*invmeat*invplant*speedmult);
					cells[Layer::FRUITS][scx][scy]-= fruittake;
					intake+= FRUITINTAKE*fruittake/FRUITWASTE;
					a->addIntake(conf::FRUIT_TEXT, fruittake);
				}

				// proportion intake via metabolism
				a->repcounter -= a->metabolism*(1-MIN_INTAKE_HEALTH_RATIO)*intake;
				if (a->repcounter<0) a->encumbered= true;
				else a->encumbered= false;

				a->health += (1-a->metabolism*(1-MIN_INTAKE_HEALTH_RATIO))*intake;
				//---END FOOD---//


				//hazards
				float hazard= cells[Layer::HAZARDS][scx][scy];
				if (hazard>0){
					//the young are spry, and don't take much damage from hazard
					float agemult= 1.0;
					if(a->age<TENDERAGE) agemult= 0.5+0.5*agents[i].age/TENDERAGE;

					float damage= HAZARDDAMAGE*agemult*pow(hazard, HAZARDPOWER);
					if(hazard>0.9) damage*= HAZARDEVENT_MULT; //if a hazard event, apply multiplier

					a->addDamage(conf::DEATH_HAZARD, damage);
				}

				//waste deposit
				int freqwaste= (int)max(1.0f, a->out[Output::WASTE_RATE]*MAXWASTEFREQ);
				//agents can control the rate of deposit, [1,MAXWASTEFREQ] frames
				if (modcounter%freqwaste==0){
					//agents fill up hazard cells only up to 9/10, because any greater is a special event
					if(hazard<0.9) hazard+= HAZARDDEPOSIT*freqwaste*a->health;
					
					cells[Layer::HAZARDS][scx][scy]= capm(hazard, 0, 0.9);
				}
			}

			//land/water (always interacted with)
			float land= cells[Layer::ELEVATION][scx][scy];
			if(a->jump>0){ //jumping while underwater allows one to breathe at lung value 0.5, aka the agent is "floating"
				land= max(land, Elevation::BEACH_MID);
			}
			float dd= pow(land - a->lungs,2);
			if (dd>=0.01){ //a difference of 0.01 or less between lung and land type lets us skip taking damage
				if(dd>=0.1) a->encumbered= true; //this should reeeeeally be some sort of "leg type" mechanic, not lungs
				if(HEALTHLOSS_BADTERRAIN!=0) a->addDamage(conf::DEATH_BADTERRAIN, HEALTHLOSS_BADTERRAIN*dd);
			}

			if (a->health>2){ //if health has been increased over the cap, limit
				if(OVERHEAL_REPFILL!=0) a->repcounter -= (a->health-2)*OVERHEAL_REPFILL; //if enabled, allow overflow to first fill the repcounter
				a->health= 2;
			}
		} else {
			//agent is dead. Just check for meat and refresh corpse counter if meat amount is high enough
			float meat= cells[Layer::MEATS][scx][scy];
			if(meat>0.25) a->carcasscount= 1; //1 because 0 triggers meat dropping... it's a mess, I'm sorry
		}

	}

	#if defined(_DEBUG)
	if(DEBUG) printf("/AA"); //PLEASE note: this is the '/' end flag for CA, merged with AA, the next section.
	//If you split this func, remember to split this!
	#endif

	//process agent-agent dynamics
	if (modcounter%2==0) { //we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents

		//first, we'll determine for all agents if they are near enough to another agent to warrent processing them
		float highestdist= max(max(max(FOOD_SHARING_DISTANCE, SEXTING_DISTANCE), GRABBING_DISTANCE), SPIKELENGTH+MEANRADIUS*3);
		//SEXTING_DISTANCE can be replaced here - it's not used here
		for (int i=0; i<(int)agents.size(); i++) {
			agents[i].near= false;
			agents[i].dhealth= 0; //better place for this now, since modcounter%2
			if (agents[i].health<=0) continue; //skip dead agents

			for (int j=0; j<i; j++) {
				if (agents[i].pos.x<agents[j].pos.x-highestdist
					|| agents[i].pos.x>agents[j].pos.x+highestdist
					|| agents[i].pos.y>agents[j].pos.y+highestdist
					|| agents[i].pos.y<agents[j].pos.y-highestdist) continue;
				else if (agents[j].health<=0) {
					if(agents[i].isGrabbing() && agents[i].grabID==agents[j].id) agents[i].grabID= -1;
					continue;
				} else {
					agents[i].near= true;
					agents[j].near= true;
					break;
					continue;
				}
			}
		}

		#pragma omp parallel for schedule(dynamic)
		for (int i=0; i<(int)agents.size(); i++) {
			if(agents[i].health<=0) continue;
			if(!agents[i].near){
				//no one is near enough this agent to process interactions. break grabbing and skip
				agents[i].grabID= -1;
				continue;
			}

			Agent* a= &agents[i];

			for (int j=0; j<(int)agents.size(); j++) {
				if (i==j || !agents[j].near) continue;
				if(agents[j].health==0) continue; //health == because we want to weed out bots who died already via other causes

				Agent* a2= &agents[j];

				float d= (a->pos-a2->pos).length();
				float sumrad= a->radius+a2->radius;

				//---HEALTH GIVING---//
				if (FOOD_SHARING_DISTANCE>0 && FOODTRANSFER!=0 && !a->isSelfish(conf::MAXSELFISH) && a2->health<2) {
					//all non-selfish agents allow health trading

					float rd= a->isGiving() ? FOOD_SHARING_DISTANCE : sumrad+1;
					//rd is the max range allowed to agent j. If generous, range allowed, otherwise bots must basically touch

					if (d<=rd) {
						//initiate transfer
						float healthrate= a->isGiving() ? FOODTRANSFER*a->give : FOODTRANSFER*2*a->give;
						if(d<=sumrad+1 && a->isGiving()) healthrate= FOODTRANSFER;
						//healthrate goes from 0->1 for give [0,0.5], and from 0.5->1 for give (0.5,1]. Is maxxed when touching and generous
						a2->health += healthrate;
						a->addDamage(conf::DEATH_GENEROUSITY, healthrate);
						a2->dhealth += healthrate; //only for drawing
						a->dhealth -= healthrate;
						if (a2->health>2) a2->health= 2;
					}
				}

				//---COLLISIONS---///
				if (BUMP_PRESSURE>0 && d<sumrad && a->jump<=0 && a2->jump<=0) {
					//if inside each others radii and neither are jumping, fix physics
					float ov= (sumrad-d);
					if (ov>0 && d>0.00001) {
						if (ov>TOOCLOSE && TOOCLOSE>0) {//if bots are too close, they get injured before being pushed away
							float aagemult= 1;
							float a2agemult= 1;
							if(a->age<TENDERAGE) aagemult= a->age/TENDERAGE;
							if(a2->age<TENDERAGE) a2agemult= a2->age/TENDERAGE;
							float DMG1= ov*DAMAGE_COLLIDE*sumrad/2/a->radius*aagemult; //larger, younger bots take less damage, bounce less
							float DMG2= ov*DAMAGE_COLLIDE*sumrad/2/a2->radius*a2agemult;

							if(DEBUG) printf("\na collision occured. Damage on agent a: %.4f. Damage on agent a2: %.4f\n", DMG1, DMG2);
							a->addDamage(conf::DEATH_COLLIDE, DMG1);
							a2->addDamage(conf::DEATH_COLLIDE, DMG2);

							if(a->health==0){
								break;
								continue;
							}
							if(a2->health==0){
								a->killed++;
								continue;
							}

							//only trigger collision splash if another splash not being displayed
							if(a->indicator<=0) a->initSplash(conf::RENDER_MAXSPLASHSIZE*0.5,0,0.5,1);
							if(a2->indicator<=0) a2->initSplash(conf::RENDER_MAXSPLASHSIZE*0.5,0,0.5,1);
							
							a->freshkill= FRESHKILLTIME; //this agent was hit this turn, giving full meat value
							a2->freshkill= FRESHKILLTIME;
						}
						float ff1= capm(ov/d*a2->radius/a->radius*BUMP_PRESSURE, 0, 2); //the radii come in here for inertia-like effect
						float ff2= capm(ov/d*a->radius/a2->radius*BUMP_PRESSURE, 0, 2);
						a->pos.x-= (a2->pos.x-a->pos.x)*ff1;
						a->pos.y-= (a2->pos.y-a->pos.y)*ff1;
						a2->pos.x+= (a2->pos.x-a->pos.x)*ff2;
						a2->pos.y+= (a2->pos.y-a->pos.y)*ff2;

						a->borderRectify();
						a2->borderRectify();

//						printf("%f, %f, %f, %f, and %f\n", a->pos.x, a->pos.y, a2->pos.x, a2->pos.y, ov);
					}
				} //end collision mechanics

				//---SPIKING---//
				//small spike doesn't count. If the target is jumping in midair, can't attack either
				if(a->isSpikey(SPIKELENGTH) && d<=(a2->radius + SPIKELENGTH*a->spikeLength) && a2->jump<=0){
					Vector2f v(1,0);
					v.rotate(a->angle);
					float diff= v.angle_between(a2->pos-a->pos);
					if (fabs(diff)<M_PI/2){ //need to be in front
						float spikerange= d*abs(sin(diff)); //get range to agent from spike, closest approach
						if (a2->radius>spikerange) { //other agent is properly aligned and in range!!! getting close now...
							//need to calculate the velocity differential of the agents
							Vector2f velocitya(a->pos.x-a->dpos.x, a->pos.y-a->dpos.y);
							Vector2f velocitya2(a2->pos.x-a2->dpos.x, a2->pos.y-a2->dpos.y);

							velocitya-= velocitya2;
							float diff2= velocitya.angle_between(a2->pos-a->pos);
							float veldiff= velocitya.length()*cap(cos(diff2));

							if(veldiff>conf::VELOCITYSPIKEMIN){
								//these two are in collision and agent a has extended spike and is going decently fast relatively! That's a hit!
							
								float DMG= DAMAGE_FULLSPIKE*a->spikeLength*veldiff*(1-1.0/(1+expf(-(a2->radius/2-MEANRADIUS))));
								//tiny, small, & average agents take full damage, scaled by the difference in velocity. 
								//large (2*MEANRADIUS) agents take half damage, and huge agents (2*MEANRADIUS+10) take no damage from spikes at all

								if(DEBUG) printf("\nan agent received spike damage: %.4f\n", DMG);
								a2->addDamage(conf::DEATH_SPIKE, DMG); 
								a2->freshkill= FRESHKILLTIME; //this agent was hit this turn, giving full meat value
								if(a2->id==SELECTION) addEvent("Selected Agent was Stabbed!", EventColor::BLOOD);

								a->hits++;
								a->spikeLength= 0; //retract spike down
								a->initSplash(conf::RENDER_MAXSPLASHSIZE*0.5*DMG,1,0.5,0); //orange splash means bot has spiked the other bot. nice!
								tryPlayAudio(conf::SFX_STAB1, a2->pos.x, a2->pos.y, randn(1.0,0.2));

								if (a2->health==0){ 
									//red splash means bot has killed the other bot. Murderer!
									a->initSplash(conf::RENDER_MAXSPLASHSIZE,1,0,0);
									if(a->id==SELECTION) addEvent("Selected Agent Killed Another!", EventColor::RED);
									a->killed++;
									continue;
								} else if(a->id==SELECTION) addEvent("Selected Agent Stabbed Another!", EventColor::ORANGE);

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
				if(a->jawPosition>0 && d<=(sumrad+12.0)) { //only bots that are almost touching may chomp
					Vector2f v(1,0);
					v.rotate(a->angle);
					float diff= v.angle_between(a2->pos-a->pos);
					if (fabs(diff)<M_PI/6) { //advantage over spike: wide AOE
						float DMG= DAMAGE_JAWSNAP*a->jawPosition*(a->radius/a2->radius); //advantage over spike: large agents do more damage to smaller agents

						if(DEBUG) printf("\nan agent received bite damage: %.4f\n", DMG);
						a2->addDamage(conf::DEATH_BITE, DMG);
						a2->freshkill= FRESHKILLTIME;
						if(a2->id==SELECTION) addEvent("Selected Agent was Bitten!", EventColor::BLOOD);

						a->hits++;
						a->initSplash(conf::RENDER_MAXSPLASHSIZE*0.5*DMG,1,1,0); //yellow splash means bot has chomped the other bot. ouch!

						if (a2->health==0){ 
							//red splash means bot has killed the other bot. Murderer!
							a->initSplash(conf::RENDER_MAXSPLASHSIZE,1,0,0);
							if(a->id==SELECTION) addEvent("Selected Agent Killed Another!", EventColor::RED);
							a->killed++;
							if(a->grabID==a2->id) a->grabID= -1;
							continue;
						} else if(a->id==SELECTION) addEvent("Selected Agent Bit Another!", EventColor::ORANGE);
					}
				} //end jaw mechanics

				//---GRABBING---//
				//doing this last because agent deaths may occur up till now, and we don't want to worry about holding onto things that die while holding them
				if(GRAB_PRESSURE!=0 && GRABBING_DISTANCE>0 && a->isGrabbing() && a2->health>0) {
					if(d<=GRABBING_DISTANCE+a->radius){
						Vector2f v(1,0);
						v.rotate(a->angle+a->grabangle);
						float diff= v.angle_between(a2->pos-a->pos);

						//check init grab
						if(a->grabID==-1){
							if (fabs(diff)<M_PI/4 && randf(0,1)<0.3) { //very wide AOE centered on a's grabangle, 30% chance any one bot is picked
								a->grabID= a2->id;
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

								float ff1= a2->radius/a->radius*a->grabbing*GRAB_PRESSURE; //the radii come in here for inertia-like effect
								float ff2= a->radius/a2->radius*a->grabbing*GRAB_PRESSURE;
								a->pos.x-= (tpos.x-a2->pos.x)*ff1;
								a->pos.y-= (tpos.y-a2->pos.y)*ff1;
								a2->pos.x+= (tpos.x-a2->pos.x)*ff2;
								a2->pos.y+= (tpos.y-a2->pos.y)*ff2;

								a->borderRectify();
								a2->borderRectify();
							}

							//the graber gets rotated toward the grabbed agent by a ratio of their output strength, limited to 0.08 radian
							a->angle+= capm(a->grabbing*diff, -0.08, 0.08);
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

	#if defined(_DEBUG)
	if(DEBUG) printf("/");
	#endif
}

void World::healthTick()
{
	#if defined(_DEBUG)
	if(DEBUG) printf("H");
	#endif

	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].health>0){
			//"natural" causes of death: age, wheel activity, excesive brain activity
			//baseloss starts by penalizing high average wheel speed
			float baseloss= HEALTHLOSS_WHEELS*(fabs(agents[i].w1) + fabs(agents[i].w2))/2;

			//getting older reduces health.
			baseloss += (float)agents[i].age/MAXAGE*HEALTHLOSS_AGING;

			//if boosting, init (baseloss + age loss) * metab loss is multiplied by boost loss
			if (agents[i].boost) {
				baseloss *= HEALTHLOSS_BOOSTMULT;
			}

			//brain activity reduces health
			if(HEALTHLOSS_BRAINUSE!=0) baseloss += HEALTHLOSS_BRAINUSE*agents[i].getActivity();

			agents[i].addDamage(conf::DEATH_NATURAL, baseloss);
			if (agents[i].health==0) continue; //agent died, we must move on.

			//remove health from lack of "air", a straight up penalty to all bots for large population
			agents[i].addDamage(conf::DEATH_TOOMANYAGENTS, HEALTHLOSS_NOOXYGEN*agents.size()/AGENTS_MAX_NOOXYGEN);
			if (agents[i].health==0) continue; //agent died, we must move on!

			//apply temperature discomfort
			agents[i].addDamage(conf::DEATH_BADTEMP, HEALTHLOSS_BADTEMP*agents[i].discomfort);
		}
	}

	#if defined(_DEBUG)
	if(DEBUG) printf("/");
	#endif
}

void World::setSelection(int type) {
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
				if (agents[i].health>0 && agents[i].age>maxage) { 
				   maxage= agents[i].age; 
				   maxi= i;
			   }
			}
		} else if(type==Select::BEST_GEN){
			//highest generation: same as with Oldest, but this time the generation counter
			int maxgen= 0;
			for (int i=0; i<(int)agents.size(); i++) {
			   if (agents[i].health>0 && agents[i].gencount>maxgen) { 
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
				if (agents[i].health>0 && agents[i].age>0 && agents[i].exhaustion<minexhaust) {
					minexhaust= agents[i].exhaustion;
					maxi= i;
				}
			}
		} else if(type==Select::PRODUCTIVE){
			float maxprog= 0;
			//Productivity: find the agent who's been most successful at having children
			for (int i=0; i<(int)agents.size(); i++) {
				if(agents[i].health>0 && agents[i].age!=0){
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
				if (agents[i].health>0 && index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		} else if (type==Select::RELATIVE){
			//Relative: this special mode will calculate the closest relative to the selected if the selected died. Good mode to leave on fast mode
			int select= getSelectedAgent();
			if(select>=0){
				if(agents[select].health<=0){
					int index= getClosestRelative(select);
					if(index>=0) {
						maxi= index;
						printf("---> New relative selected.\n");
					}
				} else maxi= select;
			}
		} else if (type==Select::BEST_HERBIVORE){
			//best of a given stomach type, not counting gen 0. calculates stomach score and then compares; unlike types subtract
			float maxindex= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= agents[i].stomach[Stomach::PLANT] - agents[i].stomach[Stomach::MEAT] - agents[i].stomach[Stomach::FRUIT];
				if (agents[i].health>0 && agents[i].gencount>0 && index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		} else if (type==Select::BEST_FRUGIVORE){
			//best of a given stomach type, not counting gen 0. calculates stomach score and then compares; unlike types subtract
			float maxindex= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= agents[i].stomach[Stomach::FRUIT] - agents[i].stomach[Stomach::MEAT] - agents[i].stomach[Stomach::PLANT];
				if (agents[i].health>0 && agents[i].gencount>0 && index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		} else if (type==Select::BEST_CARNIVORE){
			//best of a given stomach type, not counting gen 0. calculates stomach score and then compares; unlike types subtract
			float maxindex= -1;
			for (int i=0; i<(int)agents.size(); i++) {
				float index= agents[i].stomach[Stomach::MEAT] - agents[i].stomach[Stomach::FRUIT] - agents[i].stomach[Stomach::PLANT];
				if (agents[i].health>0 && agents[i].gencount>0 && index>maxindex) {
					maxindex= index;
					maxi= i;
				}
			}
		}
			
		if (maxi!=-1) {
			if(agents[maxi].id!= SELECTION) setSelectedAgent(maxi);
		} else if (type!=Select::MANUAL) {
			SELECTION= -1;
		}
	}
}

bool World::setSelectionRelative(int posneg) 
{
	//returns bool because GLView would like to know if successful or not
	int sid= getSelectedAgent();
	if(sid>=0){
		//get selected species id
		int species= agents[sid].species;
		int bestidx= sid;
		int bestdd= MAXDEVIATION;

		//for each agent, check if its alive, not exactly like us, within maxdeviation, and a positive difference in speciesID
		for(int i=0; i<agents.size(); i++){
			if(agents[i].health<=0 || agents[i].species==species) continue;

			int dd= posneg*(agents[i].species - species);
			if(dd>MAXDEVIATION || dd<0) continue;

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

void World::setSelectedAgent(int idx) {
	if (agents[idx].id==SELECTION) SELECTION= -1; //toggle selection if already selected
	else SELECTION= agents[idx].id; //otherwise, select as normal

	if(agents[idx].health>0) agents[idx].initSplash(10,1.0,1.0,1.0);
	setControl(false);
}

int World::getSelectedAgent() const {
	//retrieve array index of selected agent, returns -1 if none
	int idx= -1;
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].id==SELECTION) idx= i;
	}
	return idx;
}

int World::getClosestRelative(int idx) {
	//retrieve the index of the given agent's closest living relative. Takes age, gencount, stomach, lungs, and species id into account, returns -1 if none
	int nidx= -1;
	int meta, metamax= 1;
	std::string bestrelative= "";

	if(idx>=0){
		for (int i=0; i<(int)agents.size(); i++) {
			if(idx==i) continue;

			if(abs(agents[idx].species-agents[i].species)<MAXDEVIATION && agents[i].health>0){
				//choose an alive agent that is within maxdeviation; closer the better
				meta= 1+MAXDEVIATION-abs(agents[idx].species-agents[i].species);
			} else continue; //if you aren't related, you're off to a very bad start if we're trying to find relatives... skip!

			if(agents[i].age<TENDERAGE) meta+= 3; //choose young agents at least
			if(agents[idx].gencount+1==agents[i].gencount) { meta+= 5; bestrelative= "youngling"; } //choose children or nephews and nieces
			else if(agents[idx].gencount==agents[i].gencount) { meta+= 3; bestrelative= "cousin"; } //at least choose cousins... 
			else if(agents[idx].gencount==agents[i].gencount+1) { meta+= 2; bestrelative= "elder"; } //...or parents/aunts/uncles
			if(agents[idx].age==agents[i].age) { meta+= 10; bestrelative= "sibling"; }
			if(agents[idx].parentid==agents[i].id) { meta+= 4; bestrelative= "mother"; }
			if(agents[idx].id==agents[i].parentid) { meta+= 8; bestrelative= "daughter"; }

			for(int j=0; j<Stomach::FOOD_TYPES; j++){ //penalize mis-matching stomach types harshly
				meta-= 5*abs(agents[idx].stomach[j]-agents[i].stomach[j]); //diff of 0.2 scales to a -1 penalty, max possible= -15
			}
			meta-= abs(agents[idx].lungs-agents[i].lungs); //penalize mis-matching lung types, max possible= -1
			meta-= abs(agents[idx].temperature_preference-agents[i].temperature_preference); //penalize mis-matching temp preference, max possible: -1
			
			//if the meta counter is better than the best selection so far*, return our new target
			//*note that it has to do better than MAXDEVIATION as a baseline because we want ONLY relatives, and if no matches, we return -1
			if(meta>metamax){
				nidx= i;
				metamax= meta;
			}
		}
	}

	if(bestrelative!="") {
		bestrelative= "Autoselected living " + bestrelative;
		const char *temp= bestrelative.c_str();
		addEvent(temp, EventColor::BLACK);
	} else if (nidx!=-1) addEvent("Autoselected distant cousin", EventColor::BLACK);
	else addEvent("No More Living Relatives!", EventColor::BLACK);

	//IMPORTANT: we may return -1 here; DO NOT USE FOR ARRAYS DIRECTLY
	return nidx;
}

int World::getSelectedID() const {
	//retrieve world->SELECTION
	return SELECTION;
}


void World::selectedHeal() {
	//heal selected agent
	int sidx= getSelectedAgent();
	if(sidx>=0){
		agents[sidx].health= 2.0;
		agents[sidx].carcasscount= -1;
	}
}


void World::selectedKill() {
	//kill (delete) selected agent
	int sidx= getSelectedAgent();
	if(sidx>=0){
		if(agents[sidx].health<=0) agents[sidx].carcasscount= conf::CARCASSFRAMES;
		else agents[sidx].addDamage(conf::DEATH_USER, 9001);
	}
}

void World::selectedBabys() {
	//force selected agent to assexually reproduce
	int sidx= getSelectedAgent();
	if(sidx>=0){
		reproduce(sidx, sidx);
	}
}

void World::selectedMutate() {
	//mutate selected agent
	int sidx= getSelectedAgent();
	if(sidx>=0){
		agents[sidx].liveMutate();
		if(agents[sidx].id==SELECTION) addEvent("The Selected Agent Was Mutated!", EventColor::PURPLE);
	}
}

void World::selectedStomachMut() {
	//rotate selected agent's stomach
	int sidx= getSelectedAgent();
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
	int sidx= getSelectedAgent();
	if(sidx>=0){
		agents[sidx].printSelf();
	}
}

void World::selectedTrace(int mode) {
	//perform a traceback on the selected agent's outputs as called by mode
	int sidx= getSelectedAgent();
	if(sidx>=0){
		if(mode==1){
			//get all the wheel output inputs
			printf("==========TRACEBACK START===========\n");
			agents[sidx].traceBack(Output::LEFT_WHEEL_F);
			agents[sidx].traceBack(Output::LEFT_WHEEL_B);
			agents[sidx].traceBack(Output::RIGHT_WHEEL_F);
			agents[sidx].traceBack(Output::RIGHT_WHEEL_B);
			printf("====================================\n");
		}
	}
}

void World::selectedInput(bool state) {
	//set the selected agent's user input
	int sidx= getSelectedAgent();
	if(sidx>=0){
		agents[sidx].in[Input::PLAYER]= (float)state;
	}
}

void World::getFollowLocation(float &xi, float &yi) {
	int sidx= getSelectedAgent();
	if (sidx>=0){
		xi= agents[sidx].pos.x;
		yi= agents[sidx].pos.y;
	}
}

bool World::processMouse(int button, int state, int x, int y, float scale)
{
	//now returns true if an agent was selected manually
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
{
	agent.id= idcounter;
	idcounter++;
	agents.push_back(agent);
}

bool World::addLoadedAgent(float x, float y)
{
	if(loadedagent.brain.boxes.size()==BRAINSIZE){
		Agent a= loadedagent;
		a.pos.x= x; a.pos.y= y;
		a.angle= randf(-M_PI,M_PI);
		addAgent(a);
	} else return false;
	return true;
}

void World::addAgents(int num, int set_stomach, bool set_lungs, float nx, float ny)
{
	for (int i=0;i<num;i++) {
		Agent a(BRAINSIZE, MEANRADIUS, REP_PER_BABY, DEFAULT_MUTCHANCE, DEFAULT_MUTSIZE);

		if (set_stomach==Stomach::PLANT) a.setHerbivore(); //if told to predetermine stomach type
		else if (set_stomach==Stomach::MEAT) a.setCarnivore();
		else if (set_stomach==Stomach::FRUIT) a.setFrugivore();
		else if (set_stomach==-2) i%2==0 ? a.setHerbivore() : a.setFrugivore();
		else if (getEpoch()==0 && a.isCarnivore()) randf(0,1)<0.5 ? a.setHerbivore() : a.setFrugivore(); //epoch 0 has no carnivores forced
		
		int scx= 0,scy= 0,count=0;
		while(DISABLE_LAND_SPAWN){
			scx= (int) a.pos.x/conf::CZ;
			scy= (int) a.pos.y/conf::CZ;
			if(cells[Layer::ELEVATION][scx][scy]<Elevation::BEACH_MID) break;
			a.setPosRandom(conf::WIDTH, conf::HEIGHT);
			count++;
			if(STATlandratio==1.0 || count>1000000) {
				printf("ALERT: spawn failed to find a suitable water cell. Enabling land spawn!");
				DISABLE_LAND_SPAWN= false;
			}
		}

		if (nx!=-1 || ny!=-1){ //if custom location given
			a.pos.x= nx;
			a.pos.y= ny;
		}

		if (set_lungs){ //if told to fix lungs for agent's position
			a.lungs= cap(randn((float)cells[Layer::ELEVATION][scx][scy],0.2));
		}

		addAgent(a);
	}
}

void World::reproduce(int i1, int i2)
{
	Agent mother= agents[i1]; //mother is reproducing agent. her health, her repcounter, and her base stats are divided/reset/used
	Agent father= agents[i2]; //father is a nearby agent that may have been sexting, or just the mother again. either way, repcounter is reset

	float healthloss= 0; //health lost by mother for assexual reproduction
	if (i1==i2){ //if assexual rep, father is just the mother again
		father= agents[i1];
		healthloss= agents[i1].radius/MEANRADIUS*HEALTHLOSS_ASSEX;

		agents[i1].initSplash(conf::RENDER_MAXSPLASHSIZE,0,0.8,0); //green splash means agent asexually reproduced

	}else{ //otherwise, it's sexual
		agents[i1].initSplash(conf::RENDER_MAXSPLASHSIZE,0,0,0.8);
		agents[i2].initSplash(conf::RENDER_MAXSPLASHSIZE,0,0,0.8); //blue splashes mean agents sexually reproduced.
	}

	float newhealth= agents[i1].health/(agents[i1].numbabies + 1) - mother.repcounter/mother.maxrepcounter;
	//repcounter should be negative or zero here, so its actually giving more health for the overflow

	agents[i1].health= newhealth;
	agents[i1].addDamage(conf::DEATH_ASEXUAL, healthloss);

	//Reset the MOTHER's repcounter ONLY (added bonus of sexual rep and allows possible dichotomy of sexes)
	agents[i1].resetRepCounter(MEANRADIUS, REP_PER_BABY);

	if (SELECTION==agents[i1].id) printf("The Selected Agent has Reproduced and had %i Babies!\n", agents[i1].numbabies);

	#pragma omp parallel for ordered schedule(dynamic) //allow the CREATION of each baby to happen on a new thread
	for (int i=0;i<agents[i1].numbabies;i++) {
		Agent daughter= mother.reproduce(father, MEANRADIUS, REP_PER_BABY);

		daughter.health= newhealth;
		
		#pragma omp ordered //restore order and collapse threads to prevent count errors below
		{
		if (i1!=i2){
			daughter.hybrid= true; //if parents are not the same agent (sexual reproduction), then mark the child
			agents[i2].children++;
		}

		agents[i1].children++;

		addAgent(daughter);
		}
	}
}

void World::writeReport()
{
	if(DEMO) return;
	printf("Writing Report, Epoch: %i, Day: %i\n", getEpoch(), getDay());
	//save all kinds of nice data stuff
	int randgen= 0;
	int randspec= 0;
	int randseed= 0;
	float randradius= 0;
	float randmetab= 0;
	float randmutchance= 0;
	float randmutsize= 0;
	float randtemppref= 0;
	int randchildren= 0;

	int randagent= randi(0,agents.size());
	randgen= agents[randagent].gencount;
	randspec= agents[randagent].species;
	for (int i= 0; i<(int)agents[randagent].brain.boxes.size(); i++){
		if (agents[randagent].brain.boxes[i].seed>randseed) randseed=agents[randagent].brain.boxes[i].seed;
	}
	randradius= agents[randagent].radius;
	randmetab= agents[randagent].metabolism;
	randmutchance= agents[randagent].MUTCHANCE;
	randmutsize= agents[randagent].MUTSIZE;
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
	fprintf(fr, "Growth*:\t%f\tMutation*:\t%i\t",
		DROUGHTMULT, MUTEVENTMULT);
	//print world stats: agent specifics
	fprintf(fr, "#Herbi:\t%i\t#Frugi:\t%i\t#Carni:\t%i\t#Terra:\t%i\t#Amphib:\t%i\t#Aqua:\t%i\t#Hybrids:\t%i\t#Spikes:\t%i\t",
		getHerbivores(), getFrugivores(), getCarnivores(), getLungLand(), getLungAmph(), getLungWater(), getHybrids(), getSpiky());
	//print world stats: cell counts
	fprintf(fr, "#Plant:\t%i\t#Meat:\t%i\t#Hazard:\t%i\t#Fruit:\t%i\t",
		getFood(), getMeat(), getHazards(), getFruit());
	//print random selections: Genome, brain seeds, [[[generation]]]
	fprintf(fr, "RGenome:\t%i\tRSeed:\t%i\tRGen:\t%i\tRRadius:\t%f\tRMetab:\t%f\tRTempP:\t%f\tRChildren:\t%i\t",
		randspec, randseed, randgen, randradius, randmetab, randtemppref, randchildren);
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
	return STATallplant/FOODWASTE*FOODINTAKE/2/100; //#food * 1/FOODWASTE tick/food * FOODINTAKE/1 health/tick * 1/2 agent/health = #agents
	// /100 because... reasons
}

float World::getFruitSupp() const
{
	return STATallfruit/FRUITWASTE*FRUITINTAKE/2/100;
}

float World::getMeatSupp() const
{
	return STATallmeat/MEATWASTE*MEATINTAKE/2/100;
}

float World::getHazardSupp() const
{
	return STATallhazard*HAZARDDAMAGE/2*50; //BUG? #hazard * HAZARDDAMAGE/1/1 health/hazard/tick * 1/2 agent/health != #agents killed per tick
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

			if (agents[i].isSpikey(SPIKELENGTH)) STATspiky++;

			if (agents[i].hybrid) {
				STAThybrids++;
				if (agents[i].gencount>STATbesthybrid) STATbesthybrid= agents[i].gencount;
			}
		}
		else STATdead++;
	}

	//cell layers
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			if(cells[Layer::PLANTS][i][j]>=0.5) STATplants++;
			if(cells[Layer::FRUITS][i][j]>=0.5) STATfruits++;
			if(cells[Layer::MEATS][i][j]>=0.5) STATmeats++;
			if(cells[Layer::HAZARDS][i][j]>=0.5) STAThazards++;
			STATallplant+= cells[Layer::PLANTS][i][j];
			STATallfruit+= cells[Layer::FRUITS][i][j];
			STATallmeat+= cells[Layer::MEATS][i][j];
			STATallhazard+= cells[Layer::HAZARDS][i][j];
		}
	}
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

void World::addEvent(const char * text, int type)
{
	//use EventColor:: namespace to access available colors
	std::pair <const char *,std::pair <int,int> > data;
	data.first= text;
	data.second.first= type;
	data.second.second= conf::EVENTS_HALFLIFE;
	events.push_back(data);
}

void World::dismissNextEvents(int count)
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

void World::init()
{
	//ALL .cfg constants must be initially declared in world.h and defined here.
	NO_TIPS= false;
	SPAWN_LAKES= conf::SPAWN_LAKES;
	DISABLE_LAND_SPAWN= conf::DISABLE_LAND_SPAWN;
	MOONLIT= conf::MOONLIT;
	MOONLIGHTMULT= conf::MOONLIGHTMULT;
	DROUGHTS= conf::DROUGHTS;
	DROUGHT_MIN= conf::DROUGHT_MIN;
	DROUGHT_MAX= conf::DROUGHT_MAX;
	MUTEVENTS= conf::MUTEVENTS;
	MUTEVENT_MAX= conf::MUTEVENT_MAX;

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

	CONTINENTS= conf::CONTINENTS;
    OCEANPERCENT= conf::OCEANPERCENT;
    GRAVITYACCEL= conf::GRAVITYACCEL;
    BUMP_PRESSURE= conf::BUMP_PRESSURE;
	GRAB_PRESSURE= conf::GRAB_PRESSURE;
	BRAINSIZE= conf::BRAINSIZE;
    BOTSPEED= conf::BOTSPEED;
    BOOSTSIZEMULT= conf::BOOSTSIZEMULT;
	BOOSTEXAUSTMULT= conf::BOOSTEXAUSTMULT;
	SOUNDPITCHRANGE= conf::SOUNDPITCHRANGE;

    FOODTRANSFER= conf::FOODTRANSFER;
	BASEEXHAUSTION= conf::BASEEXHAUSTION;
	EXHAUSTION_MULT= conf::EXHAUSTION_MULT;
	MEANRADIUS= conf::MEANRADIUS;
    SPIKESPEED= conf::SPIKESPEED;
    FRESHKILLTIME= conf::FRESHKILLTIME;
	TENDERAGE= conf::TENDERAGE;
    MINMOMHEALTH= conf::MINMOMHEALTH;
	MIN_INTAKE_HEALTH_RATIO= conf::MIN_INTAKE_HEALTH_RATIO;
	FUN= 0;
	SUN_RED= 1.0;
	SUN_GRE= 1.0;
	SUN_BLU= 0.8;
    REP_PER_BABY= conf::REP_PER_BABY;
	OVERHEAL_REPFILL= conf::OVERHEAL_REPFILL;
//    LEARNRATE= conf::LEARNRATE;
    MAXDEVIATION= conf::MAXDEVIATION;
	DEFAULT_MUTCHANCE= conf::DEFAULT_MUTCHANCE;
	DEFAULT_MUTSIZE= conf::DEFAULT_MUTSIZE;
	LIVE_MUTATE_CHANCE= conf::LIVE_MUTATE_CHANCE;
    MAXAGE= conf::MAXAGE;
	MAXWASTEFREQ= conf::MAXWASTEFREQ;

    DIST= conf::DIST;
    SPIKELENGTH= conf::SPIKELENGTH;
    TOOCLOSE= conf::TOOCLOSE;
    FOOD_SHARING_DISTANCE= conf::FOOD_SHARING_DISTANCE;
    SEXTING_DISTANCE= conf::SEXTING_DISTANCE;
    GRABBING_DISTANCE= conf::GRABBING_DISTANCE;

    HEALTHLOSS_WHEELS= conf::HEALTHLOSS_WHEELS;
    HEALTHLOSS_BOOSTMULT= conf::HEALTHLOSS_BOOSTMULT;
    HEALTHLOSS_BADTEMP= conf::HEALTHLOSS_BADTEMP;
    HEALTHLOSS_AGING= conf::HEALTHLOSS_AGING;
    HEALTHLOSS_BRAINUSE= conf::HEALTHLOSS_BRAINUSE;
    HEALTHLOSS_SPIKE_EXT= conf::HEALTHLOSS_SPIKE_EXT;
    HEALTHLOSS_BADTERRAIN= conf::HEALTHLOSS_BADTERRAIN;
    HEALTHLOSS_NOOXYGEN= conf::HEALTHLOSS_NOOXYGEN;
    HEALTHLOSS_ASSEX= conf::HEALTHLOSS_ASSEX;

	DAMAGE_FULLSPIKE= conf::DAMAGE_FULLSPIKE;
	DAMAGE_COLLIDE= conf::DAMAGE_COLLIDE;
    DAMAGE_JAWSNAP= conf::DAMAGE_JAWSNAP;

	STOMACH_EFF= conf::STOMACH_EFF;

    FOODINTAKE= conf::FOODINTAKE;
    FOODDECAY= conf::FOODDECAY;
    FOODGROWTH= conf::FOODGROWTH;
    FOODWASTE= conf::FOODWASTE;
    FOODADDFREQ= conf::FOODADDFREQ;
    FOODSPREAD= conf::FOODSPREAD;
    FOODRANGE= conf::FOODRANGE;

    FRUITINTAKE= conf::FRUITINTAKE;
    FRUITDECAY= conf::FRUITDECAY;
    FRUITWASTE= conf::FRUITWASTE;
    FRUITADDFREQ= conf::FRUITADDFREQ;
    FRUITREQUIRE= conf::FRUITREQUIRE;

    MEATINTAKE= conf::MEATINTAKE;
    MEATDECAY= conf::MEATDECAY;
    MEATWASTE= conf::MEATWASTE;
    MEATVALUE= conf::MEATVALUE;

    HAZARDFREQ= conf::HAZARDFREQ;
	HAZARDEVENT_MULT= conf::HAZARDEVENT_MULT;
    HAZARDDECAY= conf::HAZARDDECAY;
    HAZARDDEPOSIT= conf::HAZARDDEPOSIT;
    HAZARDDAMAGE= conf::HAZARDDAMAGE;
	HAZARDPOWER= conf::HAZARDPOWER;

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
	tips.push_back("Press 'q' to center map and graphs");
	tips.push_back("Press 'spacebar' to pause");
	tips.push_back("Press 'h' for detailed interface help");
	tips.push_back("Zoom in with '>' to see more details");
	tips.push_back("Zoom with middle mouse click&drag");
	tips.push_back("Pan around with left click&drag");
	tips.push_back("Dead agents are semi-transparent");
	tips.push_back("Cycle layer views with 'k' & 'l'");
	tips.push_back("Reselect all layers with 'o'");
	tips.push_back("Overgrowth Epoch: more plants");
	tips.push_back("Drought Epoch: less plant growth");
	tips.push_back("Press 'h' for detailed interface help");
//	tips.push_back("Press 'g' for graphics details");
	tips.push_back("Press 'm' to activate fast mode");
	tips.push_back("Demo prevents report.txt changes");
	tips.push_back("Patience, Epoch 0 species are rare");
	tips.push_back("Select 'New World' to end Demo");
	tips.push_back("Agents display splashes for events");
	tips.push_back("Yellow spash = agent bit another");
	tips.push_back("Orange spash= agent stabbed other");
	tips.push_back("Red spash = agent killed another");
	tips.push_back("Green spash= asexually reproduced");
	tips.push_back("Blue spash = sexually reproduced");
	tips.push_back("Purple spash = mutation occured");
	tips.push_back("Press 'm' to simulate at max speed");
	tips.push_back("Use settings.cfg to change sim rules");
	tips.push_back("'report.txt' logs useful data");
	tips.push_back("Epoch 0 is boring. Press 'm'!");
	tips.push_back("The settings.cfg is FUN, isnt it?!");
	tips.push_back("Reports periodically saved to file");
	tips.push_back("Press 'm' to also disable Demo mode");
	tips.push_back("See more options with right-click");
	tips.push_back("Use settings.cfg to change constants");
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
	//get world constants from config file, settings.cfg
	char line[64], *pos;
	char var[16];
	char dataval[16];
	int i; //integer buffer
	float f; //float buffer

	//first check version number
	FILE* cf = fopen("settings.cfg", "r");
	if(cf){
		addEvent("settings.cfg detected & loaded", EventColor::CYAN);
		printf("tweaking the following constants: ");
		while(!feof(cf)){
			fgets(line, sizeof(line), cf);
			pos= strtok(line,"\n");
			sscanf(line, "%s%s", var, dataval);

			if(strcmp(var, "V=")==0){ //strcmp= 0 when the arguements equal
				sscanf(dataval, "%f", &f);
				if(f!=conf::VERSION) {
					fclose(cf);
					printf("CONFIG FILE VERSION MISMATCH!\n EXPECTED 'V= %3.2f', found 'V= %3.2f'\n", conf::VERSION, f);
					printf("Need to overwrite config to avoid serious errors!");
					printf("Exit now to keep your custom settings file; otherwise, hit enter to overwrite. . .");
					cin.get();
					writeConfig();
					readConfig();
					break;
				}
			}else if(strcmp(var, "SPAWN_LAKES=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)SPAWN_LAKES) printf("SPAWN_LAKES, ");
				if(i==1) SPAWN_LAKES= true;
				else SPAWN_LAKES= false;
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
				if(i!=((int)DEMO-1)) printf("DEMO (NO_DEMO), ");
				if(i==1) DEMO= false; //DEMO vs NODEMO: setting is reversed, and we IGNORE it if it's 0 (otherwise we would get weird unintended behavior when loading/saving)
				//else DEMO= true;
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
			}else if(strcmp(var, "CONTINENTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=CONTINENTS) printf("CONTINENTS, ");
				CONTINENTS= i;
			}else if(strcmp(var, "OCEANPERCENT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=OCEANPERCENT) printf("OCEANPERCENT, ");
				OCEANPERCENT= f;
			}else if(strcmp(var, "GRAVITYACCEL=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=GRAVITYACCEL) printf("GRAVITYACCEL, ");
				GRAVITYACCEL= f;
			}else if(strcmp(var, "BUMP_PRESSURE=")==0 || strcmp(var, "REACTPOWER=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BUMP_PRESSURE) printf("BUMP_PRESSURE, ");
				BUMP_PRESSURE= f;
			}else if(strcmp(var, "GRAB_PRESSURE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=GRAB_PRESSURE) printf("GRAB_PRESSURE, ");
				GRAB_PRESSURE= f;
			}else if(strcmp(var, "BRAINSIZE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=BRAINSIZE) printf("BRAINSIZE, ");
				BRAINSIZE= i;
			}else if(strcmp(var, "BOTSPEED=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BOTSPEED) printf("BOTSPEED, ");
				BOTSPEED= f;
			}else if(strcmp(var, "BOOSTSIZEMULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BOOSTSIZEMULT) printf("BOOSTSIZEMULT, ");
				BOOSTSIZEMULT= f;
			}else if(strcmp(var, "BOOSTEXAUSTMULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BOOSTEXAUSTMULT) printf("BOOSTEXAUSTMULT, ");
				BOOSTEXAUSTMULT= f;
			}else if(strcmp(var, "SOUNDPITCHRANGE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SOUNDPITCHRANGE) printf("SOUNDPITCHRANGE, ");
				SOUNDPITCHRANGE= f;
			}else if(strcmp(var, "FOODTRANSFER=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FOODTRANSFER) printf("FOODTRANSFER, ");
				FOODTRANSFER= f;
			}else if(strcmp(var, "BASEEXHAUSTION=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=BASEEXHAUSTION) printf("BASEEXHAUSTION, ");
				BASEEXHAUSTION= f;
			}else if(strcmp(var, "EXHAUSTION_MULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=EXHAUSTION_MULT) printf("EXHAUSTION_MULT, ");
				EXHAUSTION_MULT= f;
			}else if(strcmp(var, "MEANRADIUS=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEANRADIUS) printf("MEANRADIUS, ");
				MEANRADIUS= f;
			}else if(strcmp(var, "SPIKESPEED=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SPIKESPEED) printf("SPIKESPEED, ");
				SPIKESPEED= f;
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
			}else if(strcmp(var, "SUN_BLU=")==0){
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
			}else if(strcmp(var, "MAXDEVIATION=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MAXDEVIATION) printf("MAXDEVIATION, ");
				MAXDEVIATION= f;
			}else if(strcmp(var, "DEFAULT_MUTCHANCE=")==0 || strcmp(var, "MUTCHANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DEFAULT_MUTCHANCE) printf("DEFAULT_MUTCHANCE, ");
				DEFAULT_MUTCHANCE= f;
			}else if(strcmp(var, "DEFAULT_MUTSIZE=")==0 || strcmp(var, "MUTSIZE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DEFAULT_MUTSIZE) printf("DEFAULT_MUTSIZE, ");
				DEFAULT_MUTSIZE= f;
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
			}else if(strcmp(var, "DIST=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=DIST) printf("DIST, ");
				DIST= f;
			}else if(strcmp(var, "SPIKELENGTH=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=SPIKELENGTH) printf("SPIKELENGTH, ");
				SPIKELENGTH= f;
			}else if(strcmp(var, "TOOCLOSE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=TOOCLOSE) printf("TOOCLOSE, ");
				TOOCLOSE= f;
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
			}else if(strcmp(var, "STOMACH_EFF=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=STOMACH_EFF) printf("STOMACH_EFF, ");
				STOMACH_EFF= f;
			}else if(strcmp(var, "FOODINTAKE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FOODINTAKE) printf("FOODINTAKE, ");
				FOODINTAKE= f;
			}else if(strcmp(var, "FOODDECAY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FOODDECAY) printf("FOODDECAY, ");
				FOODDECAY= f;
			}else if(strcmp(var, "FOODGROWTH=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FOODGROWTH) printf("FOODGROWTH, ");
				FOODGROWTH= f;
			}else if(strcmp(var, "FOODWASTE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FOODWASTE) printf("FOODWASTE, ");
				FOODWASTE= f;
			}else if(strcmp(var, "FOODADDFREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=FOODADDFREQ) printf("FOODADDFREQ, ");
				FOODADDFREQ= i;
			}else if(strcmp(var, "FOODSPREAD=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FOODSPREAD) printf("FOODSPREAD, ");
				FOODSPREAD= f;
			}else if(strcmp(var, "FOODRANGE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=FOODRANGE) printf("FOODRANGE, ");
				FOODRANGE= i;
			}else if(strcmp(var, "FRUITINTAKE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FRUITINTAKE) printf("FRUITINTAKE, ");
				FRUITINTAKE= f;
			}else if(strcmp(var, "FRUITDECAY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FRUITDECAY) printf("FRUITDECAY, ");
				FRUITDECAY= f;
			}else if(strcmp(var, "FRUITWASTE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FRUITWASTE) printf("FRUITWASTE, ");
				FRUITWASTE= f;
			}else if(strcmp(var, "FRUITADDFREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=FRUITADDFREQ) printf("FRUITADDFREQ, ");
				FRUITADDFREQ= i;
			}else if(strcmp(var, "FRUITREQUIRE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=FRUITREQUIRE) printf("FRUITREQUIRE, ");
				FRUITREQUIRE= f;
			}else if(strcmp(var, "MEATINTAKE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEATINTAKE) printf("MEATINTAKE, ");
				MEATINTAKE= f;
			}else if(strcmp(var, "MEATDECAY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEATDECAY) printf("MEATDECAY, ");
				MEATDECAY= f;
			}else if(strcmp(var, "MEATWASTE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEATWASTE) printf("MEATWASTE, ");
				MEATWASTE= f;
			}else if(strcmp(var, "MEATVALUE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MEATVALUE) printf("MEATVALUE, ");
				MEATVALUE= f;
			}else if(strcmp(var, "HAZARDFREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=HAZARDFREQ) printf("HAZARDFREQ, ");
				HAZARDFREQ= i;
			}else if(strcmp(var, "HAZARDEVENT_MULT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARDEVENT_MULT) printf("HAZARDEVENT_MULT, ");
				HAZARDEVENT_MULT= f;
			}else if(strcmp(var, "HAZARDDECAY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARDDECAY) printf("HAZARDDECAY, ");
				HAZARDDECAY= f;
			}else if(strcmp(var, "HAZARDDEPOSIT=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARDDEPOSIT) printf("HAZARDDEPOSIT, ");
				HAZARDDEPOSIT= f;
			}else if(strcmp(var, "HAZARDDAMAGE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARDDAMAGE) printf("HAZARDDAMAGE, ");
				HAZARDDAMAGE= f;
			}else if(strcmp(var, "HAZARDPOWER=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=HAZARDPOWER) printf("HAZARDPOWER, ");
				HAZARDPOWER= f;
			}
		}
		if(cf){
			printf("and done!\n");
			fclose(cf);
		}

	} else {
		#if defined(_DEBUG)
		printf("_DEBUG: settings.cfg did not exist and suppressing creation in debug.\n");
		#else
		printf("settings.cfg did not exist! Its ok, I gave you a new one to tinker with\n");
		writeConfig();
		#endif
	}
}

void World::writeConfig()
{
	//called if settings.cfg is missing or version number of program is greater
	//happens after initializing and skips loading of config
	printf("encoding new config file..");

	FILE* cf= fopen("settings.cfg", "w");

	fprintf(cf, "settings.cfg\nThis file will be regenerated with default values if missing (regeneration is not necessary for the program to work)\nModify any value in this file to change constants in Evagents.\nAll changes will require a reset or relaunch to take effect\n");
	fprintf(cf, "\n");
	fprintf(cf, "Any lines that the program can't recognize are silently ignored, and anything that is changed is reported by the program.\nPlease note the sim only takes the last entry of a flag's value if there are duplicates. Also, can you find the hidden config flags?...\n");
	fprintf(cf, "\n");
	fprintf(cf, "Remember to RIGHT-CLICK and select LOAD CONFIG, or relaunch the program after modification!!!");
	fprintf(cf, "\n\n");
	fprintf(cf, "V= %f \t\t\t//Version number this file was created in. If different than program's, will ask user to exit, or it will overwrite.\n", conf::VERSION);
	fprintf(cf, "\n");
	fprintf(cf, "NO_TIPS= %i \t\t\t//if true (=1), prevents tips from being displayed. This value is whatever was set in program when this file was saved\n", NO_TIPS);
	fprintf(cf, "NO_DEMO= %i \t\t\t//if true (=1), this will prevent demo mode from running at start. Default= 1 when writing a new config\n", 1);
	fprintf(cf, "SPAWN_LAKES= %i \t\t\t//if true (=1), and if terrain generation forms too much or too little land, the generator takes a moment to put in lakes (or islands)\n", NO_TIPS);
	fprintf(cf, "DISABLE_LAND_SPAWN= %i \t\t//true-false flag for disabling agents from spawning on land. 0= land spawn allowed, 1= not allowed. Is GUI-controllable. This value is whatever was set in program when this file was saved\n", DISABLE_LAND_SPAWN);
	fprintf(cf, "MOONLIT= %i \t\t\t//true-false flag for letting agents see other agents at night. 0= no eyesight, 1= see agents at half light. Is GUI-controllable and saved/loaded. This value is whatever was set in program when this file was saved\n", MOONLIT);
	fprintf(cf, "MOONLIGHTMULT= %f \t\t//the amount of minimum light MOONLIT provides. If set to 1, daylight cycles are not processed at all and the world becomes bathed in eternal sunlight. 0.25 default\n", conf::MOONLIGHTMULT);
	fprintf(cf, "DROUGHTS= %i \t\t\t//true-false flag for if the drought/overgrowth mechanic is enabled. 0= constant normal growth, 1= variable. Is GUI-controllable and saved/loaded. This value is whatever was set in program when this file was saved\n", DROUGHTS);
	fprintf(cf, "DROUGHT_MIN= %f \t\t//minimum multiplier value of droughts, below which it gets pushed back toward 1.0\n", conf::DROUGHT_MIN);
	fprintf(cf, "DROUGHT_MAX= %f \t\t//maximum multiplier value of droughts (overgrowth), above which it gets pushed back toward 1.0\n", conf::DROUGHT_MAX);
	fprintf(cf, "MUTEVENTS= %i \t\t\t//true-false flag for if the mutation event mechanic is enabled. 0= constant mutation rates, 1= variable. Is GUI-controllable and saved/loaded. This value is whatever was set in program when this file was saved\n", MUTEVENTS);
	fprintf(cf, "MUTEVENT_MAX= %i \t\t//integer max possible multiplier for mutation event mechanic. =1 effectively disables. =0 enables 50%% chance of no live mutations epoch. Negative values increase this chance (-1 => 66%%, -2 => 75%%, etc), thereby decreasing the chance of any live mutation epochs at all. Useful if you want to increase base mutation rates.\n", conf::MUTEVENT_MAX);
	fprintf(cf, "\n");
	fprintf(cf, "MINFOOD= %i \t\t\t//Minimum number of food cells which must have food during simulation. 0= off\n", conf::MIN_PLANT);
	fprintf(cf, "INITFOODDENSITY= %f \t//initial density of full food cells. Is a decimal percentage of the world cells. Use 'INITFOOD= #' to set a number\n", conf::INITPLANTDENSITY);
	fprintf(cf, "//INITFOOD= %i \t\t\t//remove '//' from before the flag to enable, overrides INITFOODDENSITY, is a raw # of cells which must have food at init\n", 0);
	fprintf(cf, "INITFRUITDENSITY= %f \t//initial density of full fruit cells. Is a decimal percentage of the world cells.\n", conf::INITFRUITDENSITY);
	fprintf(cf, "//INITFRUIT= %i \t\t\t//remove '//' from before the flag to enable, overrides INITFRUITDENSITY, is a raw # of cells which must have fruit at init\n", 0);
	fprintf(cf, "INITMEATDENSITY= %f \t//initial density of full meat cells. Is a decimal percentage of the world cells. Use 'INITMEAT= #' to set a number\n", conf::INITMEATDENSITY);
	fprintf(cf, "//INITMEAT= %i \t\t\t//remove '//' from before the flag to enable, overrides INITMEATDENSITY, is a raw # of cells which must have meat at init\n", 0);
	fprintf(cf, "INITHAZARDDENSITY= %f \t//initial density of full hazard cells. Is a decimal percentage of the world cells.\n", conf::INITHAZARDDENSITY);
	fprintf(cf, "//INITHAZARD= %i \t\t//remove '//' from before the flag to enable, overrides INITHAZARDDENSITY, is a raw # of cells which must have hazard at init\n", 0);
	fprintf(cf, "AGENTS_MIN_NOTCLOSED= %i \t//minimum number of agents. Random agents will get spawned in until this number is reached.\n", conf::AGENTS_MIN_NOTCLOSED);
	fprintf(cf, "AGENTS_MAX_SPAWN= %i \t\t//if more agents than this number exist (live agents), random spawns no longer occur\n", conf::AGENTS_MAX_SPAWN);
	fprintf(cf, "AGENTSPAWN_FREQ= %i \t\t//how often does a random agent get spawned while below ENOUGHAGENTS? Higher values are less frequent\n", conf::AGENTSPAWN_FREQ);
	fprintf(cf, "AGENTS_MAX_NOOXYGEN= %i \t//number of agents at which the full HEALTHLOSS_NOOXYGEN is applied\n", conf::AGENTS_MAX_NOOXYGEN);
	fprintf(cf, "\n");
	fprintf(cf, "REPORTS_PER_EPOCH= %i \t\t//number of times to record data per epoch. 0 for off\n", conf::REPORTS_PER_EPOCH);
	fprintf(cf, "FRAMES_PER_EPOCH= %i \t//number of frames before epoch is incremented by 1\n", conf::FRAMES_PER_EPOCH);
	fprintf(cf, "FRAMES_PER_DAY= %i \t\t//number of frames it takes for the daylight cycle to go completely around the map\n", conf::FRAMES_PER_DAY);
	fprintf(cf, "\n");
	fprintf(cf, "CONTINENTS= %i \t\t\t//number of 'continents' generated on the land layer. Not guaranteed to be accurate\n", conf::CONTINENTS);
	fprintf(cf, "OCEANPERCENT= %f \t\t//decimal ratio of terrain layer which will be ocean. Approximately\n", conf::OCEANPERCENT);
	fprintf(cf, "GRAVITYACCEL= %f \t\t//how fast an agent will 'fall' after jumping. 0= jump disabled, 0.1+ = super-gravity\n", conf::GRAVITYACCEL);
	fprintf(cf, "BUMP_PRESSURE= %f \t//the restoring force between two colliding agents. 0= no reaction (disables TOOCLOSE and all collisions). I'd avoid negative values if I were you...\n", conf::BUMP_PRESSURE);
	fprintf(cf, "GRAB_PRESSURE= %f \t//the restoring force between and agent and its grab target. 0= no reaction (disables grab function), negative values push agents away\n", conf::GRAB_PRESSURE);
	fprintf(cf, "BRAINSIZE= %i \t\t\t//number boxes in every agent brain. Sim will NEVER make brains smaller than # Inputs + # Outputs. Saved per world, loaded worlds will override this value\n", conf::BRAINSIZE);
	fprintf(cf, "\n");
	fprintf(cf, "BOTSPEED= %f \t\t//fastest possible speed of agents. This effects so much of the sim I dont advise changing it\n", conf::BOTSPEED);
	fprintf(cf, "BOOSTSIZEMULT= %f \t//how much speed boost do agents get when boost is active?\n", conf::BOOSTSIZEMULT);
	fprintf(cf, "BOOSTEXAUSTMULT= %f \t//how much exhaustion from brain outputs is multiplied by when boost is active?\n", conf::BOOSTEXAUSTMULT);
	fprintf(cf, "SOUNDPITCHRANGE= %f \t//range below hearhigh and above hearlow within which external sounds fade in. Would not recommend extreme values near or beyond [0,0.5]\n", conf::SOUNDPITCHRANGE);
	fprintf(cf, "FOODTRANSFER= %f \t\t//how much health is transferred between two agents trading food per tick? =0 disables all generosity\n", conf::FOODTRANSFER);
	fprintf(cf, "BASEEXHAUSTION= %f \t//base value of exhaustion. When negative, is essentially the sum amount of output allowed before healthloss. Would not recommend >=0 values\n", conf::BASEEXHAUSTION);
	fprintf(cf, "EXHAUSTION_MULT= %f \t//multiplier applied to outputsum + BASEEXHAUSTION\n", conf::EXHAUSTION_MULT);
	fprintf(cf, "MEANRADIUS= %f \t\t//\"average\" agent radius, range [0.2*this,2.2*this) (only applies to random agents, no limits on mutations). This effects SOOOO much stuff, and I would not recommend setting negative unless you like crashing programs.\n", conf::MEANRADIUS);
	fprintf(cf, "SPIKESPEED= %f \t\t//how quickly can the spike be extended? Does not apply to retraction, which is instant\n", conf::SPIKESPEED);
	fprintf(cf, "FRESHKILLTIME= %i \t\t//number of ticks after a spike, collision, or bite that an agent will still drop full meat\n", conf::FRESHKILLTIME);
	fprintf(cf, "TENDERAGE= %i \t\t\t//age (in 1/10ths) of agents where full meat, hazard, and collision damage is finally given. These multipliers are reduced via *age/TENDERAGE. 0= off\n", conf::TENDERAGE);
	fprintf(cf, "MINMOMHEALTH= %f \t\t//minimum amount of health required for an agent to have a child\n", conf::MINMOMHEALTH);
	fprintf(cf, "MIN_INTAKE_HEALTH_RATIO= %f //minimum metabolism ratio of intake always sent to health. 0= no restrictions (agent metabolism has full control), 1= 100%% health, no babies ever. default= 0.5\n", conf::MIN_INTAKE_HEALTH_RATIO);
	fprintf(cf, "REP_PER_BABY= %f \t\t//amount of food required to be consumed for an agent to reproduce, per baby\n", conf::REP_PER_BABY);
	fprintf(cf, "OVERHEAL_REPFILL= %i \t\t//true-false flag for letting agents redirect overfill health (>2) to repcounter. 1= conserves matter, 0= extra intake is destroyed, punishing overeating\n", conf::OVERHEAL_REPFILL);
//	fprintf(cf,	"LEARNRATE= %f\n", conf::LEARNRATE);
	fprintf(cf, "MAXDEVIATION= %f \t//maximum difference in species ID a crossover reproducing agent will be willing to tolerate\n", conf::MAXDEVIATION);
	fprintf(cf, "DEFAULT_MUTCHANCE= %f \t//the default chance of mutations occurring (note that various mutations modify this value up or down)\n", conf::DEFAULT_MUTCHANCE);
	fprintf(cf, "DEFAULT_MUTSIZE= %f \t//the default magnitude of mutations (note that various mutations modify this value up or down)\n", conf::DEFAULT_MUTSIZE);
	fprintf(cf, "LIVE_MUTATE_CHANCE= %f \t//chance, per tick, that a given agent will be mutated alive. Not typically harmful. Can be increased by mutation events if enabled.\n", conf::LIVE_MUTATE_CHANCE);
	fprintf(cf, "MAXAGE= %i \t\t\t//Age at which the full HEALTHLOSS_AGING amount is applied to an agent\n", conf::MAXAGE);
	fprintf(cf, "MAXWASTEFREQ= %i \t\t//max waste frequency allowed for agents. Agents can select [1,this]. Default= 200, 1= no agent control allowed, 0= program crash\n", conf::MAXWASTEFREQ);
	fprintf(cf, "\n");
	fprintf(cf, "DIST= %f \t\t//how far the senses can detect other agents or cells\n", conf::DIST);
	fprintf(cf, "SPIKELENGTH= %f \t\t//full spike length. Should not be more than DIST. 0 disables interaction\n", conf::SPIKELENGTH);
	fprintf(cf, "TOOCLOSE= %f \t\t//how much two agents can be overlapping before they take damage. 0 disables interaction\n", conf::TOOCLOSE);
	fprintf(cf, "FOOD_SHARING_DISTANCE= %f //how far away can food be shared between agents? Should not be more than DIST. 0 disables interaction\n", conf::FOOD_SHARING_DISTANCE);
	fprintf(cf, "SEXTING_DISTANCE= %f \t//how far away can two agents sexually reproduce? Should not be more than DIST. 0 disables interaction\n", conf::SEXTING_DISTANCE);
	fprintf(cf, "GRABBING_DISTANCE= %f \t//how far away can an agent grab another? Should not be more than DIST. 0 disables interaction\n", conf::GRABBING_DISTANCE);
	fprintf(cf, "\n");
	fprintf(cf, "HEALTHLOSS_WHEELS= %f \t//How much health is lost for an agent driving at full speed\n", conf::HEALTHLOSS_WHEELS);
	fprintf(cf, "HEALTHLOSS_BOOSTMULT= %f \t//how much boost costs (set to 1 to nullify boost cost; its a multiplier)\n", conf::HEALTHLOSS_BOOSTMULT);
	fprintf(cf, "HEALTHLOSS_BADTEMP= %f \t//how quickly health drains in non-preferred temperatures\n", conf::HEALTHLOSS_BADTEMP);
	fprintf(cf, "HEALTHLOSS_AGING= %f \t//health lost at MAXAGE. Note that this damage is applied to all agents in proportion to their age.\n", conf::HEALTHLOSS_AGING);
	fprintf(cf, "HEALTHLOSS_BRAINUSE= %f \t//how much health is reduced for each box in the brain being active\n", conf::HEALTHLOSS_BRAINUSE);
	fprintf(cf, "HEALTHLOSS_SPIKE_EXT= %f \t//how much health an agent looses for extending spike\n", conf::HEALTHLOSS_SPIKE_EXT);
	fprintf(cf, "HEALTHLOSS_BADTERRAIN= %f //how much health is lost if in totally opposite environment\n", conf::HEALTHLOSS_BADTERRAIN);
	fprintf(cf, "HEALTHLOSS_NOOXYGEN= %f \t//how much agents are penalized when total agents = AGENTS_MAX_NOOXYGEN\n", conf::HEALTHLOSS_NOOXYGEN);
	fprintf(cf, "HEALTHLOSS_ASSEX= %f \t//multiplier for radius/MEANRADIUS penalty on mother for asexual reproduction\n", conf::HEALTHLOSS_ASSEX);
	fprintf(cf, "\n");
	fprintf(cf, "DAMAGE_FULLSPIKE= %f \t//health multiplier of spike injury. Note: it is effected by spike length and relative velocity of the agents\n", conf::DAMAGE_FULLSPIKE);
	fprintf(cf, "DAMAGE_COLLIDE= %f \t//how much health is lost upon collision. Note that =0 does not disable the event (see TOOCLOSE above)\n", conf::DAMAGE_COLLIDE);
	fprintf(cf, "DAMAGE_JAWSNAP= %f \t//when jaws snap fully (0->1), this is the damage applied to agents in front\n", conf::DAMAGE_JAWSNAP);
	fprintf(cf, "\n");
	fprintf(cf, "STOMACH_EFF= %f \t\t//the worst possible multiplier produced from having at least two stomach types at 1. =0.1 is harsh. =1 disables (always full efficiency)\n", conf::STOMACH_EFF);
	fprintf(cf, "\n");
	fprintf(cf, "FOODINTAKE= %f \t\t//how much plant food can feed an agent per tick?\n", conf::FOODINTAKE);
	fprintf(cf, "FOODDECAY= %f \t\t//how much does food decay per tick? (negative values make it grow everywhere instead)\n", conf::FOODDECAY);
	fprintf(cf, "FOODGROWTH= %f \t\t//how much does food increase by on a cell with both plant and hazard? (fertilizer effect). =0 disables\n", conf::FOODGROWTH);
	fprintf(cf, "FOODWASTE= %f \t\t//how much food disappears when an agent eats it?\n", conf::FOODWASTE);
	fprintf(cf, "FOODADDFREQ= %i \t\t//how often does a random cell get set to full food randomly? Lower values are more frequent\n", conf::FOODADDFREQ);
	fprintf(cf, "FOODSPREAD= %f \t\t//probability of a plant cell being seeded to a nearby cell. 0.0002= VERY fast food growth\n", conf::FOODSPREAD);
	fprintf(cf, "FOODRANGE= %i \t\t\t//distance that a single cell of food can seed. Units in cells.\n", conf::FOODRANGE);
	fprintf(cf, "\n");
	fprintf(cf, "FRUITINTAKE= %f \t\t//how much fruit can feed an agent per tick?\n", conf::FRUITINTAKE);
	fprintf(cf, "FRUITDECAY= %f \t\t//how much fruit decays per tick? This is applied *2 if less than FRUITREQUIRE plant is in the cell\n", conf::FRUITDECAY);
	fprintf(cf, "FRUITWASTE= %f \t\t//how much fruit disappears when an agent eats it?\n", conf::FRUITWASTE);
	fprintf(cf, "FRUITADDFREQ= %i \t\t//how often does a high-plant-food cell get set to full fruit? Higher values are less frequent (must have at least FRUITREQUIRE plant)\n", conf::FRUITADDFREQ);
	fprintf(cf, "FRUITREQUIRE= %f \t\t//minimum plant food on same cell required for fruit to persist or populate\n", conf::FRUITREQUIRE);
	fprintf(cf, "\n");
	fprintf(cf, "MEATINTAKE= %f \t\t//how much meat can feed an agent per tick?\n", conf::MEATINTAKE);
	fprintf(cf, "MEATDECAY= %f \t\t//how much meat decays on a cell per tick? (negative values make it grow everywhere instead)\n", conf::MEATDECAY);
	fprintf(cf, "MEATWASTE= %f \t\t//how much meat disappears when an agent eats it?\n", conf::MEATWASTE);
	fprintf(cf, "MEATVALUE= %f \t\t//how much meat an agent's body is worth?\n", conf::MEATVALUE);
	fprintf(cf, "\n");
	fprintf(cf, "HAZARDFREQ= %i \t\t\t//how often an instant hazard appears?\n", conf::HAZARDFREQ);
	fprintf(cf, "HAZARDEVENT_MULT= %f \t//multiplier for agents standing in a cell with a hazard event ongoing. multipied after HAZARDDAMAGE\n", conf::HAZARDEVENT_MULT);
	fprintf(cf, "HAZARDDECAY= %f \t\t//how much non-event hazard decays on a cell per tick? (negative values make it grow everywhere instead)\n", conf::HAZARDDECAY);
	fprintf(cf, "HAZARDDEPOSIT= %f \t//how much hazard is placed by an agent per tick? (Ideally. Agents can control the frequency and amount that they deposit, but in sum it should equal this per tick)\n", conf::HAZARDDEPOSIT);
	fprintf(cf, "HAZARDPOWER= %f \t\t//power of the hazard layer value, applied before HAZARDDAMAGE. default= 0.5 (remember, hazard in range [0,1])\n", conf::HAZARDPOWER);
	fprintf(cf, "HAZARDDAMAGE= %f \t\t//how much health an agent looses while on a filled hazard cell per tick? (note that 9/10 of this is max waste damage)\n", conf::HAZARDDAMAGE);
	fprintf(cf, "\n");
	fprintf(cf, "SUN_RED= %f \t\t//???\n", SUN_RED);
	fprintf(cf, "SUN_GRE= %f \t\t//???\n", SUN_GRE);
	fprintf(cf, "SUN_BLU= %f \t\t//???\n", SUN_BLU);
	if(STATallachieved) fprintf(cf, "FUN= 1\n");
	fclose(cf);

	printf(". done!\n");
}