#pragma once

#include <string>

#define NUMEYES 6
#define NUMEARS 2
#define CONNS 5
#define STACKS 3

#ifndef SETTINGS_H
#define SETTINGS_H

//defines for GUI button handles. Order changes nothing
namespace GUIButtons{
enum {
	TOGGLE_LAYERS= 0,
	TOGGLE_PLACEAGENTS
};};

//defines for reading/writing GUI handles. Order changes nothing
namespace RWOpen{
enum {
	STARTLOAD= 0,
	NEWWORLD,
	IGNOREOVERLOAD,
	UNSAVEDALERT,
	BASICLOAD,
	BASICSAVE,
	BASICSAVEAGENT,
	BASICLOADAGENT,
	ADVANCEDLOADAGENT,
	ADVANCEDLOAD,

	//Don't add beyond this entry!
	HANDLES
};};

//defines for reading/writing GUI handles. Order changes nothing
namespace RWClose{
enum {
	BASICLOAD= 0,
	BASICSAVE,
	NEWWORLD,
	CANCELLOAD,
	CANCELLOADALERT,
	CANCELSAVE,
	CANCELALERT,
	BASICSAVEOVERWRITE,
	BASICSAVEAGENT,
	BASICLOADAGENT,
	ADVANCEDLOADAGENT,
	ADVANCEDLOAD,

	//Don't add beyond this entry!
	HANDLES
};};

//defines for tile (buttons) data. Order changes nothing except my sanity
namespace MainTiles{
enum {
	SAD= 0, //Selected Agent Display, our first tile! It has lots of sub-tiles that get made automatically
	TOOLBOX,

	//Don't add beyond this entry!
	TILES
};};

//defines for ui dimensions and useful values throughout the program
namespace UID{
	const int CHARHEIGHT= 9;
	const int CHARWIDTH= 7; //height and width (on screen) allowed for text characters
	const int HUDSWIDTH= 88; //the width we are assuming when it comes to text space in the Selected Agent Display.
	const int TILEMARGIN= 10; //what is the min margin between a sub-tile and the next tile above (or window if main tile)
	const int TILEBEVEL= 5; //bevel size on tiles. 5 was scaled for the SAD; consider code for smaller tiles to have smaller bevels
	const int SADWIDTH= 440; //Selected Agent Display width. SHOULD be tied to HUDSWIDTH above, but there's a lot more stuff going on in there
	const int BUFFER= 6; //extra buffer space, mostly for text
	const int EVENTSWIDTH= 210; //Event toast width (length?)... x-dimension-size
};


namespace EventColor{
enum {
	WHITE= 0,	//type 0: white		- Selected agent event
	RED,		//type 1: red		- Selected agent killed / population bad
	GREEN,		//type 2: green		- population good
	BLUE,		//type 3: blue		- Selected sexual reproduction
	YELLOW,		//type 4: yellow	- tips / autoselect changed poorly
	CYAN,		//type 5: cyan		- simulation notification and controls
	PURPLE,		//type 6: purple	- Selected mutation event
	BROWN,		//type 7: brown		- Selected decayed
	NEON,		//type 8: neon		- Selected assexual reproduction
	MINT,		//type 9: mint		- simulation start/restart/change
	MULTICOLOR,	//type 10: multicolor -achievements
	PINK,		//type 11: pink		- global change
	ORANGE,		//type 12: orange	- Selected attacked
	BLOOD,		//type 13: bloodred - selected bad
	LAVENDER,	//type 14: lavender - global change (mutations)
	LIME,		//type 15: lime		- global good
	BLACK		//type 16+: black	- 
};};

//defines for mouse modes. Order changes nothing. NOT USED CURRENTLY
namespace MouseMode{
enum {
	NONE= 0,
	POPUPS, //show popups only
	SELECT, //select agent only
	POPUPS_SELECT, //DEFAULT select agents and show popups
	PLACE, //place agents only
	POPUPS_PLACE, //show popups and place agents

	//Don't add beyond this entry!
	MODES
};};

