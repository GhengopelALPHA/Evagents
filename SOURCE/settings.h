#pragma once

#include <string>
#include "vmath.h"

#define NUMEYES 6
#define NUMEARS 4

#ifndef SETTINGS_H
#define SETTINGS_H

//defines for GUI button handles. Order changes nothing
namespace GUIButtons {
const enum {
	TOGGLE_LAYERS= 0,
	TOGGLE_PLACEAGENTS
};};

//defines for GL mouse buttons. We don't have any control over this really, it's an OpenGL thing, and it only supports 3 mouse buttons
namespace GLMouse {
const enum {
	LEFT= 0,
	MIDDLE,
	RIGHT,
	BUTTONS
};};

//defines for reading/writing GUI handles. Order changes nothing
namespace RWOpen {
const enum {
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
namespace RWClose {
const enum {
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
namespace MainTiles {
const enum {
	LAD= 0, //Selected Agent Display, our first tile! It has lots of sub-tiles that get created automatically
	TOOLBOX, //UNUSED, will hold many of the GLUI functions eventually
	EVENTS, //UNUSED, will eventually migrate events to GLView and handle them as tiles

	//Don't add beyond this entry!
	TILES
};};

//defines for ui dimensions and useful values throughout the program
namespace UID {
	const int CHARHEIGHT= 9;
	const int CHARWIDTH= 6; //height and width (on screen) allowed for text characters
	const int HUDSWIDTH= 88; //the column width we are assuming when it comes to text space in the Selected Agent Display.
	const int TILEMARGIN= 9; //what is the min margin between a sub-tile and the next tile above (or window if main tile)
	const int TILEBEVEL= 5; //bevel size on tiles. 5 was scaled for the LAD; consider code for smaller tiles to have smaller bevels
	const int TINYTILEWIDTH= 20; //the size of tiny tiles
	const int LADHEIGHTLINES = 11; //how many lines of text the LAD is configured to be sized for
	const int LADWIDTH= 440; //Loaded / seLected Agent Display width. SHOULD be tied to HUDSWIDTH above, but there's a lot more stuff going on in there
	const int BUFFER= 6; //extra buffer space, mostly for text
	const int EVENTSWIDTH= 210; //Event toast width (length?)... x-dimension-size
	const int GRAPHWIDTH = 10020; //x-d width of the on-world graph
	const int GRAPHBUFFER = 15;
};

//defines for the graph's vertical guide lines, indicating days and epochs (or nothing). Order changes nothing
namespace GuideLine {
const enum {
	NONE = 0,
	DAY,
	EPOCH
};};

namespace EventColor {
const enum {
	//				Default:										- Demo Only:
	WHITE= 0,	//	simulation notification							- agent event
	RED,		//	global population decrease						- agent killed
	GREEN,		//	global population increase						- agent generosity
	BLUE,		//	neutral population change						- agent sexual reproduction
	YELLOW,		//													- tips, agent jumped, agent bit another										
	CYAN,		//	control help, global ice age					- agent collision, grab
	PURPLE,		//													- agent mutation
	BROWN,		//													- agent decayed
	NEON,		//													- agent assexual reproduction
	MINT,		//	simulation start/restart/change
	PINK,		//	global change (overgrowth/drought bad)
	ORANGE,		//	global hadean or extreme climate				- agent stabbed another
	BLOOD,		//	load agent failed								- agent was stabbed or bitten (non-fatal)
	LAVENDER,	//	global change (mutations)
	LIME,		//	global change (overgrowth/drought good)
	MULTICOLOR,	//	achievements ONLY
	BLACK		//	epoch count, multi-use, Autoselect changed
};};

//defines for mouse modes. Order changes nothing.
namespace MouseMode{
const enum {
	NONE= 0,
	SELECT, //select agent only
	PLACE_AGENT, //place agents only
	PLACE_VALUE, //place cell value to cell layer

	//Don't add beyond this entry!
	MODES
};};

//defines for LAD (Loaded Agent Display) visual modes. Order changes nothing.
namespace LADVisualMode{
const enum {
	NONE= 0,
	GHOST, //draw a ghost agent, copying all motion and events
	DAMAGES, //draw the damage amounts pie chart
	INTAKES, //draw the cell intake amounts pie chart

	//Don't add beyond this entry!
	MODES
};};

//defines for LAD (Loaded Agent Display) data modes. Order changes nothing.
namespace LADDataMode{
const enum {
	NONE= 0,
	COMMON, //show traits and stats that are commonly looked at
	TRAITS, //only show traits (fixed values), and more of them
	STATS, //only show stats (changing values), and more of them
	//PROFILE, //display the selected agent's profile, TODO

	//Don't add beyond this entry!
	MODES
};};

//defines for selected agent heads up display ordering. Changing order here changes arrangement order
//keep in mind, it is displayed in 3 columns, so the 4th entry will be in the same column as the 1st,
//5th with the 2nd, etc
namespace LADData{
	const enum {
		HEALTH= 0,		//stat
		DHEALTH,		//stat
		REPCOUNTER,		//stat
		EXHAUSTION,		//stat
		EXHAUSTION_VAL,	//stat
		AGE,			//stat
		CARCASSCOUNT,	//stat
		FRESHKILL,		//stat
		GENERATION,		//stat (fixed)
		STOMACH,		
		METABOLISM,	
		NUMBABIES,
		SPECIESID,
		PARENTID,		//UNUSED
		STRENGTH,
		HYBRID,			//stat (fixed)
		KINRANGE,
		MOTION,			//stat
		REVERSE,		//stat
		JUMP_VAL,		//stat
		WHEEL_L,		//stat
		WHEEL_R,		//stat
		SEXPROJECT,		//stat
		SEXPROJECT_VAL,	//stat
		SEXPROJECTBIAS,
		RADIUS,
		GIVING,			//stat
		GIVING_VAL,		//stat
		WASTE,			//stat
		WASTE_VAL,		//stat
		LUNGS,		
		GRAB,			//stat
		GRAB_VAL,		//stat
		SPIKE,			//stat
		TEMPPREF,
		VOLUME,			//stat
		BITE,			//stat
		CHAMOVID,
		TONE,			//stat
		NEAR,			//stat
		STAT_HITS,		//stat
		STAT_KILLED,	//stat
		STAT_CHILDREN,	//stat
		BRAINSIZE,
		BRAINLIVECONNS,
		BRAINMUTCHANCE,
		BRAINMUTSIZE,
		GENEMUTCHANCE,
		GENEMUTSIZE,
		CLOCK1_FREQ,
		CLOCK2_FREQ,
		EYE_MULT_SEE_AGENTS,
		EYE_MULT_SEE_CELLS,
		EAR_MULT_HEAR_AGENTS,
		BLOOD_MULT_SENSE,
		SMELL_MULT_SENSE,
		NUMGENES,

		//Don't add beyond this entry!
		HUDS
	};

	// arrays with the order of the above ids set for their particular display.
	const int COMMON[UID::LADHEIGHTLINES*3] = {
		HEALTH,
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
		KINRANGE,
		MOTION,
		SEXPROJECT,
		RADIUS,
		GIVING,
		WASTE,
		LUNGS,
		GRAB,
		SPIKE,
		TEMPPREF,
		VOLUME,
		BITE,
		CHAMOVID,
		TONE,
		STAT_KILLED,
		STAT_CHILDREN,
		BRAINMUTCHANCE,
		BRAINMUTSIZE,
		BRAINSIZE,
		GENEMUTCHANCE,
		GENEMUTSIZE,
		BRAINLIVECONNS
	};

	const int TRAITS[UID::LADHEIGHTLINES*3] = {
		HEALTH,
		CHAMOVID,
		STOMACH,

		NUMGENES,
		RADIUS,
		METABOLISM,
		
		LUNGS,
		TEMPPREF,
		GENERATION,

		STRENGTH,
		SEXPROJECTBIAS,
		NUMBABIES,
		
		SPECIESID,
		KINRANGE,
		HYBRID,

		CLOCK1_FREQ,
		CLOCK2_FREQ,
		-1,

		EYE_MULT_SEE_AGENTS,
		EYE_MULT_SEE_CELLS,
		EAR_MULT_HEAR_AGENTS,

		BLOOD_MULT_SENSE,
		SMELL_MULT_SENSE,
		-1,

		BRAINMUTCHANCE,
		BRAINMUTSIZE,
		BRAINSIZE,

		GENEMUTCHANCE,
		GENEMUTSIZE,
		BRAINLIVECONNS,

		-1,
		-1,
		-1
	};

	const int STATS[UID::LADHEIGHTLINES*3] = {
		HEALTH,
		REPCOUNTER,
		EXHAUSTION,

		AGE,
		NEAR,
		EXHAUSTION_VAL,

		WHEEL_L,
		MOTION,
		WHEEL_R,

		REVERSE,
		JUMP_VAL,
		CARCASSCOUNT,

		SPIKE,
		GIVING,
		SEXPROJECT,

		BITE,
		GIVING_VAL,
		SEXPROJECT_VAL,

		GRAB,
		WASTE,
		-1,

		GRAB_VAL,
		WASTE_VAL,
		-1,

		STAT_HITS,
		STAT_KILLED,
		STAT_CHILDREN,

		VOLUME,
		TONE,
		-1,

		-1,
		-1,
		-1
	};
};

//defines for read-write modes. Changing order does nothing
namespace ReadWriteMode{
const enum {
	OFF = 0,
	READY,
	WORLD,
	CELL,
	AGENT,
	GENE,
	BOX,
	CONN,
	EYE,
	EAR,

	//Don't add beyond this entry!
	MODES
};};

//defines for layer data. Making changes here changes cells[LAYERS][][] in World. Order does not matter (except in ReadWrite...)
namespace Layer{
const enum {
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
const enum {
	ELEVATION,
	PLANTS,
	FRUITS,
	MEATS,
	HAZARDS,	
	TEMP,
	LIGHT,

	//Don't add beyond this entry!
	DISPLAYS
};};

//defines for agent visualization. Changing order here changes cycle order and menu-listing order
namespace Visual{
const enum {
	NONE= 0,
	RGB,
	STOMACH,
	LUNGS,
	DISCOMFORT,
	HEALTH,
	METABOLISM,
	REPCOUNTER,
	STRENGTH,
	VOLUME,
	BRAINMUTATION,
	GENEMUTATION,
	GENERATIONS,
	AGE_HYBRID,
	SPECIES,
	CROSSABLE,
	
	//Don't add beyond this entry!
	VISUALS
};};

//defines for selected agent visualization. Changing order here changes menu-listing order
namespace Profile{
const enum {
	NONE= 0,
	INOUT,
	BOXES,
	BRAIN,
	SOUND,
	EYES,
	METABOLISM,

	//Don't add beyond this entry!
	PROFILES
};};

//defines for types of agent counts, both for reporting and the static display. Changing order here changes listing order
namespace LiveCount{
const enum {
	HERBIVORE = 0,
	FRUGIVORE,
	CARNIVORE,
	AQUATIC,
	AMPHIBIAN,
	TERRESTRIAL,
	HYBRID,
	SPIKED,
	BITEY,

	//Don't add beyond this entry!
	COUNTS
};};

//defines for the static display in the top-left corner. Changing order here changes listing order
namespace StaticDisplay{
const enum {
	PAUSED= 0,
	DEMO,
	CLOSED,
	DAYCYCLES,
	CLIMATE,
	DROUGHT,
	MUTATIONS,
	AUTOSELECT,
	FOLLOW,
	USERCONTROL,

	//Don't add beyond this entry!
	STATICDISPLAYS
};};

//defines for the extra static display lines in the top-left corner. Changing order here changes listing order but ONLY within the group
namespace StaticDisplayExtra{
const enum {
	OCEANPERCENT= 0,
	AVG_LAYERS, //I strongly recommend keeping AVG_LAYERS and xAVG_LAYERS together in this order
	xAVG_LAYERS= AVG_LAYERS + Layer::LAYERS - 3, //-3 because we don't handle LIGHT or ELEVATION (-1-2)
	LIVECOUNTS,
	xLIVECOUNTS= LIVECOUNTS + LiveCount::COUNTS - 1, //same comment as AVG_LAYERS
	OXYGEN,

	//Don't add beyond this entry!
	STATICDISPLAYS
};};

//defines for the debug static display lines in the top-left corner. Changing order here changes listing order but ONLY within the group
namespace StaticDisplayDebug{
const enum {
	WWIDTH,
	WHEIGHT,
	MODCOUNT,
	GLMODCOUNT,
	GLPOS,
	GLSCALEMULT,
	GLMOUSEPOS,
	WORLDMOUSEPOS,
	RESOLUTION,
	ISRENDERING_,

	//Don't add beyond this entry!
	STATICDISPLAYS
};};

//defines for global agent stats display in the data region. Changing order here changes arrangement order. UNUSED
namespace DataDisplay{
const enum {
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

//defines for selection code. Changing order here changes menu-listing order
namespace Select {
const enum {
	NONE= 0,
	MANUAL,
	RELATIVE,
	OLDEST,
	BEST_GEN,
	BEST_HERBIVORE,
	BEST_FRUGIVORE,
	BEST_CARNIVORE,
	BEST_AQUATIC,
	BEST_AMPHIBIAN,
	BEST_TERRESTRIAL, //After this 10th entry, selection modes keys are shift + top number keys
	HEALTHY,
	ENERGETIC,
	FASTEST, 
	PRODUCTIVE,
	AGGRESSIVE,
	SEXIEST,
	GENEROUS_EST,
	KINRANGE_EST,

	//Don't add beyond this entry!
	SELECT_TYPES
};};

//defines for reproduction modes. I don't forsee any reason to add more or change their order; it effects no visuals, except color (for now)
namespace RepType {
const enum {
	ASEXUAL,
	FEMALE,
	MALE
};};

//defines for sound types. Order changes nothing.
namespace Hearing {
const enum {
	VOICE,
	MOVEMENT,
	//INTAKE,

	//Don't add beyond this entry!
	TYPES
};};

//defines for brain input code. Changing order here changes input-to-brain order and visualization order
namespace Input {
const enum {
	EYES,
	xEYES= EYES+NUMEYES*3-1, //I strongly recommend keeping EYES and xEYES together in this order
	HEALTH,
	REPCOUNT,
	RANDOM,
	CLOCK1,
	CLOCK2,
	CLOCK3,
	EARS,
	xEARS= EARS+NUMEARS-1, //same comment as xEYES
	BLOOD,
	TEMP,
	PLAYER,
	BOT_SMELL,
	FRUIT_SMELL,
	MEAT_SMELL,
	HAZARD_SMELL,
	WATER_SMELL,
	
	//Don't add beyond this entry!
	INPUT_SIZE
};};

//defines for brain output code. Changing order here changes brain-to-output order (reversed) and visualization order
namespace Output {
const enum {
	LEFT_WHEEL_F= 0,
	RIGHT_WHEEL_F,
	LEFT_WHEEL_B,
	RIGHT_WHEEL_B,
	REVERSE,			//are we reversing wheel outputs? >0.5: true, <=0.5: false
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
const enum {
	PLANT= 0,
	MEAT,
	FRUIT,

	//don't add beyond this entry!
	FOOD_TYPES
};};

//defines for connection types, used exclusively in CPBrain. Order changes nothing
namespace ConnType {
const enum {
	NORMAL = 0,
	INVERTED,
	DELTA,

	//don't add beyond this entry!
	CONN_TYPES
};};

//defines for gene traits. every agent has exactly one gene for each entry below //can have any number of genes, even 0 (typically defaults the trait to 1.0)
//Order changes nothing
namespace Trait {
const enum {
	SPECIESID, //TEMP, will be replaced with raw gene count matching later
	KINRANGE,					//range from the species ID (later just Genes count) that this agent will actively reproduce with
	BRAIN_MUTATION_CHANCE,		//Chance: how often do mutations occur? For the brain
	BRAIN_MUTATION_SIZE,		//Size: how significant are they?
	GENE_MUTATION_CHANCE,		//same as above, but for the Genes
	GENE_MUTATION_SIZE,
	RADIUS,						//radius of agent
//	HEIGHT, //UNUSED, physical height of the agent. Typically in the neighborhood of radius
	SKIN_RED,
	SKIN_GREEN,
	SKIN_BLUE,					//genetic color traits of the agent.  
	SKIN_CHAMOVID,				//how strongly the output colors are overriding the genetic colors. 0= full genes, 1= full outputs
	STRENGTH,					//how "strongly" the wheel speeds are pushed towards their outputs (their muscle strength). In range (0, 1]
	THERMAL_PREF,				//what temperature does this agent like? In range [0, 1]
	LUNGS,						//what type of environment does this agent need? In range [0 (for "water"), 1 (for "land")]. See Elevation namespace
	NUM_BABIES,					//number of children this agent creates with every reproduction event
	SEX_PROJECT_BIAS,			//a physical bias trait, making sexual reproduction easier for some species/members. in range [-1,1]
	METABOLISM,					//rate that food is diverted from +health to +repcounter. There is a minimum threshold for this that is scaled to.
	STOMACH,					//stomach, see Stomach namespace for descriptions and order
	xSTOMACH = STOMACH + Stomach::FOOD_TYPES - 1, // Do NOT put anything between this and the last entry!!
	CLOCK1_FREQ,				//the frequencies of the two static clocks of this bot
	CLOCK2_FREQ,
	EYE_SEE_AGENTS,				//multiplier for agent's vision of each other. 1 is normal vision, 0 is blind, 2+ increases sensitivity
	EYE_SEE_CELLS,				//multiplier for agent's vision of cells. 1 is normal vision, 0 is blind, 2+ increases sensitivity
//	STRUCT_EYE, //UNUSED
	EAR_HEAR_AGENTS,				//multiplier for agent's hearing of other agent's vocalizations and wheel movements
//	STRUCT_EAR, //UNUSED
	BLOOD_SENSE,				//multiplier for agent's blood sense
	SMELL_SENSE,						//multiplier for agent's smell sense. Effects all types of smell (agent, water, meat, fruit, hazard) simultaneously

	
	//don't add beyond this entry!
	TRAIT_TYPES
};};

namespace R {
	const std::string WORLD_SAVE_EXT= ".SAV";
	const std::string AGENT_SAVE_EXT= ".AGT";
};

namespace conf {
	
	//DEFAULTS: All of what follows are defaults, and if settings.cfg exists, are subsituted with that file's values if VERSION #'s match
	//SIM ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ SIM
	const int WIDTH = 15000;  //width and height of simulation
	const int HEIGHT = 12000;
	const int WWIDTH = 1100;  //initial window width and height
	const int WHEIGHT = 700;

	const float VERSION= 0.08; //current program settings version. ++0.01 IF CHANGES MADE AND PROGRAM UPDATED

	//sound file defines
	const char SFX_CHIRP1[]= "sounds/agents/chirp1.ogg";
	const char SFX_CHIRP2[]= "sounds/agents/chirp2.ogg";
	const char SFX_SMOOCH1[]= "sounds/agents/smooch1.ogg";
	const char SFX_PURR1[]= "sounds/agents/purr1.ogg";
	const char SFX_PLOP1[]= "sounds/agents/plop1.ogg";
	const char SFX_DEATH1[]= "sounds/agents/death1.ogg";
	const char SFX_STAB1[]= "sounds/agents/stab1.ogg";
	const char SFX_CHOMP1[]= "sounds/agents/chomp1.ogg";
	const char SFX_CHOMP2[]= "sounds/agents/chomp2.ogg";
	const char SFX_GRAB1[] = "sounds/agents/grab1.ogg";
	const char SFX_RIP1[] = "sounds/agents/rip1.ogg";

	const char SFX_BEACH1[]= "sounds/environment/beach1.ogg";

	const char SFX_UI_RELATIVESELECT[]= "sounds/interaction/selection1.ogg";
	const char SFX_UI_AUTOSELECT[]= "sounds/interaction/selection2.ogg";
	const char SFX_UI_AUTOSELECT_FAIL[]= "sounds/interaction/select_fail1.ogg";
	const char SFX_UI_MANUALSELECT[]= "sounds/interaction/selection3.ogg";
	const char SFX_UI_CLEAR_AGENT[] = "sounds/interaction/woosh1.ogg";
	const char SFX_UI_ZAP1[] = "sounds/interaction/zap1.ogg";
	const char SFX_UI_ZAP2[] = "sounds/interaction/zap2.ogg";
	const char SFX_UI_ZAP3[] = "sounds/interaction/zap3.ogg";

	const std::string MUSIC_FOLDER= "sounds/music/";
	const std::string MAIN_SONG= "sounds/music/sleep no more - evanjones4.ogg";
	const std::string ETHERAL_SONG= "sounds/music/pinecone-ambient-dark - evanjones4.ogg";
	const std::string CELEBRATION_SONG= "sounds/music/endlessmotion - bensound.ogg";
	const std::string BABY_SONG= "sounds/music/sad-heaven-piano-3 - psovod.ogg";
	const std::string ADAPT_SONG= "sounds/music/sunny dream - kjartan-abel-com.ogg";
	const std::string PERSIST_SONG= "sounds/music/ambient-03-08-19 - newagesoup.ogg";
	const std::string RHYTHM_SONG= "sounds/music/beat-mfo-3b7-35 - erh.ogg";
	const std::string SLEEPY_SONG= "sounds/music/ambient-waves - erokia.ogg";
	const std::string SHIMMER_SONG= "sounds/music/pinecone-ambient - evanjones4.ogg";
	const std::string OVERGROWTH_SONG= "sounds/music/dunes - andrewkn.ogg";
	const std::string STALE_SONG= "sounds/music/msfxp6-198-stretched-piano-1 - erokia.ogg";
	const std::string INSPIRE_SONG= "sounds/music/cd-yang-001 - kevp888.ogg";
	const int NUM_SONGS= 12; // must increment when adding new songs above, and add ref to list below

	const std::string SONGS[NUM_SONGS]= {MAIN_SONG,ETHERAL_SONG,CELEBRATION_SONG,BABY_SONG,ADAPT_SONG,PERSIST_SONG,RHYTHM_SONG,
		SLEEPY_SONG,SHIMMER_SONG,OVERGROWTH_SONG,STALE_SONG,INSPIRE_SONG};

	//ui & other display standards
	const float SNAP_SPEED = 0.2; //how fast snapping to an object of interest is; 1 is instant, 0.1 is smooth, 0 is pointless
	const float ZOOM_SPEED = 0.002; //how fast zoom actions change the magnification
	const int EVENTS_DISP= 8; //how many events to display at once, at max. Will not miss events that exceed this; they'll wait
	const int EVENTS_HALFLIFE= 400; //half-life (in #ticks) for display of event messages.
	const int TIPS_DELAY= 60; //how many ticks we wait before displaying first tip
	const int TIPS_PERIOD= 800; //how many ticks we wait between tips
	const float RENDER_DEAD_ALPHA= 0.5; //the transparency of dead agents
	const float RENDER_MAXSPLASHSIZE= 35.0; //max indicator splash size on any agent
	const float RENDER_MINVOLUME= 0.01; //min volume below which the agent is considered silent (for visualizing purposes, not real)
	const int RENDER_LIFEPATH_INTERVAL= 10; //number of ticks before recording the agent's location

	const int REPORTS_PER_EPOCH = 500; //(.cfg)
	const int FRAMES_PER_EPOCH = 400000; //(.cfg)
	const int FRAMES_PER_DAY= 8000; //(.cfg)
	const int CELL_TICK_RATE= 4; //Tick rate of all cells in the world. IMPACTS PERFORMANCE!!! 1= every cell calculated every tick, 
	 // 2= every other tick for every other cell, 4= every 4th cell is calculated every 4 ticks, etc. This value also multiplies all cell 
	 // growth/decay rates so no differences should be observed. Setting to 5+ will cause light to look weird, and has diminishing performance returns

	//WORLD ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ WORLD
	const int CZ= 50; //cell size in pixels, for food squares. Should divide well into Width, Height
	const int MIN_PLANT= 2000; //(.cfg)
	const float INITPLANTDENSITY= 0.8; //(.cfg)
	const float INITFRUITDENSITY= 0.4; //(.cfg)
	const float INITMEATDENSITY= 0.001; //(.cfg)
	const float INITHAZARDDENSITY= 0.003; //(.cfg)

	//terrain gen
	const int CONTINENTS= 2; //(.cfg)
	const int CONTINENT_ROUGHNESS = 4; //(.cfg)
	const int CONTINENT_SPREAD= 20; //how many cells each continent "seed" will, at max, spread from another
	const float LOWER_ELEVATION_CHANCE= 0.08; //what's the chance that the terrain will drop a level instead of stay the same when "spreading"?
	const float AMPHIBIAN_THRESHOLD = 0.55; // at what lung value is an agent no longer considered an amphibian, but rather terrestrial?
	const float OCEANPERCENT= 0.65; //(.cfg)
	const bool SPAWN_LAKES= true; //(.cfg)
	const float ISLANDNESS = 0.05; //(.cfg)
	const int FEATURES_TO_SPAWN = 1; //(.cfg)

	//simulation standards
	const int AUTOSAVE_FREQUENCY_DAYS = 5; //frequency, in days, that autosaves are made. Always based off 0,0, so every multiple of this day will make a save
	const int AGENTS_MIN_NOTCLOSED= 50; //(.cfg)
	const int AGENTS_MAX_SPAWN= 600; //(.cfg)
	const int AGENTSPAWN_FREQ= 75; //(.cfg)
	const int AGENTS_MAX_NOOXYGEN= 2400; //(.cfg)
	const int SPECIESID_RANGE= 9000; //species ID range between (-this,this) that agents will spawn with. Note mutations may take individuals beyond this range

	//physics
	const float GRAVITY_ACCELERATION= 0.010; //(.cfg)
	const float BUMP_PRESSURE= 0.1; //(.cfg)
	const float GRAB_PRESSURE= 0.05; //(.cfg)
	const float GRAB_ROTATION_LIMIT = 0.08; // This is the max possible -/+ value an agent's angle can change due to grab pressure, in radians
	const float SOUND_PITCH_RANGE= 0.1; //(.cfg)
	const float WHEEL_VOLUME= 0.1; //multiplier of the wheel speeds when being heard
	const float WHEEL_TONE= 0.125; //tone value that wheels are heard at, in range [0,1]
	const float AMBIENT_LIGHT_PERCENT= 0.25; //(.cfg)
	const bool AGENTS_SEE_CELLS = true; //(.cfg & save)
	const bool AGENTS_DONT_OVERDONATE = false; //(.cfg & save)
	const int MAX_WASTE_FREQ = 500; //(.cfg)

	//World settings
	const bool DISABLE_LAND_SPAWN= true; //(.cfg & GUI)
	const bool MOONLIT= true; //(.cfg, save, & GUI)
	const float MOONLIGHTMULT= 0.25; //(.cfg & save)
	const bool DROUGHTS= true; //(.cfg, save, & GUI)
	const float DROUGHT_STDDEV= 0.3; // The standard deviation of changes to the DROUGHTMULT
	const int DROUGHT_WEIGHT_TO_RANDOM= 31; //the weight multiple of the current DROUGHTMULT when averaged DOWN (randf(0,1) has a weight of 1)
	const int DROUGHT_WEIGHT_TO_ONE= 2; //the weight multiple of the current DROUGHTMULT when averaged to ONE (1.0 has a weight of 1). changing this relative to _ZERO should be interesting
	const float DROUGHT_NOTIFY= 0.2; //+/- this value from 1.0 of drought shows notifications
	const float DROUGHT_MIN= 0.6; //(.cfg & save)
	const float DROUGHT_MAX= 1.5; //(.cfg & save)
	const bool MUTEVENTS= true; //(.cfg, save, & GUI)
	const int MUTEVENT_MAX= 3; //(.cfg)
	const float MUTEVENT_CHANCE= 0.25; //chance of a mutation event that has a multiple other than 1
	const bool CLIMATE= true; //(.cfg, save, & GUI)
	const float CLIMATE_INTENSITY= 0.009; //(.cfg & GUI)
	const float CLIMATE_INTENSITY_EPOCH_MULT= 6.0; //multiplier applied to the above value when selecting new climate values at start of epochs
	const float CLIMATEMULT_AVERAGE= 0.5; //value that the world's climate mult tries to average back towards
	const int CLIMATEMULT_WEIGHT= 3; // the weight multiple of the current CLIMATEMULT when averaged (CLIMATEMULT_AVERAGE has a weight of 1)
	const bool CLIMATE_AFFECT_FLORA= true; //(.cfg)
	const float CLIMATE_KILL_FLORA_ZONE= 0.1; //temperature zone (in absolute units) that flora struggles within. A value of 0 disables plant decay, so using CLIMATE_AFFECT_FLORA= true only reduces seeding

	//BOTS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ BOTS

	//brain settings
	const int BRAINBOXES= 70 + Output::OUTPUT_SIZE; //(.cfg)
	const int BRAINCONNS= 300; //(.cfg)
	const float BRAIN_CONN_WEIGHT_STD= 5; //std of the randn centered on 0 that new connection weights generate with. 
	const float BRAIN_CONN_WEIGHT_MUTATION_DAMPEN= 0.1; //multiplier applied to new connection weights that are randomized and new conn mutations.
	const float BRAIN_CONN_BIAS_STD= 2; //std of the randn centered on 0 that new connection biases generate with.
	const float BRAIN_CONN_RELU_STD= 0.25; //std of the randn centered on 0 that new connection relu's generate with.
	const float BRAIN_BOX_BIAS_STD= 0.5;  //std of the randn centered on 0 that new box biases generate with.
	const float BRAIN_BOX_GW_RANGE= 2; // +/- range of values new box global weights generate within.
	const float BRAIN_BOX_KP_MAX = 1.2; //max value between [0,this) that new box kp (dampening) values generate within. Remember that the difference to a new output is multiplied by this to get the target value
	const float LEARN_RATE= 0.001; // rate of Stimulant effecting weights. TODO: (.cfg) CHANGE TO LEARN FROM USER INPUT?
	const float BRAIN_DIRECTINPUTS= 0.3; //probability of random brain conns on average which will connect directly to inputs
	const float BRAIN_NEGATIVE_GW_BOXES= 0.2; //probability and approx ratio of random brain's boxes that will have a negative global weight
	const float BRAIN_INVERTCONNS= 0.5; //probablility of random brain conns which are inverted (the input value is (1-x) before weight)
	const float BRAIN_CHANGECONNS= 0.15; //probablility of random brain conns which are change sensitive
	const float BRAIN_MIRRORCONNS= 0.5; //probablility of random brain conns which will be made as mirror-compare connections with a random other connection, range(0,i)
//	const float BRAIN_TRACESTRENGTH= 0.1; //when performing a traceback, what minimum absolute weight of connections will count for tracing

	//gene/trait settings
	const float MEANRADIUS= 10.0; //(.cfg)
	const float TINY_RADIUS = 5.0; //radius below which an agent is considered tiny.
	const float GENEROSITY_RATE= 0.1; //(.cfg)
	const float MAXSELFISH= 0.01; //Give value below which an agent is considered selfish
	const float SPIKESPEED= 0.003; //(.cfg)
	const float JAW_INTAKE_MAX_MULT = 3; //(.cfg)
	const float JAW_RESET_SPEED = 0.003; //the speed factor at which the jaw returns to 0 (open & ready)
	const float VELOCITYSPIKEMIN= 0.2; //minimum velocity difference between two agents in the positive direction to be spiked by the other
	const float BITE_EATS_RATIO = 1.5; //minimum size ratio of a bitting agent where it will eat the target whole and gain instant reward. Default is 1.5* the size of the target
	const bool SPAWN_MIRROR_EYES = true; //(.cfg)
	const bool PRESERVE_MIRROR_EYES = true; //(.cfg)
	const float MIN_TEMP_DISCOMFORT_THRESHOLD = 0.005; //minimum discomfort value below which it's just overridden to 0

	//mutations
	const int OVERRIDE_KINRANGE= -1; //(.cfg)
	const int VISUALIZE_RELATED_RANGE= 30; // what's the range in addition to agent's kinrange that we go ahead and say maybe they were related
	const int BRAINSEEDHALFTOLERANCE= 3; //the difference in brain seeds before halving. A difference = this between brain seeds corresponds to 25%/75% chances. Only effects sexual reproduction
	const float META_MUTCHANCE= 0.08; //what is the chance and stddev of mutations to the mutation chances and sizes? lol
	const float META_MUTSIZE= 0.002;
	const float LIVE_MUTATE_CHANCE= 0.0001; //(.cfg)
	const float DEFAULT_BRAIN_MUTCHANCE= 0.03; //(.cfg)
	const float DEFAULT_BRAIN_MUTSIZE= 0.01; //(.cfg)
	const float DEFAULT_GENE_MUTCHANCE= 0.05; //(.cfg)
	const float DEFAULT_GENE_MUTSIZE= 0.03; //(.cfg)

	//movement settings
	const float WHEEL_SPEED= 1.5; //(.cfg)
	const float JUMP_VELOCITY_MULT= 0.5; //this value multiplies in to the final velocity value for the jump when getting set. Otherwise, velocities are in range (0,1) for jump > (0.5, 1)
	const float JUMP_MOVE_BONUS_MULT= 2.0; //(.cfg)
	const float BOOST_MOVE_MULT= 2.0; //(.cfg)
	const float WHEEL_LOCATION= 0.5; //proportion of the agent's radius that the wheels are located
	const float ENCUMBERED_MOVE_MULT= 0.3; //(.cfg)
	
	//energy settings
	const float WHEEL_ZERO_EXHAUSTION_THRESHOLD = 0.0001; //threshold value of the wheel values summed * Boost exhaustion that, if below, sets all energy drain to 0 (sleep)
	const float BOOST_EXAUSTION_MULT= 2.0; //(.cfg)
	const float BASEEXHAUSTION= -12; //(.cfg)
	const float EXHAUSTION_MULT_PER_OUTPUT= 1.0; //(.cfg)
	const float EXHAUSTION_MULT_PER_CONN= 0.025; //(.cfg)
	
	//stomach intake settings
	const float MAX_INTAKE_RATE = 0.005; //(.cfg)
	const float MIN_INTAKE_RATE = 0.0001; //(.cfg)
	const float MIN_METABOLISM_HEALTH_RATIO = 0.5; //(.cfg)

	//visual settings
	const int BLINKTIME= 8; //it's really a little thing... how many ticks the agent eyes blink for. Purely aesthetic
	const int BLINKDELAY= 105; //blink delay time. In ticks
	const int JAWRENDERTIME= 20; //time allowed for jaw to be rendered after a bite starts
	const int INPUTS_OUTPUTS_PER_ROW = 20; //visually how many inputs and outputs are we showing before starting a new row?
	const int BOXES_PER_ROW = 37; //same as above, but for boxes

	//reproduction
	const int TENDERAGE= 10; //(.cfg)
	const float MINMOMHEALTH=0.5; //(.cfg)
	const float REP_PER_BABY= 3; //(.cfg)
	const float REPCOUNTER_MIN= 8; //minimum value the Repcounter may be set to
	const float OVERHEAL_REPFILL= 0; //(.cfg)

	//distances
	const float MAX_SENSORY_DISTANCE= 400; //(.cfg)
	const float MAX_SPIKE_LENGTH=30; //(.cfg)
	const float BITE_DISTANCE = 12; //(.cfg)
	const float BUMP_DAMAGE_OVERLAP=8; //(.cfg)
	const float FOOD_SHARING_DISTANCE= 60; //(.cfg)
	const float SEXTING_DISTANCE= 60; //(.cfg)
	const float GRABBING_DISTANCE= 40; //(.cfg)
	const float SMELL_DIST_MULT= 0.5; //additional multiplier for cell and agent smell distance.
	const float BLOOD_SENSE_DISTANCE= 30; //(.cfg)

	//angles
	const float EYE_SENSE_MAX_FOV = M_PI/2; //(.cfg)
	const float EYE_SENSE_FOV_STD_DEV = 0.3; //the angle value standard deviation for field of view of new eyes
	const float BLOOD_SENSE_MAX_FOV = 3*M_PI/16; //(.cfg)
	const float GRAB_MAX_FOV = M_PI/8; //(.cfg)
	const float SPIKE_MAX_FOV = M_PI/2; //the angle maximim from the center of the spike direction where other agents may be stabbed, in radians. From center to each edge, double this for full range
	const float JAW_MAX_FOV = M_PI/6; //the angle maximim from the center of the jaws direction where other agents may be bitten, in radians. From center to each edge, double this for full range

	//life and death things
	const float HEALTH_CAP= 10.0; //max health value of agents
	const int FRESHKILLTIME= 50; //(.cfg)
	const int CORPSE_FRAMES= 400; //(.cfg)
	const float CORPSE_MEAT_MIN= 0.25; //(.cfg)
	const float DEAD_SLOWDOWN= 0.8; //slowdown multiplier of agent speed when they die

	//Health losses
	const int MAXAGE=10000; //(.cfg)
	const float HEALTHLOSS_AGING = 0.0001; //(.cfg)
	const float HEALTHLOSS_EXHAUSTION = 0.0001; //(.cfg)
	const float HEALTHLOSS_WHEELS = 0.0; //(.cfg)
	const float HEALTHLOSS_BOOSTMULT= 2.0; //(.cfg)
	const float HEALTHLOSS_BADTEMP = 0.008; //(.cfg)
	const float HEALTHLOSS_BRAINUSE= 0.0; //(.cfg)
	const float HEALTHLOSS_SPIKE_EXT= 0.0; //(.cfg)
	const float HEALTHLOSS_BADTERRAIN= 0.022; //(.cfg)
	const float HEALTHLOSS_NOOXYGEN= 0.00015; //(.cfg)
	const float HEALTHLOSS_ASEX= 3.00; //(.cfg)

	const float DAMAGE_FULLSPIKE= 12.0; //(.cfg)
	const float DAMAGE_COLLIDE= 4.0; //(.cfg)
	const float DAMAGE_JAWSNAP= 8.0; //(.cfg)

	//Death cause strings. Typically preceeded by "Killed by ", but this is just all damage sources in text form
	//please note: there are two "words" sepparated by a single space. In reporting, this is used for uniquely identifying death causes
	//The number of these should match exactly the above sources of health loss and damage, plus generosity transfer and user kill, minus boostmult and wheels (natural)
	const char DEATH_TOOYOUNG[]= "Mutation? TooYoung";
	const char DEATH_SPIKERAISE[]= "Spike Raising";
	const char DEATH_HAZARD[]= "a Hazard";
	const char DEATH_BADTERRAIN[]= "Suffocation .";
	const char DEATH_GENEROSITY[]= "Excessive Generosity";
	const char DEATH_COLLIDE[]= "a Collision";
	const char DEATH_SPIKE[]= "a Stab!";
	const char DEATH_BITE[]= "a Bite!";
	const char DEATH_NATURAL[]= "Natural Causes";
	const char DEATH_TOOMANYAGENTS[]= "LackOf Oxygen"; //again, this is funky because reporting is expecting only one space...
	const char DEATH_BADTEMP[]= "Temp Discomfort";
	const char DEATH_USER[]= "God (you)";
	const char DEATH_ASEXUAL[]= "Child Birth";


	//LAYERS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ LAYERS
	const float STOMACH_EFFICIENCY= 0.2; //(.cfg)
	const float CARNIVORE_TO_MEAT_EFF= 0.125; //0.05; //highest meat mult possible from full carnivores. The carivore stomach is sqrt-ed for even harsher punishment

	const char PLANT_TEXT[]= "Plant Food";
	const float PLANT_INTAKE= 0.01; //(.cfg)
	const float PLANT_DECAY = 0.000004; //(.cfg)
	const float PLANT_GROWTH= 0.000003; //(.cfg)
	const float PLANT_WASTE= 0.003;//0.0007; //(.cfg)
	const int PLANT_ADD_FREQ= 225; //(.cfg)
	const float PLANT_SPREAD= 0.00012; //(.cfg)
	const int PLANT_RANGE= 2; //(.cfg)
	const float PLANT_TENACITY= 0.1; //(.cfg)
	//Plant food is the simplest and most plentiful form of nutrition, but it takes time to consume enough

	const char FRUIT_TEXT[]= "Fruit Food";
	const float FRUIT_INTAKE = 0.02; //(.cfg)
	const float FRUIT_DECAY = 0.000003; //(.cfg)
	const float FRUIT_WASTE = 0.002; //(.cfg)
	const int FRUIT_ADD_FREQ = 2; //(.cfg)
	const float FRUIT_PLANT_REQUIRE= 0.1; //(.cfg)
	//Fruit is a quick and easy alternative to plants. Also partially randomly populated, harkening back to ScriptBots origins

	const char MEAT_TEXT[]= "Meat Food";
	const float MEAT_INTAKE= 0.04; //(.cfg)
	const float MEAT_DECAY= 0.00001; //(.cfg)
	const float MEAT_WASTE= 0.0014; //(.cfg)
	const float MEAT_DEPOSIT_VALUE= 1.0; //(.cfg)
	const float MEAT_NON_FRESHKILL_MULT = 0.5; //(.cfg)
	//Meat comes from dead bots, and is the fastest form of nutrition, IF bots can learn to find it before it decays (or make it themselves...)

	const int HAZARD_EVENT_FREQ= 30; //(.cfg)
	const float HAZARD_EVENT_MULT= 4.0; //(.cfg)
	const float HAZARD_DECAY= 0.000001; //(.cfg)
	const float HAZARD_DEPOSIT= 0.0001; //(.cfg)
	const float HAZARD_DAMAGE= 0.0032;//0.0032; //(.cfg)
	const float HAZARD_POWER= 0.5; //(.cfg)
	const float HAZARD_EVENT_POINT= 0.9; //this value splits the range of hazard into two: below, normal, above, event, with EVENT_MULT applied and special logic
	}

#endif
