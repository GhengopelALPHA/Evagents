#include "GLView.h"

#include <ctime>

#include "config.h"
#ifdef LOCAL_GLUT32
#include "glut.h"
#else
#include <GL/glut.h>
#endif

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif
#include <stdio.h>

#include <wchar.h>
#include <xutility>

//OpenGL callbacks
void gl_processNormalKeys(unsigned char key, int x, int y)
{
	GLVIEW->processNormalKeys(key, x, y);
}
void gl_processSpecialKeys(int key, int x, int y)
{
	GLVIEW->processSpecialKeys(key, x, y);
}
void gl_processReleasedKeys(unsigned char key, int x, int y)
{
	GLVIEW->processReleasedKeys(key, x, y);
}
void gl_menu(int key)
{
	GLVIEW->menu(key);
}
void gl_changeSize(int w, int h)
{
	GLVIEW->changeSize(w,h);
}
void gl_handleIdle()
{
	GLVIEW->handleIdle();
}
void gl_processMouse(int button, int state, int x, int y)
{
	GLVIEW->processMouse(button, state, x, y);
}
void gl_processMouseActiveMotion(int x, int y)
{
	GLVIEW->processMouseActiveMotion(x,y);
}
void gl_processMousePassiveMotion(int x, int y)
{
	GLVIEW->processMousePassiveMotion(x,y);
}
void gl_renderScene()
{
	GLVIEW->renderScene();
}
void glui_handleButtons(int action)
{
	GLVIEW->handleButtons(action);
}
void glui_handleRW(int action)
{
	GLVIEW->handleRW(action);
}
void glui_handleCloses(int action)
{
	GLVIEW->handleCloses(action);
}


// some local defs of some very useful methods when drawing
void RenderString(float x, float y, void *font, const char* string, float r, float g, float b, float a= 1.0)
{
	//render a string at coords x, y, with a given font, color, and alpha value
	glColor4f(r,g,b,a);
	glRasterPos2f(x, y);
	int len = (int) strlen(string);
	for (int i = 0; i < len; i++)
		glutBitmapCharacter(font, string[i]);
}

void RenderStringBlack(float x, float y, void *font, const char* string, float r, float g, float b, float a= 1.0)
{
	//render 3 strings, 2 black-colored dropshadows beforehand
	RenderString(x+1, y+1, font, string, 0, 0, 0, a);
	RenderString(x, y+1, font, string, 0, 0, 0, a);
	RenderString(x, y, font, string, r,g,b,a);
}

void drawCircle(float x, float y, float r) {
	//Draw a circle at coords x, y, with radius r
	float n;
	for (int k=0;k<17;k++) {
		n = k*(M_PI/8);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
	}
}

void drawCircleRes(float x, float y, float r, int res) {
	//draw a circle with a specified resolution. 1= 8 verts, 2= 16, 3= 24, etc
	// note you cannot input a resolution less than 1 (may change later to allow 0= 4verts, then change to unsigned int
	float n;
	if(res<1) res=1;
	for (int k=0;k<(8*res+1);k++) {
		n = k*(M_PI/4/res);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
	}
}


void drawOutlineRes(float x, float y, float r, int res) {
	//similar to above, but is meant for use with GL_LINES
	float n;
	if(res<1) res=1;
	for (int k=0;k<(8*res);k++) {
		n = k*(M_PI/4/res);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
		n = (k+1)*(M_PI/4/res);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
	}
}


void drawQuadrant(float x, float y, float r, float a, float b) {
	//Draw a quadrant based on a and b PROPORTIONS OF 2*M_PI
	if(a>b) {float temp=b; b=a; a=temp;}
	glVertex3f(x,y,0);
	float n;
	for (int k=0;k<(int)((b-a)*16+1);k++) {
		n = k*(M_PI/8)+a*2*M_PI;
		glVertex3f(x+r*sin(n),y-r*cos(n),0);
	}
	glVertex3f(x+r*sin(b*2*M_PI),y-r*cos(b*2*M_PI),0);
	glVertex3f(x,y,0);
}


GLView::GLView(World *s) :
		world(world),
		modcounter(0),
		frames(0),
		lastUpdate(0),
		mousedrag(false),
		uiclicked(false),
		ui_layerpreset(0)
{

	xtranslate= 0.0;
	ytranslate= 0.0;
	scalemult= 0.2;
	downb[GLMouse::LEFT]=0; downb[GLMouse::MIDDLE]=0; downb[GLMouse::RIGHT]=0;
	mousex=0;mousey=0;

	popupReset();
	savehelper= new ReadWrite();
	currentTime= glutGet(GLUT_ELAPSED_TIME);
	lastsavedtime= currentTime;

	ui_ladmode = LADVisualMode::GHOST;
}


GLView::~GLView()
{

}


void GLView::gotoDefaultZoom()
//Control.cpp
{
	//reset width height just this once
	wWidth = glutGet(GLUT_WINDOW_WIDTH);
	wHeight = glutGet(GLUT_WINDOW_HEIGHT);

	//do some mathy things to zoom and translate correctly
	float scaleA = (float)wWidth / (conf::WIDTH + 2200);
	float scaleB = (float)wHeight / (conf::HEIGHT + 150);
	if(scaleA > scaleB) scalemult = scaleB;
	else scalemult = scaleA;
	xtranslate = -(conf::WIDTH - 3200)/2;
	ytranslate = -(conf::HEIGHT)/2;
	live_follow = 0;
}


void GLView::processLayerPreset()
//Control.cpp
{
	//update layer bools based on current ui_layerpreset
	for(int i=0; i<DisplayLayer::DISPLAYS; i++) {
		live_layersvis[i]= i+1==ui_layerpreset ? 1 : 0;
		if(ui_layerpreset==0) live_layersvis[i]= 1;
		if(ui_layerpreset>DisplayLayer::DISPLAYS) live_layersvis[i]= i>=ui_layerpreset-DisplayLayer::DISPLAYS ? 1 : 0;
		if(ui_layerpreset>DisplayLayer::DISPLAYS*2) live_layersvis[i]= i<ui_layerpreset-DisplayLayer::DISPLAYS*2 ? 1 : 0;
	}
}


void GLView::changeSize(int w, int h)
//Control.cpp ???
{
	//Tell GLUT that were changing the projection, not the actual view
	glMatrixMode(GL_PROJECTION);

	//Reset the coordinate system
	glLoadIdentity();

	//resize viewport to new size
	glViewport(0, 0, w, h);

	//reconcile projection (required to keep everything that's visible, visible)
	glOrtho(0,w,h,0,0,1);

	//revert to normal view opperations
	glMatrixMode(GL_MODELVIEW);

	//retreive new Window w, h
	wWidth= glutGet(GLUT_WINDOW_WIDTH);
	wHeight= glutGet(GLUT_WINDOW_HEIGHT);

	//fix up some other stuff.
	scale4ksupport= (float)wWidth/(conf::WIDTH+2200)+0.01;

	//update UI elements and make sure they are still in view
	for(int i=0; i<maintiles.size(); i++){
		if(i==MainTiles::LAD){
			maintiles[i].moveElement(wWidth-UID::LADWIDTH-UID::BUFFER, UID::BUFFER);
		}
	}
}

void GLView::processMouse(int button, int state, int x, int y)
//Control.cpp
{
	if(world->isDebug()) printf("MOUSE EVENT: button=%i state=%i x=%i y=%i, scale=%f, mousedrag=%i, uiclicked= %i\n", 
		button, state, x, y, scalemult, mousedrag, uiclicked);
	
	if(!mousedrag){ //dont let mouse click do anything if drag flag is raised
		//check our tile list!
		uiclicked= false;
		checkTileListClicked(maintiles, x, y, state);

		if(!uiclicked && state==1){ //ui was not clicked, We're in the world space!

			//have world deal with it. First translate to world coordinates though
			int wx= convertMousePosToWorld(true, x);
			int wy= convertMousePosToWorld(false, y);

			if(live_cursormode==MouseMode::SELECT){
				//This line both processes mouse (via world) and sets selection mode if it was successfull
				if(world->processMouse(button, state, wx, wy, scalemult) && live_selection!=Select::RELATIVE) live_selection = Select::MANUAL;

			} else if(live_cursormode==MouseMode::PLACE_AGENT){
				if(world->addLoadedAgent(wx, wy)) printf("agent placed!    yay\n");
				else world->addEvent("No agent loaded! Load an Agent 1st", EventColor::BLOOD);
			}
		}
	}

	mousex=x; mousey=y; //update tracked mouse position
	mousedrag= false;

	downb[button]=1-state; //state is backwards, ah well
}

void GLView::processMouseActiveMotion(int x, int y)
//Control.cpp
{
	if(world->isDebug()) printf("MOUSE MOTION x=%i y=%i, buttons: LEFT: %i MIDDLE: %i RIGHT: %i\n", x, y, downb[GLMouse::LEFT], downb[GLMouse::MIDDLE], downb[GLMouse::RIGHT]);
	
	if(!uiclicked){ //if ui was not clicked

		if (downb[GLMouse::LEFT]==1) {
			//left mouse button drag: pan around
			xtranslate += (x-mousex)/scalemult;
			ytranslate += (y-mousey)/scalemult;
			if (abs(x-mousex)>6 || abs(x-mousex)>6) live_follow= 0;
			//for releasing follow if the mouse is used to drag screen, but there's a threshold
		}

		if (downb[GLMouse::MIDDLE]==1) {
			//mouse wheel. Change scale
			scalemult += conf::ZOOM_SPEED*scalemult*((mousey-y) - (mousex-x));
			if(scalemult<0.03) scalemult=0.03;
		}

	/*	if(downb[GLMouse::RIGHT]==1){ //disabled
			//right mouse button. currently used for context menu
		}*/
	}

	if(abs(mousex-x)>4 || abs(mousey-y)>4) mousedrag= true; //mouse was clearly dragged, don't select agents after
	handlePopup(x, y); //make sure we handle any popups, even if it's active motion
	mousex=x; mousey=y; 
}

void GLView::processMousePassiveMotion(int x, int y)
//Control.cpp
{
	//when mouse moved, update popup
	if (world->modcounter%2==0 || live_paused) { //add a little delay with modcounter
		handlePopup(x,y);
	}

	//old code for mouse edge scrolling.
	/*	if(y<=30) ytranslate += 2*(30-y);
	if(y>=glutGet(GLUT_WINDOW_HEIGHT)-30) ytranslate -= 2*(y-(glutGet(GLUT_WINDOW_HEIGHT)-30));
	if(x<=30) xtranslate += 2*(30-x);
	if(x>=glutGet(GLUT_WINDOW_WIDTH)-30) xtranslate -= 2*(x-(glutGet(GLUT_WINDOW_WIDTH)-30));*/
	mousex=x; mousey=y;
}

int GLView::convertMousePosToWorld(bool x, int pos)
{
	if (x) return (int) ((pos-wWidth*0.5)/scalemult-xtranslate);
	else return (int) ((pos-wHeight*0.5)/scalemult-ytranslate);
}

void GLView::handlePopup(int x, int y)
{
	int layers= getLayerDisplayCount();
	if(layers>0 && layers < DisplayLayer::DISPLAYS) { //only show popup if less than 
		//convert mouse x,y to world x,y
		int worldcx= (int)((((float)x-wWidth*0.5)/scalemult-xtranslate));
		int worldcy= (int)((((float)y-wHeight*0.5)/scalemult-ytranslate));

		if (worldcx>=0 && worldcx<conf::WIDTH && worldcy>=0 && worldcy<conf::HEIGHT) { //if somewhere in world...
			char line[128];
			int layer= 0, currentline= 0;
			worldcx/= conf::CZ;
			worldcy/= conf::CZ; //convert to cell coords

			popupReset(x+12, y); //clear and set popup position near mouse

			sprintf(line, "Cell x: %d, y: %d", worldcx, worldcy);
			popupAddLine(line);
			if(world->isDebug()) printf("worldcx: %d, worldcy %d\n", worldcx, worldcy);
			
			if(live_layersvis[DisplayLayer::ELEVATION]) {
				float landtype= ceilf(world->cells[layer][worldcx][worldcy]*10)*0.1;

				if(landtype==Elevation::DEEPWATER_LOW) strcpy(line, "Ocean");
				else if(landtype==Elevation::SHALLOWWATER) strcpy(line, "Shallows");
				else if(landtype==Elevation::BEACH_MID) strcpy(line, "Beach");
				else if(landtype==Elevation::PLAINS) strcpy(line, "Plains");
				else if(landtype==Elevation::HILL) strcpy(line, "Hills");
				else if(landtype==Elevation::STEPPE) strcpy(line, "Steppe");
				else if(landtype==Elevation::HIGHLAND) strcpy(line, "Highland");
				else if(landtype==Elevation::MOUNTAIN_HIGH) strcpy(line, "Mountain");
				popupAddLine(line);

				sprintf(line, "Height: %.2f", world->cells[Layer::ELEVATION][worldcx][worldcy]);
				popupAddLine(line);
			}
			if(live_layersvis[DisplayLayer::PLANTS]) {
				sprintf(line, "Plant: %.4f", world->cells[Layer::PLANTS][worldcx][worldcy]);
				popupAddLine(line);
			}
			if(live_layersvis[DisplayLayer::MEATS]) {
				sprintf(line, "Meat: %.4f", world->cells[Layer::MEATS][worldcx][worldcy]);
				popupAddLine(line);
			}
			if(live_layersvis[DisplayLayer::FRUITS]){
				sprintf(line, "Fruit: %.4f", world->cells[Layer::FRUITS][worldcx][worldcy]);
				popupAddLine(line);
			}
			if(live_layersvis[DisplayLayer::HAZARDS]){
				sprintf(line, "Waste: %.4f", world->cells[Layer::HAZARDS][worldcx][worldcy]);
				popupAddLine(line);
				if(world->cells[Layer::HAZARDS][worldcx][worldcy]>conf::HAZARD_EVENT_POINT) {
					strcpy(line, "LIGHTNING STRIKE!");
					popupAddLine(line);
				}
			} 
			if(live_layersvis[DisplayLayer::TEMP]){
				float temp= world->calcTempAtCoord(worldcy);
				sprintf(line, "Temp: %.1f", temp*100);
				popupAddLine(line);
				if(temp>0.8) popupAddLine("Hadean");
				else if(temp>0.6) popupAddLine("Tropical");
				else if(temp<0.2) popupAddLine("Arctic");
				else if(temp<0.4) popupAddLine("Cool");
				else popupAddLine("Temperate");
			} 
			if(live_layersvis[DisplayLayer::LIGHT]){
				sprintf(line, "Light: %.3f", world->cells[Layer::LIGHT][worldcx][worldcy]);
				popupAddLine(line);
			}
		}
		else popupReset(); //reset if mouse outside world
	}
	else popupReset(); //reset if viewing none or map
}

void GLView::processNormalKeys(unsigned char key, int x, int y)
//Control.cpp
{
	menu(key);	
}

void GLView::processSpecialKeys(int key, int x, int y)
//Control.cpp
{
	menuSpecial(key);	
}