//defines for layer data. Making changes here changes cells[LAYERS][][] in World. Order does not matter (except in ReadWrite...)
namespace Layer{
enum {
	//NOTE these are physical layers of data, as opposed to DisplayMode::, which handles drawing
	ELEVATION= 0,
	PLANTS,
	FRUITS,
	MEATS,
	HAZARDS,	
	LIGHT,

	//Don't add beyond this entry!
	LAYERS
};};

//defines for terrain elevation levels
namespace Elevation{
	const float DEEPWATER_LOW= 0;
	const float SHALLOWWATER= 0.2;
	const float BEACH_MID= 0.5;
	const float PLAINS= 0.6;
	const float HILL= 0.7;
	const float STEPPE= 0.8;
	const float HIGHLAND= 0.9;
	const float MOUNTAIN_HIGH= 1.0; //Don't add beyond this entry!
};


//defines for layer display, but this is a list of toggle-ables. Changing order here changes cycle order and menu listing order.
namespace DisplayLayer{
enum {
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
	REPCOUNTER,
	METABOLISM,
	DISCOMFORT,
	VOLUME,
	SPECIES,
	CROSSABLE,
//	REPMODE,
	
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

//defines for the static display in the top-left corner. Changing order here changes listing order
namespace StaticDisplay{
enum {
	PAUSED= 0,
	DEMO,
	CLOSED,
	DAYCYCLES,
	DROUGHT,
	MUTATIONS,
	FOLLOW,
	AUTOSELECT,
	USERCONTROL,
//	DEBUG, //this is currently very... awkwardly set up. see GLView.cpp

	//Don't add beyond this entry!
	STATICDISPLAYS
};};

//defines for global agent stats display in the data region. Changing order here changes arrangement order
namespace DataDisplay{
enum {
	blank1= 0,
	ALIVE,
	HERBIVORE,
	FRUGIVORE,
	CARNIVORE,
	DEAD,
	AQUATIC,
	AMPHIBIAN,
	TERRESTRIAL,
	HYBRID,
	SPIKY,
//	ARCTIC,
//	TEMPERATE,
//	TROPICAL,
//	BOOSTING,
//	SPECIES,
	blank2,
	PLANT50,
	FRUIT50,
	MEAT50,
	HAZARD50,
	
	//Don't add beyond this entry!
	DATADISPLAYS
};};

//defines for selected agent heads up display ordering. Changing order here changes arrangement order
//keep in mind, it is displayed in 3 columns, so the 4th entry will be in the same column as the 1st,
//5th with the 2nd, etc
namespace SADHudOverview{
enum {
	HEALTH= 0,
	REPCOUNTER,
	EXHAUSTION,
	AGE, //I'm underneath HEALTH!
	GENERATION, //I'm underneath REPCOUNTER! Etc
	STOMACH,
	METABOLISM,
	NUMBABIES,
	SPECIESID,
	STRENGTH,
	HYBRID,
	SIZE,
	MOTION,
	SEXPROJECT,
	LUNGS,
	WASTE,
	GIVING,
	TEMPPREF,
	SPIKE,
	GRAB,
	CHAMOVID,
	BITE,
	VOLUME,
	TONE,
	STAT_KILLED,
	STAT_CHILDREN,
	MUTCHANCE,
	DEATH,
	blank2,
	MUTSIZE,

