#ifndef GLVIEW_H
#define GLVIEW_H

#include "glui.h"
#include <stdarg.h>

#include "UIElement.h"
//#include "View.h"
#include "World.h"
#include "ReadWrite.h"

#include <xutility>

//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"

class GLView;

extern GLView* GLVIEW;

void gl_processNormalKeys(unsigned char key, int x, int y);
void gl_processSpecialKeys(int key, int x, int y);
void gl_processReleasedKeys(unsigned char key, int x, int y);
void gl_menu(int key);
void gl_processMouse(int button, int state, int x, int y);
void gl_processMouseActiveMotion(int x, int y);
void gl_processMousePassiveMotion(int x, int y);
void gl_changeSize(int w, int h);
void gl_handleIdle();
void gl_renderScene();
void glui_handleButtons(int action);
void glui_handleRW(int action);
void glui_handleCloses(int action);

struct Color3f
{
	Color3f() : red(0), gre(0), blu(0.1) {} //navy-blue default color
	Color3f(float v) : red(v), gre(v), blu(v) {}
	Color3f(float r, float g) : red(r), gre(g), blu(0.1) {}
	Color3f(float r, float g, float b) : red(r), gre(g), blu(b) {}
	void mix(Color3f ocolor) {
		this->red= (this->red+ocolor.red)*0.5;
		this->gre= (this->gre+ocolor.gre)*0.5;
		this->blu= (this->blu+ocolor.blu)*0.5;
	}
	void mix(float r, float g, float b) {
		this->red= (this->red+r)*0.5;
		this->gre= (this->gre+g)*0.5;
		this->blu= (this->blu+b)*0.5;
	}

	float red;
	float gre;
	float blu;
};

class GLView
{

public:
	GLView(World* w);
	~GLView();
	
	bool cullAtCoords(int x, int y); //determine if, at our translation and scale, the current x and y position deserves (generous) culling
	void drawPreAgent(const Agent &a, float x, float y, bool ghost= 0);
	void drawAgent(const Agent &a, float x, float y, bool ghost= 0);
	void drawCell(int x, int y, const float values[Layer::LAYERS]); //draws the background boxes
	void drawData(); //draws info in the left side of the sim
	void drawFinalData(); //draws some data on the world after cells but before agents
	void drawStatic(); //draws viewer-static objects
	void drawPieDisplay(float x, float y, float size, std::vector<std::pair<std::string, float>> values); //draw a pie chart at specified screen x and y

//	virtual void trySaveAgent(); //starts GLView working on saving selected agent
//	virtual void tryLoadAgent(); //loads up an agent from a file
	ReadWrite* savehelper; //for loading/saving
	void trySaveWorld(bool force= false, bool autosave= false);
	
	void setWorld(World* w);
	
	//GLUT functions
	void processNormalKeys(unsigned char key, int x, int y);
	void processSpecialKeys(int key, int x, int y);
	void processReleasedKeys(unsigned char key, int x, int y);
	void processMouse(int button, int state, int x, int y);
	void processMouseActiveMotion(int x, int y);
	void processMousePassiveMotion(int x, int y);

	void handlePopup(int x, int y);
	void menu(int key);
	void menuSpecial(int key);
	void changeSize(int w, int h);
	void handleIdle();
	void renderScene();
	void handleButtons(int action); //callback function for buttons
	void handleRW(int action); //callback function for glui loading/saving
	void handleCloses(int action); //callback function for loading gui's
	bool checkFileName(char name[32]);
	bool checkFileName(std::string name);

	void gotoDefaultZoom();
	void processLayerPreset();

	void glCreateMenu();
	int m_id, sm1_id, sm2_id, sm3_id, sm4_id; //main right-click menues
	int win1;
	void gluiCreateMenu();
	
private:
	//3f agent color defs
	Color3f setColorHealth(float health);
	Color3f setColorStomach(const float stomach[Stomach::FOOD_TYPES]);
	Color3f setColorStomach(float plant, float fruit, float meat);
	Color3f setColorTempPref(float discomfort);
	Color3f setColorMetabolism(float metabolism);
	Color3f setColorTone(float tone);
	Color3f setColorLungs(float lungs);
	Color3f setColorSpecies(float species);
	Color3f setColorCrossable(float species);
	Color3f setColorGenerocity(float give);
	Color3f setColorStrength(float strength);
	Color3f setColorRepType(int type);
	Color3f setColorRepCount(float repcount, int type);
	Color3f setColorMutations(float rate, float size);
	Color3f setColorGeneration(int gen);

	//3f agent part color defs
	std::pair<Color3f,float> setColorEar(int index);

	//3f cell color defs
	Color3f setColorCellsAll(const float values[Layer::LAYERS]);
	Color3f setColorTerrain(float val);
	Color3f setColorHeight(float val);
	Color3f setColorPlant(float val);
	Color3f setColorWaterPlant(float val);
	Color3f setColorMountPlant(float val);
	Color3f setColorFruit(float val);
	Color3f setColorWaterFruit(float val);
	Color3f setColorMeat(float val);
	Color3f setColorHazard(float val);
	Color3f setColorWaterHazard(float val);
	Color3f setColorTempCell(int val);
	Color3f setColorLight(float val);