void GLView::processReleasedKeys(unsigned char key, int x, int y)
//Control.cpp
{
	if (key=='e') {//e: User Brain Input [released]
		world->selectedInput(0);
	} else if (key=='h') { //interface help
		//MAKE SURE ALL UI CHANGES CATELOGUED HERE
		world->addEvent("UI INTERFACE HELP:", EventColor::CYAN);
		world->addEvent("Left-click an agent to select it", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("Press 'f' to follow selection", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("Pan with left-mouse drag or ", EventColor::CYAN);
		world->addEvent("  use the arrow keys", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("Zoom with middle-mouse drag or ", EventColor::CYAN);
		world->addEvent("  use the '<'/'>' keys", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("Press 'spacebar' to pause game", EventColor::CYAN);
		world->addEvent("  and to freeze these toasts", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("Press 'm' to skip rendering,", EventColor::CYAN);
		world->addEvent("  which makes sim MUCH faster!", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("Press 'n' to dismiss these toasts", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("Right-click opens more options", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'0' selects random alive agent", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'shift+0' selects random agent", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'1' selects oldest agent", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'2' selects best generation", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'3' selects best herbivore", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'4' selects best frugivore", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'5' selects best carnivore", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'6' selects healthiest agent", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'7' selects most energetic", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'8' selects most childbaring", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'9' selects most aggressive", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
//		world->addEvent("'' selects relatives continually", EventColor::CYAN);
		world->addEvent("'page up' selects relative, 1st,", EventColor::CYAN);
		world->addEvent("  speciesID+1, then any in range", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'page dn' selects relative, 1st,", EventColor::CYAN);
		world->addEvent("  speciesID-1, then any in range", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'wasd' controls selected agent", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'end' prints traits of selected agent", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'e' activates a brain input", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'+' speeds simulation. '-' slows", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'l' & 'k' cycle layer view modes", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'o' sets layer view to Reality!", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'z' & 'x' cycle agent view", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("'c' toggles spawns (closed world)", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent("Debug mode is required for:", EventColor::RED);
		world->addEvent(" Press 'delete' to kill selected", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent(" '/' heals the selected agent", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent(" '|' reproduces selected agent", EventColor::CYAN);
		world->addEvent("", EventColor::CYAN);
		world->addEvent(" '~' mutates selected agent", EventColor::CYAN);
	} else if (key==9) { //[tab] - tab toggles to manual selection mode
		live_selection = Select::MANUAL;
		world->player_control = false;
	}//else if (key=='g') { //graphics details
		//MAKE SURE ALL GRAPHICS CHANGES CATELOGUED HERE
		//world->addEvent("",9);
}

void GLView::menu(int key)
//Control.cpp
{
	if (key==27 || key==32) { //[esc](27) or [spacebar](32) - pause
		live_paused= !live_paused;
	} else if (key==43 || key=='+' || key=='=') { //[+] - speed up sim
		live_skipdraw++;
	} else if (key==45 || key=='-') { //[-] - slow down sim
		live_skipdraw--;
	} else if (key>48 && key<=57) { //number keys: select mode
		//'0':48, '1':49, '2':50, ..., '9':57
		//please note that even if we add more Select options, this code means we can never have more than the 10th one (not including NONE)
		//be accessible with number keys
		if(live_selection!=key-47) live_selection= key-47; 
		else live_selection= Select::NONE;
	} else if (key==48) { //number key 0: select random from alive
		int count = 0;
		while(count<10000){ //select random agent, among alive
			int idx= randi(0,world->agents.size());
			if (world->agents[idx].health>0.1) {
				world->setSelectedAgent(idx);
				live_selection= Select::MANUAL;
				break;
			} else count++;
		}
	} else if (key=='!') {
		menu(1048); //convert to numerical in order to handle below
	} else if (key=='@') {
		menu(1049); //convert to numerical in order to handle below
	} else if (key=='#') {
		menu(1050); //convert to numerical in order to handle below
	} else if (key=='$') {
		menu(1051); //convert to numerical in order to handle below
	} else if (key=='%') {
		menu(1052); //convert to numerical in order to handle below
	} else if (key=='^') {
		menu(1053); //convert to numerical in order to handle below
	} else if (key=='&') {
		menu(1054); //convert to numerical in order to handle below
	} else if (key=='*') {
		menu(1055); //convert to numerical in order to handle below
	} else if (key=='(') {
		menu(1056); //convert to numerical in order to handle below
	} else if (key==')') {
		world->setSelectedAgent(randi(0,world->agents.size())); //select random agent, among all
		live_selection= Select::MANUAL;
	} else if (key==62 || key=='.') { //[>] - zoom+
		scalemult += 10*conf::ZOOM_SPEED;
	} else if (key==60 || key==',') { //[<] - zoom-
		scalemult -= 10*conf::ZOOM_SPEED;
		if(scalemult<0.03) scalemult=0.03;
	} else if (key==9) { //[tab] - press sets select mode to off, and sets cursor mode to default
		live_selection= Select::NONE;
		live_cursormode= MouseMode::SELECT;
	} else if (key=='c') {
		world->setClosed( !world->isClosed() );
		live_worldclosed= (int) world->isClosed();
	} else if (key=='e') { //e: User Brain Input toggle [pressed]
		world->selectedInput(1);
//	}else if (key=='') { // : report selected's wheel inputs
//		world->selectedTrace(1);
	} else if (key=='f') {
		live_follow= !live_follow; //toggle follow selected agent
	} else if (key=='m') { //drawing / fast mode
		live_fastmode= !live_fastmode;
	} else if (key=='n') { //dismiss visible world events
		world->dismissNextEvents(conf::EVENTS_DISP);
	} else if (key=='l' || key=='k' || key=='o') { //layer profile switch; l= "next", k= "previous", o= "reality!"
		//This is a fancy bit of code. This value is init=0, which means all layers selected
		//1 to DisplayLayers:DISPLAYS -> each layer turns on one at a time, acts like previous behavior
		//DisplayLayers:DISPLAYS+1 to DisplayLayers:DISPLAYS*2 -> each layer starting with the lowest in order gets turned off, until #=DISPLAYS, where all are turned off
		//DisplayLayers:DISPLAYS*2+1 to DisplayLayers:DISPLAYS*3-1 -> each layer turns back on in normal order, until it overflows and resets to 0, all turned on
		if (key=='l') ui_layerpreset++;
		else if (key=='o') ui_layerpreset= 0;
		else ui_layerpreset--;
		if (ui_layerpreset>DisplayLayer::DISPLAYS*3-1) ui_layerpreset= 0;
		if (ui_layerpreset<0) ui_layerpreset= DisplayLayer::DISPLAYS*3-1;
		
		//now for the magic...
		processLayerPreset(); //call method to update layer bools based on ui_layerpreset

	} else if(key=='q') {
		//zoom and translocate to instantly see the whole world
		gotoDefaultZoom();
	} else if (key=='j') { //toggle hide dead
		live_hidedead = 1-live_hidedead;
	} else if (key=='z' || key=='x') { //change agent visual scheme; x= "next", z= "previous"
		if (key=='x') live_agentsvis++;
		else live_agentsvis--;
		if (live_agentsvis>Visual::VISUALS-1) live_agentsvis= Visual::NONE;
		if (live_agentsvis<Visual::NONE) live_agentsvis= Visual::VISUALS-1;

		//produce notifications if agent visual changed via buttons
/*		if(past_agentsvis!=live_agentsvis){
			switch(live_agentsvis){
				case Visual::CROSSABLE :	world->addEvent("Viewing Relatives", EventColor::CYAN);			break;
				case Visual::DISCOMFORT :	world->addEvent("Viewing Temp Discomfort", EventColor::CYAN);	break;
				case Visual::HEALTH :		world->addEvent("Viewing Health and Giving", EventColor::CYAN);	break;
				case Visual::REPCOUNTER :	world->addEvent("Viewing Rep. Counter", EventColor::CYAN);		break;
				case Visual::LUNGS :		world->addEvent("Viewing Lung Type", EventColor::CYAN);			break;
				case Visual::METABOLISM :	world->addEvent("Viewing Metabolism", EventColor::CYAN);		break;
				case Visual::NONE :			world->addEvent("Disabled Agent Rendering", EventColor::CYAN);	break;
				case Visual::RGB :			world->addEvent("Viewing Agent Colors", EventColor::CYAN);		break;
				case Visual::SPECIES :		world->addEvent("Viewing Species IDs", EventColor::CYAN);		break;
				case Visual::STOMACH :		world->addEvent("Viewing Stomachs", EventColor::CYAN);			break;
				case Visual::VOLUME :		world->addEvent("Viewing Sounds", EventColor::CYAN);			break;
			}
			//backup vis setting
			past_agentsvis= live_agentsvis;
		}
/*		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->isClosed()) glutChangeToMenuEntry(4, "Open World", 'c');
		else glutChangeToMenuEntry(4, "Close World", 'c');
		glutSetMenu(m_id);*/
	
	//user controls:
	} else if (key==119 && world->getSelectedID()!=-1) { //w (move faster)
		world->player_control= true;
		float newleft= capm(world->player_left + 0.05 + (world->player_right - world->player_left)*0.25, -1, 1);
		world->player_right= capm(world->player_right + 0.05 + (world->player_left - world->player_right)*0.25, -1, 1);
		world->player_left= newleft;
	} else if (key==115 && world->getSelectedID()!=-1) { //s (move slower/reverse)
		world->player_control= true;
		float newleft= capm(world->player_left - 0.05 + (world->player_right - world->player_left)*0.25, -1, 1);
		world->player_right= capm(world->player_right - 0.05 + (world->player_left - world->player_right)*0.25, -1, 1);
		world->player_left= newleft;
	} else if (key==97 && world->getSelectedID()!=-1) { //a (turn left)
		world->player_control= true;
		if (world->player_left > world->player_right) {
			float avg= (world->player_left + world->player_right) * 0.5;
			world->player_right= avg;
			world->player_left= avg;
		} else {
			float newleft= capm(world->player_left - 0.01 - abs(world->player_right - world->player_left)*0.25, -1, 1);
			world->player_right= capm(world->player_right + 0.01 + abs(world->player_left - world->player_right)*0.25, -1, 1);
			world->player_left= newleft;
		}
	} else if (key==100 && world->getSelectedID()!=-1) { //d (turn right)
		world->player_control= true;
		if(world->player_left < world->player_right) {
			float avg= (world->player_left + world->player_right) * 0.5;
			world->player_right= avg;
			world->player_left= avg;
		} else {	
			float newleft= capm(world->player_left + 0.01 + abs(world->player_right - world->player_left)*0.25, -1, 1);
			world->player_right= capm(world->player_right - 0.01 - abs(world->player_left - world->player_right)*0.25, -1, 1);
			world->player_left= newleft;
		}
	} else if (key==999) { //player control
		world->setControl(!world->player_control);
		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->player_control) glutChangeToMenuEntry(1, "Release Agent", 999);
		else glutChangeToMenuEntry(1, "Control Selected (w,a,s,d)", 999);
		glutSetMenu(m_id);

	} else if (key=='r') { //r key is now a defacto debugging key for whatever I need at the time
		world->cellsRoundTerrain();

	} else if (key==127) { //delete
		world->selectedKill();
	} else if (key=='/') { // / heal selected
		world->selectedHeal();
	} else if (key=='|') { // | reproduce selected
		world->selectedBabys();
	} else if (key=='~') { // ~ mutate selected
		world->selectedMutate();
	} else if (key=='`') { // ` rotate selected stomach type
		world->selectedStomachMut();

	// MENU ONLY OPTIONS:
	} else if (key==1001 || key==']') { //add agents (okay I lied this one isn't menu only)
		world->addAgents(5);
	} else if (key==1002) { //add Herbivore agents
		world->addAgents(5, Stomach::PLANT);
	} else if (key==1003) { //add Carnivore agents
		world->addAgents(5, Stomach::MEAT);
	} else if (key==1004) { //add Frugivore agents
		world->addAgents(5, Stomach::FRUIT);
	}else if (key==1005) { //debug mode
		world->setDebug( !world->isDebug() );
		live_debug= (int) world->isDebug();
/*		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->isDebug()){
			glutChangeToMenuEntry(18, "Exit Debug Mode", 1005);
			printf("Entered Debug Mode\n");
		} else glutChangeToMenuEntry(18, "Enter Debug Mode", 1005);
		glutSetMenu(m_id);*/
	}else if (key==1006) { //force-reset config
		world->writeConfig();
	}else if (key==1007) { // reset (new world)
		world->setDemo(false);
		world->reset();
		world->spawn();
		printf("WORLD RESET!\n");
		world->addEvent("World Started!", EventColor::MINT);
	} else if (key==1008) { //save world
		handleRW(RWOpen::BASICSAVE);
	} else if (key==1009) { //load world
		handleRW(RWOpen::STARTLOAD);
	} else if (key==1010) { //reload config
		world->readConfig();
		//.cfg/.sav Update live variables
		live_demomode= (int)(world->isDemo());
		live_worldclosed= (int)(world->isClosed());
		live_landspawns= (int)(world->DISABLE_LAND_SPAWN);
		live_moonlight= (int)(world->MOONLIT);
		live_droughts= (int)(world->DROUGHTS);
		live_climate= (int)(world->CLIMATE);
		live_mutevents= (int)(world->MUTEVENTS);
	} else if (key==1011) { //sanitize world (delete agents)
		world->sanitize();
	} else if (key==1012) { //print agent stats
		world->selectedPrint();
	} else if (key==1013) { //toggle demo mode
		world->setDemo(!world->isDemo());
	} else if (key>=1048 && key<1057) { //shift + number keys: select mode (cont)
		if(live_selection!=key-1037) live_selection= key-1037; //1047+10; 
		else live_selection= Select::NONE;
	} else {
		#if defined(_DEBUG)
		printf("Unmatched key pressed: %i\n", key);
		#endif
	}
}

void GLView::menuSpecial(int key) // special control keys
//Control.cpp
{
	if (key==GLUT_KEY_UP) {
	   ytranslate+= 20/scalemult;
	   live_follow= 0;
	} else if (key==GLUT_KEY_LEFT) {
		xtranslate+= 20/scalemult;
		live_follow= 0;
	} else if (key==GLUT_KEY_DOWN) {
		ytranslate-= 20/scalemult;
		live_follow= 0;
	} else if (key==GLUT_KEY_RIGHT) {
		xtranslate-= 20/scalemult;
		live_follow= 0;
	} else if (key==GLUT_KEY_PAGE_DOWN) {
		if(world->setSelectionRelative(-1)) live_selection= Select::MANUAL;
	} else if (key==GLUT_KEY_PAGE_UP) {
		if(world->setSelectionRelative(1)) live_selection= Select::MANUAL;
	} else if (key==GLUT_KEY_END) {
		world->selectedPrint();
	}
}

void GLView::glCreateMenu()
//Control.cpp ???
{
	//right-click context menu and submenus
	sm1_id= glutCreateMenu(gl_menu); //configs & UI
	glutAddMenuEntry("Dismiss Visible Events (n)", 'n');
	glutAddMenuEntry("Toggle Demo Mode", 1013);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Toggle Debug Mode", 1005);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Re(over)write Config", 1006);
	glutAddMenuEntry("Load Config", 1010);

	sm2_id= glutCreateMenu(gl_menu); //spawn more
	glutAddMenuEntry("Agents (])", 1001);
	glutAddMenuEntry("Herbivores", 1002);
	glutAddMenuEntry("Carnivores", 1003);
	glutAddMenuEntry("Frugivores", 1004);

	sm3_id= glutCreateMenu(gl_menu); //selected agent
	glutAddMenuEntry("Toggle Control (w,a,s,d)", 999);
	glutAddMenuEntry("debug Printf details", 1012);
	glutAddMenuEntry("debug Heal (/)", '/');
	glutAddMenuEntry("debug Reproduce (|)", '|');
	glutAddMenuEntry("debug Mutate (~)", '~');
	glutAddMenuEntry("debug Delete (del)", 127);

	sm4_id= glutCreateMenu(gl_menu); //world control
	glutAddMenuEntry("Load World",1009);
	glutAddMenuEntry("Toggle Closed (c)", 'c');
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Sanitize World", 1011);
	glutAddMenuEntry("New World", 1007);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Exit (esc)", 27);

	m_id = glutCreateMenu(gl_menu); 
	glutAddMenuEntry("Fast Mode (m)", 'm');
	glutAddSubMenu("UI & Config...", sm1_id);
	glutAddMenuEntry("Save World", 1008);
	glutAddSubMenu("Alter World...", sm4_id);
	glutAddSubMenu("Spawn New...", sm2_id);
	glutAddSubMenu("Selected Agent...", sm3_id);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void GLView::gluiCreateMenu()
//Control.cpp (only if not immediately switching UI systems)
{
	//GLUI menu. Slimy, yet satisfying.
	//must set our init live vars to something. Might as well do it here
	live_worldclosed= 0;
	live_paused= 0;
	live_playmusic= 1;
	live_playsounds= 1;
	live_fastmode= 0;
	live_skipdraw= 1;
	live_agentsvis= Visual::RGB;
	for(int i=0; i<DisplayLayer::DISPLAYS; i++) live_layersvis[i]= 1;
	live_waterfx= 1;
	live_profilevis= Profile::NONE;
	live_worlddetails = 0;
	live_lifepath= 0;
	live_selection= Select::MANUAL;
	live_follow= 0;
	live_autosave= 1;
	live_debug= world->isDebug();
	live_grid= 0;
	live_hidedead= 0;
	live_hidegenz= 0;
	live_demomode= (int)world->isDemo();
	live_landspawns= (int)world->DISABLE_LAND_SPAWN;
	live_moonlight= (int)world->MOONLIT;
	live_oceanpercent= world->OCEANPERCENT;
	live_droughts= (int)world->DROUGHTS;
	live_droughtmult= world->DROUGHTMULT;
	live_mutevents= (int)world->MUTEVENTS;
	live_climate= (int)world->CLIMATE;
	live_climatebias= world->CLIMATEBIAS;
	live_climatemult= world->CLIMATEMULT;
	live_cursormode= MouseMode::SELECT;
	char text[32]= "";

	#if defined(_DEBUG) //disable some extra features in debug environment
		live_autosave= 0;
		live_playmusic= 0;
	#endif

	//create GLUI and add the options, be sure to connect them all to their real vals later
	Menu = GLUI_Master.create_glui("Menu",0,1,1);
	Menu->add_checkbox("Pause",&live_paused);
	Menu->add_checkbox("Enable Autosaves",&live_autosave);

	GLUI_Panel *panel_speed= new GLUI_Panel(Menu,"Speed Control");
	Menu->add_spinner_to_panel(panel_speed,"Speed:",GLUI_SPINNER_INT,&live_skipdraw)->set_speed(0.3); //wait...
	Menu->add_checkbox_to_panel(panel_speed,"Fast Mode",&live_fastmode);

	GLUI_Rollout *rollout_world= new GLUI_Rollout(Menu,"World Options",false);

	Menu->add_button_to_panel(rollout_world,"Load World", RWOpen::STARTLOAD, glui_handleRW);
	Menu->add_button_to_panel(rollout_world,"Save World", RWOpen::BASICSAVE, glui_handleRW);
	Menu->add_button_to_panel(rollout_world,"New World", RWOpen::NEWWORLD, glui_handleRW);
	Menu->add_spinner_to_panel(rollout_world,"Ocean gen. ratio:",GLUI_SPINNER_FLOAT,&live_oceanpercent)->set_float_limits(0.0,1.0); //whoa...
	Menu->add_checkbox_to_panel(rollout_world,"Disable Spawns",&live_worldclosed);
	Menu->add_checkbox_to_panel(rollout_world,"Disable Land Spawns",&live_landspawns);
	Menu->add_checkbox_to_panel(rollout_world,"Moon Light",&live_moonlight);
	Menu->add_checkbox_to_panel(rollout_world,"Climate Change",&live_climate);
	Menu->add_spinner_to_panel(rollout_world,"Climate Bias:",GLUI_SPINNER_FLOAT,&live_climatebias)->set_float_limits(0.0,1.0);
	Menu->add_spinner_to_panel(rollout_world,"Climate Mult:",GLUI_SPINNER_FLOAT,&live_climatemult)->set_float_limits(0.0,1.0);
	Menu->add_checkbox_to_panel(rollout_world,"Global Drought Events",&live_droughts);
	Menu->add_spinner_to_panel(rollout_world,"Drought Mod:",GLUI_SPINNER_FLOAT,&live_droughtmult)->set_speed(0.1); //...this is weird
	Menu->add_checkbox_to_panel(rollout_world,"Mutation Events",&live_mutevents);

	GLUI_Rollout *rollout_vis= new GLUI_Rollout(Menu,"Visuals");

/*	GLUI_RadioGroup *group_layers= new GLUI_RadioGroup(rollout_vis,&live_layersvis);
	new GLUI_StaticText(rollout_vis,"Layer");
	for(int i=0; i<LayerPreset::DISPLAYS; i++){
		//this code allows the layers to be defined in any order
		char text[16]= "";
		if(i==LayerPreset::NONE) strcpy(text, "off");
		else if(i==LayerPreset::PLANTS) strcpy(text, "Plant");
		else if(i==LayerPreset::MEATS) strcpy(text, "Meat");
		else if(i==LayerPreset::HAZARDS) strcpy(text, "Hazard");
		else if(i==LayerPreset::FRUITS) strcpy(text, "Fruit");
		else if(i==LayerPreset::REALITY) strcpy(text, "Reality!");
		else if(i==LayerPreset::LIGHT) strcpy(text, "Light");
		else if(i==LayerPreset::TEMP) strcpy(text, "Temp");
		else if(i==LayerPreset::ELEVATION) strcpy(text, "Elevation");

		new GLUI_RadioButton(group_layers,text);
	} //old radio group system for displaying layers*/

	GLUI_Panel *panel_layers= new GLUI_Panel(rollout_vis,"Layers");
	
	for(int i=0; i<DisplayLayer::DISPLAYS; i++){
		//this code allows the layers to be defined in any order
		if(i==DisplayLayer::PLANTS) strcpy(text, "Plant");
		else if(i==DisplayLayer::MEATS) strcpy(text, "Meat");
		else if(i==DisplayLayer::HAZARDS) strcpy(text, "Hazard");
		else if(i==DisplayLayer::FRUITS) strcpy(text, "Fruit");
		else if(i==DisplayLayer::LIGHT) strcpy(text, "Light");
		else if(i==DisplayLayer::TEMP)strcpy(text, "Temp");
		else if(i==DisplayLayer::ELEVATION) strcpy(text, "Elevation");
		else strcpy(text, "UNKNOWN_DisplayLayer");
		
		Menu->add_checkbox_to_panel(panel_layers, text, &live_layersvis[i]);
	}
	Menu->add_button_to_panel(panel_layers,"Toggle All", GUIButtons::TOGGLE_LAYERS, glui_handleButtons);

	GLUI_RadioGroup *group_profiles= new GLUI_RadioGroup(rollout_vis,&live_profilevis);
	new GLUI_StaticText(rollout_vis,"Selected");
	new GLUI_Separator(rollout_vis);

	for(int i=0; i<Profile::PROFILES; i++){
		if(i==Profile::NONE) strcpy(text, "off");
		else if(i==Profile::EYES) strcpy(text, "Eyesight");
		else if(i==Profile::INOUT) strcpy(text, "In's/Out's");
		else if(i==Profile::BOXES) strcpy(text, "Boxes&IO");
		else if(i==Profile::BRAIN) strcpy(text, "Brain");
		else if(i==Profile::SOUND) strcpy(text, "Sounds");
		else if(i==Profile::METABOLISM) strcpy(text, "Metabolism");
		else strcpy(text, "UNKNOWN_Profile");
		
		new GLUI_RadioButton(group_profiles,text);
	}

	Menu->add_checkbox_to_panel(rollout_vis, "Grid on", &live_grid);
	Menu->add_checkbox_to_panel(rollout_vis, "Show Lifepath", &live_lifepath);
	Menu->add_checkbox_to_panel(rollout_vis, "Water FX", &live_waterfx);
	Menu->add_checkbox_to_panel(rollout_vis, "Extra Data", &live_worlddetails);

	Menu->add_column_to_panel(rollout_vis,true);
	GLUI_RadioGroup *group_agents= new GLUI_RadioGroup(rollout_vis, &live_agentsvis);
	new GLUI_StaticText(rollout_vis,"Agents");
	new GLUI_Separator(rollout_vis);

	for(int i=0; i<Visual::VISUALS; i++){
		if(i==Visual::NONE) strcpy(text, "off");
		else if(i==Visual::RGB) strcpy(text, "RGB");
		else if(i==Visual::STOMACH) strcpy(text, "Stomach");
		else if(i==Visual::DISCOMFORT) strcpy(text, "Discomfort");
		else if(i==Visual::VOLUME) strcpy(text, "Volume");
		else if(i==Visual::SPECIES) strcpy(text, "Species ID");
		else if(i==Visual::CROSSABLE) strcpy(text, "Crossable");
		else if(i==Visual::HEALTH) strcpy(text, "Health");
		else if(i==Visual::REPCOUNTER) strcpy(text, "Rep. Counter");
		else if(i==Visual::METABOLISM) strcpy(text, "Metabolism");
		else if(i==Visual::STRENGTH) strcpy(text, "Strength");
		else if(i==Visual::BRAINMUTATION) strcpy(text, "Brain Mut.");
		else if(i==Visual::GENEMUTATION) strcpy(text, "Gene Mut.");
		else if(i==Visual::LUNGS) strcpy(text, "Lungs");
		else if(i==Visual::GENERATIONS) strcpy(text, "Generation");
		else if(i==Visual::AGE_HYBRID) strcpy(text, "Age/Hybrids");
//		else if(i==Visual::REPMODE) strcpy(text, "Rep. Mode");
		else strcpy(text, "UNKNOWN_Visual");

		new GLUI_RadioButton(group_agents, text);
	}
	Menu->add_checkbox_to_panel(rollout_vis, "Hide Dead", &live_hidedead);
	Menu->add_checkbox_to_panel(rollout_vis, "Hide Gen 0", &live_hidegenz);
	
	GLUI_Rollout *rollout_xyl= new GLUI_Rollout(Menu,"Selection Mode",false);
	GLUI_RadioGroup *group_select= new GLUI_RadioGroup(rollout_xyl,&live_selection);

	for(int i=0; i<Select::SELECT_TYPES; i++){
		if(i==Select::OLDEST) strcpy(text, "Oldest");
		else if(i==Select::BEST_GEN) strcpy(text, "Best Gen.");
		else if(i==Select::HEALTHY) strcpy(text, "Healthiest");
		else if(i==Select::ENERGETIC) strcpy(text, "Most Energetic");
		else if(i==Select::PRODUCTIVE) strcpy(text, "Productive");
		else if(i==Select::AGGRESSIVE) strcpy(text, "Aggressive");
		else if(i==Select::NONE) strcpy(text, "off");
		else if(i==Select::MANUAL) strcpy(text, "Manual");
		else if(i==Select::RELATIVE) strcpy(text, "Relatives");
		else if(i==Select::BEST_HERBIVORE) strcpy(text, "Best Herbivore");
		else if(i==Select::BEST_FRUGIVORE) strcpy(text, "Best Frugivore");
		else if(i==Select::BEST_CARNIVORE) strcpy(text, "Best Carnivore");
		else if(i==Select::BEST_AQUATIC) strcpy(text, "Best Aquatic");
		else if(i==Select::BEST_AMPHIBIAN) strcpy(text, "Best Amphibian");
		else if(i==Select::BEST_TERRESTRIAL) strcpy(text, "Best Terran");
		else if(i==Select::FASTEST) strcpy(text, "Fastest");
		else if(i==Select::SEXIEST) strcpy(text, "Sexiest");
		else if(i==Select::GENEROUS_EST) strcpy(text, "Most Generous");
		else if(i==Select::KINRANGE_EST) strcpy(text, "Largest Kinrange");
		else strcpy(text, "UNKNOWN_Select");

		new GLUI_RadioButton(group_select,text);
		if(i==Select::NONE) Menu->add_checkbox_to_panel(rollout_xyl, "Follow Selected", &live_follow);
	}
	new GLUI_Separator(rollout_xyl);
	Menu->add_button_to_panel(rollout_xyl, "Save Selected", RWOpen::BASICSAVEAGENT, glui_handleRW);
	LoadAgentButton= Menu->add_button_to_panel(rollout_xyl, "Load Agent", RWOpen::BASICLOADAGENT, glui_handleRW);

	Menu->add_checkbox("Play Music",&live_playmusic);
	Menu->add_checkbox("Play Sounds",&live_playsounds);
	Menu->add_checkbox("Demo Mode",&live_demomode);
	Menu->add_checkbox("DEBUG",&live_debug);

	//set to main graphics window
	Menu->set_main_gfx_window(win1);
}


void GLView::handleButtons(int action)
//Control.cpp (only if not immediately switching UI systems)
{
	if (action == GUIButtons::TOGGLE_LAYERS) {
		if(ui_layerpreset != 0) ui_layerpreset = 0;
		else ui_layerpreset = DisplayLayer::DISPLAYS*2;

		processLayerPreset(); //call to method to update layer bools based on ui_layerpreset

	} else if (action == GUIButtons::TOGGLE_PLACEAGENTS) {
		if(live_cursormode != MouseMode::PLACE_AGENT) live_cursormode = MouseMode::PLACE_AGENT;
		else live_cursormode = MouseMode::SELECT;
	}
}


//====================================== READ WRITE ===========================================//

void GLView::handleRW(int action) //glui callback for saving/loading worlds
//Control.cpp (only if not immediately switching UI systems)
{
	live_paused= 1; //pause world while we work

	if (action == RWOpen::STARTLOAD){ //basic load option selected
		//Step 1 of loading: check if user really wants to reset world
		if(!world->isDemo()){ //no need for alerts if in demo mode (epoch == 0)
			//get time since last save
			int time= currentTime;

			time-= lastsavedtime;
			if(time>=0 && time<30000) handleRW(RWOpen::BASICLOAD); // if we saved a mere few seconds ago, skip to loading anyway
			else {
				std::string timetype;
				if(time<60000) { timetype= "second"; time/=1000; }
				else if(time<3600000) { timetype= "minute"; time/=60000; }
				else if(time<86400000) { timetype= "hour"; time/=3600000; }
				else { timetype= "day"; time/=86400000; }

				if(time>1) timetype+= "s";

				sprintf(buf,"This will erase the current world, last saved ~%d %s ago.", time, timetype.c_str());
				printf("%s", buf);

				Alert = GLUI_Master.create_glui("Alert",0,50,50);
				Alert->show();
				new GLUI_StaticText(Alert,"");
				new GLUI_StaticText(Alert,"Are you sure?");
				new GLUI_StaticText(Alert,buf);
				new GLUI_StaticText(Alert,"");
				new GLUI_Button(Alert,"Okay",RWOpen::IGNOREOVERLOAD, glui_handleRW);
				new GLUI_Button(Alert,"Cancel", RWClose::CANCELALERT, glui_handleCloses);

				Alert->set_main_gfx_window(win1);
			}
		} else handleRW(RWOpen::BASICLOAD);

	} else if (action == RWOpen::IGNOREOVERLOAD) {
		//alert window open before load, was ignored by user. Close it first before loading
		Alert->hide();
		handleRW(RWOpen::BASICLOAD);

	} else if (action == RWOpen::BASICLOAD) { //load option promt
		Loader = GLUI_Master.create_glui("Load World",0,50,50);

		new GLUI_StaticText(Loader,"");
		Filename= new GLUI_EditText(Loader,"Enter Save File Name (e.g, 'WORLD'):");
		new GLUI_StaticText(Loader,"");
		Filename->set_w(300);
		new GLUI_Button(Loader,"Load", RWClose::BASICLOAD, glui_handleCloses);
		new GLUI_Button(Loader,"Cancel", RWClose::CANCELLOAD, glui_handleCloses);

		Loader->set_main_gfx_window(win1);

	} else if (action == RWOpen::BASICSAVE) { //save option prompt
		Saver = GLUI_Master.create_glui("Save World",0,50,50);

		new GLUI_StaticText(Saver,"");
		Filename= new GLUI_EditText(Saver,"Type Save File Name (e.g, 'WORLD'):");
		new GLUI_StaticText(Saver,"");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save", RWClose::BASICSAVE, glui_handleCloses);
		new GLUI_Button(Saver,"Cancel", RWClose::CANCELSAVE, glui_handleCloses);

		Saver->set_main_gfx_window(win1);

	} else if (action == RWOpen::NEWWORLD){ //new world alert prompt
		Alert = GLUI_Master.create_glui("Alert",0,50,50);
		Alert->show();
		new GLUI_StaticText(Alert,"");
		new GLUI_StaticText(Alert,"Are you sure?");
		new GLUI_StaticText(Alert,"This will erase the current world and start a new one."); 
		new GLUI_StaticText(Alert,"It will load with any changes you made to the config file.");
		new GLUI_StaticText(Alert,"");
		GLUI_Button *active= new GLUI_Button(Alert,"Okay", RWClose::NEWWORLD, glui_handleCloses);
		new GLUI_Button(Alert,"Cancel", RWClose::CANCELALERT, glui_handleCloses);

		Alert->set_main_gfx_window(win1);

	} else if (action == RWOpen::BASICSAVEAGENT) { //save selected agent prompt
		Saver = GLUI_Master.create_glui("Save Agent",0,50,50);

		new GLUI_StaticText(Saver,"");
		Filename = new GLUI_EditText(Saver,"Enter Agent File Name (e.g, 'AGENT'):");
		new GLUI_StaticText(Saver,"");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save", RWClose::BASICSAVEAGENT, glui_handleCloses);
		new GLUI_Button(Saver,"Cancel", RWClose::CANCELSAVE, glui_handleCloses);

	} else if (action == RWOpen::BASICLOADAGENT) {
		if(live_cursormode != MouseMode::PLACE_AGENT){
			Loader = GLUI_Master.create_glui("Load Agent",0,50,50);

			new GLUI_StaticText(Loader,"");
			Filename = new GLUI_EditText(Loader,"Enter Agent File Name (e.g, 'AGENT'):");
			new GLUI_StaticText(Loader,"");
			Filename->set_w(300);
			new GLUI_Button(Loader,"Load", RWClose::BASICLOADAGENT, glui_handleCloses);
			new GLUI_Button(Loader,"Cancel", RWClose::CANCELLOAD, glui_handleCloses);

			Loader->set_main_gfx_window(win1);
		} else {
			live_cursormode = MouseMode::SELECT; //return cursor mode to normal if we press button while in placement mode
			live_paused = 0;
			LoadAgentButton->set_name("Load Agent");
		}

	} else if (action==RWOpen::ADVANCEDLOADAGENT) {
		Loader = GLUI_Master.create_glui("Load Agent",0,50,50);
		//this thing is all sorts of broken.

		new GLUI_StaticText(Loader,"");
		//Browser= 
		new GLUI_FileBrowser(Loader,"test");
//		Browser->fbreaddir("\\saved_agents");
//		Browser->set_allow_change_dir(0);
//		new GLUI_Button(Loader,"Load", RWClose::ADVANCEDLOADAGENT, glui_handleCloses);
		new GLUI_Button(Loader,"Cancel", RWClose::CANCELLOAD, glui_handleCloses);

		Loader->set_main_gfx_window(win1);

	} else if (action==RWOpen::ADVANCEDLOAD) { //advanced loader (NOT FUNCTIONING)
		Loader = GLUI_Master.create_glui("Load World",0,50,50);

		Browser= new GLUI_FileBrowser(Loader,"X",false);
//		Browser->fbreaddir("\\saves");
//		Browser->set_allow_change_dir(0);
//		new GLUI_Button(Loader,"Load", RWClose::BASICLOAD, glui_handleCloses);
//		new GLUI_Button(Loader,"Cancel", RWClose::CANCELLOAD, glui_handleCloses);

		Loader->set_main_gfx_window(win1);
	}
}

void GLView::handleCloses(int action) //GLUI callback for handling window closing
//Control.cpp (only if not immediately switching UI systems)
{
	live_paused= 0;

	if (action==RWClose::BASICLOAD) { //loading
		//Step 2: actual loading
		strcpy(filename,Filename->get_text());
		
		if (checkFileName(filename)) {
			//file name is valid, but is it a valid file???
			std::string address;
			address= "saves\\";
			address.append(filename);
			address.append(R::WORLD_SAVE_EXT);

			FILE *fl;
			fl= fopen(address.c_str(), "r");
			if(!fl) { //file doesn't exist. Alert the user
				Alert = GLUI_Master.create_glui("Alert",0,50,50);

				sprintf(buf,"No file could be found with the name \"%s%s\".\n", filename, R::WORLD_SAVE_EXT.c_str());
				printf("%s", buf);

				new GLUI_StaticText(Alert,"");
				new GLUI_StaticText(Alert, buf); 
				new GLUI_StaticText(Alert, "Returning to main program." );
				new GLUI_StaticText(Alert,"");
				new GLUI_Button(Alert,"Okay", RWClose::CANCELALERT, glui_handleCloses);
				Alert->show();

			} else {
				fclose(fl); //we are technically going to open the file again in savehelper... oh well

				savehelper->loadWorld(world, xtranslate, ytranslate, scalemult, filename);

				lastsavedtime= currentTime; //reset last saved time; we have nothing before the load to save!

				syncLiveWithWorld();
			}
		}
		Loader->hide();

	} else if (action==RWClose::BASICSAVE) { //saving
		trySaveWorld();
		Saver->hide();

	} else if (action==RWClose::NEWWORLD) { //resetting
		world->setDemo(false);
		world->reset();
		world->spawn();
		lastsavedtime= currentTime;
		printf("WORLD RESET!\n");
		world->addEvent("World Started!", EventColor::MINT);
		//this is getting annoying - so if the world changes any values that have a GUI, the GUI will not update. We have to force update here
		syncLiveWithWorld();
		Alert->hide();
	} else if (action==RWClose::CANCELALERT) { //Alert cancel/continue
		Alert->hide();
	} else if (action==RWClose::BASICSAVEOVERWRITE) { //close alert from save overwriting
		trySaveWorld(true,false);
		Alert->hide();
	} else if (action==RWClose::BASICSAVEAGENT) { //saving agents
		strcpy(filename, Filename->get_text());

		if (checkFileName(filename)) {
			//check the filename given to see if it exists yet
			std::string address;
			address= "saved_agents\\";
			address.append(filename);
			address.append(R::AGENT_SAVE_EXT);

//			FILE* ck = fopen(address, "r");
//			if(ck){
//				if (debug) printf("WARNING: %s already exists!\n", filename);
//
//				Alert = GLUI_Master.create_glui("Alert",0,50,50);
//				Alert->show();
//				new GLUI_StaticText(Alert, "The file name given already exists." );
//				new GLUI_StaticText(Alert, "Would you like to overwrite?" );
//				new GLUI_Button(Alert,"Okay",7, glui_handleCloses);
//				new GLUI_Button(Alert,"Cancel",4, glui_handleCloses);
//				live_paused= 1;
//				fclose(ck);
//			} else {
//				fclose(ck);
				FILE* sa = fopen(address.c_str(), "w");
				int sidx= world->getSelectedAgentIndex();
				if (sidx>=0){
					Agent *a= &world->agents[sidx];
					savehelper->saveAgent(a, sa);
					std::string selectedsaved= "Agent saved as \"";
					selectedsaved.append(filename);
					selectedsaved.append("\"");
					world->addEvent(selectedsaved, EventColor::WHITE);
				}
				fclose(sa);
//			}
		}
		Saver->hide();
	} else if (action == RWClose::BASICLOADAGENT){ //basic agent loader
		strcpy(filename, Filename->get_text());
				
		if (checkFileName(filename)) {
			//file name is valid, but is it a valid file???
			std::string address;
			address= "saved_agents\\";
			address.append(filename);
			address.append(R::AGENT_SAVE_EXT);

			FILE *fl;
			fl= fopen(address.c_str(), "r");
			if(!fl) { //file doesn't exist. Alert the user
				Alert = GLUI_Master.create_glui("Alert",0,50,50);

				sprintf(buf,"No file could be found with the name \"%s.AGT\".", filename);
				printf("%s", buf);

				new GLUI_StaticText(Alert,"");
				new GLUI_StaticText(Alert, buf); 
				new GLUI_StaticText(Alert, "Returning to main program." );
				new GLUI_StaticText(Alert,"");
				new GLUI_Button(Alert,"Okay", RWClose::CANCELALERT, glui_handleCloses);
				Alert->show();

			} else {
				fclose(fl); //we are technically going to open the file again in savehelper... oh well

				savehelper->loadAgentFile(world, address.c_str());

				live_cursormode = MouseMode::PLACE_AGENT; //activate agent placement mode
				LoadAgentButton->set_name("UNLOAD Agent");
			}
		}
		Loader->hide();

	} else if (action==RWClose::ADVANCEDLOAD) { //advanced loader
		file_name= "";
		file_name= Browser->get_file();
		strcpy(filename,file_name.c_str());

		if (checkFileName(filename)) {
			savehelper->loadWorld(world, xtranslate, ytranslate, scalemult, filename);
		}
		Loader->hide(); //NEEDS LOTS OF WORK

	} else if(action==RWClose::ADVANCEDLOADAGENT) { //advanced agent loader
		//Step 2: actual loading
		strcpy(filename,Browser->get_file());
		
		if (checkFileName(filename)) { //file name is valid, but is it a valid file???
			std::string address;
			address= "saved_agents\\";
			address.append(filename);
			address.append(R::AGENT_SAVE_EXT);

			FILE *fl;
			fl= fopen(address.c_str(), "r");
			if(!fl) { //file doesn't exist. Alert the user
				Alert = GLUI_Master.create_glui("Alert",0,50,50);

				sprintf(buf,"No file could be found with the name \"%s.AGT\".", filename);
				printf("%s", buf);

				new GLUI_StaticText(Alert,"");
				new GLUI_StaticText(Alert, buf); 
				new GLUI_StaticText(Alert, "Returning to main program." );
				new GLUI_StaticText(Alert,"");
				new GLUI_Button(Alert,"Okay", RWClose::CANCELALERT, glui_handleCloses);
				Alert->show();

			} else {
				fclose(fl); //we are technically going to open the file again in savehelper... oh well

				savehelper->loadAgentFile(world, address.c_str());
			}
		}
		Loader->hide();
	} else if(action==RWClose::CANCELLOADALERT) { //loader cancel from alert
		Loader->hide();
		Alert->hide();
	} else if(action==RWClose::CANCELLOAD) { //loader cancel
		Loader->hide();
	} else if(action==RWClose::CANCELSAVE) { //save cancel
		Saver->hide();
	}
}


bool GLView::checkFileName(char name[32])
//Control.cpp
{
	if (live_debug) printf("Filename: '%s'\n",name);
	if (!name || name=="" || name==NULL || name[0]=='\0'){
		printf("ERROR: empty filename; returning to program.\n");

		Alert = GLUI_Master.create_glui("Alert",0,50,50);
		Alert->show();
		new GLUI_StaticText(Alert,"");
		new GLUI_StaticText(Alert, "No file name given." );
		new GLUI_StaticText(Alert, "Returning to main program." );
		new GLUI_StaticText(Alert,"");
		new GLUI_Button(Alert,"Okay", RWClose::CANCELALERT, glui_handleCloses);
		return false;

	} else return true;
}

/*bool GLView::checkFileName(std::string name){

	if (debug) printf("File: '%s'\n",name);
	if (name.size()==0 || name==NULL || name[0]=='\0'){
		printf("ERROR: empty filename; returning to program.\n");

		Alert = GLUI_Master.create_glui("Alert",0,50,50);
		Alert->show();
		new GLUI_StaticText(Alert, "No file name given." );
		new GLUI_StaticText(Alert, "Returning to main program." );
		new GLUI_Button(Alert,"Okay",4, glui_handleCloses);
		return false;

	} else return true;
}
*/

void GLView::trySaveWorld(bool force, bool autosave)
//Control.cpp (only if not immediately switching UI systems)
{
	if(autosave) strcpy(filename, "AUTOSAVE");
	else strcpy(filename, Filename->get_text());

	if (checkFileName(filename)) {
		//check the filename given to see if it exists yet
		std::string address;
		address= "saves\\";
		address.append(filename);
		address.append(R::WORLD_SAVE_EXT);

		FILE* ck = fopen(address.c_str(), "r");
		if(ck && !force){
			if (live_debug) printf("WARNING: %s already exists!\n", filename);

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert,"");
			new GLUI_StaticText(Alert, "The file name given already exists." );
			new GLUI_StaticText(Alert, "Would you like to overwrite?" );
			new GLUI_StaticText(Alert,"");
			new GLUI_Button(Alert,"Okay", RWClose::BASICSAVEOVERWRITE, glui_handleCloses);
			new GLUI_Button(Alert,"Cancel", RWClose::CANCELALERT, glui_handleCloses);
			live_paused= 1;

			fclose(ck);
		} else {
			if(ck) fclose(ck);
			savehelper->saveWorld(world, xtranslate, ytranslate, scalemult, filename);
			lastsavedtime= currentTime;
			if(!autosave){
				if (!force) {
					std::string selectedsaved= "World saved as \"";
					selectedsaved.append(filename);
					selectedsaved.append("\"");
					world->addEvent(selectedsaved, EventColor::WHITE);
				} else world->addEvent("Saved World (overwritten)", EventColor::WHITE);
			}
		}
	}
}

void GLView::popupReset(float x, float y)
{
	popuptext.clear();
	popupxy[0]= x;
	popupxy[1]= y;
}

void GLView::popupAddLine(std::string line)
{
	line+= "\n";
	popuptext.push_back(line);
}


void GLView::createTile(UIElement &parent, int x, int y, int w, int h, std::string key, std::string title)
//Control.cpp
{
	//create a tile as a child of a given parent
	UIElement newtile(x,y,w,h,key,title);
	parent.addChild(newtile);

}

void GLView::createTile(int x, int y, int w, int h, std::string key, std::string title)
//Control.cpp
{
	//this creates a main tile with no parent (are you the parent?...)
	UIElement newtile(x,y,w,h,key,title);
	maintiles.push_back(newtile);
}

void GLView::createTile(UIElement &parent, int w, int h, std::string key, std::string title, bool d, bool r)
//Control.cpp
{
	int x,y;
	//this method is for creating new tiles as children of a given tile (must exist as tile, no NULLs like above) with special alignment rules

	int child= parent.children.size();
	if(child>0){
		// if another child already exists, inherit pos from it and shift according to flags
		x= parent.children[child-1].x + r*(parent.children[child-1].w + UID::TILEMARGIN) - !r*(UID::TILEMARGIN + w);
		y= parent.children[child-1].y + !r*d*(parent.children[child-1].h + UID::TILEMARGIN) - !r*!d*(UID::TILEMARGIN + h);
	} else {
		//align within parent according to flags
		x= parent.x + r*(parent.w - w - UID::TILEMARGIN) + !r*UID::TILEMARGIN;
		y= parent.y + d*(parent.h - h - UID::TILEMARGIN) + !d*UID::TILEMARGIN;
	}

	createTile(parent,x,y,w,h,key,title);
}


void GLView::checkTileListClicked(std::vector<UIElement> tiles, int mx, int my, int state)
//Control.cpp
{
	for(int ti= tiles.size()-1; ti>=0; ti--){ //iterate backwards to prioritize last-added buttons
		if(!tiles[ti].shown) continue;
		if(mx>tiles[ti].x && mx<tiles[ti].x+tiles[ti].w &&
			my>tiles[ti].y && my<tiles[ti].y+tiles[ti].h){
			//That's a click! update flag and go do stuff!!

			uiclicked= true; //this must be exposed to mouse state==0 so that it catches drags off of UI elements

			if(tiles[ti].clickable && state == 1){ //if clickable, process actions
				if(tiles[ti].key == "Follow") { 
					live_follow = 1-live_follow;
					ui_ladmode = LADVisualMode::GHOST;
				}
				else if(tiles[ti].key == "Damages") {
					if (ui_ladmode != LADVisualMode::DAMAGES) ui_ladmode = LADVisualMode::DAMAGES;
					else ui_ladmode = LADVisualMode::GHOST;
				}
				else if(tiles[ti].key=="Intake") {
					if (ui_ladmode != LADVisualMode::INTAKES) ui_ladmode = LADVisualMode::INTAKES;
					else ui_ladmode = LADVisualMode::GHOST;
				}
			}

			//finally, check our tile's sub-tiles list, and all their sub-tiles, etc.
			checkTileListClicked(tiles[ti].children, mx, my, state);
		}
	}
}


void GLView::processTiles()
//Control.cpp
{
	for(int ti= 0; ti<maintiles.size(); ti++){
		if(ti==MainTiles::LAD){
			//LAD: Loaded/seLected Agent Display
			if(world->getSelectedAgentIndex()<0 && live_cursormode != MouseMode::PLACE_AGENT) {
				maintiles[ti].hide(); continue; 
			} else maintiles[ti].show();

			for(int tichild= 0; tichild<maintiles[ti].children.size(); tichild++){
				std::string key= maintiles[ti].children[tichild].key;
				if(live_cursormode == MouseMode::PLACE_AGENT && (key=="Follow" || key=="Damages" || key=="Intake"))
					maintiles[ti].children[tichild].hide(); //hide the profiler buttons if we have an agent loaded
				else maintiles[ti].children[tichild].show();
			}

			//REALLY neat tucking display code that WON'T work because getSelectAgent returns -1 when nothing selected, but we want to keep rendering it's stuff
			//the solution may be to make the ghost a member of World...
			/*if(world->getSelectedAgentIndex()<0) { 
				if(maintiles[ti].y>-maintiles[ti].h) maintiles[ti].y--; //tuck it away when we lost a selected agent
				else { maintiles[ti].hide(); continue; } //hide and skip display completely if hidden

			} else {
				if(!maintiles[ti].shown) maintiles[ti].show(); //hide/unhide as needed
				if(maintiles[ti].y<UID::BUFFER) maintiles[ti].y++; //untuck
			}*/
		} 
	}
}


void GLView::countdownEvents()
//Control.cpp
{
	//itterate event counters when drawing, once per frame regardless of skipdraw
	int count= world->events.size();
	if(conf::EVENTS_DISP<count) count= conf::EVENTS_DISP;
	for(int i= 0; i<count; i++){
		world->events[i].second.second--;
	}
}


int GLView::getLayerDisplayCount()
{
	int count= 0;
	for(int i=0; i<DisplayLayer::DISPLAYS; i++){
		if(live_layersvis[i]==1) count++;
	}
	return count;
}


int GLView::getAgentRes(bool ghost){
	if(!ghost) return ceil((scalemult-0.25)*0.75+1);
	return 2; //force ghosts to render at resolution 2 (16 verts)
}


void GLView::syncLiveWithWorld()
{
	//.cfg/.sav make sure to put all saved world variables with GUI options here so they update properly!
	live_demomode= (int)world->isDemo();
	live_worldclosed= (int)world->isClosed();
	live_landspawns= (int)world->DISABLE_LAND_SPAWN;
	live_moonlight= (int)world->MOONLIT;
	//live_oceanpercent= world->OCEANPERCENT;
	live_droughts= (int)world->DROUGHTS;
	live_droughtmult= world->DROUGHTMULT;
	live_mutevents= (int)world->MUTEVENTS;
	live_climate= (int)world->CLIMATE;
	live_climatebias= world->CLIMATEBIAS;
	live_climatemult= world->CLIMATEMULT;
}

void GLView::handleIdle()
//Control.cpp
{
	//set proper window (we don't want to draw on nothing, now do we?!)
	if (glutGetWindow() != win1) glutSetWindow(win1); 

	#if defined(_DEBUG)
	if(world->isDebug()) printf("S"); //Start, or Sync, step
	#endif

	GLUI_Master.sync_live_all();

	//after syncing all the live vars with GLUI_Master, set the vars they represent to their proper values.
	world->setDemo(live_demomode);
	world->setClosed(live_worldclosed);
	world->DISABLE_LAND_SPAWN= (bool)live_landspawns;
	if((int)world->MOONLIT!=live_moonlight) {
		if(live_moonlight) world->addEvent("Moon formation event!", EventColor::BLACK);
		else world->addEvent("Moon, uh... explosion event?!", EventColor::BLACK);
		world->MOONLIT = (bool)live_moonlight;
	}
	world->OCEANPERCENT= live_oceanpercent;
	world->DROUGHTS= (bool)live_droughts;
	world->DROUGHTMULT= live_droughtmult;
	world->MUTEVENTS= (bool)live_mutevents;
	world->CLIMATE= (bool)live_climate;
	world->CLIMATEBIAS= live_climatebias;
	world->CLIMATEMULT= live_climatemult;
	world->domusic= (bool)live_playmusic;
	if (!live_fastmode) world->dosounds= (bool)live_playsounds;
	world->recordlifepath = (bool)live_lifepath;
	if (!live_lifepath) world->lifepath.clear();
	world->setDebug((bool) live_debug);

	#if defined(_DEBUG)
	if(world->isDebug()) printf("/");
	#endif

	if(world->getEpoch()==0){
		if(world->modcounter==0){
			//do some spare actions if this is the first tick of the game (Main Menu here???) or if we just reloaded
			gotoDefaultZoom();
			past_agentsvis= live_agentsvis;

			//create the UI tiles 
			//I don't like destroying and creating these live, but for reasons beyond me, initialization won't accept variables like glutGet()
			int ladheight= 3*UID::BUFFER + ((int)ceil((float)LADHudOverview::HUDS*0.333333))*(UID::CHARHEIGHT + UID::BUFFER);
			//The LAD requires a special height measurement based on the Hud construct, expanding it easily if more readouts are added

			maintiles.clear();
			for(int i=0; i<MainTiles::TILES; i++){
				if(i==MainTiles::LAD) {
					createTile(wWidth-UID::LADWIDTH-UID::BUFFER, UID::BUFFER, UID::LADWIDTH, ladheight, "");
					createTile(maintiles[MainTiles::LAD], UID::TINYTILEWIDTH, UID::TINYTILEWIDTH, "Follow", "F", true, false);
					createTile(maintiles[MainTiles::LAD], UID::TINYTILEWIDTH, UID::TINYTILEWIDTH, "Damages", "D", false, true);
					createTile(maintiles[MainTiles::LAD], UID::TINYTILEWIDTH, UID::TINYTILEWIDTH, "Intake", "I", false, true);
				} else if(i==MainTiles::TOOLBOX) {
					createTile(UID::BUFFER, 190, 50, 300, "", "Tools");
					createTile(maintiles[MainTiles::TOOLBOX], UID::BUFFER+UID::TILEMARGIN, 215, 30, 30, "UnpinUI", "UI.");
					createTile(maintiles[MainTiles::TOOLBOX], 30, 30, "test", "test", true, false);
					
					maintiles[MainTiles::TOOLBOX].hide();
				}
			}
		}

		//display tips
		if(!live_fastmode){ //not while in fast mode
			if(!world->NO_TIPS || world->isDebug()){ //and not if world says NO or is in debug mode
				if(world->events.size() < conf::EVENTS_DISP && world->modcounter % conf::TIPS_PERIOD == conf::TIPS_DELAY) {
					//don't add tips if we got a lot of events showing
					int rand_tip = randi(0,world->tips.size());
					int tip_for_modcounter = (int)((float)world->modcounter/conf::TIPS_PERIOD);

					if(world->getDay() <= 2) {
						//based on the math of 2 day * 8000 ticks / day * 1 tip / TIPS_PERIOD ticks, we get 20 tips in linear fashion
						int selected_tip = tip_for_modcounter < world->tips.size() ? tip_for_modcounter : rand_tip;
						world->addEvent( world->tips[selected_tip].c_str(), EventColor::YELLOW );
					} else if (world->getDay() <= 7) {
						//the next 5 game days show the first few tips from the array of tips, but with some randomness possible
						int selected_tip = tip_for_modcounter < rand_tip ? tip_for_modcounter : rand_tip;
						world->addEvent( world->tips[selected_tip].c_str(), EventColor::YELLOW );
					} else if (world->modcounter % (conf::TIPS_PERIOD*2) == conf::TIPS_DELAY) {
						//the rest get displayed more rarely, until tips get turned off
						world->addEvent( world->tips[rand_tip].c_str(), EventColor::YELLOW );
					}
				}
			} else if (world->modcounter == conf::TIPS_DELAY) {
				//provide a tip about re-enabling tips
				world->addEvent("To re-enable tips, see settings.cfg", EventColor::YELLOW);
			}
		}
	}

	modcounter++;
	if (!live_paused) world->update();

	//pull back some variables which can change in-game that also have GUI selections
	syncLiveWithWorld();	

	//autosave world periodically, based on world time
	if (live_autosave==1 && !world->isDemo() && world->modcounter%(world->FRAMES_PER_DAY*10)==0){
		trySaveWorld(true,true);
	}

	//Do some recordkeeping and let's intelligently prevent users from simulating nothing
	if(live_worldclosed==1 && world->getAgents()<=0) {
		live_worldclosed= 0;
		live_autosave= 0; //I'm gonna guess you don't want to save the world anymore
		world->addEvent("Disabled Closed world, nobody was home!", EventColor::MINT);
	}

	//show FPS and other stuff
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	frames++;
	if ((currentTime - lastUpdate) >= 1000) {
		char ratemode;
		if(live_fastmode){
			ratemode= 'T';
			if(live_paused) frames= 0; //if we're paused and in fast mode, we're not exactly rendering any frames
			//we technically are rendering if just paused tho, so this check should stay here
		} else ratemode='F';

		sprintf( buf, "Evagents - %cPS: %d Alive: %d Epoch: %d Day %d",
			ratemode, frames, world->getAlive(), world->getEpoch(), world->getDay() );
		glutSetWindowTitle( buf );

		world->timenewsong--; //reduce our song delay timer
		frames = 0;
		lastUpdate = currentTime;
	}

	if (!live_fastmode) {
		if(live_playsounds) world->dosounds= true;

		if (live_skipdraw>0) {
			if (modcounter%live_skipdraw==0) renderScene();	//increase fps by skipping drawing
		}
		else { //we will decrease fps by waiting using clocks
			clock_t endwait;
			float mult=-0.008*(live_skipdraw-1); //negative b/c live_skipdraw is negative. ugly, ah well
			endwait = clock () + mult * CLOCKS_PER_SEC ;
			while (clock() < endwait) {}
			renderScene();
		}
	} else {
		//world->audio->stopAllSounds(); DON'T do this, too jarring to cut off all sounds
		world->dosounds= false;

		//in fast mode, our sim forgets to check nearest relative and reset the selection
		if(live_selection==Select::RELATIVE) world->setSelection(live_selection);
		//but we don't want to do this for all selection types because it slows fast mode down
	}

	glDisable(GL_POLYGON_SMOOTH);
	#if defined(_DEBUG)
	if(world->isDebug()){
		glEnable(GL_POLYGON_SMOOTH); //this will render thin lines on all polygons. It's a bug, but it's a feature!
		printf("\n"); //must occur at end of tick!
	}
	#endif

	isrendering_ = false; //use this to test if a ui is visible while running. It's for debugging
}

void GLView::renderScene()
{
	#if defined(_DEBUG)
	if(world->isDebug()) printf("D");
	#endif

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();

	glTranslatef(wWidth*0.5, wHeight*0.5, 0.0f);	
	glScalef(scalemult, scalemult, 1.0f);
	glTranslatef(xtranslate, ytranslate, 0.0f); //translate the view as needed

	//See declaration of scalemult for reference
	world->audio->setDefault3DSoundMinDistance(100.0f/(scalemult*0.5+1)); //IMPORTANT: this is a true radius that sounds play at 100% volume within
	world->audio->setListenerPosition(vec3df(xtranslate, ytranslate, 100.0f/scalemult), vec3df(0,0,-1)); //update the listener position. Note the z coord

	//handle world agent selection interface
	world->setSelection(live_selection);
	if (world->getSelectedID()==-1 && live_selection!=Select::MANUAL && live_selection!=Select::NONE) {
		live_selection= Select::NONE;
		world->addEvent("No valid Autoselect targets", EventColor::BLACK);
	}

	if(live_follow==1){ //if we're following,
		float xi=0, yi=0;
		world->getFollowLocation(xi, yi); //get the follow location from the world (location of selected agent)

		if(xi!=0 && yi!=0){
			// if we got to move the screen completly accross the world, jump instead
			if(abs(-xi-xtranslate)>0.95*conf::WIDTH || abs(-yi-ytranslate)>0.95*conf::HEIGHT){
				xtranslate= -xi; ytranslate= -yi;
			} else {
				float speed= conf::SNAP_SPEED;
				if(scalemult>0.5) speed= cap(speed*pow(scalemult + 0.5,3));
				xtranslate+= speed*(-xi-xtranslate); ytranslate+= speed*(-yi-ytranslate);
			}
		}
	}

	//update the popup text sparingly if it already exists
	if(popupxy[0]>=0 && popupxy[1]>=0 && world->modcounter%8==0) {
		handlePopup(mousex, mousey);
	}

	//update our UI elements
	processTiles();
	
	//DRAW THINGS!
	//Draw data layer on world. Don't cull this; it's important
	drawData(); 

	//draw cell layer. cull if the given cell is unseen
	if(getLayerDisplayCount()>0) {
		for(int cx=0;cx<world->CW;cx++) {
			for(int cy=0;cy<world->CH;cy++) {
				if(cullAtCoords(cx*conf::CZ, cy*conf::CZ)) continue;

				float values[Layer::LAYERS];

				for(int l=0;l<Layer::LAYERS; l++) {
					values[l]= world->cells[l][cx][cy];
				}
				drawCell(cx,cy,values);
			}
		}
	}

	drawFinalData();
	
	//draw all agents, also culled if unseen
	std::vector<Agent>::const_iterator it;
	for ( it = world->agents.begin(); it != world->agents.end(); ++it) {
		if(cullAtCoords(it->pos.x,it->pos.y)) continue;
		drawPreAgent(*it,it->pos.x,it->pos.y); //draw things before/under all agents
	}
	for ( it = world->agents.begin(); it != world->agents.end(); ++it) {
		if(cullAtCoords(it->pos.x,it->pos.y)) continue;
		drawAgent(*it,it->pos.x,it->pos.y); //draw agents themselves
	}

	//Draw static things, like UI and stuff. Never culled
	drawStatic();

	glPopMatrix();
	glutSwapBuffers();

	if(!live_paused) countdownEvents();

	#if defined(_DEBUG)
	if(world->isDebug()) printf("/");
	#endif
}



//---------------Start color section!-------------------//

Color3f GLView::setColorHealth(float health)
{
	Color3f color;
	float minorscalehealth = health/2;
	float majorscalehealth = health/world->HEALTH_CAP;
	color.red= ((int)(health*1000)%2==0) ? (1.0 - minorscalehealth) : 0;
	color.gre= health<=0.1 ? health : sqrt(minorscalehealth);
	color.blu= majorscalehealth*majorscalehealth;
	return color;
}

Color3f GLView::setColorStomach(const float stomach[Stomach::FOOD_TYPES])
{
	float plant= stomach[Stomach::PLANT];
	float fruit= stomach[Stomach::FRUIT];
	float meat= stomach[Stomach::MEAT];
	Color3f color;
	color.red= meat + fruit*0.9 - plant/4*(1 + fruit);
	color.gre= plant/2 + fruit/3*(3 + 0.5*plant*(meat - 2) - meat);
	color.blu= meat*plant*(0.45 + pow(fruit,2)); //these have been *carefully* selected and balanced
	return color;
}

Color3f GLView::setColorStomach(float plant, float fruit, float meat)
{
	float stomach[Stomach::FOOD_TYPES];
	for (int i=0; i < Stomach::FOOD_TYPES; i++) {
		if (i == Stomach::PLANT) stomach[i] = plant;
		else if (i == Stomach::FRUIT) stomach[i] = fruit;
		else if (i == Stomach::MEAT) stomach[i] = meat;
	}
	return setColorStomach(stomach);
}

Color3f GLView::setColorTempPref(float discomfort)
{
	Color3f color;
	if (discomfort>0) {
		color.red= 2*sqrt(cap(2*discomfort))*(1.3-discomfort);
		color.gre= 0.9-2*discomfort;
		color.blu= (2-16*discomfort)/4;
	} else if (discomfort<0) {
		color.red= (-discomfort)*(1+discomfort);
		color.gre= 0.8+2*discomfort;
		color.blu= 1+discomfort;
	} else {
		color.gre= 1.0;
	}

	return color;
}

Color3f GLView::setColorMetabolism(float metabolism)
{
	Color3f color;
	color.red= 8*(0.8-metabolism)*(metabolism);
	color.gre= 3*(metabolism-0.2)*(2.2-2*metabolism);
	color.blu= 2*(5*metabolism-2);
	return color;
}

Color3f GLView::setColorTone(float tone)
{
	Color3f color;
	color.red= tone<0.8 ? 1.6-3*tone : -3.5+4*tone; //best put these into an online graphing calculator...
	color.gre= tone<0.5 ? 5*tone-0.2 : 2.925-3*tone;
	color.blu= 4*tone-1.75;
	if(tone>0.45 && tone<0.55) {
		color.red*= 10*fabs(tone-0.5)+0.5;
		color.gre*= 5*fabs(tone-0.5)+0.75;
		color.blu*= 10*fabs(tone-0.5)+0.5;
	} else if (tone<0.1) color.red*= 2.5*(tone)+0.75;
	return color;
}

Color3f GLView::setColorLungs(float lungs)
{
	Color3f color;
	color.red= (0.2+lungs)*(0.4-lungs) + 25*(lungs-0.45)*(0.8-lungs);
	color.gre= lungs>0.1 ? 2.45*(lungs)*(1.2-lungs) : 0.0;
	color.blu= 1-1.5*lungs;
	return color;
}

Color3f GLView::setColorSpecies(float species)
{
	Color3f color;
	color.red= (cos((float)species/178*M_PI)+1.0)/2.0;
	color.gre= (sin((float)species/101*M_PI)+1.0)/2.0;
	color.blu= (cos((float)species/67*M_PI)+1.0)/2.0;
	return color;
}

Color3f GLView::setColorCrossable(float species)
{
	Color3f color(0.7, 0.7, 0.7);
	//all agents look grey if unrelated or if none is selected, b/c then we don't have a reference

	int selectedindex = world->getSelectedAgentIndex();
	if(selectedindex>=0){
		float deviation= abs(species - world->agents[selectedindex].species); //species deviation check
		if (deviation==0) { //exact copies: cyan
			color.red= 0.2;
			color.gre= 0.9;
			color.blu= 0.9;
		} else if (deviation<=world->agents[selectedindex].kinrange) {
			//female-only reproducable relatives: navy blue
			color.red= 0;
			color.gre= 0;
		} else if (deviation<=world->agents[selectedindex].kinrange + conf::VISUALIZE_RELATED_RANGE) {
			//possible relatives: purple
			color.gre= 0.4;
		}
	}
	return color;
}

Color3f GLView::setColorGenerocity(float give)
{
	Color3f color;
	float val= cap(abs(give)*10/world->GENEROSITY_RATE)*2/3;
	if(give>0) color.gre= val;
	else color.red= val;
	if(abs(give)<0.001) { color.blu= 0.75; color.gre= 0.5; }
	return color;
}

Color3f GLView::setColorStrength(float strength)
{
	Color3f color;
	color.red= cap(2-strength)/2;
	color.gre= (cap(2*strength)+2*cap(strength))/4;
	color.blu= strength;
	return color;
}

Color3f GLView::setColorRepType(int type)
{
	Color3f color;
	if(type==RepType::ASEXUAL){
		color.gre= 1;
	} else if(type==RepType::FEMALE){
		color.blu= 1;
	} else if(type==RepType::MALE){
		color.red= 1;
	}
	return color;
}

Color3f GLView::setColorRepCount(float repcount, int type)
{
	Color3f color= setColorRepType(type);

	float val= 0.25 + 0.75*powf(cap(1-repcount*0.25), 2); //repcounter of 4 starts getting brighter, but otherwise we always display a little color
	color.red*= val; color.gre*= val; color.blu*= val;

	if(repcount < 3) { //repcounter of 3 starts getting brighter...
		int mod= 12;
		if(repcount<1) mod= 3; //repcounter of 1 starts blinking more rapidly
		if((int)(abs(repcount)*1000)%mod>=mod*0.5){
			if(repcount < 0) { //negative repcounter indicates agent is ready to reproduce but something prevents it (sexprojection, health below minimum)
				color.red= 1.0;
				color.gre= 1.0;
				color.blu= 1.0;
			} else {
				color.red*= 0.25; color.gre*= 0.25; color.blu*= 0.25;
			}
		}
	}
	return color;
}

Color3f GLView::setColorMutations(float rate, float size)
{
	Color3f color;
	color.red= sqrt(3*rate)+cap(size-0.25);
	color.gre= 4*size + cap(2*rate-0.25);
	color.blu= powf((float)4*size,0.25)+cap(rate-0.5);

	return color;
}

Color3f GLView::setColorGeneration(int gen)
{
	Color3f color;
	float relgen= cap((gen - world->STATlowestgen)*world->STATinvgenrange);

	if (gen==world->STAThighestgen && gen!=0) { color.red= 0.2; color.gre= 0.9; color.blu= 0.9; }
	else {
		color.red= powf(relgen,0.25);
		color.gre= relgen;
		color.blu= powf(relgen,4);
	}

	return color;
}

Color3f GLView::setColorAgeHybrid(int age, bool hybrid)
{
	float agefact = pow(1.1 - (float)age/std::max(world->STAThighestage, 10), 2);
	Color3f color(agefact);

	if (age == world->STAThighestage) {
		if (world->modcounter%10 >= 5) color= Color3f(1.0);
		if (world->FUN) color= Color3f(randf(0,1),randf(0,1),randf(0,1));
	}
	if (hybrid) { 
		color.red = 0;
		color.blu+= 0.1;
		color.gre = color.blu>1 ? 5*(color.blu-1) : 0;
		
	}
	return color;
}

std::pair<Color3f,float> GLView::setColorEar(int index)
{
	float alp= 0;
	Color3f color(0,0,0);
	switch(index) {
		case 0: color.gre= 1; alp= 0.1; break;
		case 1: color.red= 1; alp= 0.1; break;
		case 2: color.red= 0.3; color.blu= 1; color.gre= 0.3; alp= 0.15; break;
		case 3: color.red= 1; color.gre= 1; color.blu= 1; alp= 0.08; break;
		default: color.red= 0.4; color.gre= 0.4; color.blu= 0.4; alp= 0.01; break;
	}

	return std::make_pair(color, alp);
}

Color3f GLView::setColorRadius(float radius)
{
	Color3f color;
	if (radius < world->MEANRADIUS + conf::TINY_RADIUS) {
		if (radius < conf::TINY_RADIUS) color= Color3f(1.0); //tiny agents always appear white; no meaningful differences between them here
		else color = setColorTone( (radius - conf::TINY_RADIUS) / world->MEANRADIUS ); //show a color scale between TINY_RADIUS and MEAN_RADIUS + TINY_RADIUS
	} else {
		//at > MEAN_RADIUS + TINY_RADIUS, restart the color scale, but dimmer
		color = setColorTone( (radius - conf::TINY_RADIUS - world->MEANRADIUS) / world->MEANRADIUS );
		color.red /= 2; color.gre /= 2; color.blu /= 2; 
	}
}


//Cell layer colors

Color3f GLView::setColorTerrain(float val)
{
	Color3f color;
	if(val==Elevation::MOUNTAIN_HIGH){ //rocky
		color.red= 0.9;
		color.gre= 0.9;
		color.blu= 0.9;
	} else if(val==Elevation::BEACH_MID){ //beach
		color.red= 0.9;
		color.gre= 0.9;
		color.blu= 0.6;
	} else if(val==Elevation::DEEPWATER_LOW) { //(deep) water
		color.red= 0.2;
		color.gre= 0.3;
		color.blu= 0.9;
	} else if(val==Elevation::SHALLOWWATER) { //(shallow) water
		color.red= 0.3;
		color.gre= 0.4;
		color.blu= 0.9;
	} else { //dirt
		color.red= 0.25;
		color.gre= 0.2;
		color.blu= 0.1;
	}
	return color;
}

Color3f GLView::setColorHeight(float val)
{
	if (val<1) val *= 0.95;
	Color3f color(val,val,val);
	return color;
}

Color3f GLView::setColorPlant(float val)
{
	Color3f color(0.0,val/2,0.0);
	return color;
}

Color3f GLView::setColorWaterPlant(float val)
{
	Color3f color(0.0,val/2*0.4,0.0);
	return color;
}

Color3f GLView::setColorMountPlant(float val)
{
	Color3f color(-val/2*0.2,val/2*0.2,-val/2*0.2);
	return color;
}

Color3f GLView::setColorFruit(float val)
{
	Color3f color(val/2,val/2,0.0);
	return color;
}

Color3f GLView::setColorWaterFruit(float val)
{
	Color3f color(val/2*0.8,val/2*0.5,0.0);
	return color;
}

Color3f GLView::setColorMeat(float val)
{
	Color3f color(val*3/4,0.0,0.0);
	return color;
}

Color3f GLView::setColorHazard(float val)
{
	Color3f color(val/2,0.0,val/2*0.9);
	if(val>conf::HAZARD_EVENT_POINT){
		color.red= 1.0;
		color.gre= 1.0;
		color.blu= 1.0;
	}
	return color;
}

Color3f GLView::setColorWaterHazard(float val)
{
	Color3f color(-val/2*0.2,-val/2*0.3,-val/2);
	if(val>conf::HAZARD_EVENT_POINT){
		color.red= 1.0;
		color.gre= 1.0;
		color.blu= 1.0;
	}
	return color;
}

Color3f GLView::setColorTempCell(int val)
{
	float temp= cap(world->calcTempAtCoord(val)-0.02);
	Color3f color(sqrt(cap(1.3*temp-0.2)), cap(1.2*(1-1.1*temp)), pow(1-temp,3)); //revert "temp" to "val" to fix for cell-based temp layer
	if(temp>0.3) {
		float blu_gre= cap(2.5*temp-0.9);
		color.red= color.red + cap(2.5*temp-1.25);
		if(temp>0.6) {
			color.gre= color.gre + cap(1.8-2*temp);
		} else color.gre= color.gre + blu_gre;
		color.blu= color.blu - blu_gre;
	}
	return color;
}

Color3f GLView::setColorLight(float val)
{
	Color3f color;
	//if moonlight is a thing, then set a lower limit
	if(world->MOONLIT && world->MOONLIGHTMULT!=1.0) {
		color.red= world->SUN_RED*val>world->MOONLIGHTMULT ? world->SUN_RED*val : world->MOONLIGHTMULT;
		color.gre= world->SUN_GRE*val>world->MOONLIGHTMULT ? world->SUN_GRE*val : world->MOONLIGHTMULT;
		color.blu= world->SUN_BLU*val>world->MOONLIGHTMULT ? world->SUN_BLU*val : world->MOONLIGHTMULT;
	} else {
		color.red= cap(world->SUN_RED*val);
		color.gre= cap(world->SUN_GRE*val);
		color.blu= cap(world->SUN_BLU*val);
	}
	return color;
}



bool GLView::cullAtCoords(int x, int y)
{
	//determine if the object at the x and y WORLD coords can be rendered, returning TRUE if it should be CULLED
	if(y > wHeight*0.5/scalemult-ytranslate + 2*conf::CZ) return true;
	if(x > wWidth*0.5/scalemult-xtranslate + 2*conf::CZ) return true;
	if(y < -wHeight*0.5/scalemult-ytranslate - 2*conf::CZ) return true;
	if(x < -wWidth*0.5/scalemult-xtranslate - 2*conf::CZ) return true;
	return false;
}
	


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~DRAW BEFORE AGENTs~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//



void GLView::drawPreAgent(const Agent& agent, float x, float y, bool ghost)
{
	float n;
	float r= agent.radius;
	float rp= agent.radius+2.8;

	if ((live_agentsvis!=Visual::NONE && (live_hidedead==0 || !agent.isDead()) && (live_hidegenz==0 || agent.gencount>0)) || ghost) {
		// don't render if visual set to NONE, or if we're hiding the agent for some reason

		glPushMatrix(); //switch to local position coordinates
		glTranslatef(x,y,0);

		if(world->isAgentSelected(agent.id) && !ghost){

			if(live_profilevis==Profile::EYES || world->isDebug()){
				//eyesight FOV's for the eyesight profile
				for(int q=0;q<NUMEYES;q++) {
					if(agent.isTiny() && !agent.isTinyEye(q)) continue;
					float aa= agent.angle + agent.eyes[q].dir + agent.eyes[q].fov;
					glBegin(GL_POLYGON);

					glColor4f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3],0.15);

					float rangemult= 1.0; //TODO: different types will show shorter ranges
					for (int n=0; n<4; n++) {
						glVertex3f(rangemult*world->MAX_SENSORY_DISTANCE*cos(aa), rangemult*world->MAX_SENSORY_DISTANCE*sin(aa), 0);
						aa-= agent.eyes[q].fov/2;
					}
					glVertex3f(rangemult*world->MAX_SENSORY_DISTANCE*cos(aa), rangemult*world->MAX_SENSORY_DISTANCE*sin(aa), 0);
					glColor4f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3],0.5);
					glVertex3f(0,0,0);
					glEnd();
				}
			}

			if(world->isDebug()){
				//draw DIST range on selected in DEBUG
				glBegin(GL_LINES);
				glColor3f(1,0,1);
				drawOutlineRes(0, 0, world->MAX_SENSORY_DISTANCE, ceil(scalemult)+6);
				glEnd();

				//spike effective zone
				if(agent.isSpikey(world->SPIKE_LENGTH)){
					glBegin(GL_POLYGON);
					glColor4f(1,0,0,0.25);
					glVertex3f(0,0,0);
					//displaying quarter fov because spikes are tricky and depend on the other's size too
					glVertex3f((r+agent.spikeLength*world->SPIKE_LENGTH)*cos(agent.angle+conf::SPIKE_MAX_FOV/4),
							   (r+agent.spikeLength*world->SPIKE_LENGTH)*sin(agent.angle+conf::SPIKE_MAX_FOV/4),0);
					glColor4f(1,0.25,0.25,0.75);
					glVertex3f((r+agent.spikeLength*world->SPIKE_LENGTH)*cos(agent.angle),
							   (r+agent.spikeLength*world->SPIKE_LENGTH)*sin(agent.angle),0);
					glColor4f(1,0,0,0.25);
					glVertex3f((r+agent.spikeLength*world->SPIKE_LENGTH)*cos(agent.angle-conf::SPIKE_MAX_FOV/4),
							   (r+agent.spikeLength*world->SPIKE_LENGTH)*sin(agent.angle-conf::SPIKE_MAX_FOV/4),0);
					glVertex3f(0,0,0);
					glEnd();
				}

				//grab effective zone and dist
				glBegin(GL_LINES);
				glColor4f(0,1,1,1);
				drawOutlineRes(0, 0, r+world->GRABBING_DISTANCE, ceil(scalemult)+2);
				glEnd();

				if(agent.isGrabbing()){
					glBegin(GL_POLYGON);
					glColor4f(0,1,1,0);
					glVertex3f(0,0,0);
					glColor4f(0,1,1,0.25);
					float grabfov = world->GRAB_MAX_FOV;
//					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle+grabfov*2),
//							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle+grabfov*2),0);
					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle+grabfov),
							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle+grabfov),0);
					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle),
							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle),0);
					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle-grabfov),
							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle-grabfov),0);
//					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle-grabfov*2),
//							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle-grabfov*2),0);
					glColor4f(0,1,1,0);
					glVertex3f(0,0,0);
					glEnd();
				}

				//health-sharing/give range
				if(!agent.isSelfish(conf::MAXSELFISH)){
					float sharedist= agent.isGiving() ? world->FOOD_SHARING_DISTANCE : (agent.give>conf::MAXSELFISH ? r+5 : 0);
					glBegin(GL_POLYGON);
					glColor4f(0,0.5,0,0.25);
					drawCircle(0,0,sharedist);
					glEnd();
				}
			}

			glTranslatef(-90,24+0.5*r,0);

			if(scalemult > .2){
				//Draw one of the profilers based on the mode we have selected
				
				if(live_profilevis==Profile::INOUT || live_profilevis==Profile::BOXES || live_profilevis==Profile::BRAIN){
					float value;
					float ioboxsize = 14; //x&y size of boxes
					float boxsize= 8;
					float xoffset= 1; //offsets for when "whitespace" is needed
					float yoffset= 8;
					float drawx = 0; //current draw positions
					float drawy = 0;

					glTranslatef(-70,0,0); //translate x more to make room. Remember to translate back by inverse amount at the end of this block

					//Draw inputs and outputs in in/out mode AND brain mode
					glBegin(GL_QUADS);
					for (int j=0;j<Input::INPUT_SIZE;j++) {
						value= agent.in[j];
						glColor3f(value,value,value);
						glVertex3f(drawx, drawy, 0.0f);
						glVertex3f(drawx + ioboxsize, drawy, 0.0f);
						glVertex3f(drawx + ioboxsize, drawy + ioboxsize, 0.0f);
						glVertex3f(drawx, drawy + ioboxsize, 0.0f);
						if(scalemult > .7){
							//draw text initials on inputs if zoomed in close
							glEnd();
							float textx = drawx + ioboxsize/3;
							float texty = drawy + ioboxsize*2/3;

							if(j>=Input::EYES && j<=Input::xEYES && j%3==0){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "R", 1.0f, 0.0f, 0.0f);
							} else if(j>=Input::EYES && j<=Input::xEYES && j%3==1){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "G", 0.0f, 1.0f, 0.0f);
							} else if(j>=Input::EYES && j<=Input::xEYES && j%3==2){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "B", 0.0f, 0.0f, 1.0f);
							} else if(j==Input::CLOCK1 || j==Input::CLOCK2 || j==Input::CLOCK3){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "Q", 0.0f, 0.0f, 0.0f);
							} else if(j==Input::TEMP){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "T", value,(2-value)/2,(1-value));
							} else if(j>=Input::EARS && j<=Input::xEARS){
								std::pair<Color3f, float> earcolor= setColorEar(j-Input::EARS);
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "E", earcolor.first.red, earcolor.first.gre, earcolor.first.blu);
							} else if(j==Input::HEALTH){
								Color3f healthcolor= setColorHealth(agent.health);
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "H", healthcolor.red, healthcolor.gre, healthcolor.blu);
							} else if(j==Input::REPCOUNT){
								Color3f repcountcolor= setColorRepCount(agent.repcounter, agent.getRepType());
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "R", cap(0.2+0.8*repcountcolor.red), cap(0.2+0.8*repcountcolor.gre), cap(0.2+0.8*repcountcolor.blu));
							} else if(j==Input::BLOOD){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "B", 0.6f, 0.0, 0.0f);
							} else if(j==Input::FRUIT_SMELL){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "S", 1.0f, 1.0f, 0.0f);
							} else if(j==Input::HAZARD_SMELL){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "S", 0.9f, 0.0f, 0.81f);
							} else if(j==Input::MEAT_SMELL){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "S", 1.0f, 0.0f, 0.1f);
							} else if(j==Input::WATER_SMELL){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "S", 0.3f, 0.3f, 0.9f);
							} else if(j==Input::BOT_SMELL){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "S", 1.0f, 1.0f, 1.0f);
							} else if(j==Input::PLAYER){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "+", 1.0f, 1.0f, 1.0f);
							} else if(j==Input::RANDOM){
								float col = randf(0,1);
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "?", col, col, col);
							}
							glBegin(GL_QUADS);
						}

						//set up for next draw location
						drawx+= ioboxsize + xoffset;
						if ((j+1)%conf::INPUTS_OUTPUTS_PER_ROW==0) { //if we over-ran the x-axis, reset x and move y to start next row
							drawx= 0;
							drawy+= ioboxsize + xoffset;
						}
					}

					drawy+= ioboxsize + yoffset;

					if(live_profilevis==Profile::BOXES || live_profilevis==Profile::BRAIN){
						//draw brain in brain profile mode, complements the i/o as drawn above and below
						drawx= 0;

						for (int j = 0; j < agent.brain.boxes.size(); j++) {
							if (!agent.brain.boxes[j].dead) {
								value = agent.brain.boxes[j].out;
								if(j < agent.brain.boxes.size() - Output::OUTPUT_SIZE) glColor3f(value,value,value);
								else glColor3f(0.5*value,0.65*value,0.15+0.85*value);
								
								glVertex3f(drawx, drawy, 0.0f);
								glVertex3f(drawx + boxsize, drawy, 0.0f);
								glVertex3f(drawx + boxsize, drawy + boxsize, 0.0f);
								glVertex3f(drawx, drawy + boxsize, 0.0f);

								if (live_profilevis==Profile::BRAIN && agent.brain.boxes[j].gw <= 0) {
									//draw a red outline for inverted boxes when showing Brain mode
									glEnd();
									glBegin(GL_LINES);
									glColor3f(1,0,0);
									glVertex3f(drawx+1, drawy+1, 0.0f);
									glVertex3f(drawx + boxsize-1, drawy+1, 0.0f);
									glVertex3f(drawx + boxsize-1, drawy+1, 0.0f);
									glVertex3f(drawx + boxsize-1, drawy + boxsize-1, 0.0f);
									glVertex3f(drawx + boxsize-1, drawy + boxsize-1, 0.0f);
									glVertex3f(drawx+1, drawy + boxsize-1, 0.0f);
									glVertex3f(drawx+1, drawy + boxsize-1, 0.0f);
									glVertex3f(drawx+1, drawy+1, 0.0f);
									glEnd();
									glBegin(GL_QUADS);
								}
							} else {
								//draw a red x for dead boxes
								glEnd();
								glBegin(GL_LINES);
								glColor3f(0.75,0,0);

								glVertex3f(drawx, drawy, 0.0f);
								glVertex3f(drawx + boxsize, drawy + boxsize, 0.0f);
								glVertex3f(drawx + boxsize, drawy, 0.0f);
								glVertex3f(drawx, drawy + boxsize, 0.0f);
								glEnd();
								glBegin(GL_QUADS);
							}

							drawx+= boxsize; //no offsets for brain boxes
							if ((j+1)%conf::BOXES_PER_ROW==0) {
								drawx= 0;
								if(j+1<agent.brain.boxes.size()) drawy+= boxsize;
							}
						}
						drawy+= boxsize + yoffset;
					}
				
					//draw outputs
					drawx= 0;

					for (int j=0;j<Output::OUTPUT_SIZE;j++) {
						value= agent.out[j];
						if(j==Output::RED) glColor3f(value,0,0);
						else if (j==Output::GRE) glColor3f(0,value,0);
						else if (j==Output::BLU) glColor3f(0,0,value);
						//else if (j==Output::JUMP) glColor3f(value,value,0); removed due to being too attention-grabbing when nothing can happen due to exhaustion
						else glColor3f(value,value,value);

						glVertex3f(drawx, drawy, 0.0f);
						glVertex3f(drawx + ioboxsize, drawy, 0.0f);
						glVertex3f(drawx + ioboxsize, drawy + ioboxsize, 0.0f);
						glVertex3f(drawx, drawy + ioboxsize, 0.0f);

						if (live_profilevis==Profile::BRAIN) {
							int target = agent.brain.boxes.size() - Output::OUTPUT_SIZE + j;
							if (agent.brain.boxes[target].gw <= 0) {
								//draw a red outline for outputs with inverted boxes when showing Brain mode
								glEnd();
								glBegin(GL_LINES);
								glColor3f(1,0,0);
								glVertex3f(drawx+1, drawy+1, 0.0f);
								glVertex3f(drawx + ioboxsize-1, drawy+1, 0.0f);
								glVertex3f(drawx + ioboxsize-1, drawy+1, 0.0f);
								glVertex3f(drawx + ioboxsize-1, drawy + ioboxsize-1, 0.0f);
								glVertex3f(drawx + ioboxsize-1, drawy + ioboxsize-1, 0.0f);
								glVertex3f(drawx+1, drawy + ioboxsize-1, 0.0f);
								glVertex3f(drawx+1, drawy + ioboxsize-1, 0.0f);
								glVertex3f(drawx+1, drawy+1, 0.0f);
								glEnd();
								glBegin(GL_QUADS);
							}
						}

						if(scalemult > .7){
							glEnd();
							float textx = drawx + ioboxsize/3;
							float texty = drawy + ioboxsize*2/3;

							if(j==Output::LEFT_WHEEL_B || j==Output::LEFT_WHEEL_F){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "!", 1.0f, 0.0f, 1.0f);
							} else if(j==Output::RIGHT_WHEEL_B || j==Output::RIGHT_WHEEL_F){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "!", 0.0f, 1.0f, 0.0f);
							} else if(j==Output::VOLUME){
								float compcol = value>0.75 ? 0.0f : 1.0f;
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "V", compcol, compcol, compcol);
							} else if(j==Output::TONE){
								Color3f tonecolor= setColorTone(agent.tone);
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "T", tonecolor.red, tonecolor.gre, tonecolor.blu);
							} else if(j==Output::CLOCKF3){
								float compcol = value>0.75 ? 0.0f : 1.0f;
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "Q", compcol, compcol, compcol);
							} else if(j==Output::SPIKE){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "S", 1.0f, 0.0f, 0.0f);
							} else if(j==Output::PROJECT){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "X", 0.6f, 0.0f, 0.6f);
							} else if(j==Output::JAW){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, ">", 1.0f, 1.0f, 0.0f);
							} else if(j==Output::GIVE){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "G", 0.0f, 0.3f, 0.0f);
							} else if(j==Output::GRAB){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "G", 0.0f, 0.6f, 0.6f);
							} else if(j==Output::GRAB_ANGLE){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "<", 0.0f, 0.6f, 0.6f);
							} else if(j==Output::JUMP){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "J", 1.0f, 1.0f, 0.0f);
							} else if(j==Output::BOOST){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "B", 0.6f, 0.6f, 0.6f);
							} else if(j==Output::STIMULANT){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "$", 0.0f, 1.0f, 1.0f);
							} else if(j==Output::WASTE_RATE){
								RenderString(textx, texty, GLUT_BITMAP_HELVETICA_12, "W", 0.9f, 0.0f, 0.81f);
							}
							glBegin(GL_QUADS);
						}
						drawx+= ioboxsize + xoffset;
						if ((j+1)%conf::INPUTS_OUTPUTS_PER_ROW==0) {
							drawx= 0;
							drawy+= ioboxsize + xoffset;
						}
					}
					glEnd();

					if (live_profilevis==Profile::BRAIN) {
						//further, if we have selected the full BRAIN profile, draw lines connecting boxes via the brain's live conns list

						int inputlines = (int)ceil((float)Input::INPUT_SIZE/(float)conf::INPUTS_OUTPUTS_PER_ROW);
						//int brainlines = (int)ceil((float)agent.brain.boxes.size/(float)conf::BOXES_PER_ROW);

						float sx,sy,tx,ty;

						for (int c = 0; c < agent.brain.lives.size(); c++) {
							int sid = agent.brain.lives[c].sid;
							int tid = agent.brain.lives[c].tid;
							float w = agent.brain.lives[c].w;
							int type = agent.brain.lives[c].type;
							float absw = abs(w);
							float acc;

							if (absw > 1.0) glLineWidth((int)ceil(absw/2));
							else {
								absw *= 5; //adjust alpha so we aren't completely invisible
								glLineWidth(2);
							}
							
							//check if input, use raw acc value for alpha mult. otherwise, use delta for box values
							if (sid < 0) {
								// Input lines ALWAYS shown with width of 2
								//glLineWidth(2);
								if (type == ConnType::INVERTED) acc = 1-agent.in[-sid-1]; // Invert it if the conn is an inverted type
								else acc = agent.in[-sid-1];
							} else {
								//this serves to hide the majority of connections which aren't "doing" anything at a given moment
								acc = cap(10*abs(agent.brain.boxes[sid].out - agent.brain.boxes[sid].oldout));
							}
							acc = std::max((float)0.1, acc); //always display something
							
							//filter by target id; if in range of outputs, change target coords to match output range
							if (tid >= agent.brain.boxes.size() - Output::OUTPUT_SIZE) {
								//for all output-targeting connections
								ty = inputlines*ioboxsize + xoffset + 3*yoffset + boxsize/2 + boxsize*(int)((float)(agent.brain.boxes.size())/(float)conf::BOXES_PER_ROW);
								tx = ioboxsize/2 + (ioboxsize+xoffset)*((Output::OUTPUT_SIZE - agent.brain.boxes.size() + tid)%conf::BOXES_PER_ROW);
							} else {
								//for all brain interior connections
								ty = inputlines*ioboxsize + xoffset + yoffset + boxsize/2 + boxsize*(int)((float)(tid)/(float)conf::BOXES_PER_ROW);
								tx = boxsize/2 + boxsize*(tid%conf::BOXES_PER_ROW);
							}

							if (tid == sid) {
								//must happen before glBegin
								glLineWidth(2);
							}

							glBegin(GL_LINES);
							if (tid == sid) {
								glColor4f(cap(-w/5), cap(w/5), 0, 0.3*absw*acc);
								drawOutlineRes(tx, ty, 3, 1);
							} else {
								if (sid < 0) {
									sy = ioboxsize/2 + (ioboxsize+xoffset)*(int)((float)(-sid-1)/(float)conf::INPUTS_OUTPUTS_PER_ROW);
									sx = ioboxsize/2 + (ioboxsize+xoffset)*((-sid-1)%conf::INPUTS_OUTPUTS_PER_ROW);
								} else { 
									sy = inputlines*ioboxsize + yoffset + boxsize/2 + boxsize*(int)((float)(sid)/(float)conf::BOXES_PER_ROW);
									sx = boxsize/2 + boxsize*(sid%conf::BOXES_PER_ROW);
								}
								float blu = type == ConnType::INVERTED ? cap(abs(w)/5+0.1) : 0.1;
								glColor4f(cap(-w/5+0.1), cap(w/5 + cap(-w/5)*0.75+0.1), blu, 0.3*absw/5*acc);
								glVertex3f(sx, sy, 0.0f);
								glVertex3f((sx+tx)/2, (sy+ty)/2, 0.0f);

								glVertex3f((sx+tx)/2, (sy+ty)/2, 0.0f);
								glColor4f(1, 1, 1, 0.3*absw/5*acc);
								glVertex3f(tx, ty, 0.0f);
							}
							glEnd();
						}
						glLineWidth(1);
					}

					glTranslatef(70,0,0); //translate back for brain displays (see top of this block)
				
				// endif brain profiles

				} else if (live_profilevis==Profile::EYES){
					//eyesight profile. Draw a box with colored disks
					glBegin(GL_QUADS);
					glColor3f(0,0,0.1);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 60, 0);
					glVertex3f(180, 60, 0);
					glVertex3f(180, 0, 0);
					glEnd();

					for(int q=0;q<NUMEYES;q++){
						if(agent.isTiny() && !agent.isTinyEye(q)) continue;

						float eyex= 165-agent.eyes[q].dir/2/M_PI*150;
						float eyer= 1+agent.eyes[q].fov/M_PI*40;
						glBegin(GL_POLYGON);
						glColor3f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3]);
						drawCircle(eyex, 30, eyer);
						glEnd();

						glBegin(GL_LINES);
						glColor3f(0.75,0.75,0.75);
						drawOutlineRes(eyex, 30, eyer, 3);
						glEnd();
					}

					glBegin(GL_LINES);
					glColor3f(0.25,0.25,0.25);
					glVertex3f(0, 60, 0);
					glVertex3f(180, 60, 0);
					glColor3f(agent.real_red,agent.real_gre,agent.real_blu);
					glVertex3f(180, 0, 0);
					glVertex3f(0, 0, 0);

					glColor3f(0.7,0,0);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 60, 0);
					glVertex3f(180, 60, 0);
					glVertex3f(180, 0, 0);
					
					glEnd();

				// endif eye profile

				} else if (live_profilevis==Profile::SOUND) {
					//sound profiler
					glBegin(GL_QUADS);
					glColor3f(0.1,0.1,0.1);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 80, 0);
					glVertex3f(180, 80, 0);
					glVertex3f(180, 0, 0);
					
					//each ear gets its hearing zone plotted
					for(int q=0;q<NUMEARS;q++) {
						std::pair<Color3f, float> earcolor= setColorEar(q);

						glColor4f(earcolor.first.red, earcolor.first.gre, earcolor.first.blu, earcolor.second); 

						//Draw a trapezoid indicating the full range the ear hears, including the limbs
						float displace = (agent.ears[q].high - agent.ears[q].low)*0.5;
						if(displace > world->SOUND_PITCH_RANGE) displace = world->SOUND_PITCH_RANGE;
						float heightratio = 1 - cap(displace*world->INV_SOUND_PITCH_RANGE); //when displace= the SOUND_PITCH_RANGE, 

						glVertex3f(2+176*cap(agent.ears[q].low + displace), 2 + 78*heightratio, 0); //top-left	  L  _______  H		     L___H
						glVertex3f(2+176*cap(agent.ears[q].low), 78, 0); //	bottom-left							    /|     |\*			   |
						glVertex3f(2+176*cap(agent.ears[q].high), 78, 0); //bottom-right						   / |h    | \* but also  /|\*
						glVertex3f(2+176*cap(agent.ears[q].high - displace), 2 + 78*heightratio, 0); // top-right /_d|_____|__\*		 /d|_\*
					
						if(agent.ears[q].low+displace*2 < agent.ears[q].high) {
							//draw a box that indicates the zone that this ear hears at full volume
							glVertex3f(2+176*cap(agent.ears[q].low + displace), 2, 0);
							glVertex3f(2+176*cap(agent.ears[q].low + displace), 78, 0);
							glVertex3f(2+176*cap(agent.ears[q].high - displace), 78, 0);
							glVertex3f(2+176*cap(agent.ears[q].high - displace), 2, 0);
						} 

						if(agent.isTiny()) break; //only one ear for tiny agents
						
					}
					glEnd();

					//top corner labels
					if(scalemult > .7){
						RenderString(8, 8, GLUT_BITMAP_HELVETICA_12, "low", 0.7f, 0.7f, 0.7f);
						RenderString(180-8-22, 8, GLUT_BITMAP_HELVETICA_12, "high", 0.7f, 0.7f, 0.7f);
					}

					//box outline
					glBegin(GL_LINES);
					glColor3f(0.8,0.8,0.8);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 80, 0);
					glVertex3f(0, 80, 0);
					glVertex3f(180, 80, 0);
					glVertex3f(180, 80, 0);
					glVertex3f(180, 0, 0);
					glVertex3f(180, 0, 0);
					glVertex3f(0, 0, 0);
					glEnd();

					//now show our own sound, colored by tone
					glLineWidth(3);
					glBegin(GL_LINES);
					Color3f tonecolor= setColorTone(agent.tone);
					glColor3f(tonecolor.red, tonecolor.gre, tonecolor.blu);
					glVertex3f(2+176*agent.tone, 78, 0);
					glVertex3f(2+176*agent.tone, 78-76*agent.volume, 0);
					glEnd();
					glLineWidth(1);

					glBegin(GL_LINES); //finally, show all the sounds the agent is in range of hearing
					for(int e=0;e<world->selectedSounds.size();e++) {
						//unpackage each sound entry
						float volume= (int)world->selectedSounds[e];
						float tone= world->selectedSounds[e]-volume;
						volume/= 100;

						float fiz= 1;
						if(volume>=1) volume= cap(volume-1.0);
						else fiz= 0.3;

						//display half-lime, half-magenta line for the wheel sounds of others
						if(tone==conf::WHEEL_TONE) glColor4f(0.0,0.8,0.0,fiz);
						else glColor4f(0.7,0.7,0.7,fiz);

						glVertex3f(2+176*tone, 78, 0);
						glVertex3f(2+176*tone, 78-76*volume, 0);

						if(tone==conf::WHEEL_TONE){
							glColor4f(0.8,0.0,0.8,fiz);
							glVertex3f(2+176*tone, 78, 0);
							glVertex3f(2+176*tone, 78-38*volume, 0);
						}
					}
					glEnd();
				
				// endif sound profile

				} else if (live_profilevis==Profile::METABOLISM) {
					//metabolism profiler: provides a "key" showing all possible colors, an indicator for intake, for health and rep counter distribution, and labels!
					glBegin(GL_QUADS);
					glColor3f(0.1,0.1,0.1);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 80, 0);
					glVertex3f(180, 80, 0);
					glVertex3f(180, 0, 0);
					glEnd();

					//draw a series of circles for the "key", colored for the real metabolism colors
					for (int n=0; n<=100; n++) {
						glBegin(GL_POLYGON);
						float factor = (float)n/100;
						Color3f col = setColorMetabolism(factor);
						glColor4f(col.red, col.gre, col.blu, 0.4);
						drawCircleRes(40 + n, 5, 3, 1);
						glEnd();
					}

					// marker showing the selected agent's metabolism
					glBegin(GL_POLYGON);
					glColor3f(1,1,1);
					glVertex3f(40 + 100*agent.metabolism, 9, 0);
					glVertex3f(40 + 100*agent.metabolism - 2, 13, 0);
					glVertex3f(40 + 100*agent.metabolism + 2, 13, 0);
					glEnd();

					//draw vertical bars showing max possible intake, and for real intake now (TODO)
					float metabmult = world->getMetabolismRatio(agent.metabolism);
					glBegin(GL_QUADS); //repcount
					glColor3f(0.1,0.3,0.3);
					glVertex3f(110, 65 - 50*metabmult, 0);
					glVertex3f(110, 65, 0);
					glVertex3f(160, 65, 0);
					glVertex3f(160, 65 - 50*metabmult, 0);

					glBegin(GL_QUADS); //health
					glColor3f(0.1,0.3,0.1);
					glVertex3f(20, 65 - 50*(1-metabmult), 0);
					glVertex3f(20, 65, 0);
					glVertex3f(70, 65, 0);
					glVertex3f(70, 65 - 50*(1-metabmult), 0);
					glEnd();

					//TODO: live intake

					//draw divider line and MIN_INTAKE_HEALTH_RATIO threshold
					glBegin(GL_LINES);
					glColor3f(0.5,0.5,0.5);
					glVertex3f(90, 20, 0);
					glVertex3f(90, 70, 0);

					float minhealth = world->MIN_METABOLISM_HEALTH_RATIO;
					if (minhealth > 0) {
						glColor3f(0.1,0.8,0.1);
						glVertex3f(20, 65 - 50*minhealth, 0);
						glVertex3f(70, 65 - 50*minhealth, 0);
					}
					glEnd();

					//some text descriptions
					if(scalemult > .7){
						RenderString(20, 75, GLUT_BITMAP_HELVETICA_12, "Health", 0.1f, 0.8f, 0.1f);
						RenderString(180-68, 75, GLUT_BITMAP_HELVETICA_12, "Rep-count", 0.1f, 0.9f, 0.9f);
						if (minhealth < 0.2) minhealth = 0.2; //at this point, this val is only used for the text
						RenderString(22, 65 - 50*minhealth + UID::CHARHEIGHT, GLUT_BITMAP_HELVETICA_12, "min ratio", 0.1f, 0.8f, 0.1f);
					}
				}

			}
			glTranslatef(90,-24-0.5*r,0); //return to agent location for later stuff

		}

		/*
		//draw lines connecting connected brain boxes, but only in debug mode (NEEDS SIMPLIFICATION)
		if(world->isDebug()){
			glEnd();
			glBegin(GL_LINES);
			float offx=0;
			ss=30;
			xboxsize=ss;
			for (int j=0;j<BRAINBOXES;j++) {
				for(int k=0;k<CONNS;k++){
					int j2= agent.brain.boxes[j].id[k];
					
					//project indices j and j2 into pixel space
					float x1= 0;
					float y1= 0;
					if(j<Input::INPUT_SIZE) { x1= j*ss; y1= yy; }
					else { 
						x1= ((j-Input::INPUT_SIZE)%30)*ss;
						y1= yy+ss+2*ss*((int) (j-Input::INPUT_SIZE)/30);
					}
					
					float x2= 0;
					float y2= 0;
					if(j2<Input::INPUT_SIZE) { x2= j2*ss; y2= yy; }
					else { 
						x2= ((j2-Input::INPUT_SIZE)%30)*ss;
						y2= yy+ss+2*ss*((int) (j2-Input::INPUT_SIZE)/30);
					}
					
					float ww= agent.brain.boxes[j].w[k];
					if(ww<0) glColor3f(-ww, 0, 0);
					else glColor3f(0,0,ww);
					
					glVertex3f(x1,y1,0);
					glVertex3f(x2,y2,0);
				}
			}
		}*/

		//draw indicator of all agents... used for various events
		if (agent.indicator>0) {
			if(ghost){
				//for ghosts, I'm gonna draw a little dot to the top-left for indicator splashes
				float xi= -18-agent.radius;
				glBegin(GL_POLYGON);
				glColor4f(agent.ir,agent.ig,agent.ib,1.0);
				drawCircle(xi, xi, 3);
				glEnd();
			} else {
				glBegin(GL_POLYGON);
				glColor4f(agent.ir,agent.ig,agent.ib,0);
				glVertex3f(0,0,0);
				glColor4f(agent.ir,agent.ig,agent.ib,0.75);
				drawCircle(0, 0, r+((int)agent.indicator));
				glColor4f(agent.ir,agent.ig,agent.ib,0);
				glVertex3f(0,0,0);
				glEnd();
			}
		}

		//draw giving/receiving
		if(agent.dhealth!=0){
			glBegin(GL_POLYGON);
			glColor4f(1,1,1,0);
			glVertex3f(0,0,0);

			float mag= (live_agentsvis==Visual::HEALTH && !ghost) ? 3 : 1;//draw sharing as a thick green or red outline, bigger if viewing health
			Color3f generocitycolor= setColorGenerocity(agent.dhealth);
			
			glColor4f(generocitycolor.red, generocitycolor.gre, generocitycolor.blu, 1);
			drawCircle(0, 0, rp*mag);
			glColor4f(1,1,1,0);
			glVertex3f(0,0,0);
			glEnd();
		}

		//sound waves!
		if(live_agentsvis==Visual::VOLUME && !ghost && agent.volume>conf::RENDER_MINVOLUME){
			Color3f tonecolor= setColorTone(agent.tone);

			if(scalemult > 1) glLineWidth(3);
			else if(scalemult > .3) glLineWidth(2);
			
			float volume= agent.volume;
			float count= agent.tone*15+1;

			for (int l=0; l<=(int)count; l++){
				float dist= world->MAX_SENSORY_DISTANCE*(l/count) + 2*(world->modcounter%(int)(world->MAX_SENSORY_DISTANCE/2));
				if (dist>world->MAX_SENSORY_DISTANCE) dist-= world->MAX_SENSORY_DISTANCE;
				float distfratio= dist*world->INV_MAX_SENSORY_DISTANCE;
				//this dist func works in 3 parts: first, we give it a displacement based on the l-th ring / count
				//then, we take modcounter, and divide it by the distance, giving us a proportion of the radius
				//The weird *2 and /2 bits are to increase the speed of the waves. Leave them
				//finally, if the dist is too large, wrap it down by subtracting MAX_SENSORY_DISTANCE. And we take a ratio value too to use later
				glBegin(GL_LINES);
				glColor4f(tonecolor.red, tonecolor.gre, tonecolor.blu, cap((conf::RENDER_MINVOLUME+1.2*volume-distfratio)));
				drawOutlineRes(0, 0, dist, (int)(ceil(2*scalemult)+1+5*distfratio));
				glEnd();
			}
			glLineWidth(1);
		}

		if(scalemult > .3 && !ghost) {//hide extra visual data if really far away

			int xo=8+agent.radius;
			int yo=-21;

			//health
			glBegin(GL_QUADS);
			glColor3f(0,0,0);
			glVertex3f(xo,yo,0);
			glVertex3f(xo+5,yo,0);
			glVertex3f(xo+5,yo+42,0);
			glVertex3f(xo,yo+42,0);

			glColor3f(0,0.8,0);
			glVertex3f(xo,yo+42*(1-agent.health/world->HEALTH_CAP),0);
			glVertex3f(xo+5,yo+42*(1-agent.health/world->HEALTH_CAP),0);
			glColor3f(0,0.6,0);
			glVertex3f(xo+5,yo+42,0);
			glVertex3f(xo,yo+42,0);

			//energy
			xo+= 7;
			glColor3f(0,0,0);
			glVertex3f(xo,yo,0);
			glVertex3f(xo+5,yo,0);
			glVertex3f(xo+5,yo+42,0);
			glVertex3f(xo,yo+42,0);

			float exh= 2/(1+exp(agent.exhaustion));
			glColor3f(0.8*exh,0.8*exh,cap(1-agent.exhaustion)*0.75);
			glVertex3f(xo,yo+42*(1-exh),0);
			glVertex3f(xo+5,yo+42*(1-exh),0);
			glColor3f(0.8,0.8,0);
			glVertex3f(xo+5,yo+42,0);
			glVertex3f(xo,yo+42,0);

			//repcounter
			xo+= 7;
			glColor3f(0,0,0);
			glVertex3f(xo,yo,0);
			glVertex3f(xo+5,yo,0);
			glVertex3f(xo+5,yo+42,0);
			glVertex3f(xo,yo+42,0);

			glColor3f(0,0.7,0.7);
			glVertex3f(xo,yo+42*cap(agent.repcounter/agent.maxrepcounter),0);
			glVertex3f(xo+5,yo+42*cap(agent.repcounter/agent.maxrepcounter),0);
			glColor3f(0,0.5,0.6);
			glVertex3f(xo+5,yo+42,0);
			glVertex3f(xo,yo+42,0);

			//end side-by-side displays
			glEnd();
			
			if(world->isDebug()) {
				int ypos = 0;
				int ydisplace = 12;
				glTranslatef(8+agent.radius, -32, 0);
				//print hidden stats & values
				if(agent.near) {
					RenderString(0, ypos, GLUT_BITMAP_HELVETICA_12, "near!", 0.8f, 1.0f, 1.0f);
					ypos-= ydisplace;
				}

				//wheel speeds
				sprintf(buf2, "w1: %.3f", agent.w1);
				RenderString(0, ypos, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 0.0f, 1.0f);
				ypos-= ydisplace;

				sprintf(buf2, "w2: %.3f", agent.w2);
				RenderString(0, ypos, GLUT_BITMAP_HELVETICA_12, buf2, 0.0f, 1.0f, 0.0f);
				ypos-= ydisplace;

				//exhaustion readout
				sprintf(buf2, "exh: %.3f", agent.exhaustion);
				RenderString(0, ypos, GLUT_BITMAP_HELVETICA_12, buf2, 1.0f, 1.0f, 0.0f);
			}
		}

		//end local coordinate stuff
		glPopMatrix();

		if(world->isAgentSelected(agent.id) && !ghost) {//extra info for selected agent
			//debug stuff
			if(world->isDebug()) {
				//debug sight lines: connect to anything selected agent sees
				glBegin(GL_LINES);
				for (int i=0;i<(int)world->linesA.size();i++) {
					glColor3f(1,1,1);
					glVertex3f(world->linesA[i].x,world->linesA[i].y,0);
					glVertex3f(world->linesB[i].x,world->linesB[i].y,0);
				}
				world->linesA.resize(0);
				world->linesB.resize(0);
				
				//debug cell smell box: outlines all cells the selected agent is "smelling"

				int scx= (int) (agent.pos.x/conf::CZ);
				int scy= (int) (agent.pos.y/conf::CZ);

				int celldist = (int)ceil( world->MAX_SENSORY_DISTANCE*conf::SMELL_DIST_MULT/(float)conf::CZ/2 );
				int minx = std::max(scx - celldist, 0)*conf::CZ;
				int maxx = std::min(scx+1 + celldist, world->CW)*conf::CZ;
				int miny = std::max(scy - celldist, 0)*conf::CZ;
				int maxy = std::min(scy+1 + celldist, world->CH)*conf::CZ;

				glColor3f(0,1,0);
				glVertex3f(minx,miny,0);
				glVertex3f(minx,maxy,0);
				glVertex3f(minx,maxy,0);
				glVertex3f(maxx,maxy,0);
				glVertex3f(maxx,maxy,0);
				glVertex3f(maxx,miny,0);
				glVertex3f(maxx,miny,0);
				glVertex3f(minx,miny,0);

				glEnd();
			}
		}
	}
}