	//Don't add beyond this entry!
	HUDS
};};

//defines for selected agent heads up display ordering, for stats, so similar to above, but more specific. Changing order here changes arrangement order
//keep in mind, it is displayed in 3 columns, so the 4th entry will be in the same column as the 1st,
//5th with the 2nd, etc
//these are STATS, so they can CHANGE!
namespace SADHudStats{
enum {
	HEALTH= 0,
	REPCOUNTER,
	EXHAUSTION,
	AGE, //I'm underneath HEALTH!
	MOTION, //I'm underneath REPCOUNTER! Etc
	SEXPROJECT,
	WASTE,
	GIVING,
	SPIKE,
	GRAB,
	BITE,
	VOLUME,
	TONE,
	STAT_KILLED,
	STAT_CHILDREN,

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
	BEST_HERBIVORE,
	BEST_FRUGIVORE,
	BEST_CARNIVORE,
	HEALTHY,
	ENERGETIC,
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
//GOAL is to make it so that each of these represents a "node" or organ, and maybe allow for node growths later (actual physical body parts)
namespace Genetypes {
enum {
	ADD_STRUCT_NODE= 0, //UNUSED
	MULT_MAXHEALTH, //UNUSED
	ADD_INT_MAXAGE, //UNUSED
	MULT_MUTCHANCE, //old MUTRATE1
	MULT_MUTSIZE, //old MUTRATE2
	MULT_RADIUS,
	MULT_RED,
	MULT_GREEN,
	MULT_BLUE,
	MULT_CHAMOVID,
	MULT_WHEELSTRENGTH,
	ADD_INT_BABY,
	ADD_FLOAT_THERMAL, //UNUSED
	MULT_THERMAL,
	MULT_LUNG,
	MULT_METABOLISM,
	MULT_STOMACH_H,
	MULT_STOMACH_F,
	MULT_STOMACH_M,
	ADD_FLOAT_MAXREPCOUNTER, //will this be used?
	ADD_FLOAT_SEXPROJECT,
	MULT_EYE_SEE_AGENT,
	ADD_STRUCT_EYE, //UNUSED
	MULT_EAR_HEAR_AGENT,
	ADD_STRUCT_EAR, //UNUSED
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

	const float VERSION= 0.06; //current program settings version. ++0.01 IF CHANGES MADE AND PROGRAM UPDATED

	//sound file defines
	const char SFX_CHIRP1[]= "sounds/agents/chirp1.ogg";
	const char SFX_CHIRP2[]= "sounds/agents/chirp2.ogg";
	const char SFX_SMOOCH1[]= "sounds/agents/smooch1.ogg";
	const char SFX_PLOP1[]= "sounds/agents/plop1.ogg";
	const char SFX_DEATH1[]= "sounds/agents/death1.ogg";
	const char SFX_STAB1[]= "sounds/agents/stab1.ogg";

	const char SFX_BEACH1[]= "sounds/environment/beach1.ogg";

	const std::string MAIN_SONG= "sounds/music/sleep no more - evanjones4.ogg";
	const std::string ETHERAL_SONG= "sounds/music/pinecone-ambient-dark - evanjones4.ogg";
	const std::string BABY_SONG= "sounds/music/sad-heaven-piano-3 - psovod.ogg";
	const std::string ADAPT_SONG= "sounds/music/sunny dream - kjartan-abel-com.ogg";
	const std::string PERSIST_SONG= "sounds/music/ambient-03-08-19 - newagesoup.ogg";
	const std::string ENERGY_SONG= "sounds/music/beat-mfo-3b7-35 - erh.ogg";
	const std::string SLEEPY_SONG= "sounds/music/ambient-waves - erokia.ogg";
	const std::string SHIMMER_SONG= "sounds/music/pinecone-ambient - evanjones4.ogg";
	const std::string OVERGROWTH_SONG= "sounds/music/dunes - andrewkn.ogg";
	const std::string SONGS[9]= {MAIN_SONG,ETHERAL_SONG,BABY_SONG,ADAPT_SONG,PERSIST_SONG,ENERGY_SONG,SLEEPY_SONG,SHIMMER_SONG,OVERGROWTH_SONG};

	const float SNAP_SPEED = 0.2; //how fast snapping to an object of interest is; 1 is instant, 0.1 is smooth, 0 is pointless
	const float ZOOM_SPEED = 0.002; //how fast zoom actions change the magnification
	const int EVENTS_DISP= 8; //how many events to display at once, at max. Will not miss events that exceed this; they'll wait
	const int EVENTS_HALFLIFE= 400; //half-life (in #ticks) for display of event messages.
	const int TIPS_DELAY= 60; //how many ticks we wait before displaying first tip
	const int TIPS_PERIOD= 800; //how many ticks we wait between tips
	const float RENDER_DEAD_ALPHA= 0.5; //the transparency of dead agents
	const float RENDER_MAXSPLASHSIZE= 35.0; //max indicator splash size on any agent
	const float RENDER_MINVOLUME= 0.01; //min volume below which the agent is considered silent (for visualizing purposes, not real)

