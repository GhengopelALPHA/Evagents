#pragma once

#define NUMEYES 6
#define NUMEARS 2
#define CONNS 5
#define STACKS 3

#ifndef SETTINGS_H
#define SETTINGS_H


//defines for layer data. Making changes here changes cells[LAYERS][][] in World. Order does not matter (except in ReadWrite...)
namespace Layer{
enum {
	//NOTE these are physical layers of data, as opposed to Display::, which handles drawing
	ELEVATION= 0,
	PLANTS,
	FRUITS,
	MEATS,
	HAZARDS,	
	LIGHT,

	//Don't add beyond this entry!
	LAYERS
};};

//defines for layer display. Changing order here changes cycle order and menu listing order
namespace Display{
enum {
	NONE= 0,
	REALITY,
	ELEVATION,
	PLANTS,
	MEATS,
	FRUITS,
	HAZARDS,	
	TEMP,
	LIGHT,

	//Don't add beyond this entry!
	DISPLAYS
};};

//defines for agent visualization. Changing order here changes cycle order and menu-listing order
namespace Visual{
enum {
	NONE= 0,
	RGB,
	STOMACH,
	LUNGS,
	HEALTH,
	DISCOMFORT,
	VOLUME,
	SPECIES,
	CROSSABLE,
	METABOLISM,

	//Don't add beyond this entry!
	VISUALS
};};

//defines for selected agent visualization. Changing order here changes menu-listing order
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

//defines for selected agent heads up display ordering. Changing order here changes arrangement order
//keep in mind, it is displayed in 3 columns, so the 4th entry will be in the same column as the 1st,
//5th with the 2nd, etc
namespace Hud{
enum {
	HEALTH= 0,
	REPCOUNTER,
	EXHAUSTION,
	AGE,
	GENERATION,
	STOMACH,
	METBOLISM,
	NUMBABIES,
	SIZE,
	STRENGTH,
	HYBRID,
	LUNGS,
	MOTION,
	SEXPROJECT,
	TEMPPREF,
	SPIKE,
	GIVING,
	MUTRATE1,
	BITE,
	GRAB,
	MUTRATE2,
	WASTE,
	VOLUME,
	CHAMOVID,
	STAT_KILLED,
	STAT_CHILDREN,
	SPECIESID,
	blank,
	DEATH,

	//Don't add beyond this entry!
	HUDS
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
	//EXTREMOPHILE,

	//Don't add beyond this entry!
	SELECT_TYPES
};};

//defines for extremophile code. Changing order here changes the seeked extremophile rotation
//UNUSED CURRENTLY
namespace Extremo {
enum {
	DEEPEST= 0,
	HIGHEST,
	HERBIVORE,
	FRUGIVORE,
	CARNIVORE,
	COLDEST,
	WARMEST,
	SMALLEST,
	BIGGEST,
	MUTABLE,

	//Don't add beyond this entry!
	EXTREME_TYPES
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
	GRAB_ANGLE,
	PROJECT,
	STIMULANT,
	WASTE_RATE,
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

//defines for genes. every agent can have any number of genes, even 0 (typically defaults the trait to 1.0)
//each gene also adds to the reproduction cost
//GOAL is to make it so that each of these represents a "node" or organ, and maybe allow for node growth (actualy physical body parts)
namespace Genetypes {
enum {
	ADD_NODE= 0, //UNUSED
	MULT_MAXHEALTH, //UNUSED
	ADD_MAXAGE, //UNUSED
	MULT_MUTCHANCE, //old MUTRATE1
	MULT_MUTRATE, //old MUTRATE2
	MULT_RADIUS,
	MULT_RED,
	MULT_GREEN,
	MULT_BLUE,
	MULT_CHAMOVID,
	MULT_WHEELSTRENGTH,
	ADD_BABY,
	ADD_THERMAL, //UNUSED
	MULT_THERMAL,
	MULT_LUNG,
	MULT_METABOLISM,
	MULT_STOMACH_H,
	MULT_STOMACH_F,
	MULT_STOMACH_M,
	ADD_MAXREPCOUNTER,
	ADD_SEXPROJECT,
	MULT_EYE_SEE_AGENT,
	ADD_EYE, //UNUSED
	MULT_EAR_HEAR_AGENT,
	ADD_EAR, //UNUSED
	MULT_CLOCK1,
	MULT_CLOCK2,
	MULT_CLOCK3,
	MULT_SENSE_BLOOD,
	MULT_SMELL_AGENTS,
	
	
	//don't add beyond this entry!
	GENE_TYPES
};};

namespace conf {
	