//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~DRAW AGENT~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//



void GLView::drawAgent(const Agent& agent, float x, float y, bool ghost)
{
	float n;
	float r= agent.radius;
	float rp= agent.radius+2.8;
	float rad= r;

	//jumping agents appear magnified
	if (agent.pos.z > 0) rad+= (agent.pos.z + 1);

	glPushMatrix(); //switch to local position coordinates
	glTranslatef(x,y,0);

	//handle selected agent outliner
	if (live_agentsvis!=Visual::NONE && world->isAgentSelected(agent.id) && !ghost) {
		//draw selection outline
		glLineWidth(2);
		glBegin(GL_LINES);
		glColor3f(1,1,1);
		drawOutlineRes(0, 0, rad+4/scalemult, ceil(scalemult+r/10)), 
		glEnd();
		glLineWidth(1);
	}

	if ((live_agentsvis!=Visual::NONE && (live_hidedead==0 || !agent.isDead())) && (live_hidegenz==0 || agent.gencount>0) || ghost) {
		// don't render if visual set to NONE, or if we're hiding the agent for some reason

		//agent body color assignment
		Color3f color;
		if (live_agentsvis==Visual::RGB || (live_agentsvis==Visual::NONE && ghost)){ //real rgb values
			color= Color3f(agent.real_red, agent.real_gre, agent.real_blu);
		} else if (live_agentsvis==Visual::STOMACH){
			color = setColorStomach(agent.stomach);
		} else if (live_agentsvis==Visual::DISCOMFORT){
			color= setColorTempPref(agent.discomfort);
		} else if (live_agentsvis==Visual::VOLUME) {
			color= Color3f(agent.volume*(agent.volume+0.1));
		} else if (live_agentsvis==Visual::SPECIES){ 
			color= setColorSpecies(agent.species);
		} else if (live_agentsvis==Visual::CROSSABLE){ //crossover-compatable to selection
			color= setColorCrossable(agent.species);
		} else if (live_agentsvis==Visual::HEALTH) {
			color= setColorHealth(agent.health);
		} else if (live_agentsvis==Visual::REPCOUNTER) {
			color= setColorRepCount(agent.repcounter, agent.getRepType());
		} else if (live_agentsvis==Visual::METABOLISM){
			color= setColorMetabolism(agent.metabolism);
		} else if (live_agentsvis==Visual::STRENGTH){
			color= setColorStrength(agent.strength);
		} else if (live_agentsvis==Visual::BRAINMUTATION){
			color= setColorMutations(agent.brain_mutation_chance, agent.brain_mutation_size);
		} else if (live_agentsvis==Visual::GENEMUTATION){
			color= setColorMutations(agent.gene_mutation_chance, agent.gene_mutation_size);
		} else if (live_agentsvis==Visual::LUNGS){
			color= setColorLungs(agent.lungs);
		} else if (live_agentsvis==Visual::GENERATIONS){
			color= setColorGeneration(agent.gencount);
		} else if (live_agentsvis==Visual::AGE_HYBRID){
			color= setColorAgeHybrid(agent.age, agent.hybrid);
		}
		

		//the center gets its own alpha and darkness multiplier. For now, tied to reproduction mode (asexual, sexual F, or sexual M). See centerrender def
		float centeralpha= cap(agent.centerrender);
		float centercmult= agent.centerrender>1.0 ? -0.5*agent.centerrender+1.25 : 0.5*agent.centerrender+0.25;
		if (agent.isTiny() && live_agentsvis!=Visual::RGB) {
			//we can't see the difference on tiny agents, so reset them
			centeralpha= 1.0;
			centercmult= 1.0;
		}

		// when we're zoomed out far away, do some things differently, but not to the ghost (selected agent display)
		if( scalemult <= scale4ksupport && !ghost) {
			rad= 2.2/scalemult; //this makes agents a standard size when zoomed out REALLY far, but not on the ghost

		}

		if (agent.isDead() || (scalemult <= 0.3 && !ghost) || live_agentsvis!=Visual::RGB) {
			//when zoomed out too far to want to see agent details, or if agent is dead (either ghost render or not), draw center as solid
			centeralpha= 1; 
			centercmult= 1;
		}
		
		//mult for dead agents, to display more uniform & transparent parts
		float dead= agent.isDead() ? conf::RENDER_DEAD_ALPHA : 1;


		//we are now ready to start drawing
		if (scalemult > .3 || ghost) { //dont render eyes, ears, or boost if zoomed out, but always render them on ghosts

			//draw eye lines
			for(int q=0;q<NUMEYES;q++) {
				if(agent.isTiny() && !agent.isTinyEye(q)) break; //skip extra eyes for tinies
				float aa= agent.angle+agent.eyes[q].dir;
				
				glBegin(GL_LINES);
				glColor4f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3],0.75*dead);

				glVertex3f(rad*cos(aa), rad*sin(aa),0);
				glVertex3f((2*rad+30)*cos(aa), (2*rad+30)*sin(aa), 0); //this draws whiskers. Don't get rid of or change these <3

				glEnd();
			}

			//ears
			for(int q=0;q<NUMEARS;q++) {
				glBegin(GL_POLYGON);
				float aa= agent.angle + agent.ears[q].dir;
				//the centers of ears are black
				glColor4f(0,0,0,0.35);
				glVertex3f(rad*cos(aa), rad*sin(aa),0);

				//color ears differently if we are set on the sound profile or debug
				if(live_profilevis==Profile::SOUND || world->isDebug()) {
					std::pair<Color3f,float> earcolor= setColorEar(q);

					glColor4f(earcolor.first.red, earcolor.first.gre, earcolor.first.blu, 0.5*dead); 
				} else glColor4f(agent.real_red,agent.real_gre,agent.real_blu,0.5*dead);

				drawCircle(rad*cos(aa), rad*sin(aa), 2*agent.hear_mod);
				glColor4f(0,0,0,0.35);
				glVertex3f(rad*cos(aa), rad*sin(aa),0);
				glEnd();
				if(agent.isTiny()) break; //tiny agents have only one ear
			}

			//boost blur
			//this needs a rework if we're going to keep agent transparency a thing (used for asexual mode indicator)
			if (agent.boost){
				Vector3f displace= agent.dpos - agent.pos;
				for(int q=1;q<4;q++){
					glBegin(GL_POLYGON);
					glColor4f(color.red,color.gre,color.blu,dead*centeralpha*0.1);
					glVertex3f(0,0,0);
					glColor4f(color.red,color.gre,color.blu,0.25);
					drawCircle(capm(displace.x, -1, 1)*(q*10), capm(displace.y, -1, 1)*(q*10), r);
					glColor4f(color.red,color.gre,color.blu,dead*centeralpha*0.1);
					glVertex3f(0,0,0);
					glEnd();
				}
			}

			//vis grabbing (if enabled)
			if(world->GRAB_PRESSURE!=0 && agent.isGrabbing()){
				glLineWidth(2);
				glBegin(GL_LINES);

				if(agent.grabID==-1 || ghost){
					glColor4f(0.0,0.7,0.7,0.75);

					float mult= agent.grabID==-1 ? 1 : 0;
					float aa= agent.angle+agent.grabangle+world->GRAB_MAX_FOV*mult;
					float ab= agent.angle+agent.grabangle-world->GRAB_MAX_FOV*mult;
					glVertex3f(r*cos(aa), r*sin(aa),0);
					glVertex3f((world->GRABBING_DISTANCE+r)*cos(aa), (world->GRABBING_DISTANCE+r)*sin(aa), 0);

					glVertex3f(r*cos(ab), r*sin(ab),0);
					glVertex3f((world->GRABBING_DISTANCE+r)*cos(ab), (world->GRABBING_DISTANCE+r)*sin(ab), 0);

				}

				glEnd();
				glLineWidth(1);
			}
		}
		

		//body
		glBegin(GL_POLYGON); 
		glColor4f(color.red*centercmult,color.gre*centercmult,color.blu*centercmult,dead*centeralpha);
		glVertex3f(0,0,0);
		glColor4f(color.red,color.gre,color.blu,dead);
		drawCircleRes(0, 0, rad, getAgentRes(ghost));
		glColor4f(color.red*centercmult,color.gre*centercmult,color.blu*centercmult,dead*centeralpha);
		glVertex3f(0,0,0);
		glEnd();


		//start drawing a bunch of lines
		glBegin(GL_LINES);

		//outline and spikes are effected by the zoom magnitude
		float blur= cap(4.5*scalemult-0.5);
		if (ghost) blur= 1; //disable effect for static rendering

		//jaws
		if ((scalemult > .08 || ghost) && agent.jawrend>0) {
			//dont render jaws if zoomed too far out, but always render them on ghosts, and only if they've been active within the last few ticks
			glColor4f(0.9,0.9,0,blur);

			// 1/2*JAW_MAX_FOV because being able to bite relies on other agents size. Plus it looks stupid but works great
			float jawangle = ( 1 - powf(abs(agent.jawPosition), 0.5) ) * conf::JAW_MAX_FOV / 2;
			if(agent.jawrend == conf::JAWRENDERTIME) jawangle = conf::JAW_MAX_FOV / 2; //at the start of the timer the jaws are rendered open for aesthetic
			float jawlength = world->BITE_DISTANCE - 2; //shave just a little bit off the jaw length

			glVertex3f(rad*cos(agent.angle), rad*sin(agent.angle), 0);
			glVertex3f((jawlength+rad)*cos(agent.angle+jawangle), (jawlength+rad)*sin(agent.angle+jawangle), 0);
			glVertex3f(rad*cos(agent.angle), rad*sin(agent.angle), 0);
			glVertex3f((jawlength+rad)*cos(agent.angle-jawangle), (jawlength+rad)*sin(agent.angle-jawangle), 0);
		}
		glEnd();

		//outline
		if (agent.isAirborne() && scalemult > scale4ksupport) { //draw jumping as a thick outline if scale mult calls for it
			glLineWidth(3);
		}
		glBegin(GL_LINES);

		if (live_agentsvis != Visual::HEALTH && agent.isDead()){ //draw outline as grey if dead, unless visualizing health
			glColor4f(0.7,0.7,0.7,dead);

		} else {
			//otherwise, color as agent's body if zoomed out, color as above if normal zoomed
			Color3f close_outline_color = Color3f(0);
			if (agent.isTiny()){
				close_outline_color = Color3f(1);
			}

			//if zoomed out, want to blur with our center color
			Color3f outline_color = color;

			glColor4f(
			cap(close_outline_color.red*blur + (1-blur)*outline_color.red),
			cap(close_outline_color.gre*blur + (1-blur)*outline_color.gre),
			cap(close_outline_color.blu*blur + (1-blur)*outline_color.blu),
			dead);
		}
		drawOutlineRes(0, 0, rad, getAgentRes(ghost));
		glEnd();
		glLineWidth(1);


		//some final stuff to render over top of the body but only when zoomed in or on the ghost
		if(scalemult > .1 || ghost){

			//spike, but only if sticking out
			if (agent.isSpikey(world->SPIKE_LENGTH)) {
				glBegin(GL_LINES);
				glColor4f(0.7,0,0,blur);
				glVertex3f(0,0,0);
				glVertex3f((world->SPIKE_LENGTH*agent.spikeLength)*cos(agent.angle),
						   (world->SPIKE_LENGTH*agent.spikeLength)*sin(agent.angle),
						   0);
				glEnd();
			}

			if (scalemult > .3 || ghost) { //render some extra stuff when we're really close
				//blood sense patch
				if (agent.blood_mod > 0.25 && world->BLOOD_SENSE_DISTANCE > 0){
					float count= floorf(agent.blood_mod*4);
					float aa= 0.02*agent.blood_mod;
					float ca= agent.angle - aa*count/2;
					//color coresponds to intensity of the input detected, opacity scaled to level of blood_mod over 0.25
					float alpha= cap(0.5*agent.blood_mod-0.25);
					Color3f bloodcolor= setColorMeat(agent.in[Input::BLOOD]+0.5);
					
					//the blood sense patch is drawn as multiple circles overlapping. Higher the blood_mult trait, the wider the patch
					for (int q = 0; q < count; q++) {
						glBegin(GL_POLYGON);
						glColor4f(bloodcolor.red, bloodcolor.gre, bloodcolor.blu, alpha);
						drawCircle((rad*7/8-1)*cos(ca), (rad*7/8-1)*sin(ca), 0.6*agent.blood_mod);
						glEnd();
						ca+= aa;
					}
				}

				//draw cute little dots for eyes
				for(int q=0;q<NUMEYES;q++) {
					if(agent.isTiny() && !agent.isTinyEye(q)) break;
					if(	(world->modcounter+agent.id)%conf::BLINKDELAY>=q*10 && 
						(world->modcounter+agent.id)%conf::BLINKDELAY<q*10+conf::BLINKTIME && 
						!agent.isDead()) continue; //blink eyes ocasionally... DAWWWWWW

					float ca= agent.angle+agent.eyes[q].dir;
					float eyerad = std::max(conf::TINY_RADIUS, rad);

					glBegin(GL_POLYGON);
					glColor4f(0.85,0.85,0.85,1.0);
					//whites of eyes indicate a > eye cell modifier
					float eyewhitesize= capm(agent.eye_see_cell_mod/2,0.1,2);
					drawCircle((eyerad-1-eyewhitesize/2)*cos(ca), (eyerad-1-eyewhitesize/2)*sin(ca), eyewhitesize);
					glEnd();

					glBegin(GL_POLYGON);
					glColor4f(0,0,0,1.0);
					//the eyes are small and tiny if the agent has a low eye mod. otherwise they are big and buggie
					float eyesize= capm(agent.eye_see_agent_mod/2,0.1,2);
					drawCircle((eyerad-1-eyesize/2)*cos(ca), (eyerad-1-eyesize/2)*sin(ca), eyesize);
					glEnd();
				}

				//render grab target line if we have one
				if(world->GRAB_PRESSURE!=0 && agent.isGrabbing() && !agent.isDead() && agent.grabID!=-1 && !ghost){
					glLineWidth(3);
					glBegin(GL_LINES);

					//agent->agent directed grab vis
					float time= (float)(currentTime)/1000;
					float mid= time - floor(time);
					glColor4f(0,0.85,0.85,1.0);
					glVertex3f(0,0,0);
					glColor4f(0,0.25,0.25,1.0);
					glVertex3f((1-mid)*agent.grabx, (1-mid)*agent.graby, 0);
					glVertex3f((1-mid)*agent.grabx, (1-mid)*agent.graby, 0);
					glColor4f(0,0.85,0.85,1.0);
					glVertex3f(agent.grabx, agent.graby, 0);

					glEnd();
					glLineWidth(1);
				}
			}
		}

		//some final final debug stuff that is also shown even on ghosts:
		if(world->isDebug() || ghost){
			//wheels and wheel speed indicators, color coded: magenta for right, lime for left
			float wheelangle= agent.angle+ M_PI/2;
			glBegin(GL_LINES);
			glColor3f(1,0,1);
			glVertex3f(agent.radius/2*cos(wheelangle), agent.radius/2*sin(wheelangle), 0);
			glVertex3f(agent.radius/2*cos(wheelangle)+35*agent.w1*cos(agent.angle), agent.radius/2*sin(wheelangle)+35*agent.w1*sin(agent.angle), 0);
			wheelangle-= M_PI;
			glColor3f(0,1,0);
			glVertex3f(agent.radius/2*cos(wheelangle), agent.radius/2*sin(wheelangle), 0);
			glVertex3f(agent.radius/2*cos(wheelangle)+35*agent.w2*cos(agent.angle), agent.radius/2*sin(wheelangle)+35*agent.w2*sin(agent.angle), 0);
			glEnd();

			glBegin(GL_POLYGON); 
			glColor3f(0,1,0);
			drawCircle(agent.radius/2*cos(wheelangle), agent.radius/2*sin(wheelangle), 1);
			glEnd();
			wheelangle+= M_PI;
			glColor3f(1,0,1);
			glBegin(GL_POLYGON); 
			drawCircle(agent.radius/2*cos(wheelangle), agent.radius/2*sin(wheelangle), 1);
			glEnd();

			if(!ghost) {
				//draw velocity vector
				glBegin(GL_LINES);
				glColor3f(0,1,1);
				glVertex3f(0,0,0);
				glVertex3f(agent.dpos.x-agent.pos.x, agent.dpos.y-agent.pos.y, 0);
				glEnd();
			}
		}
	}
	glPopMatrix(); //restore world coords
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ END DRAW AGENT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//


