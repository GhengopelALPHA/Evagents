#ifndef GLVIEW_H
#define GLVIEW_H

#include <stdarg.h>

#include "View.h"
#include "World.h"
#include "ReadWrite.h"
#include "glui.h"

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
void glui_handleRW(int action);
void glui_handleCloses(int action);

struct Color3f
{
	Color3f() : red(0), gre(0), blu(0.1) {} //navy-blue default color
	Color3f(float r, float g) : red(r), gre(g), blu(0.1) {}
	Color3f(float r, float g, float b) : red(r), gre(g), blu(b) {}
	float red;
	float gre;
	float blu;
};

class GLView : public View
{

public:
	GLView(World* w);
	virtual ~GLView();
	
	virtual void drawAgent(const Agent &a, float x, float y, bool ghost= 0);
	virtual void drawCell(int x, int y, const float values[Layer::LAYERS]); //draws the background boxes
	virtual void drawData(); //draws info in the left side of the sim
	virtual void drawStatic(); //draws viewer-static objects

	virtual void trySaveWorld(bool autosave= false); //called by world, starts GLView working on saving the world
//	virtual void trySaveAgent(); //starts GLView working on saving selected agent
//	virtual void tryLoadAgent(); //loads up an agent from a file
	ReadWrite* savehelper; //for loading/saving
	
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
	void handleRW(int action); //callback function for glui loading/saving
	void handleCloses(int action); //callback function for closing glui's

	void gotoDefaultZoom();

	void glCreateMenu();
	int m_id; //main right-click context menu
	int win1;
	void gluiCreateMenu();

	void popupReset(float x= -1, float y= -1);
	void popupAddLine(std::string line);
	
private:
	//3f agent color defs
	Color3f setColorHealth(float health);
	Color3f setColorStomach(const float stomach[Stomach::FOOD_TYPES]);
	Color3f setColorTempPref(float discomfort);
	Color3f setColorMetabolism(float metabolism);
	Color3f setColorTone(float tone);
	Color3f setColorLungs(float lungs);
	Color3f setColorSpecies(float species);
	Color3f setColorCrossable(float species);
	Color3f setColorGenerocity(float give);

	//3f cell color defs
	Color3f setColorCellsAll(const float values[Layer::LAYERS]);
	Color3f setColorTerrain(float val);
	Color3f setColorHeight(float val);
	Color3f setColorPlant(float val);
	Color3f setColorWaterPlant(float val);
	Color3f setColorMountPlant(float val);
	Color3f setColorFruit(float val);
	Color3f setColorMeat(float val);
	Color3f setColorHazard(float val);
	Color3f setColorWaterHazard(float val);
	Color3f setColorTemp(float val);
	Color3f setColorLight(float val);

	void countdownEvents();
	
	World *world; //the WORLD
	int live_worldclosed; //live variable support via glui
	int live_paused; //are we paused?
	int live_fastmode; //are we drawing?
	int live_skipdraw; //are we skipping some frames?
	int live_agentsvis; //are we drawing agents? If so, what's the scheme? see namespace "Visuals" in settings.h for details
	int past_agentsvis; //what was the previous agent visualization? helps when changing views w/o the GUI
	int live_layersvis; //what cell layer is currently active? see namespace "Layers" in settings.h
	int past_layersvis; // unused
	int live_profilevis; //what visualization profile are we displaying next to the selected agent? see namespace "Profiles"
	int live_selection; //what bot catagory are we currently trying to autoselect? see namespace "Select" in settings.h
	int live_follow; //are we following the selected agent?
	int live_autosave; //are we allowing autosaves?
	int live_grid; //override usual grid behavior and always draw grid?
	int live_landspawns; // are landspawns enabled
	int live_moonlight; //is moonlight enabled?
	int live_droughts; //are droughts and overgrowth periods enabled?
	int live_mutevents; //are variable rate mutation events enabled?

	int popupxy[2]; //simple x,y coords for the popup graphic
	std::vector<std::string> popuptext;
	
	int live_debug; //are we debugging?
	bool debug;
	GLUI * Menu;
	GLUI_FileBrowser * Browser;
	std::string file_name;
	GLUI * Loader;
	GLUI * Saver;
	GLUI * Alert;
	GLUI_EditText * Filename;
	char filename[30];

	char buf[100];
	char buf2[10]; //text buffers
	int modcounter; //tick counter
	int lastUpdate;
	int frames;
	
	float scalemult; //the viewer's scale factor (larger values are closer zoom)
	float xtranslate, ytranslate; //the viewer's x and y position
	int downb[3]; //the three buttons and their states
	int mousex, mousey;
	bool mousedrag; //was the mouse dragged? used to disable button click activity when click-moving
};

#endif // GLVIEW_H