	//DEFAULTS: All of what follows are defaults, and if settings.cfg exists, are subsituted with that file's values if VERSION #'s match
	//SIM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ SIM
	const int WIDTH = 10000;  //width and height of simulation
	const int HEIGHT = 8000;
	const int WWIDTH = 1100;  //initial window width and height
	const int WHEIGHT = 700;

	const float VERSION= 0.05; //current program settings version. ++0.01 IF CHANGES MADE AND PROGRAM UPDATED

	const float SNAP_SPEED = 0.2; //how fast snapping to an object of interest is; 1 is instant, 0.1 is smooth, 0 is pointless
	const float ZOOM_SPEED = 0.002; //how fast zoom actions change the magnification
	const int EVENTS_DISP= 8; //how many events to display at once, at max. Will not ignore or miss events that exceed this; they'll wait
	const int EVENTS_HALFLIFE= 350; //half-life (exact) for display of event messages.
	const float DEADTRANSPARENCY= 0.5; //the alpha value of dead agents

	const int REPORTS_PER_EPOCH = 200; //(.cfg)
	const int FRAMES_PER_EPOCH = 200000; //(.cfg)
	const int FRAMES_PER_DAY= 4000; //(.cfg)
	const int RECORD_SIZE = 200; // number of data points stored for the graph. Units are in reports, the frequency of which are defined above

	//WORLD ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ WORLD
	const int CZ= 50; //cell size in pixels, for food squares. Should divide well into Width, Height
	const int MINFOOD= 2000; //(.cfg)
	const float INITFOODDENSITY= 0.4; //(.cfg)
	const float INITFRUITDENSITY= 0.3; //(.cfg)

	const int NUMBOTS= 30; //(.cfg)
	const int ENOUGHBOTS= 400; //(.cfg)
	const int NOTENOUGHFREQ= 75; //(.cfg)
	const int TOOMANYBOTS= 2500; //(.cfg)

	const int CONTINENTS= 2; //(.cfg)
	const float OCEANPERCENT= 0.70; //(.cfg)
	const bool DISABLE_LAND_SPAWN= true; //(.cfg & GUI)
	const bool MOONLIT= true; //(.cfg & GUI)
	const bool DROUGHTS= true; //(.cfg & GUI)
	const float DROUGHT_NOTIFY= 0.15; //1.0 +/- this value of drought sends an event notification
	const float DROUGHT_MIN= 0.7; //(.cfg)
	const float DROUGHT_MAX= 1.3; //(.cfg)
	const bool MUTEVENTS= true; //(.cfg & GUI)
	const int MUTEVENT_MAX= 3; //(.cfg)
	const float SOUNDPITCHRANGE= 0.1; //(.cfg)
	const float OVERHEAL_REPFILL= 0; //(.cfg)

	//BOTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BOTS
	const float GRAVITYACCEL= 0.01; //(.cfg)
	const float BUMP_PRESSURE= 0.13; //(.cfg)
	const float GRAB_PRESSURE= 0.05; //(.cfg)
	const float BOTSPEED= 1.5; //(.cfg)
	const float BOOSTSIZEMULT=3; //(.cfg)
	const int CARCASSFRAMES= 3000; //number of frames before dead agents are removed

	const float FOODTRANSFER= 0.08; //(.cfg)
	const float MAXSELFISH= 0.01; //Give value below which an agent is considered selfish
	const float BASEEXHAUSTION= -5; //(.cfg)
	const float EXHAUSTION_MULT= 0.5; //(.cfg)
	const float MEANRADIUS=10.0; //(.cfg)
	const float SPIKESPEED= 0.01; //(.cfg)
	const int FRESHKILLTIME= 50; //(.cfg)
	const int TENDERAGE= 10; //(.cfg)
	const float MINMOMHEALTH=0.25; //(.cfg)
	const float MIN_INTAKE_HEALTH_RATIO= 0.5; //(.cfg)
	const float REP_PER_BABY= 4; //(.cfg)
	const float REPCOUNTER_MIN= 15; //minimum value the Repcounter may be set to
	const float MAXDEVIATION=10; //(.cfg)
	const float METAMUTRATE1= 0.0003; //what is the change in MUTRATE1 and 2 on reproduction? lol
	const float METAMUTRATE2= 0.00002;
	const float MUTCHANCE= 0.15; //(.cfg)
	const float MUTSIZE= 0.01; //(.cfg)
	const float LIVE_MUTATE_CHANCE= 0.0001; //(.cfg)
	const int MAXAGE=10000; //(.cfg)