void GLView::drawData()
{
	//print global agent stats in top-left region
	int spaceperline= 14;
	
	if(!cullAtCoords(0,-ytranslate) || !cullAtCoords(-UID::GRAPHWIDTH,-ytranslate)){ //don't do this if we can't see the left side
		/*if(scalemult>0.065){
			for(int i= 0; i<DataDisplay::DATADISPLAYS; i++){
				if(i==DataDisplay::ALIVE){
					sprintf(buf, "Num Alive: %i", world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::HERBIVORE){
					sprintf(buf, "Herbivores: %i, %.1f%%", world->getHerbivores(), 100.0*world->getHerbivores()/world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::FRUGIVORE){
					sprintf(buf, "Frugivores: %i, %.1f%%", world->getFrugivores(), 100.0*world->getFrugivores()/world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::CARNIVORE){
					sprintf(buf, "Carnivores: %i, %.1f%%", world->getCarnivores(), 100.0*world->getCarnivores()/world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::AQUATIC){
					sprintf(buf, "Aquatics: %i, %.1f%%", world->getLungWater(), 100.0*world->getLungWater()/world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::AMPHIBIAN){
					sprintf(buf, "Amphibians: %i, %.1f%%", world->getLungAmph(), 100.0*world->getLungAmph()/world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::TERRESTRIAL){
					sprintf(buf, "Terrestrials: %i, %.1f%%", world->getLungLand(), 100.0*world->getLungLand()/world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::DEAD){
					sprintf(buf, "Dead: %i", world->getDead());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::HYBRID){
					sprintf(buf, "Hybrids: %i, %.1f%%", world->getHybrids(), 100.0*world->getHybrids()/world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::SPIKY){
					sprintf(buf, "Spiky: %i, %.1f%%", world->getSpiky(), 100.0*world->getSpiky()/world->getAlive());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::PLANT50){
					sprintf(buf, "50%%+ Plant Cells: %i", world->getFood());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::FRUIT50){
					sprintf(buf, "50%%+ Fruit Cells: %i", world->getFruit());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::MEAT50){
					sprintf(buf, "50%%+ Meat Cells: %i", world->getMeat());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				} else if(i==DataDisplay::HAZARD50){
					sprintf(buf, "50%%+ Hazard Cells: %i", world->getHazards());
					RenderString(-2010, i*spaceperline/scalemult, GLUT_BITMAP_HELVETICA_12, buf, 0.4f, 0.4f, 0.4f);
				}
			}
		}*/

		//draw Graphs
		glTranslatef(-UID::GRAPHBUFFER, 0, 0);
		int worldpop = world->MAXPOP+400 > 500 ? world->MAXPOP+400 : 500;
		float yscaling = conf::HEIGHT / worldpop;
		float xinterval = (float)UID::GRAPHWIDTH / world->REPORTS_PER_EPOCH;
		int agentinterval = yscaling*100;

		//Start with gridlines: horizontal: every 100 agents, vertical: days and epochs, scrolling with data
		glBegin(GL_LINES);
		glColor3f(0.8,0.8,0.8);

		int debugcountvert = 0;
		for(int n=0;n<world->REPORTS_PER_EPOCH;n++) {
			if(world->graphGuides[n]==GuideLine::NONE) continue;
			else if (world->graphGuides[n]==GuideLine::EPOCH) glColor3f(0.8,0.4,0.3);
			glVertex3f(-UID::GRAPHWIDTH + n*xinterval, 0, 0);
			glVertex3f(-UID::GRAPHWIDTH + n*xinterval, conf::HEIGHT, 0);
			debugcountvert++;
			glColor3f(0.8,0.8,0.8);
		}

		int debugcounthoriz = 0;
		for(int n=0; n<((float)conf::HEIGHT/agentinterval); n++) {
			//if(cullAtCoords(-xtranslate,(-n*agentinterval)*scalemult)) break;
			debugcounthoriz++;
			glVertex3f(0, conf::HEIGHT - n*agentinterval, 0);
			glVertex3f(-UID::GRAPHWIDTH, conf::HEIGHT - n*agentinterval, 0);
		}

		//printf("there were %i vertical and %i horizontal lines drawn\n",debugcountvert,debugcounthoriz);
		glEnd();

		//Render data		
		for(int q=0;q<world->REPORTS_PER_EPOCH-1;q++) {
			if(world->numTotal[q]==0) continue;

			//start graphs of aquatics, amphibians, and terrestrials
			Color3f graphcolor= setColorLungs(0.0);
			glBegin(GL_QUADS); 

			//aquatic count
			glColor3f(graphcolor.red,graphcolor.gre,graphcolor.blu);
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval, conf::HEIGHT - yscaling*world->numTotal[q],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval, conf::HEIGHT - yscaling*world->numTotal[q+1],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval, conf::HEIGHT, 0);
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval, conf::HEIGHT, 0);

			//amphibian count
			graphcolor= setColorLungs(0.4);
			glColor3f(graphcolor.red,graphcolor.gre,graphcolor.blu);
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval, conf::HEIGHT - yscaling*world->numAmphibious[q] - yscaling*world->numTerrestrial[q],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval, conf::HEIGHT - yscaling*world->numAmphibious[q+1] - yscaling*world->numTerrestrial[q+1],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval, conf::HEIGHT, 0);
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval, conf::HEIGHT, 0);

			//terrestrial count
			graphcolor= setColorLungs(1.0);
			glColor3f(graphcolor.red,graphcolor.gre,graphcolor.blu);
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval, conf::HEIGHT - yscaling*world->numTerrestrial[q],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval, conf::HEIGHT - yscaling*world->numTerrestrial[q+1],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval, conf::HEIGHT, 0);
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval, conf::HEIGHT, 0);

			glEnd();
	
			glBegin(GL_LINES);
			glColor3f(0,0,0.5); //hybrid count
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval, conf::HEIGHT - yscaling*world->numHybrid[q],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval, conf::HEIGHT - yscaling*world->numHybrid[q+1],0);

			glColor3f(0,1,0); //herbivore count
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval,conf::HEIGHT - yscaling*world->numHerbivore[q],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval,conf::HEIGHT - yscaling*world->numHerbivore[q+1],0);

			glColor3f(1,0,0); //carnivore count
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval,conf::HEIGHT - yscaling*world->numCarnivore[q],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval,conf::HEIGHT - yscaling*world->numCarnivore[q+1],0);

			glColor3f(0.9,0.9,0); //frugivore count
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval,conf::HEIGHT - yscaling*world->numFrugivore[q],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval,conf::HEIGHT - yscaling*world->numFrugivore[q+1],0);

			glColor3f(0,0,0); //total count
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval,conf::HEIGHT - yscaling*world->numTotal[q],0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval,conf::HEIGHT - yscaling*world->numTotal[q+1],0);

			glColor3f(0.8,0.8,0.6); //dead count
			glVertex3f(-UID::GRAPHWIDTH + q*xinterval,conf::HEIGHT - yscaling*(world->numDead[q]+world->numTotal[q]),0);
			glVertex3f(-UID::GRAPHWIDTH +(q+1)*xinterval,conf::HEIGHT - yscaling*(world->numDead[q+1]+world->numTotal[q+1]),0);
			glEnd();
		}

		//current population vertical bars
		glBegin(GL_LINES);
		glVertex3f(0,conf::HEIGHT,0);
		glVertex3f(0,conf::HEIGHT - yscaling*world->getAgents(),0);
		glColor3f(0,0,0);
		glVertex3f(0,conf::HEIGHT,0);
		glVertex3f(0,conf::HEIGHT - yscaling*world->getAlive(),0);
		glEnd();

		//labels for current population bars
		float ydeadlabel = yscaling*world->getAgents(); //since # of dead could go over the top of the graph, clamp it (don't worry about the bar)
		float ymaxdeadlabel = conf::HEIGHT - (UID::CHARHEIGHT + 2)/scalemult;
		if (ydeadlabel > ymaxdeadlabel) ydeadlabel = ymaxdeadlabel;

		sprintf(buf2, "%i dead", world->getDead());
		RenderString((2 - ((float)UID::CHARWIDTH + 1.0f)*(float)strlen(buf2))/scalemult, conf::HEIGHT - ydeadlabel,
			GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 0.8f, 0.6f);

		sprintf(buf2, "%i agents", world->getAlive());
		RenderString((8 - ((float)UID::CHARWIDTH + 1.0f)*(float)strlen(buf2))/scalemult, conf::HEIGHT - yscaling*world->getAlive(),
			GLUT_BITMAP_HELVETICA_12, buf2, 0.0f, 0.0f, 0.0f);

		glTranslatef(UID::GRAPHBUFFER, 0, 0);
	}

	if(getLayerDisplayCount()>0){
		//draw dark background if we have at least one layer displayed, for gridlines
		glBegin(GL_QUADS);
		glColor4f(0,0,0.1,1);
		glVertex3f(0,0,0);
		glVertex3f(conf::WIDTH,0,0);
		glVertex3f(conf::WIDTH,conf::HEIGHT,0);
		glVertex3f(0,conf::HEIGHT,0);
		glEnd();
	}

	glBegin(GL_LINES);
	glColor3f(0,0,0); //border around graphs and feedback

	glVertex3f(0,0,0);
	glVertex3f(-UID::GRAPHWIDTH-UID::GRAPHBUFFER,0,0);

	glVertex3f(-UID::GRAPHWIDTH-UID::GRAPHBUFFER,0,0);
	glVertex3f(-UID::GRAPHWIDTH-UID::GRAPHBUFFER,conf::HEIGHT,0);

	glVertex3f(-UID::GRAPHWIDTH-UID::GRAPHBUFFER,conf::HEIGHT,0);
	glVertex3f(0,conf::HEIGHT,0);
	glEnd();
}