	const int REPORTS_PER_EPOCH = 200; //(.cfg)
	const int FRAMES_PER_EPOCH = 200000; //(.cfg)
	const int FRAMES_PER_DAY= 4000; //(.cfg)
	const int RECORD_SIZE = 200; // number of data points stored for the graph. Units are in reports, the frequency of which are defined above

	//WORLD ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ WORLD
	const int CZ= 50; //cell size in pixels, for food squares. Should divide well into Width, Height
	const int MIN_PLANT= 2000; //(.cfg)
	const float INITPLANTDENSITY= 0.8; //(.cfg)
	const float INITFRUITDENSITY= 0.4; //(.cfg)
	const float INITMEATDENSITY= 0.001; //(.cfg)
	const float INITHAZARDDENSITY= 0.003; //(.cfg)

	const int AGENTS_MIN_NOTCLOSED= 50; //(.cfg)
	const int AGENTS_MAX_SPAWN= 400; //(.cfg)
	const int AGENTSPAWN_FREQ= 75; //(.cfg)
	const int AGENTS_MAX_NOOXYGEN= 2500; //(.cfg)

	const int CONTINENTS= 2; //(.cfg)
	const int CONTINENT_SPREAD= 20; //how many cells each continent "seed" will, at max, spread from another
	const float LOWER_ELEVATION_CHANCE= 0.08; //what's the chance that the terrain will drop a level instead of stay the same when "spreading"?
	const float OCEANPERCENT= 0.6; //(.cfg)
	const bool SPAWN_LAKES= true; //(.cfg)
	const float GRAVITYACCEL= 0.01; //(.cfg)
	const float BUMP_PRESSURE= 0.1; //(.cfg)
	const float GRAB_PRESSURE= 0.1; //(.cfg)
	const float SOUNDPITCHRANGE= 0.1; //(.cfg)

	const bool DISABLE_LAND_SPAWN= true; //(.cfg & GUI)
	const bool MOONLIT= true; //(.cfg, save, & GUI)
	const float MOONLIGHTMULT= 0.1; //(.cfg & save)
	const bool DROUGHTS= true; //(.cfg, save, & GUI)
	const float DROUGHT_STDDEV= 0.15; // The standard deviation of changes to the DROUGHTMULT
	const int DROUGHT_WEIGHT= 2; // the weight multiple of the current DROUGHTMULT when averaged (1.0 has a weight of 1)
	const float DROUGHT_NOTIFY= 0.2; //+/- this value from 1.0 of drought shows notifications
	const float DROUGHT_MIN= 0.6; //(.cfg & save)
	const float DROUGHT_MAX= 1.5; //(.cfg & save)
	const bool MUTEVENTS= true; //(.cfg, save, & GUI)
	const int MUTEVENT_MAX= 3; //(.cfg)
	const float MUTEVENT_CHANCE= 0.25; //chance of a mutation event that has a multiple other than 1

	//BOTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BOTS

	//brain settings
	const int BRAINSIZE= 160; //(.cfg)
	const float LEARNRATE= 0.001; // CHANGE TO LEARN FROM USER INPUT
	const bool ENABLE_LEARNING= true; //
	const float BRAIN_DIRECTINPUTS= 0.2; //probability of random brain conns on average which will connect directly to inputs
	const float BRAIN_DEADCONNS= 0.35; //probability of random brain conns which are "dead" (that is, weight = 0)
	const float BRAIN_CHANGECONNS= 0.05; //probablility of random brain conns which are change sensitive
	const float BRAIN_MEMCONNS= 0.01; //probablility of random brain conns which are memory type
	const float BRAIN_TRACESTRENGTH= 0.1; //when performing a traceback, what minimum absolute weight of connections will count for tracing
//	const float BRAIN_MIRRORCONNS= 0.05; //
//	const float BRAIN_ANDCONNS= 0.2; //probability of random brain conns that multiply in instead of add.