	//distances
	const float DIST= 400; //(.cfg)
	const float SPIKELENGTH=30; //(.cfg)
	const float TOOCLOSE=12; //(.cfg)
	const float FOOD_SHARING_DISTANCE= 60; //(.cfg)
	const float SEXTING_DISTANCE= 60; //(.cfg)
	const float GRABBING_DISTANCE= 40; //(.cfg)

	//Health losses
	const float HEALTHLOSS_WHEELS = 0.0; //(.cfg)
	const float HEALTHLOSS_BOOSTMULT=4; //(.cfg)
	const float HEALTHLOSS_BADTEMP = 0.0046; //(.cfg)
	const float HEALTHLOSS_AGING = 0.0001; //(.cfg)
	const float HEALTHLOSS_BRAINUSE= 0.0; //(.cfg)
	const float HEALTHLOSS_SPIKE_EXT= 0.0; //(.cfg)
	const float HEALTHLOSS_BADTERRAIN= 0.022; //(.cfg)
	const float HEALTHLOSS_NOOXYGEN= 0.0002; //(.cfg)
	const float HEALTHLOSS_ASSEX= 0.4; //(.cfg)

	const float DAMAGE_FULLSPIKE= 4.0; //(.cfg)
	const float DAMAGE_COLLIDE= 3.0; //(.cfg)
	const float DAMAGE_JAWSNAP= 3.0; //(.cfg)

	//brain settings
	const int BRAINSIZE= 160; //(.cfg)
	const float LEARNRATE= 0.001; // CHANGE TO LEARN FROM USER INPUT
	const bool ENABLE_LEARNING= true; //(.cfg & GUI)
	const float BRAIN_DIRECTINPUTS= 0.1; //probability of random brain conns on average which will connect directly to inputs
	const float BRAIN_DEADCONNS= 0.35; //probability of random brain conns which are "dead" (that is, weight = 0)
	const float BRAIN_CHANGECONNS= 0.05; //probablility of random brain conns which are change sensitive
	const float BRAIN_MEMCONNS= 0.01; //probablility of random brain conns which are memory type
	const float BRAIN_TRACESTRENGTH= 0.1; //when performing a traceback, what minimum absolute weight of connections will count for tracing
//	const float BRAIN_MIRRORCONNS= 0.05; //
//	const float BRAIN_ANDCONNS= 0.2; //probability of random brain conns that multiply in instead of add.

	//LAYERS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LAYERS
	const float STOMACH_EFF= 0.25; //(.cfg)

	const float FOODINTAKE= 0.0091; //(.cfg)
	const float FOODDECAY = 0.000004; //(.cfg)
	const float FOODGROWTH= 0.000004; //(.cfg)
	const float FOODWASTE= 0.0023;//0.0007; //(.cfg)
	const int FOODADDFREQ= 250; //(.cfg)
	const float FOODSPREAD= 0.00015; //(.cfg)
	const int FOODRANGE= 2; //(.cfg)
	//Plant food is the simplest and most plentiful form of nutrition, but it takes time to consume enough

	const float FRUITINTAKE = 0.020; //(.cfg)
	const float FRUITDECAY = 0.000003; //(.cfg)
	const float FRUITWASTE = 0.0023; //0.0014; //(.cfg)
	const int FRUITADDFREQ = 4; //(.cfg)
	const float FRUITREQUIRE= 0.1; //(.cfg)
	//Fruit is a quick and easy alternative to plants. Also partially randomly populated, harkening back to ScriptBots origins

	const float MEATINTAKE= 0.09; //(.cfg)
	const float MEATDECAY= 0.0001; //(.cfg)
	const float MEATWASTE= 0.0023; //0.002; //(.cfg)
	const float MEATVALUE= 1.0; //(.cfg)
	//Meat comes from dead bots, and is the fastest form of nutrition, IF bots can learn to find it before it decays

	const int HAZARDFREQ= 20; //(.cfg)
	const float HAZARDDECAY= 0.000002; //(.cfg)
	const float HAZARDDEPOSIT= 0.0035; //(.cfg)
	const float HAZARDDAMAGE= 0.003;//0.0025; //(.cfg)
	const float HAZARDPOWER= 0.5; //(.cfg)
	}

#endif