void GLView::drawFinalData()
{
	if (live_lifepath) {
		//render lifepath data!
		glLineWidth(2);
		glBegin(GL_LINES);
		glColor3f(0.5,0.75,0.75);
		for (int i = 1; i < world->lifepath.size(); i++) {
			//skip this draw if is spans too far (like at world borders)
			if (abs(world->lifepath[i-1].x - world->lifepath[i].x) > 100 || abs(world->lifepath[i-1].y - world->lifepath[i].y) > 100) continue;

			float colmul = (float)i/std::max((float)i, (float)world->lifepath.size() - 300);
			glColor3f(0.5*colmul,0.75*colmul,0.75*colmul); //color gradually changes from cyan to black, starting with first indexes

			glVertex3f(world->lifepath[i-1].x, world->lifepath[i-1].y ,0);
			glVertex3f(world->lifepath[i].x, world->lifepath[i].y ,0);
			
			if (i+1 >= world->lifepath.size()) glVertex3f(world->lifepath[i].x, world->lifepath[i].y ,0);
		}
		if (world->lifepath.size() > 10000) pop_front(world->lifepath); //limit the vector so we don't eat the user's memory sticks

		//add render to last vertex, the selected agent coords
		Agent* selected = world->getSelectedAgent();
		if (selected != NULL) glVertex3f(selected->pos.x, selected->pos.y ,0);

		glEnd();
		glLineWidth(1);
	} //end lifepath
}