	//general settings
	const float BOTSPEED= 1.0; //(.cfg)
	const float ENCUMBEREDMULT= 0.3; //speed multiplier for being encumbered
	const float MEANRADIUS=10.0; //(.cfg)
	const float BOOSTSIZEMULT= 2.0; //(.cfg)
	const float BOOSTEXAUSTMULT= 4.0; //(.cfg)
	const float BASEEXHAUSTION= -6; //(.cfg)
	const float EXHAUSTION_MULT= 0.5; //(.cfg)
	const int MAXWASTEFREQ= 200; //(.cfg)
	const float FOODTRANSFER= 0.1; //(.cfg)
	const float MAXSELFISH= 0.01; //Give value below which an agent is considered selfish
	const float SPIKESPEED= 0.01; //(.cfg)
	const float VELOCITYSPIKEMIN= 0.2; //minimum velocity difference between two agents in the positive direction to be spiked by the other
	const int BLINKTIME= 8; //it's really a little thing... how many ticks the agent eyes blink for. Purely aesthetic
	const int BLINKDELAY= 105; //blink delay time. In ticks
	const int JAWRENDERTIME= 25; //time allowed for jaw to be rendered after a bite starts

	//reproduction
	const int TENDERAGE= 10; //(.cfg)
	const float MINMOMHEALTH=0.25; //(.cfg)
	const float MIN_INTAKE_HEALTH_RATIO= 0.5; //(.cfg)
	const float REP_PER_BABY= 4; //(.cfg)
	const float REPCOUNTER_MIN= 15; //minimum value the Repcounter may be set to
	const float OVERHEAL_REPFILL= 0; //(.cfg)

	//mutations
	const float MAXDEVIATION= 10; //(.cfg)
	const int BRAINSEEDHALFTOLERANCE= 5; //the difference in brain seeds before halving. A difference = this between brain seeds corresponds to 25%/75% chances
	const float META_MUTCHANCE= 0.1; //what is the chance and stddev of mutations to the mutation chances and sizes? lol
	const float META_MUTSIZE= 0.001;
	const float DEFAULT_MUTCHANCE= 0.11; //(.cfg)
	const float DEFAULT_MUTSIZE= 0.015; //(.cfg)
	const float LIVE_MUTATE_CHANCE= 0.0001; //(.cfg)

	//distances
	const float DIST= 400; //(.cfg)
	const float SPIKELENGTH=30; //(.cfg)
	const float TOOCLOSE=8; //(.cfg)
	const float FOOD_SHARING_DISTANCE= 60; //(.cfg)
	const float SEXTING_DISTANCE= 60; //(.cfg)
	const float GRABBING_DISTANCE= 40; //(.cfg)
//	const float BLOOD_SENSE_DISTANCE= 50; //(.cfg)

	//deathly things
	const int FRESHKILLTIME= 50; //(.cfg)
	const int CORPSE_FRAMES= 400; //(.cfg)
	const float CORPSE_MEAT_MIN= 0.25; //(.cfg)
	const float DEADSLOWDOWN= 0.8; //slowdown multiplier of agent speed when they die

	//Health losses
	const int MAXAGE=10000; //(.cfg)
	const float HEALTHLOSS_AGING = 0.0001; //(.cfg)
	const float HEALTHLOSS_WHEELS = 0.0; //(.cfg)
	const float HEALTHLOSS_BOOSTMULT= 2.0; //(.cfg)
	const float HEALTHLOSS_BADTEMP = 0.0046; //(.cfg)
	const float HEALTHLOSS_BRAINUSE= 0.0; //(.cfg)
	const float HEALTHLOSS_SPIKE_EXT= 0.0; //(.cfg)
	const float HEALTHLOSS_BADTERRAIN= 0.022; //(.cfg)
	const float HEALTHLOSS_NOOXYGEN= 0.0002; //(.cfg)
	const float HEALTHLOSS_ASSEX= 0.25; //(.cfg)

