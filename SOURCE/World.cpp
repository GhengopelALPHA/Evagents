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
		SELECTION(-1),
		DROUGHTMULT(1.0),
		MUTEVENTMULT(1),
		pcontrol(false),
		pright(0),
		pleft(0)
{
	//inititalize happens only once when launching program (like right now!)
	init();	
	//reset happens any time we have to clear the world or change variables (now, or when loading a save or starting a new world)
	reset();
	//spawn happens whenever we wish to seed a world with random food, terrain, and bots (now, or after starting a new world)
	spawn();

	printf("WORLD MADE!\n");
}

void World::reset()
{
	current_epoch= 0;
	modcounter= 0;
	idcounter= 0;

	//try loading constants config IF NOT DEBUGGING IN EDITOR!!!
	#if defined(_DEBUG)
	printf("SKIPPING READING CONFIG\n");
	#else
	readConfig();
	#endif

	//tidy up constants
	CW= conf::WIDTH/conf::CZ;
	CH= conf::HEIGHT/conf::CZ; //note: should add custom variables from loaded saves here possibly
	//BUG: this overrides INIT___ values if set in config. We told the user those values override this
	INITFOOD = (int)(INITFOODDENSITY*CW*CH);
	INITFRUIT = (int)(INITFRUITDENSITY*CW*CH);

	if(DEBUG) printf("INITFOOD,INITFRUIT: %i, %i\n", INITFOOD, INITFRUIT);
	if(BRAINSIZE<(Input::INPUT_SIZE+Output::OUTPUT_SIZE)) {
		printf("BRAINSIZE config value was too small.\n It has been reset to min allowed (Inputs+Outputs)\n");
		BRAINSIZE= Input::INPUT_SIZE+Output::OUTPUT_SIZE;
	}

	printf("sanitizing agents.\n");
	agents.clear();

	//handle layers
	for(int cx=0; cx<(int)CW; cx++){
		for(int cy=0; cy<(int)CH; cy++){
			for(int l=0;l<Layer::LAYERS;l++){
				cells[l][cx][cy]= 0;
			}
//			cells[TEMPLAYER][cx][cy]= 2.0*abs((float)cy/CH - 0.5); [old temperature indicating code]
		}
	}

	//open report file; null it up if it exists
	FILE* fr = fopen("report.txt", "w");
	fclose(fr);

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

	addEvent("World Started!",9);
}

void World::spawn()
{
	findStats();//needs to be outside loop to start the loop
	printf("growing food.\n");
	//handle init food layers
	float targetfood= (float)INITFOOD/CW/CH;
	float targetfruit= (float)INITFRUIT/CW/CH;
//	if(DEBUG) 
	printf("targets: %f food, %f fruit\n", targetfood, targetfruit);

	if(targetfood>0.25 || targetfruit>0.25){ //if desired amount of food/fruit is more than 0.25, let's help random seed by throwing all cells
		for(int cx=0; cx<(int)CW; cx++){
			for(int cy=0; cy<(int)CH; cy++){
				if(targetfood>=1) cells[Layer::PLANTS][cx][cy]= 1.0;
				else cells[Layer::PLANTS][cx][cy]= cap(randn(targetfood,1-targetfood));

				if(targetfruit>=1) cells[Layer::FRUITS][cx][cy]= 1.0;
				else cells[Layer::FRUITS][cx][cy]= cap(randn(targetfruit,1-targetfruit));

	//			cells[TEMPLAYER][cx][cy]= 2.0*abs((float)cy/CH - 0.5); [old temperature indicating code]
			}
		}
	}

	while(getFood()<INITFOOD || getFruit()<INITFRUIT) {
		//pollinate plants
		int rx= randi(0,CW);
		int ry= randi(0,CH);
		if(getFood()<INITFOOD) {
			cells[Layer::PLANTS][rx][ry] += randf(0.25, 1.0);
			cells[Layer::PLANTS][rx][ry]= cap(cells[Layer::PLANTS][rx][ry]);
		}
		if (getFruit()<INITFRUIT) {
			//pollinate fruit
			cells[Layer::FRUITS][rx][ry] = 1.0;
		}
		findStats();
	}

	//spawn land masses
	cellsLandMasses();

	//add init agents
	printf("programming agents.\n");
	addAgents(100, -2); //-2 creates both herbivores and frugivores alternately
	findStats(); //without this, selecting "New World" in the GUI would infiniloop program somewhere... I'm too tired to figure out where
}


void World::cellsLandMasses()
{
	//creates land masses for the layer given
	int leftcount= CW*CH;

	printf("clearing land.\n");
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			cells[Layer::ELEVATION][i][j]= -1; //"null" all cells
		}
	}

	if(DISABLE_LAND_SPAWN && OCEANPERCENT<0.333) printf("CAUTION: land spawns are disabled, but high land:water ratio detected...\n");

	for (int i=0;i<1.5*(1-pow(OCEANPERCENT,6))*(CONTINENTS+1);i++) {
		//spawn init continents (mountains= 1, beach= 0.5, between is normal terrain)
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::ELEVATION][cx][cy]= i%2==0 ? 1 : 0.7; //50% of the land spawns are type BUGFIX "Plains", = 0.6
	}
	for (int i=0;i<1.85*(sqrt((float)CW*CH)/1000*pow((float)2.5,3*OCEANPERCENT)*(CONTINENTS+1))-1;i++) {
		//spawn oceans (water= 0)
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::ELEVATION][cx][cy]= 0;
	}

	printf("moving tectonic plates.\n");
	while(leftcount!=0){
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				float height= cells[Layer::ELEVATION][i][j];
				//land spread
				if (height>0.5){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2);//+2 to correct for randi [x,y)
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[Layer::ELEVATION][ox][oy]==-1 && randf(0,1)<0.9) {//90% chance to spread
						if(height<=0.6){ //will not reduce level 0.6 and below, allowing us to create beaches (0.5) later
							cells[Layer::ELEVATION][ox][oy]= height;
						} else cells[Layer::ELEVATION][ox][oy]= randf(0,1)<0.9 ? height : height-0.1;
						//90% chance it remains the same, 10% chance it reduces by 0.1
					}
				}

				//water spread
				else if (height==0){
					int ox= randi(i-1,i+2);
					int oy= randi(j-1,j+2);
					if (ox<0) ox+= CW;
					if (ox>CW-1) ox-= CW;
					if (oy<0) oy+= CH;
					if (oy>CH-1) oy-= CH;
					if (cells[Layer::ELEVATION][ox][oy]==-1 && randf(0,1)<OCEANPERCENT*8) cells[Layer::ELEVATION][ox][oy]= 0;
				}
			}
		}
		
		leftcount= 0;
		for(int i=0;i<CW;i++) {
			for(int j=0;j<CH;j++) {
				if (cells[Layer::ELEVATION][i][j]==-1){
					leftcount++;

				} else if (cells[Layer::ELEVATION][i][j]>0.5){
					//if this is land, check nearby cells. If 2+ are water, turn into beach
					int watercount= 0;
					if(i==0 || j==0 || i==CW-1 || j==CH-1) watercount= 1; //edges get a helping hand (without this there are weird beaches)
					for(int oi=max(0,i-1);oi<min(CW,i+2);oi++) {
						for(int oj=max(0,j-1);oj<min(CH,j+2);oj++) {
							if(cells[Layer::ELEVATION][oi][oj]==0) watercount++;
						}
					}

					if(watercount>=2) cells[Layer::ELEVATION][i][j]=0.5;
				}
			}
		}
	}

	STATlandratio= 0;
	for(int i=0;i<CW;i++) {
		for(int j=0;j<CH;j++) {
			if(cells[Layer::ELEVATION][i][j]>=0.5) STATlandratio++;
		}
	}
	STATlandratio/= CW*CH;
	printf("The percent of world which is land is %.2f.\n",STATlandratio);

	if(STATlandratio>0.75){
		DISABLE_LAND_SPAWN= false;
		printf("Enabled Land Spawns due to high land:water ratio.\n");
	}
}