	void popupReset(float x= -1, float y= -1);
	void popupAddLine(std::string line);

	void createTile(UIElement &parent, int x, int y, int w, int h, std::string key, std::string title= "");
	void createTile(int x, int y, int w, int h, std::string key, std::string title= "");
	void createTile(UIElement &parent, int w, int h, std::string key, std::string title= "", bool d= true, bool r= false);
	void checkTileListClicked(std::vector<UIElement> tiles, int mx, int my, int state);

	void processTiles(); //process all tiles. Make them move, hide/unhide, etc

	int* renderTile(UIElement t, bool maintile= false, int bevel= UID::TILEBEVEL); //render a given (sub-)tile
	void renderAllTiles(); //render all tiles. main tiles and any of their children
	void countdownEvents(); //MERGE WITH TILES

	int getLayerDisplayCount(); //get the number of display layers we have running. If more than 1, use Reality! mode. If 0, disable drawing of cells
	int getAgentRes(bool ghost= false); //get desired agent resolution
	
	World *world; //the WORLD
	void syncLiveWithWorld(); //sync all important variables with their World counterparts, because they could have been changed by world
	//live variable support via glui
	int live_demomode; //are we in demo mode?
	int live_mousemode; //what mode is the mouse using?
	int live_worldclosed; //is the world closed?
	int live_paused; //are we paused?
	int live_playmusic; //is music allowed to play?
	int live_playsounds; //are any sounds allowed to play at all?
	int live_fastmode; //are we drawing?
	int live_skipdraw; //are we skipping some frames?
	int live_agentsvis; //are we drawing agents? If so, what's the scheme? see namespace "Visuals" in settings.h for details
	int live_layersvis[DisplayLayer::DISPLAYS]; //list of bools keeping track of which layers we're displaying.
	int live_waterfx; //are we rendering water effects?
	int live_profilevis; //what visualization profile are we displaying next to the selected agent? see namespace "Profiles"
	int live_lifepath; //are we collecting and showing the agent lifepath data?
	int live_selection; //what bot catagory are we currently trying to autoselect? see namespace "Select" in settings.h
	int live_follow; //are we following the selected agent?
	int live_autosave; //are we allowing autosaves?
	int live_grid; //override usual grid behavior and always draw grid?
	int live_hidedead; //hide dead agents?
	int live_hidegenz; //hide generation zero agents? (no hard feelings)
	int live_landspawns; // are landspawns enabled
	int live_moonlight; //is moonlight enabled?
	float live_oceanpercent; //what is the setting for the percentage of water in the world?
	int live_droughts; //are droughts and overgrowth periods enabled?
	float live_droughtmult; //value of the drought modifier
	int live_mutevents; //are variable rate mutation events enabled?
	int live_climate; //are climate changes enabled?
	float live_climatebias; //current climate bias
	float live_climatemult; //how extreme are climate changes? Effects both bias and mult
	int live_cursormode; //what mode are we on with the cursor? 0= select mode, 1= place mode
	int live_debug; //are we debugging?

	//not live variables, but pretty closely related
	int past_agentsvis; //what was the previous agent visualization? helps when changing views w/o the GUI

	//popup containers
	int popupxy[2]; //simple x,y coords for the popup graphic
	std::vector<std::string> popuptext; //potentially multi-line container of popup text

	//Menu, file, and load/saving system
	GLUI * Menu;
	GLUI_FileBrowser * Browser;
	std::string file_name;
	GLUI * Loader;
	GLUI_Button * LoadAgentButton;
	GLUI * Saver;
	GLUI * Alert;
	GLUI_EditText * Filename;
	char filename[32];
	int lastsavedtime;

	char buf[100];
	char buf2[10]; //text buffers
	int modcounter; //tick counter BUGGY DO NOT USE
	int currentTime; //always up to date live time variable
	int lastUpdate;
	int frames;
	int wWidth;
	int wHeight; //window width and height, to reduce glutGet calls
	
	float scalemult; //the viewer's scale factor (larger values are closer zoom)
	//REF: scalemult= 5.0 is VERY zoomed in, 1= every detail should be visible, 0.0859= current default zoom, 0.03= minimum scale
	//many details stop rendering at scale= .3
	float xtranslate, ytranslate; //the viewer's x and y position
	int downb[GLMouse::BUTTONS]; //the three buttons and their states
	int mousex, mousey;
	bool mousedrag; //was the mouse dragged recently? used to disable button click activity when click-moving
	bool uiclicked; //was the ui clicked recently? used to disable drag fuctions if inital click was on UI

	int ui_layerpreset; //user can select a preset of layer displays using this
	int ui_ladmode; //what rendering mode are we using for the Loaded Agent Display?
	bool ui_movetiles; //are we allowing tiles to be moved?
	std::vector<UIElement> maintiles; //list of interactive tile buttons! WIP

	float scale4ksupport; //because reasons, we are tracking the scalemult that allows all agents to show a default size...
};

#endif // GLVIEW_H
