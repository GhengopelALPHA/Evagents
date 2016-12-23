#pragma once

#define NUMEYES 6
#define NUMEARS 2
#define CONNS 5

#ifndef SETTINGS_H
#define SETTINGS_H


//defines for layer code. Changing order here changes cycle order and menu-listing order
namespace Layer{
enum {
	//NOTE that GLVeiw's use of these must be incremented by 1 because 0 is used for disable draw
	LAND= 0,
	PLANTS,
	FRUITS,
	MEATS,
	HAZARDS,	
	TEMP,
	LIGHT,

	//Don't add beyond this entry!
	LAYERS
};};

//defines for agent visualization. Changing order here changes cycle order and menu-listing order
namespace Visual{
enum {
	NONE= 0,
	RGB,
	STOMACH,
	CROSSABLE,
	HEALTH,
	SPECIES,
	DISCOMFORT,
	VOLUME,
	METABOLISM,
	LUNGS,

	//Don't add beyond this entry!
	VISUALS
};};

//defines for selected agent visualization. Changing order here changes cycle order and menu-listing order
namespace Profile{
enum {
	NONE= 0,
	INOUT,
	BRAIN,
	SOUND,
	EYES,

	//Don't add beyond this entry!
	PROFILES
};};

//defines for selection code. Changing order here changes menu-listing order
namespace Select {
enum {
	NONE= 0,
	MANUAL,
	OLDEST,
	BEST_GEN,
	HEALTHY,
	PRODUCTIVE,
	AGGRESSIVE,
	RELATIVE,

	//Don't add beyond this entry!
	SELECT_TYPES
};};

//defines for brain input code. Changing order here changes input-to-brain order and visualization order
namespace Input {
enum {
	EYES,
	xEYES= EYES+NUMEYES*3-1, //I strongly recommend keeping EYES and xEYES together in this order
	HEALTH,
	RANDOM,
	CLOCK1,
	CLOCK2,
	CLOCK3,
	HEARING1,
	HEARING2,
	BOT_SMELL,
	BLOOD,
	TEMP,
	PLAYER,
	FRUIT_SMELL,
	MEAT_SMELL,
	HAZARD_SMELL,
	WATER_SMELL,
	
	//Don't add beyond this entry!
	INPUT_SIZE
};};

//defines for brain output code. Changing order here changes brain-to-output order (reversed) and visualization order
namespace Output {
enum {
	//LEFT RIGHT BOOST JUMP R G B VOLUME GIVING SPIKE CHOICE STIMULANT
	// 0	 1	   2    3   4 5 6   7	   8	  9	    10		11
	LEFT_WHEEL_F= 0,
	RIGHT_WHEEL_F,
	LEFT_WHEEL_B,
	RIGHT_WHEEL_B,
	BOOST,
	JUMP,
	RED,
	GRE,
	BLU,
	VOLUME,
	TONE,
	GIVE,
	SPIKE,
	JAW,
	GRAB,
	PROJECT,
	STIMULANT,
	CLOCKF3,

	//don't add beyond this entry!
	OUTPUT_SIZE
};};

namespace Stomach {
enum {
	PLANT= 0,
	MEAT,
	FRUIT,

	//don't add beyond this entry!
	FOOD_TYPES
};};

namespace conf {
	
	//DEFAULTS: All of what follows are defaults, and if settings.cfg exists, are subsituted with that file's values
	//SIM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ SIM
	const int WIDTH = 10000;  //width and height of simulation
	const int HEIGHT = 8000;
	const int WWIDTH = 1100;  //initial window width and height
	const int WHEIGHT = 700;

	const float VERSION= 0.01; //current program version. IF ANY OF THESE VALUES ARE CHANGED, THIS SHOULD BE CHANGED AS WELL! ++0.01

	const float SNAP_SPEED = 0.2; //how fast snapping to an object of interest is; 1 is instant, 0.1 is smooth, 0 is pointless
	const float ZOOM_SPEED = 0.002; //how fast zoom actions change the magnification
	const int EVENTS_DISP= 8; //how many events to display at once, at max. Will not ignore or miss events that exceed this; they'll wait
	const int EVENTS_HALFLIFE= 300; //half-life (exact) for display of event messages.

	const int CZ= 50; //cell size in pixels, for food squares. Should divide well into Width, Height
	const int MINFOOD= 2000; //(.cfg)
	const float INITFOODDENSITY= 0.00010; //(.cfg)
	const float INITFRUITDENSITY= 0.00006; //(.cfg)
	const int NUMBOTS= 30; //(.cfg)
	const int ENOUGHBOTS= 500; //(.cfg)
	const int TOOMANYBOTS= 1800; //(.cfg)

	const int REPORTS_PER_EPOCH = 10; //(.cfg)
	const int FRAMES_PER_EPOCH = 10000; //(.cfg)
	const int FRAMES_PER_DAY= 2500; //(.cfg)
	const int RECORD_SIZE = 200; // number of data points stored for the graph. Units are in reports, the frequency of which are defined above

	const int CONTINENTS= 2; //(.cfg)
	const float OCEANPERCENT= 0.65; //(.cfg)
	const bool MOONLIT= true; //(.cfg)
	const float GRAVITYACCEL= 0.01; //(.cfg)
	const float REACTPOWER= 0.13; //(.cfg)
	const float SPIKEMULT= 3; //(.cfg)