void GLView::drawStatic()
{
	/*start setup*/
	glMatrixMode(GL_PROJECTION);
	// save previous matrix which contains the
	//settings for the perspective projection
	glPushMatrix();
	glLoadIdentity();

	int ww= wWidth;
	int wh= wHeight;
	// set a 2D orthographic projection
	gluOrtho2D(0, ww, 0, wh);
	// invert the y axis, down is positive
	glScalef(1, -1, 1);
	// move the origin from the bottom left corner to the upper left corner
	glTranslatef(0, -wh, 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	/*end setup*/

	//begin things that we actually want to draw staticly
	//First up, the status displays in the top left
	int currentline= 1;
	int spaceperline= 16;

	for (int line=0; line<StaticDisplay::STATICDISPLAYS; line++){
		//now display the main static displays
		if (line == StaticDisplay::PAUSED)	{
			if(live_paused){
				float redness= 0.5*abs(sin((float)(currentTime)/250));
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Paused", 0.5f+redness, 0.55f-redness, 0.55f-redness);
				currentline++; //this appears in every if statement because we only move to next line if we added a line
			}
		} else if (line == StaticDisplay::FOLLOW) {
			if(live_follow!=0) {
				if(world->getSelectedAgentIndex()>=0) RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Following", 0.75f, 0.75f, 0.75f);
				else RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "No Follow Target", 0.75f, 0.75f, 0.75f);
				currentline++;
			}
		} else if (line == StaticDisplay::AUTOSELECT) {
			if(live_selection!=Select::NONE && live_selection!=Select::MANUAL) {
				switch (live_selection) {
					case Select::RELATIVE : sprintf(buf, "Relatives Autoselect");			break;
					case Select::AGGRESSIVE : sprintf(buf, "Aggression Stat Autoselect");	break;
					case Select::BEST_GEN : sprintf(buf, "Highest Gen. Autoselect");		break;
					case Select::HEALTHY : sprintf(buf, "Healthiest Autoselect");			break;
					case Select::ENERGETIC : sprintf(buf, "Energetic Autoselect");			break;
					case Select::OLDEST : sprintf(buf, "Oldest Autoselect");				break;
					case Select::PRODUCTIVE : sprintf(buf, "Most Children Autoselect");		break;
					case Select::BEST_CARNIVORE : sprintf(buf, "Best Carni. Autoselect");	break;
					case Select::BEST_FRUGIVORE : sprintf(buf, "Best Frugi. Autoselect");	break;
					case Select::BEST_HERBIVORE : sprintf(buf, "Best Herbi. Autoselect");	break;
					case Select::BEST_AQUATIC : sprintf(buf, "Best Aquatic Autoselect");	break;
					case Select::BEST_AMPHIBIAN : sprintf(buf, "Best Amphibian Autoselect"); break;
					case Select::BEST_TERRESTRIAL : sprintf(buf, "Best Terran Autoselect"); break;
					case Select::FASTEST : sprintf(buf, "Fastest Agent Autoselect");		break;
					case Select::SEXIEST : sprintf(buf, "Sexual Projection Autoselect");	break;
					case Select::GENEROUS_EST : sprintf(buf, "Most Generous Autoselect");	break;
					case Select::KINRANGE_EST : sprintf(buf, "Highest Kin Range Autoselect"); break;
					default : sprintf(buf, "UNKNOWN_Select");
				}
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.8f, 0.5f);
				currentline++;
			}
		} else if (line == StaticDisplay::CLOSED) {
			if(world->isClosed()) {
				float shift= 0.5*abs(sin((float)(currentTime)/750));
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Closed World", 1.0f-shift, 0.55f-shift, 0.5f+shift);
				currentline++;
			}
		} else if (line == StaticDisplay::DROUGHT) {
			if(world->isDrought()) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Active Drought", 1.0f, 1.0f, 0.0f);
				currentline++;
			} else if(world->isOvergrowth()) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Active Overgrowth", 0.0f, 0.65f, 0.25f);
				currentline++;
			}
		} else if (line == StaticDisplay::MUTATIONS) {
			if(world->MUTEVENTMULT>1) {
				sprintf(buf, "%ix Mutation Rate", world->MUTEVENTMULT);
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.75f, 0.0f, 1.0f);
				currentline++;
			}
		} else if (line == StaticDisplay::USERCONTROL) {
			if(world->player_control) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "User Control Active", 0.0f, 0.2f, 0.8f);
				currentline++;
			}
		} else if (line == StaticDisplay::DEMO) {
			if(world->isDemo()) {
				float greness= 0.25*abs(cos((float)(currentTime)/500));
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Demo mode", 0.25f-greness, 0.45f+greness, 0.4f-greness);
				currentline++;
			}
		} else if (line == StaticDisplay::DAYCYCLES) {
			if(world->MOONLIT && world->MOONLIGHTMULT==1.0) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Permanent Day", 1.0f, 1.0f, 0.3f);
				currentline++;
			} else if (!world->MOONLIT || world->MOONLIGHTMULT==0) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Moonless Nights", 0.6f, 0.6f, 0.6f);
				currentline++;
			}
		} else if (line == StaticDisplay::CLIMATE) {
			if(world->isIceAge()) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Ice Age", 0.3f, 0.9f, 0.9f);
				currentline++;
			} else if(world->isHadean()) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Hadean Climate", 1.0f, 0.55f, 0.15f);
				currentline++;
			} else if(world->isExtreme()) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Extreme Climate", 1.0f, 0.75f, 0.4f);
				currentline++;
			}
		}
	}

	if (live_worlddetails) {
		//display static world details
		for (int line=0; line<StaticDisplayExtra::STATICDISPLAYS; line++){
			if (line == StaticDisplayExtra::OCEANPERCENT) {
				sprintf(buf, "%% Water: %.3f", 1-world->getLandRatio());
			} else if (line == StaticDisplayExtra::OXYGEN) {
				sprintf(buf, "Avg Oxy Dmg: %.4f", world->HEALTHLOSS_NOOXYGEN*world->agents.size());
			} else if (line >= StaticDisplayExtra::LIVECOUNTS && line <= StaticDisplayExtra::xLIVECOUNTS) {
				int index = line - StaticDisplayExtra::LIVECOUNTS;
				switch (index) {
					case LiveCount::AMPHIBIAN : sprintf(buf, "Amphibians: %i", world->getLungAmph());	break;
					case LiveCount::AQUATIC : sprintf(buf, "Aquatics: %i", world->getLungWater());		break;
					case LiveCount::TERRESTRIAL : sprintf(buf, "Terrans: %i", world->getLungLand());	break;
					case LiveCount::CARNIVORE : sprintf(buf, "Carnivores: %i", world->getCarnivores()); break;
					case LiveCount::FRUGIVORE : sprintf(buf, "Frugivores: %i", world->getFrugivores()); break;
					case LiveCount::HERBIVORE : sprintf(buf, "Herbivores: %i", world->getHerbivores()); break;
					case LiveCount::SPIKED : sprintf(buf, "Spiky: %i", world->getSpiky());				break;
					case LiveCount::HYBRID : sprintf(buf, "Hybrids: %i", world->getHybrids());			break;
					default : sprintf(buf, "UNKNOWN_LiveCount_in_StaticDisplayExtra");
				}
			} else if (line >= StaticDisplayExtra::AVG_LAYERS && line <= StaticDisplayExtra::xAVG_LAYERS) {
				int index = line - StaticDisplayExtra::AVG_LAYERS + 1; //+1 to align with Layers::
				switch (index) {
					case Layer::PLANTS : sprintf(buf, "Avg Plant/cell: %.3f", world->getPlantAvg());	break;
					case Layer::FRUITS : sprintf(buf, "Avg Fruit/cell: %.3f", world->getFruitAvg());	break;
					case Layer::MEATS : sprintf(buf, "Avg Meat/cell: %.3f", world->getMeatAvg());		break;
					case Layer::HAZARDS : sprintf(buf, "Avg Hazard/cell: %.3f", world->getHazardAvg());	break;
					default : sprintf(buf, "UNKNOWN_Layer_in_StaticDisplayExtra");
				}
			}
			RenderString(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.0, 0.0, 0.0);
			currentline++;
		}
	}

	if (world->isDebug()) {
		currentline++;
		for (int line=0; line<StaticDisplayDebug::STATICDISPLAYS; line++){
			//now display the debug static displays
			if (line == StaticDisplayDebug::GLMODCOUNT) {
				sprintf(buf, "GL modcounter: %i", modcounter);
			} else if (line == StaticDisplayDebug::MODCOUNT) {
				sprintf(buf, "World modcounter: %i", world->modcounter);
			} else if (line == StaticDisplayDebug::GLMOUSEPOS) {
				sprintf(buf, "GL mouse pos: (%i, %i)", mousex, mousey);
			}  else if (line == StaticDisplayDebug::WORLDMOUSEPOS) {
				int wx= convertMousePosToWorld(true, mousex);
				int wy= convertMousePosToWorld(false, mousey);
				sprintf(buf, "World mouse pos: (%i, %i)", wx, wy);
			} else if (line == StaticDisplayDebug::GLSCALEMULT) {
				sprintf(buf, "GL scalemult: %.4f", scalemult);
			} else if (line == StaticDisplayDebug::RESOLUTION) {
				sprintf(buf, "Agent res: %i", getAgentRes());
			} else if (line == StaticDisplayDebug::ISRENDERING_) {
				sprintf(buf, "Render flag: %s", isrendering_ ? "true" : "false");
			}
			RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
			currentline++;
		}
	}



	//center axis markers
	glBegin(GL_LINES);
	glColor4f(0,1,0,0.4); //green y-axis
	glVertex3f(ww/2,wh/2,0);
	glVertex3f(ww/2,wh/2+15,0);

	glColor4f(1,0,0,0.4); //red x-axis
	glVertex3f(ww/2,wh/2,0);
	glVertex3f(ww/2+15,wh/2,0);
	glEnd();


	//event display y coord, based on Selected Agent Hud to avoid overlap
	int euy= 20 + 2*UID::BUFFER+((int)ceil((float)LADHudOverview::HUDS/3))*(UID::CHARHEIGHT+UID::BUFFER);

	//event log display
	float ss= 18;
	float toastbase= 0;
	int fadetime= conf::EVENTS_HALFLIFE-20;

	int count= world->events.size();
	if(count>conf::EVENTS_DISP) count= conf::EVENTS_DISP; //cap number of notifications displayed

	for(int eo= 0; eo<count; eo++){
		int counter= world->events[eo].second.second;

		float fade= cap((-abs((float)counter)+conf::EVENTS_HALFLIFE)/20);

		float move= 0;


		if(counter>fadetime) move= powf(((float)counter-fadetime)/7.4,3);
		else if (counter<-fadetime) move= powf(((float)counter+fadetime)/7.4,3);

		if(eo==0){
			toastbase= move;
			//move= 0;
		}

		float red= 0;
		float gre= 0;
		float blu= 0;
		int type= world->events[eo].second.first;
		switch(type) {
			case EventColor::WHITE : 	red= 0.9; gre= 0.9; blu= 0.8;	break; //type 0: white
			case EventColor::RED : 		red= 0.8; gre= 0.1; blu= 0.1;	break; //type 1: red
			case EventColor::GREEN : 	red= 0.1; gre= 0.6;				break; //type 2: green
			case EventColor::BLUE : 	gre= 0.3; blu= 0.9;				break; //type 3: blue
			case EventColor::YELLOW :	red= 0.8; gre= 0.8; blu= 0.1;	break; //type 4: yellow
			case EventColor::CYAN :		gre= 0.9; blu= 0.9;				break; //type 5: cyan
			case EventColor::PURPLE :	red= 0.6; blu= 0.6;				break; //type 6: purple
			case EventColor::BROWN :	red= 0.5; gre= 0.3; blu= 0.1;	break; //type 7: brown
			case EventColor::NEON :		gre= 1.0;						break; //type 8: neon
			case EventColor::MINT :		gre= 0.4; blu= 0.6;				break; //type 9: mint
			case EventColor::MULTICOLOR:red= 1/(1+world->modcounter%31); gre= 1/(1+world->modcounter%23); blu= 1/(1+world->modcounter%14); break; //type 10: all the colors!
			case EventColor::PINK :		red= 1.0; gre= 0.5; blu= 0.5;	break; //type 11: pink
			case EventColor::ORANGE:	red= 0.9; gre= 0.6;				break; //type 12: orange
			case EventColor::BLOOD :	red= 0.5;						break; //type 13: bloodred
			case EventColor::LAVENDER :	red= 0.8; gre= 0.6;	blu= 1.0;	break; //type 14: lavender
			case EventColor::LIME :		red= 0.5; gre= 1.0;	blu= 0.3;	break; //type 15: lime
			case EventColor::BLACK : break;
		}

		glBegin(GL_QUADS);
		glColor4f(red,gre,blu,0.6*fade);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH, euy+5+(2+eo)*ss+toastbase+move,0);
		glVertex3f(ww-UID::BUFFER, euy+5+(2+eo)*ss+toastbase+move,0);
		glColor4f(red*0.75,gre*0.75,blu*0.75,0.6*fade);
		glVertex3f(ww-UID::BUFFER, euy+5+(3+eo)*ss+toastbase+move,0);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH, euy+5+(3+eo)*ss+toastbase+move,0);
		glEnd();

		RenderStringBlack(ww-UID::EVENTSWIDTH, euy+10+2.5*ss+eo*ss+toastbase+move, GLUT_BITMAP_HELVETICA_12, world->events[eo].first.c_str(), 1.0f, 1.0f, 1.0f, fade);
	}

	if(world->events.size()>conf::EVENTS_DISP) {
		//display a blinking graphic indicating there are more notifications
		float blink= 0.5*abs(sin((float)(currentTime)/400));

		glLineWidth(3);
		glBegin(GL_LINES);
		glColor4f(0.55-blink,0.5+blink,0.5+blink,0.75);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH/2-5, euy+10+2*ss+conf::EVENTS_DISP*ss+5*blink,0);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH/2, euy+5+10+2*ss+conf::EVENTS_DISP*ss+5*blink,0);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH/2, euy+5+10+2*ss+conf::EVENTS_DISP*ss+5*blink,0);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH/2+5, euy+10+2*ss+conf::EVENTS_DISP*ss+5*blink,0);
		glEnd();
		glLineWidth(1);
	}

	if(live_paused && world->events.size()>0) {
		//display a blinking graphic indicating notifications are paused when paused
		float redness= 0.5*(1-abs(sin((float)(currentTime)/250)));

		glLineWidth(3);
		glBegin(GL_LINES);
		glColor4f(0.5+redness,0.55-redness,0.55-redness,0.75);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH/2-2.5, euy-5+40,0);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH/2-2.5, euy-15+40,0);
		glEnd();
		glBegin(GL_LINES);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH/2+2.5, euy-5+40,0);
		glVertex3f(ww-UID::BUFFER-UID::EVENTSWIDTH/2+2.5, euy-15+40,0);
		glEnd();
		glLineWidth(1);
	}

	//do the thing:
	renderAllTiles();

	//draw the popup if available
	if(popupxy[0]>=0 && popupxy[1]>=0) {
		//first, split up our popup text and count the number of splits
		int linecount= popuptext.size();
		int maxlen= 1;
		
		for(int l=0; l<linecount; l++) {
			if(popuptext[l].size()>maxlen) maxlen= popuptext[l].size();
		}

		float popuplen= (maxlen+(float)(maxlen*maxlen)/100+1)*5-8;
		float popupheight= (float)linecount*13+6;

		glBegin(GL_QUADS);
		glColor4f(0,0.4,0.6,0.65);
		glVertex3f(popupxy[0],popupxy[1],0);
		glVertex3f(popupxy[0],popupxy[1]+popupheight,0);
		glVertex3f(popupxy[0]+popuplen,popupxy[1]+popupheight,0);
		glVertex3f(popupxy[0]+popuplen,popupxy[1],0);
		glEnd();

		glBegin(GL_LINES);
		glColor3f(0,0,0);
		glVertex3f(popupxy[0],popupxy[1],0);
		glVertex3f(popupxy[0],popupxy[1]+popupheight,0);
		glVertex3f(popupxy[0],popupxy[1]+popupheight,0);
		glVertex3f(popupxy[0]+popuplen,popupxy[1]+popupheight,0);
		glVertex3f(popupxy[0]+popuplen,popupxy[1]+popupheight,0);
		glVertex3f(popupxy[0]+popuplen,popupxy[1],0);
		glVertex3f(popupxy[0]+popuplen,popupxy[1],0);
		glVertex3f(popupxy[0],popupxy[1],0);
		glEnd();

		for(int l=0; l<popuptext.size(); l++) {
			RenderString(popupxy[0]+5, popupxy[1]+13*(l+1), GLUT_BITMAP_HELVETICA_12, popuptext[l].c_str(), 0.8f, 1.0f, 1.0f);
		}
	}

	/*start clean up*/
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	// restore previous projection matrix
	glPopMatrix();

	// get back to modelview mode
	glMatrixMode(GL_MODELVIEW);
	/*end clean up*/
}