	const float DAMAGE_FULLSPIKE= 4.0; //(.cfg)
	const float DAMAGE_COLLIDE= 3.0; //(.cfg)
	const float DAMAGE_JAWSNAP= 3.0; //(.cfg)

	//Death cause strings. Typically preceeded by "Killed by ", but this is just all damage sources in text form
	//please note: there are two "sections" sepparated by a single space. In reporting, this is used for uniquely identifying death causes
	//The number of these should match exactly the above sources of health loss and damage, plus generosity transfer and user kill, minus boostmult and wheels (natural)
	const char DEATH_SPIKERAISE[]= "Spike Raising";
	const char DEATH_HAZARD[]= "a Hazard";
	const char DEATH_BADTERRAIN[]= "Suffocation .";
	const char DEATH_GENEROUSITY[]= "Excessive Generosity";
	const char DEATH_COLLIDE[]= "Excessive Generosity";
	const char DEATH_SPIKE[]= "a Murder";
	const char DEATH_BITE[]= "a Murder"; //these are currently the same, because currently I'm not monitoring bites vs stabs. will change later
	const char DEATH_NATURAL[]= "Natural Causes";
	const char DEATH_TOOMANYAGENTS[]= "LackOf Oxygen"; //again, this is funky because reporting is expecting only one space...
	const char DEATH_BADTEMP[]= "Temp Discomfort";
	const char DEATH_USER[]= "God (you)";
	const char DEATH_ASEXUAL[]= "Child Birth";


	//LAYERS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LAYERS
	const float STOMACH_EFF= 0.125; //(.cfg)
	const float CARNIVORE_MEAT_EFF= 0.125; //0.05; //highest meat mult possible from full carnivores. The carivore stomach is sqrt-ed for even harsher punishment

	const char FOOD_TEXT[]= "Plant Food";
	const float FOODINTAKE= 0.01; //(.cfg)
	const float FOODDECAY = 0.000004; //(.cfg)
	const float FOODGROWTH= 0.000003; //(.cfg)
	const float FOODWASTE= 0.0023;//0.0007; //(.cfg)
	const int FOODADDFREQ= 250; //(.cfg)
	const float FOODSPREAD= 0.00012; //(.cfg)
	const int FOODRANGE= 2; //(.cfg)
	//Plant food is the simplest and most plentiful form of nutrition, but it takes time to consume enough

	const char FRUIT_TEXT[]= "Fruit Food";
	const float FRUITINTAKE = 0.020; //(.cfg)
	const float FRUITDECAY = 0.000003; //(.cfg)
	const float FRUITWASTE = 0.0023; //0.0014; //(.cfg)
	const int FRUITADDFREQ = 4; //(.cfg)
	const float FRUITREQUIRE= 0.1; //(.cfg)
	//Fruit is a quick and easy alternative to plants. Also partially randomly populated, harkening back to ScriptBots origins

	const char MEAT_TEXT[]= "Meat Food";
	const float MEATINTAKE= 0.1; //(.cfg)
	const float MEATDECAY= 0.00002;//0.00001; //(.cfg)
	const float MEATWASTE= 0.008; //0.0023; //(.cfg)
	const float MEATVALUE= 1.0; //(.cfg)
	//Meat comes from dead bots, and is the fastest form of nutrition, IF bots can learn to find it before it decays (or make it themselves...)

	const int HAZARDFREQ= 30; //(.cfg)
	const float HAZARDEVENT_MULT= 4.0; //(.cfg)
	const float HAZARDDECAY= 0.000002; //(.cfg)
	const float HAZARDDEPOSIT= 0.00006; //(.cfg)
	const float HAZARDDAMAGE= 0.001;//0.0032; //(.cfg)
	const float HAZARDPOWER= 0.5; //(.cfg)
	}

#endif