	//BOTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BOTS
	const int BRAINSIZE= 150; //(.cfg)
	const float BOTSPEED= 0.3; //(.cfg)
	const float BOOSTSIZEMULT=2; //(.cfg)
	const float SOUNDPITCHRANGE= 0.1; //(.cfg)
	const float FOODTRANSFER= 0.03; //(.cfg)
	const float MEANRADIUS=10.0; //(.cfg)
	const float SPIKESPEED= 0.01; //(.cfg)
	const int FRESHKILLTIME= 10; //(.cfg)
	const int TENDERAGE= 10; //(.cfg)
	const float MINMOMHEALTH=0.25; //(.cfg)
	const float REPRATE=25; //(.cfg)
	const bool OVERHEAL_REPFILL= false; //(.cfg)
	const float LEARNRATE= 0.001; // 0.02 (high-gen feedback) //how quickly a conn weight can change from use
	const float MAXDEVIATION=10; //(.cfg)
	const float METAMUTRATE1= 0.0003; //what is the change in MUTRATE1 and 2 on reproduction? lol
	const float METAMUTRATE2= 0.00002;
	const float MUTCHANCE= 0.14; //(.cfg)
	const float MUTSIZE= 0.009; //(.cfg)
	const int MAXAGE=10000; //(.cfg)
	const int CARCASSFRAMES= 1500; //number of frames before dead agents are removed

	//distances
	const float DIST= 400; //(.cfg)
	const float SPIKELENGTH=30; //(.cfg)
	const float TOOCLOSE=14; //(.cfg)
	const float FOOD_SHARING_DISTANCE= 60; //(.cfg)
	const float SEXTING_DISTANCE= 60; //(.cfg)
	const float GRABBING_DISTANCE= 40; //(.cfg)

	const float HEALTHLOSS_WHEELS = 0.00005; //(.cfg)
	const float HEALTHLOSS_BOOSTMULT=4; //(.cfg)
	const float HEALTHLOSS_BADTEMP = 0.004; //(.cfg)
	const float HEALTHLOSS_AGING = 0.0; //(.cfg)
	const float HEALTHLOSS_BRAINUSE= 0.0; //(.cfg)
	const float HEALTHLOSS_BUMP= 0.005; //(.cfg)
	const float HEALTHLOSS_SPIKE_EXT= 0.0; //(.cfg)
	const float HEALTHLOSS_BADAIR= 0.01; //(.cfg)
	const float HEALTHLOSS_NOOXYGEN= 0.0003; //(.cfg)
	const float HEALTHLOSS_ASSEX= 0.4; //(.cfg)
	const float HEALTHLOSS_JAWSNAP= 1.5; //(.cfg)

	const float BRAIN_DIRECTINPUT= 0.1; //probability of random brain conns on average which will connect directly to inputs
	const float BRAIN_DEADCONNS= 0.3; //probability of random brain conns which are "dead" (that is, weight = 0)
	const float BRAIN_CHANGECONNS= 0.15; //probablility of random brain conns which are change sensitive
	const float BRAIN_MEMCONN= 0.01; //probablility of random brain conns which are memory type

	//LAYERS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LAYERS
	const float STOMACH_EFF= 0.3; //(.cfg)

	const float FOODINTAKE= 0.009; //(.cfg)
	const float FOODDECAY = 0.000004; //(.cfg)
	const float FOODGROWTH= 0.000004; //(.cfg)
	const float FOODWASTE= 0.0006; //(.cfg)
	const float FOODMAX= 0.5; //how much food per cell can there be at max?
	const int FOODADDFREQ= 250; //(.cfg)
	const float FOODSPREAD= 0.00015; //(.cfg)
	const int FOODRANGE= 2; //(.cfg)
	//Plant food is the simplest and most plentiful form of nutrition, but it takes a long time to consume enough

	const float FRUITINTAKE = 0.02; //(.cfg)
	const float FRUITDECAY = 0.00001; //(.cfg)
	const float FRUITWASTE = 0.0013; //(.cfg)
	const float FRUITMAX = 0.5;
	const int FRUITADDFREQ = 3; //(.cfg)
	const float FRUITREQUIRE= 0.5; //(.cfg)
	//Fruit is a quick and easy alternative to plants. Also randomly populated, harkening back to ScriptBots origins

	const float MEATINTAKE= 0.05; //(.cfg)
	const float MEATDECAY= 0.000001; //(.cfg)
	const float MEATWASTE= 0.003; //(.cfg)
	const float MEATMAX= 0.5; //how much meat per cell can there be at max?
	const float MEATVALUE= 1; //(.cfg)
	//Meat comes from dead bots, and is the fastest form of nutrition, IF bots can learn to find it before it decays...

	const int HAZARDFREQ= 20; //(.cfg)
	const float HAZARDDECAY= 0.000002; //(.cfg)
	const float HAZARDDEPOSIT= 0.0012; //(.cfg)
	const float HAZARDDAMAGE= 0.008; //(.cfg)
	const float HAZARDMAX= 0.5; //how much hazard per cell can there be at max? more than 9/10 of this qualifies as an instant hazard
	}

#endif