void World::update()
{
	modcounter++; //mod at init== 1, this is intentional
	vector<int> dt;

	//Process periodic events
	if(DEBUG) printf("P");

	//display tips
	if(!NOTIPS){
		if(modcounter%800==1 && getEpoch()==0 && getDay()<4){ //mod%==1 because it skips 0 by design
			addEvent(tips[min((int)(modcounter/800),randi(0,tips.size()))].c_str(),4);
		} else if(modcounter%FRAMES_PER_DAY==1 && getEpoch()==0){
			addEvent(tips[randi(0,tips.size())].c_str(),4);
		}
	}

	//update achievements, write report, record counts, display population events
	if(!STATuseracted && pcontrol) { //outside of the report if statement b/c it can happen anytime
		addEvent("User, Take the Wheel!", 10);
		STATuseracted= true;}

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
				addEvent("First Species Suriving!", 10);
				STATfirstspecies= true;}
			if(!STATfirstpopulation && getAlive()>ENOUGHBOTS) {
				addEvent("First Large Population!", 10);
				STATfirstpopulation= true;}
			if(!STATfirstglobal && STATbestaquatic>=5 && STATbestterran>=5) {
				addEvent("First Global Population(s)!", 10);
				STATfirstglobal= true;}
			if(!STATstrongspecies && max(STATbestherbi,max(STATbestfrugi,STATbestcarni))>=500) {
				addEvent("First to Generation 500!", 10);
				STATstrongspecies= true;}
			if(!STATstrongspecies2 && max(STATbestherbi,max(STATbestfrugi,STATbestcarni))>=1000) {
				addEvent("First to Generation 1000!", 10);
				STATstrongspecies2= true;}
			if(STATallachieved && FUN==true) FUN= false;
			if(!STATallachieved && STATuseracted && STATfirstspecies && STATfirstpopulation && STATfirstglobal && STATstrongspecies && STATstrongspecies2){
				addEvent("All Achievements Achieved!", 10);
				STATallachieved= true;
				FUN= true;}

			int preeventcount= events.size(); //count of events before checks. Will cause next cycle to skip, reducing repeat event flags

			//generic stats between last report and current
			if(numTotal[ptr]<=numTotal[lastptr]*0.95-20 && current_epoch>0) {
				addEvent("Population Bust!",1);}
			if(current_epoch>0 && numTotal[ptr]==NUMBOTS && STATfirstspecies && numTotal[lastptr]>NUMBOTS*1.1+5) {
				addEvent("Population Crash!",1);}
			if(numTotal[ptr]>=numTotal[lastptr]*1.05+20) {
				//if having a major population boom or bust, check major phenotypes
				if(numHerbivore[ptr]>=numHerbivore[lastptr]*1.05+20 && numHerbivore[ptr]>NUMBOTS) {
					addEvent("Herbivore (H) Bloom",2);}
				if(numCarnivore[ptr]>=numCarnivore[lastptr]*1.05+20 && numCarnivore[ptr]>NUMBOTS) {
					addEvent("Carnivore (C) Bloom",2);}
				if(numFrugivore[ptr]>=numFrugivore[lastptr]*1.05+20 && numFrugivore[ptr]>NUMBOTS) {
					addEvent("Frugivore (F) Bloom",2);}
				if(numAmphibious[ptr]>=numAmphibious[lastptr]*1.05+20 && numAmphibious[ptr]>NUMBOTS && (numTerrestrial[ptr]>NUMBOTS || numAquatic[ptr]>NUMBOTS)) {
					addEvent("Amphibian (A) Bloom",2);}
				if(numTerrestrial[ptr]>=numTerrestrial[lastptr]*1.05+20 && numTerrestrial[ptr]>NUMBOTS && (numAmphibious[ptr]>NUMBOTS || numAquatic[ptr]>NUMBOTS)) {
					addEvent("Terrestrial (T) Bloom",2);}
				if(numAquatic[ptr]>=numAquatic[lastptr]*1.05+20 && numAquatic[ptr]>NUMBOTS && (numAmphibious[ptr]>NUMBOTS || numTerrestrial[ptr]>NUMBOTS)) {
					addEvent("Aquatic (T) Bloom",2);}
			} else {
				//if not having a major population boom or bust, check slower rates of growth/death
				if(ptr>10 && numTotal[ptr]>numTotal[ptr-1] && numTotal[ptr-1]>numTotal[ptr-2] && numTotal[ptr-2]>numTotal[ptr-3]
					&& numTotal[ptr-3]>numTotal[ptr-4] && numTotal[ptr-4]>numTotal[ptr-5] && numTotal[ptr-5]>numTotal[ptr-6]
					&& numTotal[ptr-6]>numTotal[ptr-7] && numTotal[ptr-7]>numTotal[ptr-8] && numTotal[ptr-8]>numTotal[ptr-9]
					&& numTotal[ptr-9]>numTotal[ptr-10]) {
						addEvent("Steady Population Bloom",2);}
				else if(ptr>10 && numTotal[ptr]<numTotal[ptr-1] && numTotal[ptr-1]<numTotal[ptr-2] && numTotal[ptr-2]<numTotal[ptr-3]
					&& numTotal[ptr-3]<numTotal[ptr-4] && numTotal[ptr-4]<numTotal[ptr-5] && numTotal[ptr-5]<numTotal[ptr-6]
					&& numTotal[ptr-6]<numTotal[ptr-7] && numTotal[ptr-7]<numTotal[ptr-8] && numTotal[ptr-8]<numTotal[ptr-9]
					&& numTotal[ptr-9]<numTotal[ptr-10]) {
						addEvent("Steady Population Bust",1);}
			}

			if(events.size()>preeventcount) lastptr= -ptr; //new event? skip next cycle of checks; otherwise, update
			else lastptr= ptr;
		} else lastptr*= -1;
		
		ptr++;
		if(ptr == conf::RECORD_SIZE) ptr = 0;

		writeReport();

		deaths.clear();
	} else if (modcounter%12==0) findStats(); //ocasionally collect stats regardless


	if (modcounter>=FRAMES_PER_EPOCH) {
		modcounter=0;
		current_epoch++;

		//adjust global drought multiplier at end of epoch
		if(DROUGHTS){
			DROUGHTMULT= randn(DROUGHTMULT, 0.08);
			if(DROUGHTMULT>DROUGHT_MAX || DROUGHTMULT<DROUGHT_MIN) DROUGHTMULT= randn(1.0, 0.05);
			if(DROUGHTMULT<1.0-conf::DROUGHT_NOTIFY) addEvent("Global Drought Epoch!",11);
			if(DROUGHTMULT>1.0+conf::DROUGHT_NOTIFY) addEvent("Global Overgrowth Epoch!",2);
			printf("Set Drought Multiplier to %f\n", DROUGHTMULT);
		} else DROUGHTMULT= 1.0;

		//adjust global mutation rate
		if(MUTEVENTS){
			MUTEVENTMULT= randf(0,1)<0.5 ? randi(min(1,MUTEVENT_MAX),max(1,MUTEVENT_MAX+1)) : 1;
			if(MUTEVENTMULT<0) MUTEVENTMULT= 0;
			if(MUTEVENTMULT==0) addEvent("No Mutation chance Epoch!?!",11);
			else if(MUTEVENTMULT==2) addEvent("Double Mutation chance Epoch!",11);
			else if(MUTEVENTMULT==3) addEvent("TRIPPLE Mutation chance Epoch!",11);
			else if(MUTEVENTMULT==4) addEvent("QUADRUPLE Mutation chance Epoch!",11);
			else if(MUTEVENTMULT>4) addEvent("Mutation chance > *4!!!",11);
			printf("Set Mutation Multiplier to %i\n", MUTEVENTMULT);
		}
	}
	if ((modcounter%FOODADDFREQ==0 && !CLOSED) || getFood()<MINFOOD) {
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::PLANTS][cx][cy]= 1.0;
	}
	if (modcounter%HAZARDFREQ==0) {
		int cx=randi(0,CW);
		int cy=randi(0,CH);
		cells[Layer::HAZARDS][cx][cy]= cap((cells[Layer::HAZARDS][cx][cy]/90+0.99));
	}
	if (modcounter%FRUITADDFREQ==0) {
		while (true) {
			int cx=randi(0,CW);
			int cy=randi(0,CH);
			if (cells[Layer::PLANTS][cx][cy]>FRUITREQUIRE) {
				cells[Layer::FRUITS][cx][cy]= 1.0;
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
				if (hazard>0 && plant<1.0) {
					plant+= FOODGROWTH*DROUGHTMULT*hazard; //food grows out of waste/hazard
				}
			}
			if (randf(0,1)<FOODSPREAD*DROUGHTMULT && plant>=0.5) { //changed to plant just to see what happens
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
			if (plant<=FRUITREQUIRE && fruit>0){
				fruit-= FRUITDECAY; //fruit decays, double if lack of plant life
			}
			fruit-= FRUITDECAY;
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
			cells[Layer::LIGHT][cx][cy]= capm(0.5+sin((cx*2*M_PI)/CW-(modcounter+current_epoch*FRAMES_PER_EPOCH)*2*M_PI/FRAMES_PER_DAY), 0, 1);
			//end light
		}
	}
	if(DEBUG) printf("/");

	//give input to every agent. Sets in[] array
	setInputs();

	//brains tick. computes in[] -> out[]
	brainsTick();

	//reset any counter variables per agent and do other stuff before processOutputs and healthTick
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];

		//process indicator (used in drawing, and when negative, used to expire dead agents)
		if(a->indicator>0) a->indicator-= 0.5;
		if(a->indicator<0) a->indicator-= 1;
		if(a->indicator<-conf::CARCASSFRAMES) a->indicator= 0;

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
					if(a->id==SELECTION) addEvent("The Selected Agent Was Mutated!", 6);
					STATlivemutations++;
					if(DEBUG) printf("mutation event\n");
				}
			}

			//Age goes up!
			if (modcounter%(FRAMES_PER_DAY/10)==0) a->age+= 1;

			//update jaw
			if(a->jawPosition>0) {
				a->jawPosition*= -1;
				a->jawrend= 15;
			} else if (a->jawPosition<0) a->jawPosition= min(0.0, a->jawPosition + 0.05*(2 + a->jawPosition));

			//exhaustion gets increased
			a->exhaustion= max((float)0,a->exhaustion + a->getOutputSum() + BASEEXHAUSTION)*EXHAUSTION_MULT;

			//temp discomfort gets re-calculated mod%10, to simulate realistic heat absorption/release
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

				if(SEXTING_DISTANCE>0 && agents[i].sexproject>0.5){
					if(agents[i].sexproject>1.0) continue;; //'fathers' cannot themselves reproduce

					for (int j=0; j<(int)agents.size(); j++) {
						if(i==j || !(agents[j].sexproject>0.5) || agents[j].repcounter>0) continue;
						float d= (agents[i].pos-agents[j].pos).length();
						float deviation= abs(agents[i].species - agents[j].species); //species deviation check
						if (d<=SEXTING_DISTANCE && deviation<=MAXDEVIATION) {
							//this adds agent[i].numbabies to world, with two parents
							if(DEBUG) printf("Attempting to have children...");
							reproduce(i, j); //reproduce resets mother's rep counter, not father's
							if(agents[i].id==SELECTION) addEvent("Selected Agent Sexually Reproduced", 3);
							if(DEBUG) printf(" Success!\n");
							break;
							continue;
						}
					}
				} else {
					//this adds agents[i].numbabies to world, but with just one parent (budding)
					if(DEBUG) printf("Attempting budding...");
					reproduce(i, i);
					if(agents[i].id==SELECTION) addEvent("Selected Agent Assexually Budded", 8);
					if(DEBUG) printf(" Success!\n");
					continue;
				}
			}
			if(FUN && randf(0,1)<0.4){
				if(agents[i].indicator<=0) agents[i].indicator= 60;
				agents[i].ir= randf(0,1);
				agents[i].ig= randf(0,1);
				agents[i].ib= randf(0,1);
			}
		}
	}

	processInteractions();
	//DO NOT add causes of death after this!!!

	for (int i=0; i<(int)agents.size(); i++) {
		//process dead agents. first distribute meat
		if(agents[i].health<0) agents[i].writeIfKilled("Killed by Something ?");
		if (agents[i].health==0 && agents[i].indicator==-1) { 
			if (SELECTION==agents[i].id){
				printf("The Selected Agent was %s!\n", agents[i].death);
				addEvent("The Selected Agent Died!");
			}
		
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
		if (iter->health <=0 && iter->indicator==0) {
			if(iter->id==SELECTION) addEvent("The Selected Agent has decayed", 7);
			iter= agents.erase(iter);
		} else {
			++iter;
		}
	}

	//add new agents, if environment isn't closed
	if (!CLOSED) {
		int alive= agents.size()-getDead();
		//make sure environment is always populated with at least NUMBOTS bots
		if (alive<NUMBOTS) {
			if(DEBUG) printf("Attempting agent conservation program...");
			addAgents(NUMBOTS-alive);
			if(DEBUG) printf(" Success!\n");
		}
		if (alive<ENOUGHBOTS && modcounter%NOTENOUGHFREQ==0 && randf(0,1)<0.5) {
			if(DEBUG) printf("Attempting random spawn...");
			addAgents(1); //every now and then add random bot in if population too low
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

	if(DEBUG) printf("tick\n"); //must occur at/towards end of tick

}

void World::setInputs()
{
	if(DEBUG) printf("I");

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

		//FOOD
//		a->in[#]= cells[Layer::PLANTS][cx][cy]/conf::FOODMAX;

//		a->in[#]= cells[Layer::MEATS][cx][cy]/conf::MEATMAX;

		//SOUND SMELL EYES
		float light= cells[Layer::LIGHT][scx][scy]; //grab light level

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
				water+= cells[Layer::ELEVATION][scx][scy]<0.5 ? 1 : 0; // all water smells the same, only water cells detected
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
						if(getSelection()==a->id){
							int volfact= (int)(a->hear_mod*(1-eardist/DIST)*ovolume*100);
							float finalfact= otone + volfact;
							selectedSounds.push_back(finalfact);
						}

						if(otone>=a->hearhigh[q]) continue;
						if(otone<=a->hearlow[q]) continue;
						if(getSelection()==a->id){
							//modify the selected sound listing if its actually heard so that we can change the visual
							selectedSounds[selectedSounds.size()-1]+= 100;
						}
						float tonemult= cap(min((a->hearhigh[q] - otone)/SOUNDPITCHRANGE,(otone - a->hearlow[q])/SOUNDPITCHRANGE));
						hearsum[q]+= a->hear_mod*(1-eardist/DIST)*ovolume*tonemult;

						if(a->isTiny()) break; //small agents only get one ear input
					}
				}

				float ang= (a2->pos- a->pos).get_angle(); //current angle between bots

				//eyes: bot sight
				//we will skip all eyesight if our agent is in the dark (light==0) without a moonlit sky
				if(light!=0 || MOONLIT){
					for(int q=0;q<NUMEYES;q++){
						float aa = a->angle + a->eyedir[q];
						if (aa<-M_PI) aa += 2*M_PI;
						if (aa>M_PI) aa -= 2*M_PI;
						
						float diff1= aa- ang;
						if (fabs(diff1)>M_PI) diff1= 2*M_PI- fabs(diff1);
						diff1= fabs(diff1);
						
						float fov = a->eyefov[q];
						if (diff1<fov) {
							//we see a2 with this eye. Accumulate stats
							float lightmult= light;
							if(MOONLIT) lightmult= max((float)0.5, light);
							float mul1= lightmult*a->eye_see_agent_mod*(fabs(fov-diff1)/fov)*(1-d/DIST);/*(1-d/DIST)*/;
							r[q] += mul1*a2->real_red;
							g[q] += mul1*a2->real_gre;
							b[q] += mul1*a2->real_blu;
							if(a->id==SELECTION && isDebug()){ //debug sight lines, get coords
								linesA.push_back(a->pos);
								linesB.push_back(a2->pos);
							}
						}

						if(a->isTiny() && q+1>=NUMEYES/2) break; //small agents have half-count of eyes
					}
				}
				
				//blood sensor
				float forwangle= a->angle;
				float diff4= forwangle- ang;
				if (fabs(forwangle)>M_PI) diff4= 2*M_PI- fabs(forwangle);
				diff4= fabs(diff4);
				if (diff4<PI38) {
					float mul4= ((PI38-diff4)/PI38)*(1-d/DIST);
					blood+= a->blood_mod*mul4*(1-agents[j].health/2); //remember: health is in [0,2]
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
	if(DEBUG) printf("/");
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
	if(DEBUG) printf("O");
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		Agent* a= &agents[i];
		float exh= 1.0+a->exhaustion; //exhaustion reduces agent output->physical 

		if (a->health<=0) {
			//dead agents continue to exist, come to a stop, and loose their voice, but skip everything else
			float sp= (a->w1+a->w2)/2;
			a->w1= 0.75*sp;
			a->w2= 0.75*sp;

			a->volume*= 0.2;
			if(a->volume<0.0001) a->volume= 0;

			a->sexproject= 0; //no necrophilia
			continue;
		}
		if (a->jump<=0) { //if not jumping, then change wheel speeds. otherwise, we want to keep wheel speeds constant
			if (pcontrol && a->id==SELECTION) {
				a->w1= pright;
				a->w2= pleft;
			} else {
				a->w1+= a->strength*(a->out[Output::LEFT_WHEEL_F]/exh - a->out[Output::LEFT_WHEEL_B]/exh - a->w1);
				a->w2+= a->strength*(a->out[Output::RIGHT_WHEEL_F]/exh - a->out[Output::RIGHT_WHEEL_B]/exh - a->w2);
			}
		}
		a->real_red+= 0.2*((1-a->chamovid)*a->gene_red + a->chamovid*a->out[Output::RED]-a->real_red);
		a->real_gre+= 0.2*((1-a->chamovid)*a->gene_gre + a->chamovid*a->out[Output::GRE]-a->real_gre);
		a->real_blu+= 0.2*((1-a->chamovid)*a->gene_blu + a->chamovid*a->out[Output::BLU]-a->real_blu);
		// exhaustion does not alter color, but chamovid vs pigmentation is important
		if (a->jump<=0) a->boost= a->out[Output::BOOST]/exh>0.5; //if jump height is zero, boost can change
		a->volume= a->out[Output::VOLUME]/exh;
		a->tone= a->out[Output::TONE]/exh;
		a->give= a->out[Output::GIVE]/exh;
		a->sexproject= a->sexprojectbias + a->out[Output::PROJECT]; //exhaustion does not effect the physical ability of sexual rep; should it?

		//spike length should slowly tend towards spike output
		float g= a->out[Output::SPIKE];
		if (a->spikeLength<g && !a->isTiny()) { //its easy to retract spike, just hard to put it up. Also, Tiny agents can't wield spikes (can still bite)
			a->spikeLength+=SPIKESPEED/exh; //exhaustion does not effect spike length, but rather spike extension speed
			a->health-= HEALTHLOSS_SPIKE_EXT;
			a->writeIfKilled("Killed by Spike Raising");
		} else if (a->spikeLength>g) a->spikeLength= g;

		//grab gets set
		a->grabbing= a->out[Output::GRAB]/exh;
		a->grabangle+= 0.2/exh*(a->out[Output::GRAB_ANGLE]*2*M_PI-a->grabangle); //exhaustion effects angle move speed

		//jump gets set to 2*((jump output) - 0.5) if itself is zero (the bot is on the ground) and if jump output is greater than 0.5
		if(GRAVITYACCEL>0){
			float height= (a->out[Output::JUMP]/exh - 0.5)*2;
			if (a->jump==0 && height>0 && a->age>0) a->jump= height;
		}

		//jaw *snap* mechanic
		// NOTE: NEEDS REWORK SO SIMULATION-ALTERING STEPS ARE NOT TAKEN HERE , LIKE JUMP
		float newjaw= cap(a->out[Output::JAW]/exh-a->jawOldPos);

		if (a->jawPosition==0 && a->age>0 && newjaw>0.01) a->jawPosition= newjaw;
		a->jawOldPos= a->out[Output::JAW];

		//clock 3 gets frequency set by output, but not immediately
		a->clockf3+= 0.3*(95*(1-a->out[Output::CLOCKF3])+5 - a->clockf3); //exhaustion doesn't effect clock freq
		if(a->clockf3>100) a->clockf3= 100;
		if(a->clockf3<2) a->clockf3= 2;
	}

	//move bots if this is not a prefire.
	if(!prefire){
		#pragma omp parallel for schedule(dynamic)
		for (int i=0; i<(int)agents.size(); i++) {
			Agent* a= &agents[i];

			a->jump-= GRAVITYACCEL;
			if(a->jump<-1) a->jump= 0; //-1 because we will be nice and give a "recharge" time between jumps

			float basewheel= sqrt(a->radius/MEANRADIUS);
			if (a->encumbered) basewheel*= 0.3;
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
	if(DEBUG) printf("/");
}

void World::processInteractions()
{
	//process interaction with cells
	if(DEBUG) printf("C");
	#pragma omp parallel for
	for (int i=0; i<(int)agents.size(); i++) {
		if(agents[i].health<=0) continue;
		Agent* a= &agents[i];

		int scx= (int) a->pos.x/conf::CZ;
		int scy= (int) a->pos.y/conf::CZ;

		if (a->jump<=0){ //no interaction with these cells if jumping
			float intake= 0;
			float speedmult= pow(1-max(abs(a->w1), abs(a->w2)),3) / (1.0+a->exhaustion);
			//exhaustion reduces agent physical actions including all intake, applied to speed for code efficiency

			float invmult= 1-STOMACH_EFF;
			float invplant=1-a->stomach[Stomach::PLANT]*invmult;
			float invmeat= 1-a->stomach[Stomach::MEAT]*invmult;
			float invfruit=1-a->stomach[Stomach::FRUIT]*invmult;

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
			}

			//meat food
			float meat= cells[Layer::MEATS][scx][scy];
			if (meat>0) { //agent eats meat
				float meattake= min(meat,MEATWASTE*a->stomach[Stomach::MEAT]*invplant*invfruit*speedmult);
				cells[Layer::MEATS][scx][scy]-= meattake;
				intake+= MEATINTAKE*meattake/MEATWASTE;
			}

			//Fruit food
			float fruit= cells[Layer::FRUITS][scx][scy];
			if (fruit>0) { //agent eats fruit
				float fruittake= min(fruit,FRUITWASTE*a->stomach[Stomach::FRUIT]*invmeat*invplant*speedmult);
				cells[Layer::FRUITS][scx][scy]-= fruittake;
				intake+= FRUITINTAKE*fruittake/FRUITWASTE;
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
				a->health -= HAZARDDAMAGE*agemult*pow(hazard, HAZARDPOWER);
				a->writeIfKilled("Killed by a Hazard");
			}

			//waste deposit
			int freqwaste= (int)ceil(0.000001 + 49*a->out[Output::WASTE_RATE]);
			if (modcounter%freqwaste==0){
				//agents can control the rate of deposit, [1,50] frames
				//agents fill up hazard cells only up to 9/10, because any greater is a special event
				if(hazard<0.9) hazard+= HAZARDDEPOSIT*freqwaste/50*a->health; // /50 b/c of freqwaste in range [1,50]
				//this whole function applied with frequency makes a rate of "0.5" most efficent
				cells[Layer::HAZARDS][scx][scy]= capm(hazard, 0, 0.9);
			}
		}

		//land/water (always interacted with)
		float land= cells[Layer::ELEVATION][scx][scy];
		if(a->jump>0) land= max(land, 0.5f); //jumping while underwater allows one to breathe at lung value 0.5
		float dd= pow(land - a->lungs,2);
		if (dd>0.01){ //a difference of 0.1 or less between lung and land type lets us skip taking damage
			a->health -= HEALTHLOSS_BADTERRAIN*dd;
			a->writeIfKilled("Killed by Suffocation .");
			if(a->health>0 && dd>=0.333) a->encumbered= true; 
		}

		if (a->health>2){ //if health has been increased over the cap, limit
			if(OVERHEAL_REPFILL!=0) a->repcounter -= (a->health-2)*OVERHEAL_REPFILL; //if enabled, allow overflow to first fill the repcounter
			a->health= 2;
		}
	}
	if(DEBUG) printf("/");

	//process agent-agent dynamics
	if(DEBUG) printf("A");
	if (modcounter%2==0) { //we dont need to do this TOO often. can save efficiency here since this is n^2 op in #agents

		//first, we'll determine for all agents if they are near enough to another agent to warrent processing them
		float highestdist= max(max(max(FOOD_SHARING_DISTANCE, SEXTING_DISTANCE), GRABBING_DISTANCE), SPIKELENGTH+MEANRADIUS*3);
		for (int i=0; i<(int)agents.size(); i++) {
			agents[i].near= false;
			agents[i].dhealth= 0; //better place for this now, since modcounter%2
			if (agents[i].health<=0) continue; //skip dead agents

			for (int j=0; j<i; j++) {
				if (agents[i].pos.x<agents[j].pos.x-highestdist
					|| agents[i].pos.x>agents[j].pos.x+highestdist
					|| agents[i].pos.y>agents[j].pos.y+highestdist
					|| agents[i].pos.y<agents[j].pos.y-highestdist) continue;
				else if (agents[j].health<=0) continue;
				else {
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
				if (agents[j].health<=0){
					if(a->grabbing>0.5 && a->grabID==agents[j].id) a->grabID= -1; //help catch grabbed agents deaths and let the grabber know
					if(agents[j].health==0) continue; //health == because we want to weed out bots who died already via other causes
				}

				Agent* a2= &agents[j];

				float d= (a->pos-a2->pos).length();
				float sumrad= a->radius+a2->radius;

				//---HEALTH GIVING---//
				if (FOOD_SHARING_DISTANCE>0 && a->give>conf::MAXSELFISH && FOODTRANSFER>0 && a2->health<2) {
					//all non-selfish agents allow health trading
					float rd= a->give>0.5 ? FOOD_SHARING_DISTANCE : a->radius+a2->radius;
					//rd is the max range allowed to agent j. If generous, range allowed, otherwise bots must touch

					if (d<=rd) {
						//initiate transfer
						float healthrate= a->give>0.5 ? FOODTRANSFER*a->give : FOODTRANSFER*2*a->give;
						if(d<=a->radius+a2->radius && a->give>0.5) healthrate= FOODTRANSFER;
						//healthrate goes from 0->1 for give [0,0.5], and from 0.5->1 for give (0.5,1]. Is maxxed when touching and generous
						a2->health += healthrate;
						a->health -= healthrate;
						a2->dhealth += healthrate; //only for drawing
						a->dhealth -= healthrate;
						a->writeIfKilled("Killed by Excessive Generosity");
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
							if(a->age<TENDERAGE) aagemult= a->age/TENDERAGE*3/4+0.25;
							if(a2->age<TENDERAGE) a2agemult= a2->age/TENDERAGE*3/4+0.25;
							float DMG= ov*DAMAGE_COLLIDE;

							a->health-= DMG*sumrad/2/a->radius*aagemult; //larger, younger bots take less damage, bounce less
							a2->health-= DMG*sumrad/2/a2->radius*a2agemult;

							const char * fix= "Killed by a Collision";
							a->writeIfKilled(fix);
							a2->writeIfKilled(fix);// fix, because these two collisions are not being redd the same

							if(a->health==0){
								break;
								continue;
							}
							if(a2->health==0){
								a->killed++;
								continue;
							}

							//only trigger event if another event not being displayed
							if(a->indicator<=0) a->initSplash(15,0,0.5,1);
							if(a2->indicator<=0) a2->initSplash(15,0,0.5,1);
							
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
				const char * fix2= "Killed by a Murder";
				//low speed doesn't count, nor does a small spike. If the target is jumping in midair, can't attack either
				if(a->isSpikey(SPIKELENGTH) && a->w1>0.1 && a->w2>0.1 && d<=(sumrad + SPIKELENGTH*a->spikeLength) && a2->jump<=0){

					//these two are in collision and agent i has extended spike and is going decent fast!
					Vector2f v(1,0);
					v.rotate(a->angle);
					float diff= v.angle_between(a2->pos-a->pos);
					if (fabs(diff)<M_PI/8) {
						//bot i is also properly aligned!!! that's a hit
						float DMG= DAMAGE_FULLSPIKE*a->spikeLength*max(a->w1,a->w2);

						a2->health-= DMG*(1-1.0/(1+expf(-(a2->radius/2-MEANRADIUS)))); //tiny, small, & average agents take full damage. 
						//large (2*MEANRADIUS) agents take half damage, and huge agents (2*MEANRADIUS+10) take no damage from spikes at all
						a2->writeIfKilled(fix2);
						a2->freshkill= FRESHKILLTIME; //this agent was hit this turn, giving full meat value

						a->spikeLength= 0; //retract spike back down

						a->initSplash(20*DMG,1,0.5,0); //orange splash means bot has spiked the other bot. nice!
						if (a2->health==0){ 
							//red splash means bot has killed the other bot. Murderer!
							a->initSplash(20*DMG,1,0,0);
							if(a->id==SELECTION) addEvent("Selected Agent Killed Another!",1);
							a->killed++;
							continue;
						} else if(a->id==SELECTION) addEvent("Selected Agent Stabbed Another!",12);

						a->hits++;

						/*Vector2f v2(1,0);
						v2.rotate(a2->angle);
						float adiff= v.angle_between(v2);
						if (fabs(adiff)<M_PI/2) {
							//this was attack from the back. Retract spike of the other agent (startle!)
							//this is done so that the other agent cant right away "by accident" attack this agent
							a2->spikeLength= 0;
						}*/
					}
				} //end spike mechanics

				//---JAWS---//
				if(a->jawPosition>0 && d<=(sumrad+12.0)) { //only bots that are almost touching may chomp
					Vector2f v(1,0);
					v.rotate(a->angle);
					float diff= v.angle_between(a2->pos-a->pos);
					if (fabs(diff)<M_PI/6) { //wide AOE
						float DMG= DAMAGE_JAWSNAP*a->jawPosition;

						a2->health-= DMG;
						a2->writeIfKilled(fix2);
						a2->freshkill= FRESHKILLTIME;
						a->hits++;

						a->initSplash(20*a->jawPosition,1,1,0); //yellow splash means bot has chomped the other bot. ouch!
						if (a2->health==0){ 
							//red splash means bot has killed the other bot. Murderer!
							a->initSplash(40*DMG,1,0,0);
							if(a->id==SELECTION) addEvent("Selected Agent Killed Another!",1);
							a->killed++;
							if(a->grabID==a2->id) a->grabID= -1;
							continue;
						} else if(a->id==SELECTION) addEvent("Selected Agent Bit Another!",12);
					}
				} //end jaw mechanics

				//---GRABBING---//
				//doing this last because agent deaths may occur up till now
				if(GRAB_PRESSURE!=0 && GRABBING_DISTANCE>0 && a->grabbing>0.5 && a2->health>0) {
					if(d<=GRABBING_DISTANCE+a->radius){
						//check init grab
						if(a->grabID==-1){
							Vector2f v(1,0);
							v.rotate(a->angle+a->grabangle);
							float diff= v.angle_between(a2->pos-a->pos);
							if (fabs(diff)<M_PI/4 && randf(0,1)<0.4) { //very wide AOE centered on a's grabangle, 40% chance any one bot is picked
								a->grabID= a2->id;
							}

						} else if (a->grabID==a2->id && d>(sumrad+1.0)) {
							//we have a grab target, and it is this agent. Pull us together!
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

							//"grab" (hehe) the coords of the other agent
							//a->grabx= a2->pos.x; a->graby= a2->pos.y;
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
	if(DEBUG) printf("/");
}

void World::healthTick()
{
	if(DEBUG) printf("H");
	#pragma omp parallel for schedule(dynamic)
	for (int i=0; i<(int)agents.size(); i++) {
		if (agents[i].health>0){
			//"natural" causes of death: age, wheel activity, metabolism, excesive brain activity
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

			//baseloss reduces health
			agents[i].health -= baseloss;
			agents[i].writeIfKilled("Killed by Natural Causes"); //this method should be called after every reduction of health
			if (agents[i].health==0) continue; //agent died, we must move on.

			//remove health from lack of "air"
			//straight up penalty to all bots for large population
			agents[i].health-= HEALTHLOSS_NOOXYGEN*agents.size()/TOOMANYBOTS;
			agents[i].writeIfKilled("Killed by LackOf Oxygen");
			if (agents[i].health==0) continue; //agent died, we must move on!

			//apply temperature discomfort (every tick)
			agents[i].health -= HEALTHLOSS_BADTEMP*agents[i].discomfort; //subtract from health
			agents[i].writeIfKilled("Killed by Temp Discomfort");
		}
	}
	if(DEBUG) printf("/");
}

void World::setSelection(int type) {
	int maxi= -1;
	//to find the desired selection, we must process all agents. Lets do this sparingly
	if (modcounter%5==0){
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
		} else if(type==Select::PRODUCTIVE){
			float maxprog= 0;
			//Productivity: find the agent who's been most successful at having children
			for (int i=0; i<(int)agents.size(); i++) {
				if(agents[i].health>0 && agents[i].age!=0){
					if (((float)agents[i].children/(float)agents[i].age)>maxprog){
						//note: productivity needs =/age b/c otherwise the oldest will almost always be the most productive
						maxprog= (float)agents[i].children/(float)agents[i].age;
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
					} else {
						setSelectedAgent(randi(0,agents.size()));
						addEvent("No More Living Relatives!", 13);
					}
				}
			}
		} //else if (type==Select::EXTREMOPHILE){
			//float 

	}
			
	if (maxi!=-1 && agents[maxi].id!= SELECTION) {
		setSelectedAgent(maxi);
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

int World::getClosestRelative(int idx) const {
	//retrieve the index of the given agent's closest living relative. Takes age, gencount, and species id into account, returns -1 if none
	int nidx= -1;
	int meta, metamax= MAXDEVIATION;

	if(idx>=0){
		for (int i=0; i<(int)agents.size(); i++) {
			if(idx==i) continue;

			if(abs(agents[idx].species-agents[i].species)<MAXDEVIATION && agents[i].health>0){
				//choose an alive agent that is within maxdeviation; closer the better
				meta= 1+MAXDEVIATION-abs(agents[idx].species-agents[i].species);
			} else meta= 0;

			if(agents[idx].age==agents[i].age) meta+= 8; //choose siblings
			if(agents[i].age==0) meta+= 5; //choose newborns
			if(agents[idx].gencount+1==agents[i].gencount) meta+= 5; //choose children or 2nd cousins
			else if(agents[idx].gencount==agents[i].gencount) meta+= 2; //at least choose cousins
			
			//else meta-= (int)(abs(agents[idx].gencount-agents[i].gencount)/2); //avoid aunts, uncles, nephews, nieces several times removed
			
			if(meta>metamax){
				nidx= i;
				metamax= meta;
			}
		}
	}
	return nidx;
}

int World::getSelection() const {
	//retrieve world->SELECTION
	return SELECTION;
}

void World::selectedHeal() {
	//heal selected agent
	int sidx= getSelectedAgent();
	if(sidx>=0){
		agents[sidx].health= 2.0;
	}
}


void World::selectedKill() {
	//kill (delete) selected agent
	int sidx= getSelectedAgent();
	if(sidx>=0){
		agents[sidx].health= -0.01;
		agents[sidx].writeIfKilled("Killed by God (you)");
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
		if(agents[sidx].id==SELECTION) addEvent("The Selected Agent Was Mutated!", 6);
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
	 
void World::draw(View* view, int layer)
{
	view->drawData();

	//draw cell layer
	if(layer!=0) {
		for(int x=0;x<CW;x++) {
			for(int y=0;y<CH;y++) {
				float values[Layer::LAYERS];

				for(int l=0;l<Layer::LAYERS; l++) {
					values[l]= cells[l][x][y];
				}

				view->drawCell(x,y,values);
			}
		}
	}
	
	//draw all agents
	vector<Agent>::const_iterator it;
	for ( it = agents.begin(); it != agents.end(); ++it) {
		view->drawAgent(*it,it->pos.x,it->pos.y);
	}

	view->drawStatic();
}

void World::addAgents(int num, int set_stomach, float nx, float ny, bool set_lungs)
{
	for (int i=0;i<num;i++) {
		Agent a(BRAINSIZE, MEANRADIUS, REP_PER_BABY, MUTCHANCE, MUTSIZE);

		if (set_stomach==Stomach::PLANT) a.setHerbivore(); //if told to predetermine stomach type
		else if (set_stomach==Stomach::MEAT) a.setCarnivore();
		else if (set_stomach==Stomach::FRUIT) a.setFrugivore();
		else if (set_stomach==-2) i%2==0 ? a.setHerbivore() : a.setFrugivore();
		else if (getEpoch()==0 && a.isCarnivore()) randf(0,1)<0.5 ? a.setHerbivore() : a.setFrugivore(); //epoch 0 has no carnivores forced

		if(DISABLE_LAND_SPAWN){
			int scx,scy;
			while(true){
				scx= (int) a.pos.x/conf::CZ;
				scy= (int) a.pos.y/conf::CZ;
				if(cells[Layer::ELEVATION][scx][scy]==0) break;
				a.setPosRandom(conf::WIDTH, conf::HEIGHT);
			}
		}

		if (set_lungs){ //if told to fix lungs for agent's position
			int scx= (int) a.pos.x/conf::CZ;
			int scy= (int) a.pos.y/conf::CZ;
			a.lungs= cap(randn((float)cells[Layer::ELEVATION][scx][scy],0.3));
		}

		if (nx!=-1 || ny!=-1){ //if custom location given
			a.pos.x= nx;
			a.pos.y= ny;
		}

		a.id= idcounter;
		idcounter++;
		agents.push_back(a);

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

		agents[i1].initSplash(30,0,0.8,0); //green splash means agent asexually reproduced

	}else{ //otherwise, it's sexual
		agents[i1].initSplash(30,0,0,0.8);
		agents[i2].initSplash(30,0,0,0.8); //blue splashes mean agents sexually reproduced.
	}

	float newhealth= agents[i1].health/(agents[i1].numbabies + 1) - mother.repcounter/mother.maxrepcounter;
	//repcounter should be negative or zero here, so its actually giving more health for the overflow

	agents[i1].health= newhealth - healthloss;
	agents[i1].writeIfKilled("Killed by Child Birth");

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

		daughter.id= idcounter;
		idcounter++;
		agents.push_back(daughter);
		}
	}
}

void World::writeReport()
{
	printf("Writing Report, Epoch: %i, Day: %i\n", getEpoch(), getDay());
	//save all kinds of nice data stuff
	int randgen= 0;
	int randspec= 0;
	int randseed= 0;
	float randradius= 0;
	float randmetab= 0;
	float randmutrate1= 0;
	float randmutrate2= 0;
	int randchildren= 0;

	randgen= agents[randi(0,agents.size())].gencount;
	randspec= agents[randi(0,agents.size())].species;
	int randagent= randi(0,agents.size());
	for (int i= 0; i<(int)agents[randagent].brain.boxes.size(); i++){
		if (agents[randagent].brain.boxes[i].seed>randseed) randseed=agents[randagent].brain.boxes[i].seed;
	}
	randradius= agents[randi(0,agents.size())].radius;
	randmetab= agents[randi(0,agents.size())].metabolism;
	randmutrate1= agents[randi(0,agents.size())].MUTRATE1;
	randmutrate2= agents[randi(0,agents.size())].MUTRATE2;
	randchildren= agents[randi(0,agents.size())].children;

/*	for(int s=0;s<(int)species.size();s++){
		if(members[s]>=3) happyspecies++; //3 agents with the same species id counts as a species
	}*/

	//death cause reporting
	std::vector<const char *> deathlog;
	std::vector<int> deathcounts;
	for (int d=0; d<(int)deaths.size(); d++) {
		bool added= false;
		for (int e=0; e<(int)deathlog.size(); e++) {
			if (added) continue;
			if (deaths[d]==deathlog[e]) {
				deathcounts[e]++;
				added= true;
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
		getHerbivores(), getFrugivores(), getCarnivores(), getLungLand(), getLungAmph(), getLungWater(), getHybrids(), getSpiked());
	//print world stats: cell counts
	fprintf(fr, "#0.75Plant:\t%i\t#0.5Meat:\t%i\t#0.5Hazard:\t%i\t#0.5Fruit:\t%i\t",
		getFood(), getMeat(), getHazards(), getFruit());
	//print random selections: Genome, brain seeds, [[[generation]]]
	fprintf(fr, "RGenome:\t%i\tRSeed:\t%i\tRGen:\t%i\tRRadius:\t%f\tRMetab:\t%f\tRChildren:\t%i\t",
		randspec, randseed, randgen, randradius, randmetab, randchildren);
	//print generations: Top Gen counts
	fprintf(fr, "TopHGen:\t%i\tTopFGen:\t%i\tTopCGen:\t%i\tTopLGen:\t%i\tTopAGen:\t%i\tTopWGen:\t%i\t",
		STATbestherbi, STATbestfrugi, STATbestcarni, STATbestterran, STATbestamphibious, STATbestaquatic);
	//print mutations and deaths: Number and Causes
	fprintf(fr, "#Mutations:\t%i\t#Deaths:\t%i\tDEATHLOG:\t",
		getMutations(),deaths.size());
	for (int e=0; e<(int)deathlog.size(); e++) {
		fprintf(fr, "%s:\t%i\t", deathlog[e], deathcounts[e]);
	}
	fprintf(fr, "\n");
	fclose(fr);
}

void World::setClosed(bool close)
{
	CLOSED = close;
}

bool World::isClosed() const
{
	return CLOSED;
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

int World::getSpiked() const
{
	return STATspiked;
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

int World::getFood() const //return plant cells with 75% max or more
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
	return STATallplant/FOODWASTE*FOODINTAKE/2/100; //#food * 1 tick/FOODWASTE food * FOODINTAKE health/1 tick * agent/2 health = #agents
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
	return STATallhazard*HAZARDDAMAGE/2*50; //BUG? #hazard * HAZARDDAMAGE health / hazard / 1 tick * agent/2 health != #agents killed per tick
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
	STATspiked= 0;
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

			if (agents[i].isSpikey(SPIKELENGTH)) STATspiked++;

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
			if(cells[Layer::PLANTS][i][j]>0.5) STATplants++;
			if(cells[Layer::FRUITS][i][j]>0.5) STATfruits++;
			if(cells[Layer::MEATS][i][j]>0.5) STATmeats++;
			if(cells[Layer::HAZARDS][i][j]>0.5) STAThazards++;
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
	//type 0: white		- Selected agent event
	//type 1: red		- Selected agent killed/global bad
	//type 2: green		- global good
	//type 3: blue		- Selected sexual reproduction
	//type 4: yellow	- tips
	//type 5: cyan		- simulation notification and controls
	//type 6: purple	- Selected mutation event
	//type 7: brown		- Selected decayed
	//type 8: neon		- Selected assexual reproduction
	//type 9: mint		- simulation start/restart
	//type 10: multicolor -achievements
	//type 11: pink		- global change
	//type 12: orange	- Selected attacked
	//type 13+: black	- 
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
	NOTIPS= false;
	DISABLE_LAND_SPAWN= conf::DISABLE_LAND_SPAWN;
	MOONLIT= conf::MOONLIT;
	DROUGHTS= conf::DROUGHTS;
	DROUGHT_MIN= conf::DROUGHT_MIN;
	DROUGHT_MAX= conf::DROUGHT_MAX;
	MUTEVENTS= conf::MUTEVENTS;
	MUTEVENT_MAX= conf::MUTEVENT_MAX;

    MINFOOD= conf::MINFOOD;
    INITFOODDENSITY= conf::INITFOODDENSITY;
    INITFRUITDENSITY= conf::INITFRUITDENSITY;
    NUMBOTS= conf::NUMBOTS;
    ENOUGHBOTS= conf::ENOUGHBOTS;
	NOTENOUGHFREQ= conf::NOTENOUGHFREQ;
    TOOMANYBOTS= conf::TOOMANYBOTS;

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
	MUTCHANCE= conf::MUTCHANCE;
	MUTSIZE= conf::MUTSIZE;
	LIVE_MUTATE_CHANCE= conf::LIVE_MUTATE_CHANCE;
    MAXAGE= conf::MAXAGE;

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
    HAZARDDECAY= conf::HAZARDDECAY;
    HAZARDDEPOSIT= conf::HAZARDDEPOSIT;
    HAZARDDAMAGE= conf::HAZARDDAMAGE;
	HAZARDPOWER= conf::HAZARDPOWER;

	//tip popups. typically the first one is guarenteed to display the moment the sim starts, then each one after has a decreasing
	//chance of being seen second, third, etc. The first 16 or so are very likely to be seen at least once durring epoch 0
	#if defined(_DEBUG)
	printf("DEBUG ENVIRONMENT DETECTED, SKIPPING TIPS, SETTING MOONLIT=false\n");
	MOONLIT= false;
	NOTIPS= true;
	#endif

	if(!NOTIPS){
		tips.push_back("Left-click an agent to select it");
		tips.push_back("Try 'f' to follow selected agent");
		tips.push_back("Try 'q' to center map and graphs");
		tips.push_back("Press 'h' for detailed interface help");
		tips.push_back("Press 'h' for detailed interface help");
		tips.push_back("Zoom in with '>' to see more details");
		tips.push_back("Dead agents are semi-transparent");
		tips.push_back("View other layers with 'k' & 'l'");
		tips.push_back("Overgrowth: more plant growth");
		tips.push_back("Drought: less plant growth");
		tips.push_back("Press 'h' for detailed interface help");
		tips.push_back("Try 'm' to activate fast mode");
		tips.push_back("Epoch 0 is boring. Press 'm'!");
		tips.push_back("Patience, Epoch 0 species are rare");
		tips.push_back("Press 'm' to simulate at max speed");
		tips.push_back("Change settings in the .cfg file");
		tips.push_back("Reports periodically saved to file");
		tips.push_back("Agents display splashes for actions");
		tips.push_back("Yellow spash = agent bit another");
		tips.push_back("Orange spash= agent stabbed other");
		tips.push_back("Red spash = agent killed another");
		tips.push_back("Green spash= asexually reproduced");
		tips.push_back("Blue spash = sexually reproduced");
		tips.push_back("Purple spash = mutation occured");
		tips.push_back("The settings.cfg is FUN, isnt it?!");
		tips.push_back("Contribute on Github!");
		tips.push_back("Press 'h' for interface help");
	}
	
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
					printf("Exit now to save your custom settings, otherwise hit enter to overwrite. . .");
					cin.get();
					writeConfig();
					readConfig();
					break;
				}
						}else if(strcmp(var, "DISABLE_LAND_SPAWN=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)DISABLE_LAND_SPAWN) printf("DISABLE_LAND_SPAWN, ");
				if(i==1) DISABLE_LAND_SPAWN= true;
				else DISABLE_LAND_SPAWN= false;
			}else if(strcmp(var, "NOTIPS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)NOTIPS) printf("NOTIPS, ");
				if(i==1) NOTIPS= true;
				else NOTIPS= false;
			}else if(strcmp(var, "MOONLIT=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=(int)MOONLIT) printf("MOONLIT, ");
				if(i==1) MOONLIT= true;
				else MOONLIT= false;
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
			}else if(strcmp(var, "MINFOOD=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=MINFOOD) printf("MINFOOD, ");
				MINFOOD= i;
			}else if(strcmp(var, "INITFOODDENSITY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=INITFOODDENSITY) printf("INITFOODDENSITY, ");
				INITFOODDENSITY= f;
			}else if(strcmp(var, "INITFOOD=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=INITFOOD) printf("INITFOOD, ");
				INITFOOD= i;
			}else if(strcmp(var, "INITFRUITDENSITY=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=INITFRUITDENSITY) printf("INITFRUITDENSITY, ");
				INITFRUITDENSITY= f;
			}else if(strcmp(var, "INITFRUIT=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=INITFRUIT) printf("INITFRUIT, ");
				INITFRUIT= i;
			}else if(strcmp(var, "NUMBOTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=NUMBOTS) printf("NUMBOTS, ");
				NUMBOTS= i;
			}else if(strcmp(var, "ENOUGHBOTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=ENOUGHBOTS) printf("ENOUGHBOTS, ");
				ENOUGHBOTS= i;
			}else if(strcmp(var, "NOTENOUGHFREQ=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=NOTENOUGHFREQ) printf("NOTENOUGHFREQ, ");
				NOTENOUGHFREQ= i;
			}else if(strcmp(var, "TOOMANYBOTS=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=TOOMANYBOTS) printf("TOOMANYBOTS, ");
				TOOMANYBOTS= i;
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
			}else if(strcmp(var, "MUTCHANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MUTCHANCE) printf("MUTCHANCE, ");
				MUTCHANCE= f;
			}else if(strcmp(var, "MUTSIZE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=MUTSIZE) printf("MUTSIZE, ");
				MUTSIZE= f;
			}else if(strcmp(var, "LIVE_MUTATE_CHANCE=")==0){
				sscanf(dataval, "%f", &f);
				if(f!=LIVE_MUTATE_CHANCE) printf("LIVE_MUTATE_CHANCE, ");
				LIVE_MUTATE_CHANCE= f;
			}else if(strcmp(var, "MAXAGE=")==0){
				sscanf(dataval, "%i", &i);
				if(i!=MAXAGE) printf("MAXAGE, ");
				MAXAGE= i;
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
		printf("settings.cfg did not exist! Its ok, I gave you a new one to tinker with\n");
		writeConfig();
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
	fprintf(cf, "V= %f \t\t\t//Version number, internal, if different than program's, will ask user to overwrite or exit\n", conf::VERSION);
	fprintf(cf, "\n");
	fprintf(cf, "NOTIPS= %i \t\t\t//if true, prevents tips from being displayed. This value is whatever was set in program when this file was saved\n", NOTIPS);
	fprintf(cf, "DISABLE_LAND_SPAWN= %i \t\t//true-false flag for disabling agents from spawning on land. 0= land spawn allowed, 1= not allowed. Is GUI-controllable, so this is only default value\n", conf::DISABLE_LAND_SPAWN);
	fprintf(cf, "MOONLIT= %i \t\t\t//true-false flag for letting agents see other agents at night. 0= no eyesight, 1= see agents at half light. Is GUI-controllable and saved/loaded, so this is only default value\n", conf::MOONLIT);
	fprintf(cf, "DROUGHTS= %i \t\t\t//true-false flag for if the drought/overgrowth mechanic is enabled. 0= constant normal growth, 1= variable. Is GUI-controllable and saved/loaded, so this is only default value\n", conf::DROUGHTS);
	fprintf(cf, "DROUGHT_MIN= %f \t\t//minimum multiplier value of droughts, below which it gets thrown back to near 1.0\n", conf::DROUGHT_MIN);
	fprintf(cf, "DROUGHT_MAX= %f \t\t//maximum multiplier value of droughts (overgrowth), above which it gets thrown back near 1.0\n", conf::DROUGHT_MAX);
	fprintf(cf, "MUTEVENTS= %i \t\t\t//true-false flag for if the mutation event mechanic is enabled. 0= normal mutation rates, 1= variable. Is GUI-controllable and saved/loaded, so this is only default value\n", conf::MUTEVENTS);
	fprintf(cf, "MUTEVENT_MAX= %i \t\t//integer max possible multiplier for mutation event mechanic. =1 effectively disables. =0 enables 50%% chance no live mutations per epoch. Negative values increase this chance (-1 => 66%%, -2 => 75%%, etc).\n", conf::MUTEVENT_MAX);
	fprintf(cf, "\n");
	fprintf(cf, "MINFOOD= %i \t\t\t//Minimum number of food cells which must have food during simulation. 0= off\n", conf::MINFOOD);
	fprintf(cf, "INITFOODDENSITY= %f \t//initial density of full food cells. Use 'INITFOOD= #' to set a number\n", conf::INITFOODDENSITY);
	fprintf(cf, "//INITFOOD= 0 \t\t\t//remove '//' from the flag to enable, overrides INITFOODDENSITY\n");
	fprintf(cf, "INITFRUITDENSITY= %f \t//initial density of full fruit cells\n", conf::INITFRUITDENSITY);
	fprintf(cf, "//INITFRUIT= 0\n");
	fprintf(cf, "NUMBOTS= %i \t\t\t//minimum number of agents\n", conf::NUMBOTS);
	fprintf(cf, "ENOUGHBOTS= %i \t\t//if more agents than this number, random spawns no longer occur\n", conf::ENOUGHBOTS);
	fprintf(cf, "NOTENOUGHFREQ= %i \t\t//how often does a random agent get spawned while below ENOUGHAGENTS? Higher values are less frequent\n", conf::NOTENOUGHFREQ);
	fprintf(cf, "TOOMANYBOTS= %i \t\t//number of agents at which the full HEALTHLOSS_NOOXYGEN is applied\n", conf::TOOMANYBOTS);
	fprintf(cf, "\n");
	fprintf(cf, "REPORTS_PER_EPOCH= %i \t\t//number of times to record data per epoch. 0 for off. (David Coleman)\n", conf::REPORTS_PER_EPOCH);
	fprintf(cf, "FRAMES_PER_EPOCH= %i \t//number of frames before epoch is incremented by 1.\n", conf::FRAMES_PER_EPOCH);
	fprintf(cf, "FRAMES_PER_DAY= %i \t\t//number of frames it takes for the daylight cycle to go completely around the map\n", conf::FRAMES_PER_DAY);
	fprintf(cf, "\n");
	fprintf(cf, "CONTINENTS= %i \t\t\t//number of 'continents' generated on the land layer. Not guaranteed to be accurate\n", conf::CONTINENTS);
	fprintf(cf, "OCEANPERCENT= %f \t\t//decimal ratio of terrain layer which will be ocean. Approximately\n", conf::OCEANPERCENT);
	fprintf(cf, "GRAVITYACCEL= %f \t\t//how fast an agent will 'fall' after jumping. 0= jump disabled, 0.1+ = super-gravity\n", conf::GRAVITYACCEL);
	fprintf(cf, "BUMP_PRESSURE= %f \t//the restoring force between two colliding agents. 0= no reaction (disables TOOCLOSE and all collisions). I'd avoid negative values if I were you...\n", conf::BUMP_PRESSURE);
	fprintf(cf, "GRAB_PRESSURE= %f \t//the restoring force between and agent and its grab target. 0= no reaction (disables grab function), negative values push agents away\n", conf::GRAB_PRESSURE);
	fprintf(cf, "BRAINSIZE= %i \t\t\t//number boxes in every agent brain. Will not make brains smaller than Inputs + Outputs. Saved per world, will override for loaded worlds\n", conf::BRAINSIZE);
	fprintf(cf, "\n");
	fprintf(cf, "BOTSPEED= %f \t\t//fastest possible speed of agents. This effects so much of the sim I dont advise changing it\n", conf::BOTSPEED);
	fprintf(cf, "BOOSTSIZEMULT= %f \t//how much speed boost do agents get when boost is active?\n", conf::BOOSTSIZEMULT);
	fprintf(cf, "SOUNDPITCHRANGE= %f \t//range below hearhigh and above hearlow within which external sounds fade in. Would not recommend extreme values near or beyond [0,0.5]\n", conf::SOUNDPITCHRANGE);
	fprintf(cf, "FOODTRANSFER= %f \t\t//how much health is transferred between two agents trading food per tick? =0 disables all generosity\n", conf::FOODTRANSFER);
	fprintf(cf, "BASEEXHAUSTION= %f \t//base value of exhaustion. When negative, is essentially the sum amount of output allowed before healthloss. Would not recommend >=0 values\n", conf::BASEEXHAUSTION);
	fprintf(cf, "EXHAUSTION_MULT= %f \t//multiplier applied to outputsum + BASEEXHAUSTION\n", conf::EXHAUSTION_MULT);
	fprintf(cf, "MEANRADIUS= %f \t\t//\"average\" agent radius, range [0.2*,2.2*) (only applies to random agents, no limits on mutations). Average agents take full damage from spikes, large (2*MEANRADIUS) agents take half damage, and huge agents (2*MEANRADIUS+10) take no damage from spikes at all. Radius also effects asexual reproduction health penalty via *(radius/MEANRADIUS). would not recommend setting <=0\n", conf::MEANRADIUS);
	fprintf(cf, "SPIKESPEED= %f \t\t//how quickly can the spike be extended? Does not apply to retraction, which is instant\n", conf::SPIKESPEED);
	fprintf(cf, "FRESHKILLTIME= %i \t\t//number of ticks after a spike, collision, or bite that an agent will still drop full meat\n", conf::FRESHKILLTIME);
	fprintf(cf, "TENDERAGE= %i \t\t\t//age (in 1/10ths) of agents where full meat, hazard, and collision damage is finally given. These multipliers are reduced via *age/TENDERAGE. 0= off\n", conf::TENDERAGE);
	fprintf(cf, "MINMOMHEALTH= %f \t\t//minimum amount of health required for an agent to have a child\n", conf::MINMOMHEALTH);
	fprintf(cf, "MIN_INTAKE_HEALTH_RATIO= %f //minimum metabolism ratio of intake sent to health. 0= no restrictions, 1= 100% health, no babies. default= 0.5\n", conf::MIN_INTAKE_HEALTH_RATIO);
	if(STATallachieved) fprintf(cf, "FUN= 1\n");
	fprintf(cf, "REP_PER_BABY= %f \t\t//amount of food required to be consumed for an agent to reproduce, per baby\n", conf::REP_PER_BABY);
	fprintf(cf, "OVERHEAL_REPFILL= %i \t\t//true-false flag for letting agents redirect overfill health (>2) to repcounter. 1= conserves matter, 0= balanced most efficient metabolism\n", conf::OVERHEAL_REPFILL);
//	fprintf(cf,	"LEARNRATE= %f\n", conf::LEARNRATE);
	fprintf(cf, "MAXDEVIATION= %f \t//maximum difference in species ID a crossover reproducing agent will be willing to tolerate\n", conf::MAXDEVIATION);
	fprintf(cf, "MUTCHANCE= %f \t\t//the chance of mutations occurring (note that various mutations modify this value up or down)\n", conf::MUTCHANCE);
	fprintf(cf, "MUTSIZE= %f \t\t//the magnitude of mutations (note that various mutations modify this value up or down)\n", conf::MUTSIZE);
	fprintf(cf, "LIVE_MUTATE_CHANCE= %f \t//chance, per tick, that a given agent will be mutated alive. Not typically harmful. Can be increased by mutation events if enabled.\n", conf::LIVE_MUTATE_CHANCE);
	fprintf(cf, "MAXAGE= %i \t\t\t//Age at which the full HEALTHLOSS_AGING amount is applied to an agent\n", conf::MAXAGE);
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
	fprintf(cf, "HEALTHLOSS_NOOXYGEN= %f \t//how much agents are penalized when total agents = TOOMANYBOTS\n", conf::HEALTHLOSS_NOOXYGEN);
	fprintf(cf, "HEALTHLOSS_ASSEX= %f \t//multiplier for radius/MEANRADIUS penalty on mother for asexual reproduction\n", conf::HEALTHLOSS_ASSEX);
	fprintf(cf, "\n");
	fprintf(cf, "DAMAGE_FULLSPIKE= %f \t//health multiplier of spike injury. Note: it is effected by spike length and wheel speed\n", conf::DAMAGE_FULLSPIKE);
	fprintf(cf, "DAMAGE_COLLIDE= %f \t//how much health is lost upon collision. Note that =0 does not disable the event (see TOOCLOSE)\n", conf::DAMAGE_COLLIDE);
	fprintf(cf, "DAMAGE_JAWSNAP= %f \t//when jaws snap fully (0->1), this is the damage applied to agents in front\n", conf::DAMAGE_JAWSNAP);
	fprintf(cf, "\n");
	fprintf(cf, "STOMACH_EFF= %f \t\t//the worst possible multiplier produced from having at least two stomach types at 1. =0.1 is harsh. =1 disables (full efficiency)\n", conf::STOMACH_EFF);
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
	fprintf(cf, "HAZARDDECAY= %f \t\t//how much non-event hazard decays on a cell per tick? (negative values make it grow everywhere instead)\n", conf::HAZARDDECAY);
	fprintf(cf, "HAZARDDEPOSIT= %f \t//how much hazard is placed by an agent per tick? (Ideally. Agents can control the frequency and amount that they deposit, but in sum it should equal this per tick)\n", conf::HAZARDDEPOSIT);
	fprintf(cf, "HAZARDPOWER= %f \t\t//power of the hazard layer value, applied before HAZARDDAMAGE. default= 0.5 (remember, hazard in range [0,1])\n", conf::HAZARDPOWER);
	fprintf(cf, "HAZARDDAMAGE= %f \t\t//how much health an agent looses while on a filled hazard cell per tick? (note that 9/10 of this is max waste damage)\n", conf::HAZARDDAMAGE);
	fclose(cf);

	printf(". done!\n");
}