void GLView::drawPieDisplay(float x, float y, float size, std::vector<std::pair<std::string,float> > values)
{
	//Draw a pie display: first, get the current sum of all values
	float sum = getSum(values);

	//skip stuff if sum is zero or negative
	if(sum>0) {
		
		//ready to draw
		float angler= 0; //it should be noted that, due to drawQuadrant taking a & b as RATIOS of a whole circle, this is the start angle as a RATIO
		float divsum= 1/sum;

		// render all values
		for(int i=0; i<values.size(); i++){
			glBegin(GL_POLYGON);

			//color is based on the first element, the string. This is input elsewhere
			Color3f *color;
			if (values[i].first==conf::DEATH_SPIKERAISE) color= new Color3f(0.7,0,0);
			else if (values[i].first==conf::DEATH_HAZARD) color= new Color3f(0.45,0,0.5);
			else if (values[i].first==conf::DEATH_BADTERRAIN) color= new Color3f(0,0,0);
			else if (values[i].first==conf::DEATH_GENEROSITY) color= new Color3f(0,0.3,0);
			else if (values[i].first==conf::DEATH_COLLIDE) color= new Color3f(0,0.8,1);
			else if (values[i].first==conf::DEATH_SPIKE || values[i].first==conf::MEAT_TEXT) color= new Color3f(1,0,0);
			else if (values[i].first==conf::DEATH_BITE || values[i].first==conf::FRUIT_TEXT) color= new Color3f(0.8,0.8,0);
			else if (values[i].first==conf::DEATH_NATURAL) color= new Color3f(0.5,0.4,0.1);
			else if (values[i].first==conf::DEATH_TOOMANYAGENTS) color= new Color3f(1,0,1);
			else if (values[i].first==conf::DEATH_BADTEMP) color= new Color3f(0.9,0.6,0);
			else if (values[i].first==conf::DEATH_USER) color= new Color3f(1,1,1);
			else if (values[i].first==conf::DEATH_ASEXUAL || values[i].first==conf::PLANT_TEXT) color= new Color3f(0,0.8,0);
			else color= new Color3f(1.0/(i+2),1.0/(i+2),1.0/(i+2));

			glColor4f(color->red, color->gre, color->blu, 1);

			float adder= values[i].second*divsum;

			drawQuadrant(x, y, size, angler, angler+adder);

			glEnd();

			//print the text
			float disp= 1.5;
			float halfer= angler+0.5*adder;
			if(halfer>0.5) disp= -(float)(0.8*UID::CHARWIDTH*values[i].first.length());
			RenderStringBlack(x+disp+size*sin(halfer*2*M_PI), y+3-(size+8)*cos(halfer*2*M_PI), GLUT_BITMAP_HELVETICA_10, values[i].first.c_str(), color->red, color->gre, color->blu);

			angler+= adder; //add adder to angler to set up for next bass
		}
	}

	//even if total is 0, report the sum on a center white circle
	glBegin(GL_POLYGON);
	glColor4f(1,1,1,1);
	drawCircle(x, y, size*0.5);
	glEnd();

	sprintf(buf, "%.2f", sum);
	RenderString(x-0.5*UID::CHARWIDTH*floorf(3.0+log10((float)sum+1.0)), y+0.5*UID::CHARHEIGHT, GLUT_BITMAP_HELVETICA_12, buf, 0, 0, 0);


}


int* GLView::renderTile(UIElement t, bool maintile, int bevel)
{
	int tx= t.x;
	int ty= t.y;
	int tw= t.w;
	int th= t.h;
	std::string title = t.title;

	int tx2= tx+tw;
	int ty2= ty+th;

	if(tw<=20 || th<=20) bevel= 3;
	if(tw<=10 || th<=10) bevel= 0;

	int tbx= tx+bevel;
	int tby= ty+bevel;
	int tbx2= tx2-bevel;
	int tby2= ty2-bevel;

	static int r[10];

	if(maintile){
		r[0]= tx;
		r[1]= ty;
		r[2]= tw;
		r[3]= th;
		r[4]= tx2;
		r[5]= ty2;
		r[6]= tbx;
		r[7]= tby;
		r[8]= tbx2;
		r[9]= tby2;
		return r;
		//for main tiles we return only an array of values
	}

	//start drawing. Also, BEGIN TEMPLATE
	//main box
	glBegin(GL_POLYGON);
	glColor4f(0,0.4,0.6,0.75);
	glVertex3f(tbx,tby,0); //top-left
	glVertex3f(tbx2,tby,0); //top-right
	glVertex3f(tbx2,tby2,0); //bottom-right
	glVertex3f(tbx,tby2,0); //bottom-left
	glEnd();

	//bevel trapezoids
	if(tw>10 && th>10){ //don't draw them if REALY small
		glBegin(GL_POLYGON); //top
		glColor4f(cap(world->SUN_RED*0.5),cap((0.4+world->SUN_GRE)*0.5),cap((0.6+world->SUN_BLU)*0.5),0.75);
		glVertex3f(tbx,tby,0);
		glVertex3f(tx,ty,0);
		glVertex3f(tx2,ty,0);
		glVertex3f(tbx2,tby,0);
		glEnd();

		glBegin(GL_POLYGON); //right
		glColor4f(0,0.3,0.45,0.75);
		glVertex3f(tbx2,tby,0);
		glVertex3f(tx2,ty,0);
		glVertex3f(tx2,ty2,0);
		glVertex3f(tbx2,tby2,0);
		glEnd();

		glBegin(GL_POLYGON); //bottom
		glColor4f(0,0.25,0.35,0.75);
		glVertex3f(tbx2,tby2,0);
		glVertex3f(tx2,ty2,0);
		glVertex3f(tx,ty2,0);
		glVertex3f(tbx,tby2,0);
		glEnd();

		glBegin(GL_POLYGON); //left
		glColor4f(cap(world->SUN_RED*0.25),cap(0.4+world->SUN_GRE*0.25),cap(0.6+world->SUN_BLU*0.25),0.75);
		glVertex3f(tbx,tby2,0);
		glVertex3f(tx,ty2,0);
		glVertex3f(tx,ty,0);
		glVertex3f(tbx,tby,0);
		glEnd();
	}

	RenderStringBlack(tbx+2, tby+UID::CHARHEIGHT+2, GLUT_BITMAP_HELVETICA_12, title.c_str(), 0.8f, 1.0f, 1.0f);
	//END TEMPLATE

	return r;
}


void GLView::renderAllTiles()
{
	int ww= wWidth;
	int wh= wHeight;

	//for all main tiles... Most of our main tiles are uniquely designed, so process them specially
	for(int ti= 0; ti<maintiles.size(); ti++){
		if(!maintiles[ti].shown) continue;

		int* t;
		t= renderTile(maintiles[ti], true); //true= ONLY get an array of x and y positions for points of interest (see renderTile() above)
		int tx(t[0]),
			ty(t[1]),
			tw(t[2]),
			th(t[3]),
			tx2(t[4]),
			ty2(t[5]),
			tbx(t[6]),
			tby(t[7]),
			tbx2(t[8]),
			tby2(t[9]); // #ratemyinit
		//yes so we are importing an array from the so-called renderTile method because I wanted to minimize the number of times I set those
		//ugly? maybe. Bad code? I'll let you decide when you want to make a change to a single tile you're adding...

		if(ti == MainTiles::LAD){
			//LAD: Loaded / seLected Agent Display
			Agent selected;
			Color3f *bg;

			if(live_cursormode == MouseMode::PLACE_AGENT) {
				selected = world->loadedagent; //load a mock agent
				selected.angle += 2*sin((float)currentTime/200); //make loaded agent spin a little, and flash a selection icon
				if(world->modcounter%20==0) selected.initSplash(20, 1.0,1.0,1.0);
				bg = new Color3f(0,0.45,0.1); //set a cool green background color
			} else {
				//LAD defines
				selected = world->agents[world->getSelectedAgentIndex()]; //load exactly the same agent as the selected
				bg = new Color3f(0,0.4,0.6); //set background to our traditional blue
			}

			//main box
			glBegin(GL_POLYGON);
			glColor4f(bg->red,bg->gre,bg->blu,0.45);
			glVertex3f(tbx,tby,0); //top-left
			glColor4f(bg->red,bg->gre,bg->blu,1);
			glVertex3f(tbx2,tby,0); //top-right
			glVertex3f(tbx2,tby2,0); //bottom-right
			glColor4f(bg->red,bg->gre,bg->blu,0.25);
			glVertex3f(tbx,tby2,0); //bottom-left
			glEnd();

			//trapezoids
			glBegin(GL_POLYGON); //top
			glColor4f(cap((bg->red+world->SUN_RED)*0.5),cap((bg->gre+world->SUN_GRE)*0.5),cap((bg->blu+world->SUN_BLU)*0.5),1);
			glVertex3f(tbx,tby,0);
			glVertex3f(tx,ty,0);
			glVertex3f(tx2,ty,0);
			glVertex3f(tbx2,tby,0);
			glEnd();

			glBegin(GL_POLYGON); //right
			glColor4f(bg->red*0.75,bg->gre*0.75,bg->blu*0.75,1);
			glVertex3f(tbx2,tby,0);
			glVertex3f(tx2,ty,0);
			glVertex3f(tx2,ty2,0);
			glVertex3f(tbx2,tby2,0);
			glEnd();

			glBegin(GL_POLYGON); //bottom
			glColor4f(bg->red*0.5,bg->gre*0.5,bg->blu*0.5,1);
			glVertex3f(tbx2,tby2,0);
			glVertex3f(tx2,ty2,0);
			glColor4f(bg->red*0.5,bg->gre*0.5,bg->blu*0.5,0.25);
			glVertex3f(tx,ty2,0);
			glVertex3f(tbx,tby2,0);
			glEnd();

			glBegin(GL_POLYGON); //left
			glColor4f(cap(bg->red+world->SUN_RED*0.25),cap(bg->gre+world->SUN_GRE*0.25),cap(bg->blu+world->SUN_BLU*0.25),0.25);
			glVertex3f(tbx,tby2,0);
			glVertex3f(tx,ty2,0);
			glVertex3f(tx,ty,0);
			glVertex3f(tbx,tby,0);
			glEnd();

			float centery = (float)(th)/2+3;
			float centerx = tx+centery-13;

			if(live_cursormode == MouseMode::PLACE_AGENT){
				RenderString(tx+centery-35, ty2-15, GLUT_BITMAP_HELVETICA_12, "Place me!", 0.8f, 1.0f, 1.0f);
				ui_ladmode = LADVisualMode::GHOST; //force ghost display mode if we're in PLACE_AGENT mode
			}

			//depending on the current LAD visual mode, we render something in our left-hand space
			if(ui_ladmode == LADVisualMode::GHOST) {
				drawPreAgent(selected, centerx, centery, true);
				drawAgent(selected, centerx, centery, true);

			} else if (ui_ladmode == LADVisualMode::DAMAGES) {
				RenderString(centerx-35, ty+40, GLUT_BITMAP_HELVETICA_12, "Damage Taken:", 0.8f, 1.0f, 1.0f);
				drawPieDisplay(centerx, centery, 32, selected.damages);

			} else if (ui_ladmode == LADVisualMode::INTAKES) {
				RenderString(centerx-35, ty+40, GLUT_BITMAP_HELVETICA_12, "Food Intaken:", 0.8f, 1.0f, 1.0f);
				drawPieDisplay(centerx, centery, 32, selected.intakes);
			} //note NONE is an option and renders nothing, leaving space for other things...

			//Agent ID
			glBegin(GL_QUADS);
			float idlength= 19+UID::CHARWIDTH*floorf(1.0+log10((float)selected.id+1.0));
			float idboxx= centerx-idlength/2-5;
			float idboxy= ty+11;
			float idboxx2= centerx+idlength/2+10;
			float idboxy2= idboxy+14;

			Color3f specie= setColorSpecies(selected.species);
			glColor4f(specie.red,specie.gre,specie.blu,1);
			glVertex3f(idboxx2,idboxy,0);
			glVertex3f(idboxx,idboxy,0); 
			glColor4f(specie.red/2,specie.gre/2,specie.blu/2,1);
			glVertex3f(idboxx,idboxy2,0); 
			glVertex3f(idboxx2,idboxy2,0); 
			glEnd();

			sprintf(buf, "ID: %d", selected.id);
			RenderString(centerx-idlength/2, idboxy2-3, GLUT_BITMAP_HELVETICA_12, buf, 0.8f, 1.0f, 1.0f);

			//Show cause of death if dead
			if(selected.isDead()){
				RenderString(tbx+UID::BUFFER-2, th-UID::TINYTILEWIDTH-UID::BUFFER-UID::TILEBEVEL*2, GLUT_BITMAP_HELVETICA_12, selected.death.c_str(), 0.8f, 1.0f, 1.0f);
			}

			Color3f defaulttextcolor(0.8,1,1);
			Color3f traittextcolor(0.6,0.7,0.8);

			for(int u=0; u<LADHudOverview::HUDS; u++) {
				bool drawbox= false;

				bool isFixedTrait= false;
				Color3f* textcolor= &defaulttextcolor;

				int ux= tx2-5-(UID::BUFFER+UID::HUDSWIDTH)*(3-(int)(u%3));
				int uy= ty+15+UID::BUFFER+((int)(u/3))*(UID::CHARHEIGHT+UID::BUFFER);
				//(ux,uy)= lower left corner of element
				//must + ul to ux, must - uw to uy, for other corners

				//Draw graphs
				if(u==LADHudOverview::HEALTH || u==LADHudOverview::REPCOUNTER || u==LADHudOverview::STOMACH || u==LADHudOverview::EXHAUSTION){
					//all graphs get a trasparent black box put behind them first
					glBegin(GL_QUADS);
					glColor4f(0,0,0,0.7);
					glVertex3f(ux-1,uy+1,0);
					glVertex3f(ux-1,uy-1-UID::CHARHEIGHT,0);
					glVertex3f(ux+1+UID::HUDSWIDTH,uy-1-UID::CHARHEIGHT,0);
					glVertex3f(ux+1+UID::HUDSWIDTH,uy+1,0);

					if(u==LADHudOverview::STOMACH){ //stomach indicators, ux= ww-100, uy= 40
						glColor3f(0,0.6,0);
						glVertex3f(ux,uy-UID::CHARHEIGHT,0);
						glVertex3f(ux,uy-(int)(UID::CHARHEIGHT*2/3),0);
						glVertex3f(selected.stomach[Stomach::PLANT]*UID::HUDSWIDTH+ux,uy-(int)(UID::CHARHEIGHT*2/3),0);
						glVertex3f(selected.stomach[Stomach::PLANT]*UID::HUDSWIDTH+ux,uy-UID::CHARHEIGHT,0);

						glColor3f(0.6,0.6,0);
						glVertex3f(ux,uy-(int)(UID::CHARHEIGHT*2/3),0);
						glVertex3f(ux,uy-(int)(UID::CHARHEIGHT/3),0);
						glVertex3f(selected.stomach[Stomach::FRUIT]*UID::HUDSWIDTH+ux,uy-(int)(UID::CHARHEIGHT/3),0);
						glVertex3f(selected.stomach[Stomach::FRUIT]*UID::HUDSWIDTH+ux,uy-(int)(UID::CHARHEIGHT*2/3),0);

						glColor3f(0.6,0,0);
						glVertex3f(ux,uy-(int)(UID::CHARHEIGHT/3),0);
						glVertex3f(ux,uy,0);
						glVertex3f(selected.stomach[Stomach::MEAT]*UID::HUDSWIDTH+ux,uy,0);
						glVertex3f(selected.stomach[Stomach::MEAT]*UID::HUDSWIDTH+ux,uy-(int)(UID::CHARHEIGHT/3),0);
					} else if (u==LADHudOverview::REPCOUNTER){ //repcounter indicator, ux=ww-200, uy=25
						glColor3f(0,0.7,0.7);
						glVertex3f(ux,uy,0);
						glColor3f(0,0.5,0.6);
						glVertex3f(ux,uy-UID::CHARHEIGHT,0);
						glColor3f(0,0.7,0.7);
						glVertex3f(cap(selected.repcounter/selected.maxrepcounter)*-UID::HUDSWIDTH+ux+UID::HUDSWIDTH,uy-UID::CHARHEIGHT,0);
						glColor3f(0,0.5,0.6);
						glVertex3f(cap(selected.repcounter/selected.maxrepcounter)*-UID::HUDSWIDTH+ux+UID::HUDSWIDTH,uy,0);
					} else if (u==LADHudOverview::HEALTH){ //health indicator, ux=ww-300, uy=25
						glColor3f(0,0.8,0);
						glVertex3f(ux,uy-UID::CHARHEIGHT,0);
						glVertex3f(selected.health/world->HEALTH_CAP*UID::HUDSWIDTH+ux,uy-UID::CHARHEIGHT,0);
						glColor3f(0,0.6,0);
						glVertex3f(selected.health/world->HEALTH_CAP*UID::HUDSWIDTH+ux,uy,0);
						glVertex3f(ux,uy,0);
					} else if (u==LADHudOverview::EXHAUSTION){ //Exhaustion/energy indicator ux=ww-100, uy=25
						float exh= 2/(1+exp(selected.exhaustion));
						glColor3f(0.8,0.8,0);
						glVertex3f(ux,uy,0);
						glVertex3f(ux,uy-UID::CHARHEIGHT,0);
						glColor3f(0.8*exh,0.8*exh,0);
						glVertex3f(exh*UID::HUDSWIDTH+ux,uy-UID::CHARHEIGHT,0);
						glVertex3f(exh*UID::HUDSWIDTH+ux,uy,0);
					}
					//end draw graphs
					glEnd();
				}

				//extra color tile for gene color
				if(u==LADHudOverview::CHAMOVID) {
					glBegin(GL_QUADS);
					glColor4f(0,0,0,0.7);
					glVertex3f(ux-2+UID::HUDSWIDTH-UID::CHARHEIGHT,uy+1,0);
					glVertex3f(ux-2+UID::HUDSWIDTH-UID::CHARHEIGHT,uy-1-UID::CHARHEIGHT,0);
					glVertex3f(ux+1+UID::HUDSWIDTH,uy-1-UID::CHARHEIGHT,0);
					glVertex3f(ux+1+UID::HUDSWIDTH,uy+1,0);

					glColor3f(selected.gene_red,selected.gene_gre,selected.gene_blu);
					glVertex3f(ux+UID::HUDSWIDTH-UID::CHARHEIGHT,uy,0);
					glVertex3f(ux+UID::HUDSWIDTH-UID::CHARHEIGHT,uy-UID::CHARHEIGHT,0);
					glVertex3f(ux+UID::HUDSWIDTH,uy-UID::CHARHEIGHT,0);
					glVertex3f(ux+UID::HUDSWIDTH,uy,0);
					glEnd();
				}

				//write text and values
				//VOLITILE TRAITS:
				if(u==LADHudOverview::HEALTH){
					if(live_agentsvis==Visual::HEALTH) drawbox= true;
					if(selected.health > 10) sprintf(buf, "Health: %.1f/%.0f", selected.health, world->HEALTH_CAP);
					else sprintf(buf, "Health: %.2f/%.0f", selected.health, world->HEALTH_CAP);

				} else if(u==LADHudOverview::REPCOUNTER){
					if(live_agentsvis==Visual::REPCOUNTER) drawbox= true;
					sprintf(buf, "Child: %.2f/%.0f", selected.repcounter, selected.maxrepcounter);
				
				} else if(u==LADHudOverview::EXHAUSTION){
					if(selected.exhaustion>5) sprintf(buf, "Exhausted!");
					else if (selected.exhaustion<2) sprintf(buf, "Energetic!");
					else sprintf(buf, "Tired.");

				} else if(u==LADHudOverview::AGE){
					if(live_agentsvis==Visual::AGE_HYBRID) drawbox= true;
					sprintf(buf, "Age: %.1f days", (float)selected.age/10);

				} else if(u==LADHudOverview::GIVING){
					if(selected.isGiving()) sprintf(buf, "Generous");
					else if(selected.give>conf::MAXSELFISH) sprintf(buf, "Autocentric");
					else sprintf(buf, "Selfish");

//				} else if(u==LADHudOverview::DHEALTH){
//					if(selected.dhealth==0) sprintf(buf, "Delta H: 0");
//					else sprintf(buf, "Delta H: %.2f", selected.dhealth);

				} else if(u==LADHudOverview::SPIKE){
					if(!selected.isDead() && selected.isSpikey(world->SPIKE_LENGTH)){
						float mw = selected.w1 > selected.w2 ? selected.w1 : selected.w2;
						if(mw < 0) mw= 0;
						float val = world->DAMAGE_FULLSPIKE*selected.spikeLength*mw;
						if(val > world->HEALTH_CAP) sprintf(buf, "Spike: Deadly!");//health
						else if(val == 0) sprintf(buf, "Spike: 0 h");
						else sprintf(buf, "Spike: ~%.2f h", val);
					} else sprintf(buf, "Not Spikey");

				} else if(u==LADHudOverview::SEXPROJECT){
					if(selected.isMale()) sprintf(buf, "Sexting (M)");
					else if(selected.isAsexual()) sprintf(buf, "Is Asexual");
					else sprintf(buf, "Sexting (F)");

				} else if(u==LADHudOverview::MOTION){
					if(selected.isAirborne()) sprintf(buf, "Airborne!");
					else if(selected.encumbered > 0) sprintf(buf, "Encumbered x%i", selected.encumbered);
					else if(selected.boost) sprintf(buf, "Boosting");
					else if(abs(selected.w1 - selected.w2) > 0.06) sprintf(buf, "Spinning...");
					else if(abs(selected.w1) > 0.2 || abs(selected.w2) > 0.2) sprintf(buf, "Sprinting");
					else if(abs(selected.w1) > 0.03 || abs(selected.w2) > 0.03) sprintf(buf, "Moving");
					else if(selected.isDead()) sprintf(buf, "Dead.");
					else sprintf(buf, "Idle");

				} else if(u==LADHudOverview::GRAB){
					if(selected.isGrabbing()){
						if(selected.grabID==-1) sprintf(buf, "Grabbing");
						else sprintf(buf, "Hold %d (%.2f)", selected.grabID, selected.grabbing);
					} else sprintf(buf, "Not Grabbing");

				} else if(u==LADHudOverview::BITE){
					if(selected.jawrend!=0) sprintf(buf, "Bite: %.2f h", abs(selected.jawPosition*world->DAMAGE_JAWSNAP));
					else sprintf(buf, "Not Biting");

				} else if(u==LADHudOverview::WASTE){
					if(selected.out[Output::WASTE_RATE]<0.05) sprintf(buf, "Has the Runs");
					else if(selected.out[Output::WASTE_RATE]<0.3) sprintf(buf, "Incontinent");
					else if(selected.out[Output::WASTE_RATE]>0.7) sprintf(buf, "Waste Prudent");
					else sprintf(buf, "Avg Waste Rate");

				} else if(u==LADHudOverview::VOLUME){
					if(live_agentsvis==Visual::VOLUME) drawbox= true;
					if(selected.volume>0.8) sprintf(buf, "Loud! (%.3f)", selected.volume);
					else if(selected.volume<=conf::RENDER_MINVOLUME) sprintf(buf, "Silent.");
					else if(selected.volume<0.2) sprintf(buf, "Quiet (%.3f)", selected.volume);
					else sprintf(buf, "Volume: %.3f", selected.volume);

				} else if(u==LADHudOverview::TONE){
					sprintf(buf, "Tone: %.3f", selected.tone);


				//STATS:
				} else if(u==LADHudOverview::STAT_CHILDREN){
					sprintf(buf, "Children: %d", selected.children);

				} else if(u==LADHudOverview::STAT_KILLED){
					sprintf(buf, "Has Killed: %d", selected.killed);


				//FIXED TRAITS
				} else if(u==LADHudOverview::STOMACH){
					if(live_agentsvis==Visual::STOMACH) drawbox= true;
					if(selected.isHerbivore()) sprintf(buf, "\"Herbivore\"");
					else if(selected.isFrugivore()) sprintf(buf, "\"Frugivore\"");
					else if(selected.isCarnivore()) sprintf(buf, "\"Carnivore\"");
					else sprintf(buf, "\"???\"...");
					isFixedTrait= true;

				} else if(u==LADHudOverview::GENERATION){
					if(live_agentsvis==Visual::GENERATIONS) drawbox= true;
					sprintf(buf, "Gen: %d", selected.gencount);
					isFixedTrait= true;

				} else if(u==LADHudOverview::TEMPPREF){
					if(live_agentsvis==Visual::DISCOMFORT) drawbox= true;
					if(selected.temperature_preference>0.8) sprintf(buf, "Hadean(%.1f)", selected.temperature_preference*100);
					else if(selected.temperature_preference>0.6)sprintf(buf, "Tropical(%.1f)", selected.temperature_preference*100);
					else if (selected.temperature_preference<0.2) sprintf(buf, "Arctic(%.1f)", selected.temperature_preference*100);
					else if (selected.temperature_preference<0.4) sprintf(buf, "Cool(%.1f)", selected.temperature_preference*100);
					else sprintf(buf, "Temperate(%.1f)", selected.temperature_preference*100);
					isFixedTrait= true;

				} else if(u==LADHudOverview::LUNGS){
					if(live_agentsvis==Visual::LUNGS) drawbox= true;
					if(selected.isAquatic()) sprintf(buf, "Aquatic(%.3f)", selected.lungs);
					else if (selected.isTerrestrial()) sprintf(buf, "Terran(%.3f)", selected.lungs);
					else sprintf(buf, "Amphibian(%.2f)", selected.lungs);
					isFixedTrait= true;

				} else if(u==LADHudOverview::SIZE){
					sprintf(buf, "Radius: %.2f", selected.radius);	
					isFixedTrait= true;

				} else if(u==LADHudOverview::BRAINMUTCHANCE){
					if(live_agentsvis==Visual::BRAINMUTATION) drawbox= true;
					sprintf(buf, "Brain ~%%: %.3f", selected.brain_mutation_chance);
					isFixedTrait= true;
				} else if(u==LADHudOverview::BRAINMUTSIZE){ 
					if(live_agentsvis==Visual::BRAINMUTATION) drawbox= true;
					sprintf(buf, "Brain ~Sz: %.3f", selected.brain_mutation_size);
					isFixedTrait= true;
				} else if(u==LADHudOverview::GENEMUTCHANCE){
					if(live_agentsvis==Visual::GENEMUTATION) drawbox= true;
					sprintf(buf, "Gene ~%%: %.3f", selected.gene_mutation_chance);
					isFixedTrait= true;
				} else if(u==LADHudOverview::GENEMUTSIZE){ 
					if(live_agentsvis==Visual::GENEMUTATION) drawbox= true;
					sprintf(buf, "Gene ~Sz: %.3f", selected.gene_mutation_size);
					isFixedTrait= true;

				} else if(u==LADHudOverview::CHAMOVID){
					if(live_agentsvis==Visual::RGB) drawbox= true;
					sprintf(buf, "Camo: %.3f", selected.chamovid);
					isFixedTrait= true;
				
				} else if(u==LADHudOverview::METABOLISM){
					if(live_agentsvis==Visual::METABOLISM) drawbox= true;
					if(selected.metabolism>0.9) sprintf(buf, "Metab:%.2f ++C", selected.metabolism);
					else if(selected.metabolism>0.6) sprintf(buf, "Metab: %.2f +C", selected.metabolism);
					else if(selected.metabolism<0.1) sprintf(buf, "Metab:%.2f ++H", selected.metabolism);
					else if(selected.metabolism<0.4) sprintf(buf, "Metab: %.2f +H", selected.metabolism);
					else sprintf(buf, "Metab: %.2f B", selected.metabolism);
					isFixedTrait= true;

				} else if(u==LADHudOverview::HYBRID){
					if(live_agentsvis==Visual::AGE_HYBRID) drawbox= true;
					if(selected.hybrid) sprintf(buf, "Is Hybrid");
					else if(selected.gencount==0) sprintf(buf, "Was Spawned");
					else sprintf(buf, "Was Budded");
					isFixedTrait= true;

				} else if(u==LADHudOverview::STRENGTH){
					if(live_agentsvis==Visual::STRENGTH) drawbox= true;
					if(selected.strength>0.7) sprintf(buf, "Strong!");
					else if(selected.strength>0.3) sprintf(buf, "Not Weak");
					else sprintf(buf, "Weak!");
					isFixedTrait= true;

				} else if(u==LADHudOverview::NUMBABIES){
					sprintf(buf, "Num Babies: %d", selected.numbabies);
					isFixedTrait= true;

				} else if(u==LADHudOverview::SPECIESID){
					if(live_agentsvis==Visual::SPECIES) drawbox= true;
					sprintf(buf, "Gene: %d", selected.species);
					isFixedTrait= true;

				} else if(u==LADHudOverview::KINRANGE){
					if(live_agentsvis==Visual::CROSSABLE) drawbox= true;
					sprintf(buf, "Kin Range: %d", selected.kinrange);
					isFixedTrait= true;

				} else if(u==LADHudOverview::BRAINSIZE){
					sprintf(buf, "Total Conns: %d", selected.brain.conns.size());
					isFixedTrait= true;

				} else if(u==LADHudOverview::BRAINLIVECONNS){
					sprintf(buf, "Live Conns: %d", selected.brain.lives.size());
					//isFixedTrait= true; //technically is a fixed trait, but it's like, really important!

//				} else if(u==LADHudOverview::DEATH && selected.isDead()){
//					sprintf(buf, selected.death.c_str());
					//isFixedTrait= true; //technically is an unchanging stat, but every agent only ever gets just one, so let's keep it bright

				} else sprintf(buf, "");

				//render box around our stat that we're visualizing to help user follow along
				if(drawbox) {
					glBegin(GL_LINES);
					glColor3f(0.7,0.7,0.75);
					glVertex3f(ux-2,uy+2,0);
					glVertex3f(ux+2+UID::HUDSWIDTH,uy+2,0);
					glVertex3f(ux+2+UID::HUDSWIDTH,uy+2,0);
					glVertex3f(ux+2+UID::HUDSWIDTH,uy-2-UID::CHARHEIGHT,0);
					glVertex3f(ux+2+UID::HUDSWIDTH,uy-2-UID::CHARHEIGHT,0);
					glVertex3f(ux-2,uy-2-UID::CHARHEIGHT,0);
					glVertex3f(ux-2,uy-2-UID::CHARHEIGHT,0);
					glVertex3f(ux-2,uy+2,0);
					glEnd();
					drawbox= false;
				}

				//Render text
				if(isFixedTrait) {
					textcolor= &traittextcolor;
					RenderStringBlack(ux, uy, GLUT_BITMAP_HELVETICA_12, buf, textcolor->red, textcolor->gre, textcolor->blu);
				} else
					RenderString(ux, uy, GLUT_BITMAP_HELVETICA_12, buf, textcolor->red, textcolor->gre, textcolor->blu);
			}


		} else {
			//all other controls
			renderTile(maintiles[ti]);
		}

		//also render any children
		for(int ct=0; ct<maintiles[ti].children.size(); ct++){
			if(maintiles[ti].children[ct].shown) renderTile(maintiles[ti].children[ct]);
		}
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ START DRAW CELLS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//


void GLView::drawCell(int x, int y, const float values[Layer::LAYERS])
{
	int layers= getLayerDisplayCount();
	if (layers!=0) { //count = 0: off. display no layers
		Color3f cellcolor(0.0, 0.0, 0.0);

		//init color additives
		Color3f light= live_layersvis[DisplayLayer::LIGHT] ? setColorLight(values[Layer::LIGHT]) : Color3f(1);

		Color3f terrain= live_layersvis[DisplayLayer::ELEVATION] ? setColorHeight(values[Layer::ELEVATION]) : Color3f(0);

		Color3f plant= live_layersvis[DisplayLayer::PLANTS] ? setColorPlant(values[Layer::PLANTS]) : Color3f(0);

		Color3f hazard= live_layersvis[DisplayLayer::HAZARDS] ? setColorHazard(values[Layer::HAZARDS]) : Color3f(0);

		Color3f fruit= live_layersvis[DisplayLayer::FRUITS] ? setColorFruit(values[Layer::FRUITS]) : Color3f(0);

		Color3f meat= live_layersvis[DisplayLayer::MEATS] ? setColorMeat(values[Layer::MEATS]) : Color3f(0);

		Color3f temp= live_layersvis[DisplayLayer::TEMP] ? setColorTempCell(y) : Color3f(0); //special for temp: until cell-based, convert y coord and process

		if (layers>1) { //if more than one layer selected, some layers display VERY differently
			//set terrain to use terrain instead of just elevation
			terrain= live_layersvis[DisplayLayer::ELEVATION] ? setColorTerrain(values[Layer::ELEVATION]) : Color3f(0);

			//stop fruit from displaying if we're also displaying plants and elevation
			if (live_layersvis[DisplayLayer::PLANTS]){ 
				fruit.red *= 0.65; fruit.gre *= 0.65; fruit.blu *= 0.65;

				if (live_layersvis[DisplayLayer::ELEVATION]) {
					fruit= Color3f(0);

					//stop displaying meat if we're also displaying plants, fruit, and elevation
					if (live_layersvis[DisplayLayer::FRUITS]) meat= Color3f(0);
				}
			}

			if (layers==2) {
				//if only disp light and another layer (except elevation), mix them lightly.
				if(live_layersvis[DisplayLayer::LIGHT] && !live_layersvis[DisplayLayer::ELEVATION]) {
					terrain= Color3f(0.5); // this allows "light" to be "seen"
				}
				//if displaying temp and another layer, mix with temp, and dim elevation too if it's the other one
				if(live_layersvis[DisplayLayer::TEMP]) {
					temp.red*= 0.45; temp.gre*= 0.45; temp.blu*= 0.45;
					if(live_layersvis[DisplayLayer::ELEVATION]) terrain= setColorHeight(values[Layer::ELEVATION]/4+0.25);
				}
								
			} else { 
				//if more than 2 layers displayed, always dim temp
				temp.red*= 0.15; temp.gre*= 0.15; temp.blu*= 0.15;
			}

			if(live_layersvis[DisplayLayer::TEMP]){ //lower brightness of resource layers if also showing temp
				plant.red*= 0.85; plant.gre*= 0.85; plant.blu*= 0.85;
				fruit.red*= 0.85; fruit.gre*= 0.85; fruit.blu*= 0.85;
				meat.red*= 0.85; meat.gre*= 0.85; meat.blu*= 0.85;
				hazard.red*= 0.85; hazard.gre*= 0.85; hazard.blu*= 0.85;
			}

		} else if(live_layersvis[DisplayLayer::LIGHT]) terrain= Color3f(1); //if only light layer, set terrain to 1 so we show something

		//If we're displaying terrain...
		if(live_layersvis[DisplayLayer::ELEVATION]){
			if(values[Layer::ELEVATION]>Elevation::BEACH_MID){ //...and cell is land...
				//...or more specifically mountain, and we're displaying plants, render plants differently
				if (live_layersvis[DisplayLayer::PLANTS] && values[Layer::ELEVATION]==Elevation::MOUNTAIN_HIGH) plant= setColorMountPlant(values[Layer::PLANTS]); //make mountain peaks

				
			} else { //...and cell is water...
				//...then change plant and hazard colors in water
				plant= live_layersvis[DisplayLayer::PLANTS] ? setColorWaterPlant(values[Layer::PLANTS]) : Color3f(0);
				hazard= live_layersvis[DisplayLayer::HAZARDS] ? setColorWaterHazard(values[Layer::HAZARDS]) : Color3f(0);

				
			}

			//need to move this to Control.cpp and make better use of audio streams
//			if(scalemult>0.3 && x%8==0 && y%8==0 && (world->modcounter+x*53+y*19)%600==400) {
//				if(values[Layer::ELEVATION]<=Elevation::BEACH_MID*0.1) 
//					world->tryPlayAudio(conf::SFX_BEACH1, 0, 0, 1.0, cap(scalemult-0.3));
//				//else world->tryPlayAudio(conf::SFX_BEACH1, 0.5+x, 0.5+y, 1.0, cap(scalemult));
//			}
		}

		//If we're displaying hazards and there's a hazard event, render it
		if(live_layersvis[DisplayLayer::HAZARDS] && values[Layer::HAZARDS]>conf::HAZARD_EVENT_POINT) { 
			light.red=1.0; light.gre=1.0; light.blu=1.0; 
		} //lightning from above

		//We're DONE! Mix all the colors!
		cellcolor.red= cap((terrain.red + plant.red + hazard.red + fruit.red + meat.red)*light.red + temp.red);
		cellcolor.gre= cap((terrain.gre + plant.gre + hazard.gre + fruit.gre + meat.gre)*light.gre + temp.gre);
		cellcolor.blu= cap(0.1+(terrain.blu + plant.blu + hazard.blu + fruit.blu + meat.blu)*light.blu + temp.blu);


		glBegin(GL_QUADS);
		glColor4f(cellcolor.red, cellcolor.gre, cellcolor.blu, 1);

		//code below makes cells into divided boxes when zoomed close up
		float gadjust= 0;
		if(scalemult>0.8 || live_grid) gadjust= scalemult<=0 ? 0 : 0.5/scalemult;

		glVertex3f(x*conf::CZ+gadjust,y*conf::CZ+gadjust,0);
		glVertex3f(x*conf::CZ+conf::CZ-gadjust,y*conf::CZ+gadjust,0);
		if (layers>1 && !(layers<3 && live_layersvis[DisplayLayer::TEMP]) && live_layersvis[DisplayLayer::ELEVATION] && live_waterfx){
			//if we're displaying Elevation & Water, switch colors for bottom half for water!
			float waterwave= (values[Layer::ELEVATION]>=Elevation::BEACH_MID) ? 0 : 
				0.03*sin((float)(currentTime-590*x*(x+y)+350*y*y*x)/250);
			glColor4f(cellcolor.red + waterwave*light.red, cellcolor.gre + waterwave*light.gre, cellcolor.blu + waterwave*light.blu, 1);
		}
		glVertex3f(x*conf::CZ+conf::CZ-gadjust,y*conf::CZ+conf::CZ-gadjust,0);
		glVertex3f(x*conf::CZ+gadjust,y*conf::CZ+conf::CZ-gadjust,0);

		//if Land/All draw mode...
		if (layers>1) {
			if(live_layersvis[DisplayLayer::MEATS] && values[Layer::MEATS]>0.03 && scalemult>0.1){
				//draw meat as a pair of centered diamonds, one more transparent than the other
				float meat= values[Layer::MEATS];
				cellcolor= setColorMeat(1.0); //meat on this layer is always bright red, but translucence is applied
				glColor4f(cellcolor.red*light.red, cellcolor.gre*light.gre, cellcolor.blu*(0.9*light.blu+0.1), meat*0.6+0.1);
				float meatsz= 0.25*meat+0.15;

				glVertex3f(x*conf::CZ+0.5*conf::CZ,y*conf::CZ+0.5*conf::CZ+meatsz*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ+meatsz*conf::CZ,y*conf::CZ+0.5*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ,y*conf::CZ+0.5*conf::CZ-meatsz*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ-meatsz*conf::CZ,y*conf::CZ+0.5*conf::CZ,0);
				
				//make a smaller, solid center
				glColor4f(cellcolor.red*light.red, cellcolor.gre*light.gre, cellcolor.blu*light.blu, 1);
				meatsz-= 0.15;

				glVertex3f(x*conf::CZ+0.5*conf::CZ,y*conf::CZ+0.5*conf::CZ+meatsz*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ+meatsz*conf::CZ,y*conf::CZ+0.5*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ,y*conf::CZ+0.5*conf::CZ-meatsz*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ-meatsz*conf::CZ,y*conf::CZ+0.5*conf::CZ,0);
			}

			if(live_layersvis[DisplayLayer::FRUITS] && values[Layer::FRUITS]>0.1 && scalemult>0.3){
				//draw fruit as a bunch of tiny diamonds, no more than 10 of them, scattered in a pseudorandom fashion
				float GR= 89.0/55;
				for(int i= 1; i<values[Layer::FRUITS]*10; i++){
					cellcolor= setColorFruit(1.6);
					//if(values[Layer::ELEVATION]<Elevation::BEACH_MID*0.1) cellcolor= setColorWaterFruit(1.6);
					//pseudo random number output system:
					//inputs: x, y of cell, the index of the fruit in range 0-10
					//method: take initial x and y into vector, and then rotate it again by golden ratio and increase distance by index
					Vector2f point(sinf((float)y),cosf((float)x));
					float dirmult= (x+y)%2==0 ? 1 : -1;
					point.rotate(dirmult*2*M_PI*GR*i);
					point.normalize();
					
//					float adjustx= i%2==0 ? 10-i%3-i/2+i%4 : i%3+i*i/5;
//					float adjusty= i%3==1 ? 3+i%2+i : (i+1)%2+i*i/6
					float fruitposx= x*conf::CZ+conf::CZ*0.5+2*point.x*i;
					float fruitposy= y*conf::CZ+conf::CZ*0.5+2*point.y*i;
					glColor4f(cellcolor.red*light.red, cellcolor.gre*light.gre, cellcolor.blu*light.blu, 1);
					glVertex3f(fruitposx,fruitposy+1.5,0);
					glVertex3f(fruitposx+1.5,fruitposy,0);
					glVertex3f(fruitposx,fruitposy-1.5,0);
					glVertex3f(fruitposx-1.5,fruitposy,0);
				}
			}

			//if(live_layersvis[DisplayLayer::HAZARDS] && values[Layer::HAZARDS]>0.1 && scalemult>0.3){
				//hazard shows as puddles of purple, distorted and in 4 places, each one indicating another 0.25
				//if (values[Layer::HAZARDS] <= conf::HAZARD_EVENT_POINT) {

		}

		if(x==(int)floorf(conf::WIDTH/conf::CZ)-1) {
			//render a "themometer" on the right side, if enabled
			temp= setColorTempCell(y);
			int margin= 1;
			int width= 2;

			glColor4f(temp.red, temp.gre, temp.blu, 1);
			glVertex3f(conf::WIDTH+margin*conf::CZ,	y*conf::CZ,0);
			glVertex3f(conf::WIDTH+margin*conf::CZ+conf::CZ*width, y*conf::CZ,0);
			glVertex3f(conf::WIDTH+margin*conf::CZ+conf::CZ*width, y*conf::CZ+conf::CZ,0);
			glVertex3f(conf::WIDTH+margin*conf::CZ,	y*conf::CZ+conf::CZ,0);
		}

		glEnd();
	}
}

void GLView::setWorld(World* w)
//Control.cpp ???
{
	world = w;
}
