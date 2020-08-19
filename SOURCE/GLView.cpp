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
	//render a string similar to above, but add a black-colored dropshadow beforehand
	int len = (int) strlen(string);
	glColor3f(0,0,0);
	glRasterPos2f(x+1, y+1);
	for (int i = 0; i < len; i++) glutBitmapCharacter(font, string[i]);
	glColor4f(r,g,b,a);
	glRasterPos2f(x, y);
	for (int i = 0; i < len; i++) glutBitmapCharacter(font, string[i]);
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
	//Draw a quadrant based on a and b angles (world reference)
	glVertex3f(x,y,0);
	float n;
	for (int k=0;k<(int)((b-a)*8/M_PI+1);k++) {
		n = k*(M_PI/8)+a;
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
	}
	glVertex3f(x+r*sin(b),y+r*cos(b),0);
	glVertex3f(x,y,0);
}


GLView::GLView(World *s) :
		world(world),
		modcounter(0),
		frames(0),
		lastUpdate(0),
		mousedrag(false),
		uiclicked(false)
{

	xtranslate= 0.0;
	ytranslate= 0.0;
	scalemult= 0.2;
	downb[0]=0;downb[1]=0;downb[2]=0;
	mousex=0;mousey=0;
	popupReset();
	savehelper= new ReadWrite();
	lastsavedtime= 0;
	currentTime= glutGet(GLUT_ELAPSED_TIME);
}

GLView::~GLView()
{

}

void GLView::gotoDefaultZoom()
{
	//reset width height just this once
	wWidth= glutGet(GLUT_WINDOW_WIDTH);
	wHeight= glutGet(GLUT_WINDOW_HEIGHT);

	//do some mathy things to zoom and translate correctly
	float scaleA= 0;
	scaleA= (float)wWidth;
	scaleA/= (conf::WIDTH+2200);
	float scaleB= (float)wHeight/(conf::HEIGHT+150);
	if(scaleA>scaleB) scalemult= scaleB;
	else scalemult= scaleA;
	xtranslate= -(conf::WIDTH-2020)/2;
	ytranslate= -(conf::HEIGHT-80)/2;
	live_follow= 0;
}

void GLView::changeSize(int w, int h)
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
}

void GLView::processMouse(int button, int state, int x, int y)
{
	if(world->isDebug()) printf("MOUSE EVENT: button=%i state=%i x=%i y=%i, scale=%f, mousedrag=%i, uiclicked= %i\n", 
		button, state, x, y, scalemult, mousedrag, uiclicked);
	
	if(!mousedrag){ //dont let mouse click do anything if drag flag is raised
		//check our tile list!
		uiclicked= false;
		checkTileListClicked(maintiles, x, y, state);

		if(!uiclicked && state==1){ //ui was not clicked
			//have world deal with it. First translate to world coordinates though
			int wx= (int) ((x-wWidth*0.5)/scalemult-xtranslate);
			int wy= (int) ((y-wHeight*0.5)/scalemult-ytranslate);

			if(world->processMouse(button, state, wx, wy, scalemult) && live_selection!=Select::RELATIVE) live_selection = Select::MANUAL;
		}
	}

	mousex=x; mousey=y; //update tracked mouse position
	mousedrag= false;

	downb[button]=1-state; //state is backwards, ah well
}

void GLView::processMouseActiveMotion(int x, int y)
{
	if(world->isDebug()) printf("MOUSE MOTION x=%i y=%i, buttons: %i %i %i\n", x, y, downb[0], downb[1], downb[2]);
	
	if(!uiclicked){ //if ui was not clicked

		if (downb[0]==1) {
			//left mouse button drag: pan around
			xtranslate += (x-mousex)/scalemult;
			ytranslate += (y-mousey)/scalemult;
			if (abs(x-mousex)>6 || abs(x-mousex)>6) live_follow= 0;
			//for releasing follow if the mouse is used to drag screen, but there's a threshold
		}

		if (downb[1]==1) {
			//mouse wheel. Change scale
			scalemult -= conf::ZOOM_SPEED*(y-mousey);
			if(scalemult<0.01) scalemult=0.01;
		}

	/*	if(downb[2]==1){ //disabled
			//right mouse button. currently used for context menu
		}*/
	}

	if(abs(mousex-x)>4 || abs(mousey-y)>4) mousedrag= true; //mouse was clearly dragged, don't select agents after
	handlePopup(x, y); //make sure we handle any popups, even if it's active motion
	mousex=x; mousey=y; 
}

void GLView::processMousePassiveMotion(int x, int y)
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

void GLView::handlePopup(int x, int y)
{
	int layers= getLayerDisplayCount();
	if(layers==1) { //want to eventually allow multiple layers up to 3
		//convert mouse x,y to world x,y
		int worldcx= (int)((((float)x-wWidth*0.5)/scalemult-xtranslate));
		int worldcy= (int)((((float)y-wHeight*0.5)/scalemult-ytranslate));

		if (worldcx>=0 && worldcx<conf::WIDTH && worldcy>=0 && worldcy<conf::HEIGHT) { //if somewhere in world...
			char line[128];
			int layer= 0, currentline= 0;
			worldcx/= conf::CZ;
			worldcy/= conf::CZ; //convert to cell coords

			if(world->isDebug()) printf("worldcx: %d, worldcy %d\n", worldcx, worldcy);
			popupReset(x+12, y); //clear and set popup position near mouse

			sprintf(line, "x: %d, y: %d", worldcx, worldcy);
			popupAddLine(line);
			
//			for(int i=0; i<layers; i++){
			if(livearray_layersvis[DisplayLayers::ELEVATION]) {	strcpy(line, "Height:"); layer= Layer::ELEVATION; }
			else if(livearray_layersvis[DisplayLayers::PLANTS]) { strcpy(line, "Plant:"); layer= Layer::PLANTS; }
			else if(livearray_layersvis[DisplayLayers::MEATS]) { strcpy(line, "Meat:");	layer= Layer::MEATS; }
			else if(livearray_layersvis[DisplayLayers::FRUITS]){ strcpy(line, "Fruit:"); layer= Layer::FRUITS; }
			else if(livearray_layersvis[DisplayLayers::HAZARDS]){ strcpy(line, "Waste:"); layer= Layer::HAZARDS; }
			else if(livearray_layersvis[DisplayLayers::TEMP]){ strcpy(line, "Temperature:"); layer= -1;	}
			else if(livearray_layersvis[DisplayLayers::LIGHT]){ strcpy(line, "Light Level:"); layer= Layer::LIGHT; }
			
			popupAddLine(line);

			if(!livearray_layersvis[DisplayLayers::TEMP]) sprintf(line, "value: %.3f", world->cells[layer][worldcx][worldcy]);
			//must handle temp layer different b/c not a real cell layer (for now)
			else sprintf(line, "value: %.3f", 2.0*abs((float)worldcy*conf::CZ/conf::HEIGHT - 0.5));
			popupAddLine(line);

			//Special extra infos
			if(livearray_layersvis[DisplayLayers::ELEVATION]) {
				int landtype= (int)ceilf(world->cells[layer][worldcx][worldcy]*10);
				switch(landtype){
					case Elevation::DEEPWATER_LOW : case 1 : strcpy(line, "Ocean");	break;
					case Elevation::SHALLOWWATER : case 3 : case 4 : strcpy(line, "Shallows");		break;
					case Elevation::BEACH_MID :		strcpy(line, "Beach");		break;
					case Elevation::PLAINS :	strcpy(line, "Plains");		break;
					case Elevation::STEPPE :	strcpy(line, "Steppe");		break;
					case Elevation::HILL :		strcpy(line, "Hill");		break;
					case Elevation::HIGHLAND :	strcpy(line, "Highland");	break;
					case Elevation::MOUNTAIN_HIGH :	strcpy(line, "Mountain");	break;
				}
				popupAddLine(line);
			}
				
			if(livearray_layersvis[DisplayLayers::HAZARDS] && world->cells[layer][worldcx][worldcy]>0.9) {
				strcpy(line, "LIGHTNING STRIKE!");
				popupAddLine(line);
			}
//			}
		}
		else popupReset(); //reset if mouse outside world
	}
	else popupReset(); //reset if viewing none or map
}

void GLView::processNormalKeys(unsigned char key, int x, int y)
{
	menu(key);	
}

void GLView::processSpecialKeys(int key, int x, int y)
{
	menuSpecial(key);	
}

void GLView::processReleasedKeys(unsigned char key, int x, int y)
{
	if (key==32) {//spacebar input [released]
		world->selectedInput(0);
	}
}

void GLView::menu(int key)
{
	if (key == 27 || key=='p') { //[esc] or "p" - pause
		live_paused= !live_paused;
	} else if (key=='h') { //interface help
		//MAKE SURE ALL UI CHANGES CATELOGUED HERE
		world->addEvent("Left-click an agent to select it",5);
		world->addEvent("Press 'f' to follow selection",5);
		world->addEvent("Pan with left-mouse drag or ",5);
		world->addEvent("  use the arrow keys",5);
		world->addEvent("Zoom with middle-mouse drag or ",5);
		world->addEvent("  use the '<'/'>' keys",5);
		world->addEvent("Press 'p' to pause",5);
		world->addEvent("Press 'n' to dismiss these toasts",5);
		world->addEvent("Press 'm' to skip rendering",5);
		world->addEvent("'e' gives selected's wheel inputs",5);
		world->addEvent("Right-click opens more options",5);
		world->addEvent("'0' selects random alive agent",5);
		world->addEvent("'shift+0' selects random agent",5);
		world->addEvent("'1' selects oldest agent",5);
		world->addEvent("'2' selects best generation",5);
		world->addEvent("'3' selects healthiest agent",5);
		world->addEvent("'4' selects most childbaring",5);
		world->addEvent("'5' selects most aggressive",5);
		world->addEvent("'6' selects relatives continually",5);
		world->addEvent("'page up' selects relative, 1st ",5);
		world->addEvent("  speciesID+1, then any in range",5);
		world->addEvent("'page dn' selects relative, 1st",5);
		world->addEvent("  speciesID-1, then any in range",5);
		world->addEvent("'wasd' controls selected agent",5);
		world->addEvent("'end' prints traits of selected",5);
		world->addEvent("'spacebar' activates special input",5);
		world->addEvent("'+' speeds simulation. '-' slows",5);
		world->addEvent("'l' & 'k' cycle layer view",5);
		world->addEvent("'z' & 'x' cycle agent view",5);
		world->addEvent("'c' toggles random spawns",5);
		world->addEvent("Debug mode is required for:",1);
		world->addEvent(" Press 'delete' to kill selected",5);
		world->addEvent(" '/' heals the selected agent",5);
		world->addEvent(" '|' reproduces selected agent",5);
		world->addEvent(" '~' mutates selected agent",5);
	} else if (key=='m') { //drawing
		live_fastmode= !live_fastmode;
		//if (live_fastmode) world->dismissNextEvents(world->events.size());
	} else if (key==43 || key=='+') { //+
		live_skipdraw++;
	} else if (key==45 || key=='-') { //-
		live_skipdraw--;
	} else if (key=='l' || key=='k') { //layer switch; l= "next", k= "previous"
		if (key=='l') ui_layerpreset++;
		else ui_layerpreset--;
		if (ui_layerpreset>LayerPreset::PRESETS-1) ui_layerpreset= LayerPreset::NONE;
		if (ui_layerpreset<LayerPreset::NONE) ui_layerpreset= LayerPreset::PRESETS-1;
	} else if (key=='z' || key=='x') { //change agent visual scheme; x= "next", z= "previous"
		if (key=='x') live_agentsvis++;
		else live_agentsvis--;
		if (live_agentsvis>Visual::VISUALS-1) live_agentsvis= Visual::NONE;
		if (live_agentsvis<Visual::NONE) live_agentsvis= Visual::VISUALS-1;

		//produce notifications if agent visual changed via buttons
		if(past_agentsvis!=live_agentsvis){
			switch(live_agentsvis){
				case Visual::CROSSABLE :	world->addEvent("Viewing Relatives", 5);			break;
				case Visual::DISCOMFORT :	world->addEvent("Viewing Temp Discomfort", 5);		break;
				case Visual::HEALTH :		world->addEvent("Viewing Health and Giving", 5);	break;
				case Visual::LUNGS :		world->addEvent("Viewing Lung Type", 5);			break;
				case Visual::METABOLISM :	world->addEvent("Viewing Metabolism", 5);			break;
				case Visual::NONE :			world->addEvent("Disabled Agent Rendering", 5);		break;
				case Visual::RGB :			world->addEvent("Viewing Agent Colors", 5);			break;
				case Visual::SPECIES :		world->addEvent("Viewing Species IDs", 5);			break;
				case Visual::STOMACH :		world->addEvent("Viewing Stomachs", 5);				break;
				case Visual::VOLUME :		world->addEvent("Viewing Sounds", 5);				break;
			}
			//backup vis setting
			past_agentsvis= live_agentsvis;
		}
	} else if (key=='c') {
		world->setClosed( !world->isClosed() );
		live_worldclosed= (int) world->isClosed();
/*		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->isClosed()) glutChangeToMenuEntry(4, "Open World", 'c');
		else glutChangeToMenuEntry(4, "Close World", 'c');
		glutSetMenu(m_id);*/
	} else if (key=='f') {
		live_follow= !live_follow; //toggle follow selected agent
	} else if(key>48 && key<Select::SELECT_TYPES+48 && key<=57) { //number keys: select mode
		//'1':49, '2':50, ..., '0':48
		if(live_selection!=key-47) live_selection= key-47; 
		else live_selection= Select::NONE;
	} else if(key==48) { //number key 0: select random from alive
		while(true){ //select random agent, among alive
			int idx= randi(0,world->agents.size());
			if (world->agents[idx].health>0.1) {
				world->setSelectedAgent(idx);
				live_selection= Select::MANUAL;
				break;
			}
		}
	} else if(key==')') {
		world->setSelectedAgent(randi(0,world->agents.size())); //select random agent, among all
		live_selection= Select::MANUAL;
	} else if(key=='q') {
		//zoom and translocate to instantly see the whole world
		gotoDefaultZoom();
	}else if (key==62) { //zoom+ >
		scalemult += 10*conf::ZOOM_SPEED;
	}else if (key==60) { //zoom- <
		scalemult -= 10*conf::ZOOM_SPEED;
		if(scalemult<0.01) scalemult=0.01;
	}else if (key==32) { //spacebar input [pressed]
		world->selectedInput(1);
//	}else if (key=='e') { // e: report selected's wheel inputs
//		world->selectedTrace(1);
	}else if (key==119) { //w (move faster)
		world->pcontrol= true;
		float newleft= capm(world->pleft + 0.05 + (world->pright-world->pleft)*0.25, -1, 1);
		world->pright= capm(world->pright + 0.05 + (world->pleft-world->pright)*0.25, -1, 1);
		world->pleft= newleft;
	}else if (key==115) { //s (move slower/reverse)
		world->pcontrol= true;
		float newleft= capm(world->pleft - 0.05 + (world->pright-world->pleft)*0.25, -1, 1);
		world->pright= capm(world->pright - 0.05 + (world->pleft-world->pright)*0.25, -1, 1);
		world->pleft= newleft;
	}else if (key==97) { //a (turn left)
		world->pcontrol= true;
		float newleft= capm(world->pleft - 0.01 - abs(world->pright-world->pleft)*0.25, -1, 1);
		world->pright= capm(world->pright + 0.01 + abs(world->pleft-world->pright)*0.25, -1, 1);
		world->pleft= newleft;
	}else if (key==100) { //d (turn right)
		world->pcontrol= true;
		float newleft= capm(world->pleft + 0.01 + abs(world->pright-world->pleft)*0.25, -1, 1);
		world->pright= capm(world->pright - 0.01 - abs(world->pleft-world->pright)*0.25, -1, 1);
		world->pleft= newleft;
	} else if (key==999) { //player control
		world->setControl(!world->pcontrol);
		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->pcontrol) glutChangeToMenuEntry(1, "Release Agent", 999);
		else glutChangeToMenuEntry(1, "Control Selected (w,a,s,d)", 999);
		glutSetMenu(m_id);

	} else if (key=='r') { //r key is now a defacto debugging key for whatever I need at the time
		world->audio->play2D("sounds/agents/chirp1.ogg");

	// DEBUG ONLY OPTIONS:
	}else if (key==127 && world->isDebug()) { //delete
		world->selectedKill();
	}else if (key=='/' && world->isDebug()) { // / heal selected
		world->selectedHeal();
	}else if (key=='|' && world->isDebug()) { // | reproduce selected
		world->selectedBabys();
	}else if (key=='~' && world->isDebug()) { // ~ mutate selected
		world->selectedMutate();
	}else if (key=='`' && world->isDebug()) { // ` rotate selected stomach type
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
	}else if (key==1007) { // reset
		world->reset();
		world->spawn();
		printf("WORLD RESET!\n");
	} else if (key==1008) { //save world
		handleRW(RWOpen::BASICSAVE);
	} else if (key==1009) { //load world
		handleRW(RWOpen::STARTLOAD);
	} else if (key==1010) { //reload config
		world->readConfig();
		//.cfg/.sav Update live variables
		live_worldclosed= (int)(world->isClosed());
		live_landspawns= (int)(world->DISABLE_LAND_SPAWN);
		live_moonlight= (int)(world->MOONLIT);
		live_droughts= (int)(world->DROUGHTS);
		live_mutevents= (int)(world->MUTEVENTS);
	} else if (key==1011) { //sanitize world (delete agents)
		world->sanitize();
	} else if (key==1012) { //print agent stats
		world->selectedPrint();
	} else if (key=='n') { //dismiss visible world events
		world->dismissNextEvents(conf::EVENTS_DISP);
	} else {
		printf("Unmatched key pressed: %i\n", key);
	}
}

void GLView::menuSpecial(int key) // special control keys
{
	if (key==GLUT_KEY_UP) {
	   ytranslate+= 20/scalemult;
	} else if (key==GLUT_KEY_LEFT) {
		xtranslate+= 20/scalemult;
	} else if (key==GLUT_KEY_DOWN) {
		ytranslate-= 20/scalemult;
	} else if (key==GLUT_KEY_RIGHT) {
		xtranslate-= 20/scalemult;
	} else if (key==GLUT_KEY_PAGE_DOWN) {
		if(world->setSelectionRelative(-1)) live_selection= Select::MANUAL;
	} else if (key==GLUT_KEY_PAGE_UP) {
		if(world->setSelectionRelative(1)) live_selection= Select::MANUAL;
	} else if (key==GLUT_KEY_END) {
		world->selectedPrint();
	}
}

void GLView::glCreateMenu()
{
	//right-click context menu and submenus
	sm1_id= glutCreateMenu(gl_menu); //configs & UI
	glutAddMenuEntry("Dismiss Visible Events (n)", 'n');
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Enter Debug Mode", 1005);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Re(over)write Config", 1006);
	glutAddMenuEntry("Load Config", 1010);

	sm2_id= glutCreateMenu(gl_menu); //spawn new
	glutAddMenuEntry("Agents (])", 1001);
	glutAddMenuEntry("Herbivores", 1002);
	glutAddMenuEntry("Carnivores", 1003);
	glutAddMenuEntry("Frugivores", 1004);

	sm3_id= glutCreateMenu(gl_menu); //selected agent
	glutAddMenuEntry("Control (w,a,s,d)", 999);
	glutAddMenuEntry("debug Printf details", 1012);
	glutAddMenuEntry("debug Heal (/)", '/');
	glutAddMenuEntry("debug Reproduce (|)", '|');
	glutAddMenuEntry("debug Mutate (~)", '~');
	glutAddMenuEntry("debug Delete (del)", 127);

	sm4_id= glutCreateMenu(gl_menu); //world control
	glutAddMenuEntry("Load World",1009);
	glutAddMenuEntry("Sanitize World", 1011);
	glutAddMenuEntry("New World", 1007);
	glutAddMenuEntry("Exit (esc)", 27);

	m_id = glutCreateMenu(gl_menu); 
	glutAddMenuEntry("Fast Mode (m)", 'm');
	glutAddSubMenu("UI & Config...", sm1_id);
	glutAddMenuEntry("Save World",1008);
	glutAddSubMenu("Alter World...", sm4_id);
	glutAddSubMenu("Spawn New...", sm2_id);
	glutAddMenuEntry("Toggle Closed (c)", 'c');
	glutAddSubMenu("Selected Agent...", sm3_id);
	glutAttachMenu(GLUT_RIGHT_BUTTON);
}

void GLView::gluiCreateMenu()
{
	//GLUI menu. Slimy, yet satisfying.
	//must set our init live vars to something. Might as well do it here
	live_worldclosed= 0;
	live_paused= 0;
	live_fastmode= 0;
	live_skipdraw= 1;
	live_agentsvis= Visual::RGB;
	for(int i=0; i<DisplayLayers::DISPLAYS; i++) livearray_layersvis[i]= 1;
	live_profilevis= Profile::NONE;
	live_selection= Select::MANUAL;
	live_follow= 0;
	live_autosave= 0;
	live_debug= world->isDebug();
	live_grid= 0;
	live_hidedead= 0;
	live_landspawns= (int)world->DISABLE_LAND_SPAWN;
	live_moonlight= (int)world->MOONLIT;
	live_droughts= (int)world->DROUGHTS;
	live_droughtmult= (int)world->DROUGHTMULT;
	live_mutevents= (int)world->MUTEVENTS;

	//create GLUI and add the options, be sure to connect them all to their real vals later
	Menu = GLUI_Master.create_glui("Menu",0,1,1);
	Menu->add_checkbox("Pause",&live_paused);
	Menu->add_checkbox("Enable Autosaves",&live_autosave);

	GLUI_Panel *panel_speed= new GLUI_Panel(Menu,"Speed Control");
	Menu->add_checkbox_to_panel(panel_speed,"Fast Mode",&live_fastmode);
	Menu->add_spinner_to_panel(panel_speed,"Speed:",GLUI_SPINNER_INT,&live_skipdraw);

	GLUI_Rollout *rollout_world= new GLUI_Rollout(Menu,"World Options",false);

	Menu->add_button_to_panel(rollout_world,"Load World", RWOpen::STARTLOAD, glui_handleRW);
	Menu->add_button_to_panel(rollout_world,"Save World", RWOpen::BASICSAVE, glui_handleRW);
	Menu->add_button_to_panel(rollout_world,"New World", RWOpen::NEWWORLD, glui_handleRW);
	Menu->add_checkbox_to_panel(rollout_world,"Disable Spawns",&live_worldclosed);
	Menu->add_checkbox_to_panel(rollout_world,"Disable Land Spawns",&live_landspawns);
	Menu->add_checkbox_to_panel(rollout_world,"Moon Light",&live_moonlight);
	Menu->add_checkbox_to_panel(rollout_world,"Mutation Events",&live_mutevents);
	Menu->add_checkbox_to_panel(rollout_world,"Global Droughts",&live_droughts);
	Menu->add_spinner_to_panel(rollout_world,"Drought Mod:",GLUI_SPINNER_FLOAT,&live_droughtmult)->set_speed(0.1); //whoa

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
	
	for(int i=0; i<DisplayLayers::DISPLAYS; i++){
		//this code allows the layers to be defined in any order
		if(i==DisplayLayers::PLANTS) 
			Menu->add_checkbox_to_panel(panel_layers, "Plant", &livearray_layersvis[DisplayLayers::PLANTS]);
		else if(i==DisplayLayers::MEATS) 
			Menu->add_checkbox_to_panel(panel_layers, "Meat", &livearray_layersvis[DisplayLayers::MEATS]);
		else if(i==DisplayLayers::HAZARDS) 
			Menu->add_checkbox_to_panel(panel_layers, "Hazard", &livearray_layersvis[DisplayLayers::HAZARDS]);
		else if(i==DisplayLayers::FRUITS) 
			Menu->add_checkbox_to_panel(panel_layers, "Fruit", &livearray_layersvis[DisplayLayers::FRUITS]);
		else if(i==DisplayLayers::LIGHT) 
			Menu->add_checkbox_to_panel(panel_layers, "Light", &livearray_layersvis[DisplayLayers::LIGHT]);
		else if(i==DisplayLayers::TEMP)
			Menu->add_checkbox_to_panel(panel_layers, "Temp", &livearray_layersvis[DisplayLayers::TEMP]);
		else if(i==DisplayLayers::ELEVATION) 
			Menu->add_checkbox_to_panel(panel_layers, "Elevation", &livearray_layersvis[DisplayLayers::ELEVATION]);
	}

	new GLUI_Separator(rollout_vis);
	GLUI_RadioGroup *group_profiles= new GLUI_RadioGroup(rollout_vis,&live_profilevis);
	new GLUI_StaticText(rollout_vis,"Selected");
	for(int i=0; i<Profile::PROFILES; i++){
		char text[16]= "";
		if(i==Profile::NONE) strcpy(text, "off");
		else if(i==Profile::BRAIN) strcpy(text, "Brain");
		else if(i==Profile::EYES) strcpy(text, "Eyesight");
		else if(i==Profile::INOUT) strcpy(text, "In's/Out's");
		else if(i==Profile::SOUND) strcpy(text, "Sounds");
		
		new GLUI_RadioButton(group_profiles,text);
	}

	Menu->add_column_to_panel(rollout_vis,true);
	GLUI_RadioGroup *group_agents= new GLUI_RadioGroup(rollout_vis,&live_agentsvis);
	new GLUI_StaticText(rollout_vis,"Agents");
	for(int i=0; i<Visual::VISUALS; i++){
		char text[16]= "";
		if(i==Visual::NONE) strcpy(text, "off");
		else if(i==Visual::RGB) strcpy(text, "RGB");
		else if(i==Visual::STOMACH) strcpy(text, "Stomach");
		else if(i==Visual::DISCOMFORT) strcpy(text, "Discomfort");
		else if(i==Visual::VOLUME) strcpy(text, "Volume");
		else if(i==Visual::SPECIES) strcpy(text, "Species ID");
		else if(i==Visual::CROSSABLE) strcpy(text, "Crossable");
		else if(i==Visual::HEALTH) strcpy(text, "Health");
		else if(i==Visual::METABOLISM) strcpy(text, "Metabolism");
		else if(i==Visual::LUNGS) strcpy(text, "Lungs");
//		else if(i==Visual::REPMODE) strcpy(text, "Rep. Mode");

		new GLUI_RadioButton(group_agents,text);
	}
	Menu->add_checkbox_to_panel(rollout_vis, "Hide Dead", &live_hidedead);
	Menu->add_checkbox_to_panel(rollout_vis, "Grid on", &live_grid);
	
	GLUI_Rollout *rollout_xyl= new GLUI_Rollout(Menu,"Selection Mode");
	GLUI_RadioGroup *group_select= new GLUI_RadioGroup(rollout_xyl,&live_selection);
	for(int i=0; i<Select::SELECT_TYPES; i++){
		char text[16]= "";
		if(i==Select::OLDEST) strcpy(text, "Oldest");
		else if(i==Select::BEST_GEN) strcpy(text, "Best Gen.");
		else if(i==Select::HEALTHY) strcpy(text, "Healthiest");
		else if(i==Select::ENERGETIC) strcpy(text, "Most Energetic");
		else if(i==Select::PRODUCTIVE) strcpy(text, "Productive");
		else if(i==Select::AGGRESSIVE) strcpy(text, "Aggressive");
		else if(i==Select::NONE) strcpy(text, "off");
		else if(i==Select::MANUAL) strcpy(text, "Manual");
		else if(i==Select::RELATIVE) strcpy(text, "Relative");
		else if(i==Select::BEST_HERBIVORE) strcpy(text, "Best Herbivore");
		else if(i==Select::BEST_FRUGIVORE) strcpy(text, "Best Frugivore");
		else if(i==Select::BEST_CARNIVORE) strcpy(text, "Best Carnivore");

		new GLUI_RadioButton(group_select,text);
		if(i==Select::NONE) Menu->add_checkbox_to_panel(rollout_xyl, "Follow Selected", &live_follow);
	}
	new GLUI_Separator(rollout_xyl);
	Menu->add_button_to_panel(rollout_xyl, "Save Selected", RWOpen::BASICSAVEAGENT, glui_handleRW);

	Menu->add_checkbox("DEBUG",&live_debug);

	//set to main graphics window
	Menu->set_main_gfx_window(win1);
}


//====================================== READ WRITE ===========================================//

void GLView::handleRW(int action) //glui callback for saving/loading worlds
{
	live_paused= 1; //pause world while we work

	if (action==RWOpen::STARTLOAD){ //basic load option selected
		//Step 1 of loading: check if user really wants to reset world
		if(world->getEpoch()>0){ //no need for alerts if epoch == 0
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

	} else if (action==RWOpen::IGNOREOVERLOAD) {
		//alert window open before load, was ignored by user. Close it first before loading
		Alert->hide();
		handleRW(RWOpen::BASICLOAD);

	} else if (action==RWOpen::BASICLOAD) { //load option promt
		Loader = GLUI_Master.create_glui("Load World",0,50,50);

		new GLUI_StaticText(Loader,"");
		Filename= new GLUI_EditText(Loader,"Enter Save Name (e.g, 'WORLD'):");
		new GLUI_StaticText(Loader,"");
		Filename->set_w(300);
		new GLUI_Button(Loader,"Load", RWClose::BASICLOAD, glui_handleCloses);
		new GLUI_Button(Loader,"Cancel", RWClose::CANCELLOAD, glui_handleCloses);

		Loader->set_main_gfx_window(win1);

	} else if (action==RWOpen::BASICSAVE) { //save option prompt
		Saver = GLUI_Master.create_glui("Save World",0,50,50);

		new GLUI_StaticText(Saver,"");
		Filename= new GLUI_EditText(Saver,"Type Save Name (e.g, 'WORLD'):");
		new GLUI_StaticText(Saver,"");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save", RWClose::BASICSAVE, glui_handleCloses);
		new GLUI_Button(Saver,"Cancel", RWClose::CANCELSAVE, glui_handleCloses);

		Saver->set_main_gfx_window(win1);

	} else if (action==RWOpen::NEWWORLD){ //new world alert prompt
		Alert = GLUI_Master.create_glui("Alert",0,50,50);
		Alert->show();
		new GLUI_StaticText(Alert,"");
		new GLUI_StaticText(Alert,"Are you sure?");
		new GLUI_StaticText(Alert,"This will erase the current world and start a new one."); 
		new GLUI_StaticText(Alert,"");
		GLUI_Button *active= new GLUI_Button(Alert,"Okay", RWClose::NEWWORLD, glui_handleCloses);
		active->activate(1);
		new GLUI_Button(Alert,"Cancel", RWClose::CANCELALERT, glui_handleCloses);

		Alert->set_main_gfx_window(win1);

	} else if (action==RWOpen::BASICSAVEAGENT) { //save selected agent prompt
		Saver = GLUI_Master.create_glui("Save Agent",0,50,50);

		new GLUI_StaticText(Saver,"");
		Filename= new GLUI_EditText(Saver,"Enter Save Name (e.g, 'AGENT'):");
		new GLUI_StaticText(Saver,"");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save", RWClose::BASICSAVEAGENT, glui_handleCloses);
		new GLUI_Button(Saver,"Cancel", RWClose::CANCELSAVE, glui_handleCloses);

	} else if (action==RWOpen::ADVANCEDLOAD) { //advanced loader (NOT FUNCTIONING)
		Loader = GLUI_Master.create_glui("Load World",0,50,50);
		Browser= new GLUI_FileBrowser(Loader,"",false, RWClose::ADVANCEDLOAD, glui_handleCloses);
	}
}

void GLView::handleCloses(int action) //GLUI callback for handling window closing
{
	live_paused= 0;

	if (action==RWClose::BASICLOAD) { //loading
		//Step 2: actual loading
		strcpy(filename,Filename->get_text());
		
		if (checkFileName(filename)) { //file name is valid, but is it a valid file???
			char address[32];
			strcpy(address,"saves\\");
			strcat(address,filename);
			strcat(address,".SAV");

			FILE *fl;
			fl= fopen(address, "r");
			if(!fl) { //file doesn't exist. Alert the user
				Alert = GLUI_Master.create_glui("Alert",0,50,50);

				sprintf(buf,"No file could be found with the name \"%s.SAV\".", filename);
				printf("%s", buf);

				new GLUI_StaticText(Alert,"");
				new GLUI_StaticText(Alert, buf); 
				new GLUI_StaticText(Alert, "Returning to main program." );
				new GLUI_StaticText(Alert,"");
				new GLUI_Button(Alert,"Okay", RWClose::CANCELALERT, glui_handleCloses);
				Alert->show();

			} else {
				fclose(fl); //we are technically going to open the file again in savehelper... oh well

				savehelper->loadWorld(world, xtranslate, ytranslate, filename);

				lastsavedtime= currentTime; //reset last saved time; we have nothing before the load to save!

				//.cfg/.sav make sure to put all saved world variables with GUI options here so they update properly!
				live_worldclosed= (int)world->isClosed();
				live_landspawns= (int)world->DISABLE_LAND_SPAWN;
				live_moonlight= (int)world->MOONLIT;
				live_droughts= (int)world->DROUGHTS;
				live_droughtmult= world->DROUGHTMULT;
				live_mutevents= (int)world->MUTEVENTS;
			}
		}
		Loader->hide();

	} else if (action==RWClose::BASICSAVE) { //saving
		trySaveWorld();
		Saver->hide();

	} else if (action==RWClose::NEWWORLD) { //resetting
		world->reset();
		world->spawn();
		lastsavedtime= currentTime;
		printf("WORLD RESET!\n");
		Alert->hide();
	} else if (action==RWClose::CANCELALERT) { //Alert cancel/continue
		Alert->hide();
	} else if (action==RWClose::BASICSAVEOVERWRITE) { //close alert from save overwriting
		trySaveWorld(true,false);
		Alert->hide();
	} else if (action==RWClose::BASICSAVEAGENT) { //saving agents
		const char *tempname= Filename->get_text();
		strcpy(filename, tempname);
		strcat(filename, ".AGT");

		if (checkFileName(filename)) {
			//check the filename given to see if it exists yet
			char address[32];
			strcpy(address,"saved_agents\\");
			strcat(address,tempname);

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
				FILE* sa = fopen(address, "w");
				int sidx= world->getSelectedAgent();
				if (sidx>=0){
					Agent *a= &world->agents[sidx];
					savehelper->saveAgent(a, sa);
				}
				fclose(sa);
//			}
		}
		Saver->hide();
//	} else if (action==7){ //overwrite agent save
//		FILE* sa = fopen(address, "w");
//		int sidx= world->getSelectedAgent();
//		if (sidx>=0){
//			savehelper->saveAgent(world->agents[sidx], sa);
//		}
//		fclose(sa);
	} else if (action==RWClose::ADVANCEDLOAD) { //advanced loader
		file_name= "";
		file_name= Browser->get_file();
		strcpy(filename,file_name.c_str());

		if (checkFileName(filename)) {
			savehelper->loadWorld(world, xtranslate, ytranslate, filename);
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


bool GLView::checkFileName(char name[32]){

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
{
	if(autosave) strcpy(filename, "AUTOSAVE");
	else strcpy(filename, Filename->get_text());

	if (checkFileName(filename)) {
		//check the filename given to see if it exists yet
		char address[32];
		strcpy(address,"saves\\");
		strcat(address,filename);
		strcat(address,".SAV");

		FILE* ck = fopen(address, "r");
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
			savehelper->saveWorld(world, xtranslate, ytranslate, filename);
			lastsavedtime= currentTime;
			if(!autosave) world->addEvent("Saved World",5);
			else if (!force) world->addEvent("Saved World (overwritten)",5);
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
{
	//create a tile as a child of a given parent
	UIElement newtile(x,y,w,h,key,title);
	parent.addChild(newtile);

}

void GLView::createTile(int x, int y, int w, int h, std::string key, std::string title)
{
	//this creates a main tile with no parent (are you the parent?...)
	UIElement newtile(x,y,w,h,key,title);
	maintiles.push_back(newtile);
}

void GLView::createTile(UIElement &parent, int w, int h, std::string key, std::string title, bool d, bool r)
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
{
	for(int ti= tiles.size()-1; ti>=0; ti--){ //iterate backwards to prioritize last-added buttons
		if(!tiles[ti].shown) continue;
		if(mx>tiles[ti].x && mx<tiles[ti].x+tiles[ti].w &&
			my>tiles[ti].y && my<tiles[ti].y+tiles[ti].h){
			//That's a click! update flag and go do stuff!!

			uiclicked= true; //this must be exposed to mouse state==0 so that it catches drags off of UI elements

			if(tiles[ti].clickable && state==1){ //if clickable, process actions
				if(tiles[ti].key=="Follow" && !live_follow) live_follow= true;
			}

			//finally, check our tile's sub-tiles list, and all their sub-tiles, etc.
			checkTileListClicked(tiles[ti].children, mx, my, state);
		}
	}
}


void GLView::processTiles()
{
	for(int ti= 0; ti<maintiles.size(); ti++){
		if(ti==MainTiles::SAD){
			//SAD: selected agent display
			if(world->getSelectedAgent()<0) { maintiles[ti].hide(); continue; }
			else maintiles[ti].show(); //hide/unhide as needed

			//REALLY neat tucking display code that WON'T work because getSelectAgent returns -1 when nothing selected, but we want to keep rendering it's stuff
			//the solution may be to make the ghost a member of World...
			/*if(world->getSelectedAgent()<0) { 
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
	for(int i=0; i<DisplayLayers::DISPLAYS; i++){
		if(livearray_layersvis[i]==1) count++;
	}
	return count;
}


void GLView::handleIdle()
{
	//set proper window (we don't want to draw on nothing, now do we?!)
	if (glutGetWindow() != win1) glutSetWindow(win1); 

	if(world->isDebug()) printf("S");
	GLUI_Master.sync_live_all();

	//after syncing all the live vars with GLUI_Master, set the vars they represent to their proper values.
	world->setClosed(live_worldclosed);
	world->DISABLE_LAND_SPAWN= (bool)live_landspawns;
	world->MOONLIT=	(bool)live_moonlight;
	world->DROUGHTS= (bool)live_droughts;
	world->DROUGHTMULT= live_droughtmult;
	world->MUTEVENTS= (bool)live_mutevents;
	world->setDebug((bool) live_debug);
	if(world->isDebug()) printf("/");

	if(world->modcounter==0 && world->getEpoch()==0){
		//do some spare actions if this is the first tick of the game (Main Menu here???) or if we just reloaded
		gotoDefaultZoom();
		past_agentsvis= live_agentsvis;

		//create the UI tiles 
		//I don't like destroying and creating these live, but for reasons beyond me, initialization won't accept variables like glutGet()
		int sadheight= 3*UID::BUFFER + ((int)ceil((float)Hud::HUDS*0.333333))*(UID::CHARHEIGHT + UID::BUFFER);
		//The SAD requires a special height measurement based on the Hud construct, expanding it easily if more readouts are added

		maintiles.clear();
		for(int i=0; i<MainTiles::TILES; i++){
			if(i==MainTiles::SAD) {
				createTile(wWidth-UID::SADWIDTH-UID::BUFFER, UID::BUFFER, UID::SADWIDTH, sadheight, "");
				createTile(maintiles[MainTiles::SAD], 20, 20, "Follow", "F", true, false);
			} else if(i==MainTiles::TOOLBOX) {
				createTile(UID::BUFFER, 190, 50, 300, "", "Tools");
				createTile(maintiles[MainTiles::TOOLBOX], UID::BUFFER+UID::TILEMARGIN, 215, 30, 30, "UnpinUI", "UI.");
				createTile(maintiles[MainTiles::TOOLBOX], 30, 30, "test", "test", true, false);
				
				maintiles[MainTiles::TOOLBOX].hide();
			}
		}

	}

	modcounter++;
	if (!live_paused) world->update();

	//pull back some variables which can change in-game that also have GUI selections
	live_droughtmult= world->DROUGHTMULT;

	//autosave world periodically, based on world time
	if (live_autosave==1 && world->modcounter%(world->FRAMES_PER_DAY*10)==0){
		trySaveWorld(true,true);
	}

	//Do some recordkeeping and let's intelligently prevent users from simulating nothing
	if(world->getAgents()<=0 && live_worldclosed==1) {
		live_worldclosed= 0;
		if(live_autosave==1) live_autosave= 0; //I'm gonna guess you don't want to save the world anymore
		world->addEvent("Disabled Closed world, nobody was home!",9);
	}

	//show FPS and other stuff
	currentTime = glutGet(GLUT_ELAPSED_TIME);
	frames++;
	if ((currentTime - lastUpdate) >= 1000) {
		char fastmode= live_fastmode ? '*' : ' ';
		sprintf( buf, "Evagents   %cFPS: %d Alive: %d Herbi: %d Carni: %d Frugi: %d Epoch: %d Day %d",
			fastmode, frames, world->getAgents()-world->getDead(), world->getHerbivores(), world->getCarnivores(),
			world->getFrugivores(), world->getEpoch(), world->getDay() );
		glutSetWindowTitle( buf );
		frames = 0;
		lastUpdate = currentTime;
	}

	if (!live_fastmode) {
		world->dosounds= true;

		if (live_skipdraw>0) {
			if (modcounter%live_skipdraw==0) renderScene();	//increase fps by skipping drawing
		}
		else { //we will decrease fps by waiting using clocks
			clock_t endwait;
			float mult=-0.005*(live_skipdraw-1); //negative b/c live_skipdraw is negative. ugly, ah well
			endwait = clock () + mult * CLOCKS_PER_SEC ;
			while (clock() < endwait) {}
			renderScene();
		}
	} else {
		//world->audio->stopAllSounds(); too jarring to cut off all sounds
		world->dosounds= false;
		//in fast mode, our sim forgets to check nearest relative and reset the selection
		if(live_selection==Select::RELATIVE) world->setSelection(live_selection);
		//but we don't want to do this for all selection types because it slows fast mode down
	}
}

void GLView::renderScene()
{
	if(world->isDebug()) printf("D");
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();

	glTranslatef(wWidth*0.5, wHeight*0.5, 0.0f);	
	glScalef(scalemult, scalemult, 1.0f);
	glTranslatef(xtranslate, ytranslate, 0.0f); //translate the view as needed

	//REF: scalemult= 5.0 is VERY zoomed in, 1= everything should be visible, 0.01= min and is super-zoomed out
	world->audio->setDefault3DSoundMinDistance(100.0f/(scalemult*0.5+1)); //IMPORTANT: this is a true radius that sounds play at 100% volume within
	world->audio->setListenerPosition(vec3df(-xtranslate, -ytranslate, 100.0f/scalemult), vec3df(0,0,1)); //update the listener position. Note the z coord

	//handle world agent selection interface
	world->setSelection(live_selection);
	if (world->getSelection()==-1 && live_selection!=Select::MANUAL && live_selection!=Select::NONE) {
		live_selection= Select::NONE;
		world->addEvent("No valid Autoselect targets", 100);
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
	//Draw data layer on world, cull it if we cannot see left side of world
	//if(!cullAtCoords(0, conf::HEIGHT) || !cullAtCoords(0, 0))  this actually effects the grid background, so I reverted it
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

	if(world->isDebug()) printf("/");
}



//---------------Start color section!-------------------//

Color3f GLView::setColorHealth(float health)
{
	Color3f color;
	color.red= ((int)(health*1000)%2==0) ? (1.0-health/2) : 0;
	color.gre= health<=0.1 ? health : powf(health/2,0.5);
	return color;
}

Color3f GLView::setColorStomach(const float stomach[Stomach::FOOD_TYPES])
{
	Color3f color;
	color.red= cap(stomach[Stomach::MEAT]+stomach[Stomach::FRUIT]-pow(stomach[Stomach::PLANT],2));
	color.gre= cap(pow(stomach[Stomach::PLANT],2)/2+stomach[Stomach::FRUIT]-stomach[Stomach::MEAT]/2);
	color.blu= stomach[Stomach::MEAT]*stomach[Stomach::PLANT]/2;
	return color;
}

Color3f GLView::setColorTempPref(float discomfort)
{
	Color3f color;
	color.red= sqrt(cap(discomfort*2));
	color.gre= discomfort==0 ? 0.75 : (1.75-discomfort)/4;
	color.blu= powf(1-discomfort,3);
	return color;
}

Color3f GLView::setColorMetabolism(float metabolism)
{
	Color3f color;
	color.red= metabolism*metabolism;
	color.gre= 2*metabolism*(1-metabolism);
	color.blu= (1-metabolism)*(1-metabolism);
	return color;
}

Color3f GLView::setColorTone(float tone)
{
	Color3f color;
	color.red= cap(1-2*tone);
	color.gre= 1-fabs(tone-0.5)*2;
	color.blu= cap(2*tone-1);
	return color;
}

Color3f GLView::setColorLungs(float lungs)
{
	Color3f color;
	color.red= cap((0.2+lungs)*(0.4-lungs) + 25*(lungs-0.45)*(0.8-lungs));
	color.gre= cap(2.45*(lungs-0.05)*(1.2-lungs));
	color.blu= cap(1-1.5*lungs);
	return color;
}

Color3f GLView::setColorSpecies(float species)
{
	Color3f color;
	color.red= (cos((float)species/175*M_PI)+1.0)/2.0;
	color.gre= (sin((float)species/111*M_PI)+1.0)/2.0;
	color.blu= (cos((float)species/71*M_PI)+1.0)/2.0;
	return color;
}

Color3f GLView::setColorCrossable(float species)
{
	Color3f color(0.7, 0.7, 0.7);
	//all agents look grey if unrelated or if none is selected, b/c then we don't have a reference

	if(world->getSelectedAgent()>=0){
		float deviation= abs(species - world->agents[world->getSelectedAgent()].species); //species deviation check
		if (deviation==0) { //exact copies: cyan
			color.red= 0.2;
			color.gre= 0.9;
			color.blu= 0.9;
		} else if (deviation<=world->MAXDEVIATION) {
			//reproducable relatives: navy blue
			color.red= 0;
			color.gre= 0;
		} else if (deviation<=3*world->MAXDEVIATION) {
			//un-reproducable relatives: purple
			color.gre= 0.4;
		}
	}
	return color;
}

Color3f GLView::setColorGenerocity(float give)
{
	Color3f color;
	float val= cap(abs(give)/world->FOODTRANSFER)*2/3;
	if(give>0) color.gre= val;
	else color.red= val; 
	return color;
}

Color3f GLView::setColorCellsAll(const float values[Layer::LAYERS])
{
	//this controls the final output colors for every cell when Reality layer selected
	Color3f color;
	bool is_land= (values[Layer::ELEVATION]>Elevation::BEACH_MID*0.1) ? true : false;

	Color3f terrain= setColorTerrain(values[Layer::ELEVATION]);
	Color3f plant= is_land ? setColorPlant(values[Layer::PLANTS]) : setColorWaterPlant(values[Layer::PLANTS]);

	if (values[Layer::ELEVATION]==Elevation::MOUNTAIN_HIGH*0.1) plant= setColorMountPlant(values[Layer::PLANTS]);
	Color3f hazard= is_land ? setColorHazard(values[Layer::HAZARDS]) : setColorWaterHazard(values[Layer::HAZARDS]);
	Color3f light= setColorLight(values[Layer::LIGHT]);

	//if hazard is white (event), then make it show no matter if its night or day
	if(hazard.red==1.0f && hazard.gre==1.0f && hazard.blu==1.0f){ light.red=1.0; light.gre=1.0; light.blu=1.0; }

	color.red= cap((terrain.red + plant.red + hazard.red)*light.red);
	color.gre= cap((terrain.gre + plant.gre + hazard.gre)*light.gre);
	color.blu= cap((terrain.blu + plant.blu + hazard.blu)*light.blu);

	return color;
}

Color3f GLView::setColorTerrain(float val)
{
	Color3f color;
	if(val==1){ //rocky
		color.red= 0.9;
		color.gre= 0.9;
		color.blu= 0.9;
	} else if(val==0.5){ //beach
		color.red= 0.9;
		color.gre= 0.9;
		color.blu= 0.6;
/*	} else if(val==0) { //(deep) water
		color.red= 0.1;
		color.gre= 0.1;
		color.blu= 0.7;*/
	} else if(val==0) { //(shallow) water
		color.red= 0.3;
		color.gre= 0.3;
		color.blu= 0.9;
	} else { //dirt [0.2,0.9]
		color.red= 0.25;
		color.gre= 0.2;
		color.blu= 0.1;
	}
	return color;
}

Color3f GLView::setColorHeight(float val)
{
	Color3f color(val,val,val);
	return color;
}

Color3f GLView::setColorPlant(float val)
{
	Color3f color(0.0,val/2,0.1);
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
	Color3f color(val/2,val/2,0.1);
	return color;
}

Color3f GLView::setColorWaterFruit(float val)
{
	Color3f color(val/2*0.8,val/2*0.5,0.1);
	return color;
}

Color3f GLView::setColorMeat(float val)
{
	Color3f color(val/2,0.0,0.1);
	return color;
}

Color3f GLView::setColorHazard(float val)
{
	Color3f color(val/2,0.0,val/2*0.9+0.1);
	if(val>0.9){
		color.red= 1.0;
		color.gre= 1.0;
		color.blu= 1.0;
	}
	return color;
}

Color3f GLView::setColorWaterHazard(float val)
{
	Color3f color(-val/2*0.2,-val/2*0.3,-val/2);
	if(val>0.9){
		color.red= 1.0;
		color.gre= 1.0;
		color.blu= 1.0;
	}
	return color;
}


Color3f GLView::setColorTemp(float val)
{
	float row= 1-cap(2*abs(val*conf::CZ/conf::HEIGHT - 0.5)-0.02);
	Color3f color(row,(2-row)/2,(1-row)*(1-row)); //revert "row" to "val" to fix for cell-based temp layer
	return color;
}

Color3f GLView::setColorLight(float val)
{
	Color3f color;
	//if moonlight is a thing, then its a blue-grey bg
	if(world->MOONLIT) {
		color.red= world->SUN_RED*val>0.3 ? cap(world->SUN_RED*val) : 0.3;
		color.gre= world->SUN_GRE*val>0.3 ? cap(world->SUN_GRE*val) : 0.3;
		color.blu= world->SUN_BLU*val>0.3 ? cap(world->SUN_BLU*val+0.1) : 0.4;
	} else {
		color.red= cap(world->SUN_RED*val);
		color.gre= cap(world->SUN_GRE*val);
		color.blu= cap(world->SUN_BLU*val+0.1);
	}
	return color;
}



bool GLView::cullAtCoords(int x, int y)
{
	//determine if the object at the x and y WORLD coords can be rendered, returning TRUE
	if(y > wHeight*0.5/scalemult-ytranslate + conf::CZ) return true;
	if(x > wWidth*0.5/scalemult-xtranslate + conf::CZ) return true;
	if(y < -wHeight*0.5/scalemult-ytranslate - conf::CZ) return true;
	if(x < -wWidth*0.5/scalemult-xtranslate - conf::CZ) return true;
	return false;
}
	


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~DRAW BEFORE AGENTs~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//



void GLView::drawPreAgent(const Agent& agent, float x, float y, bool ghost)
{
	float n;
	float r= agent.radius;
	float rp= agent.radius+2.8;

	if ((live_agentsvis!=Visual::NONE && (live_hidedead==0 || agent.health>0)) || ghost) {
		//first, calculate colors for indicators (NOTE: Indicator tags are gone now, consider removing these too if unneeded)
		Color3f healthcolor= setColorHealth(agent.health);
		Color3f stomachcolor= setColorStomach(agent.stomach);
		Color3f discomfortcolor= setColorTempPref(agent.discomfort);
		Color3f metabcolor= setColorMetabolism(agent.metabolism);
		Color3f tonecolor= setColorTone(agent.tone);
		Color3f lungcolor= setColorLungs(agent.lungs);
		Color3f speciescolor= setColorSpecies(agent.species);
		Color3f crossablecolor= setColorCrossable(agent.species);
		Color3f generocitycolor= setColorGenerocity(agent.dhealth);

		glPushMatrix(); //switch to local position coordinates
		glTranslatef(x,y,0);

		if(agent.id==world->getSelection() && !ghost){

			if(world->isDebug()){
				//draw DIST range on selected in DEBUG
				glBegin(GL_LINES);
				glColor3f(1,0,1);
				drawOutlineRes(0, 0, world->DIST, ceil(scalemult)+6);
				glEnd();

				//spike effective zone
				if(agent.isSpikey(world->SPIKELENGTH)){
					glBegin(GL_POLYGON);
					glColor4f(1,0,0,0.25);
					glVertex3f(0,0,0);
					glVertex3f((r+agent.spikeLength*world->SPIKELENGTH)*cos(agent.angle+M_PI/8),
							   (r+agent.spikeLength*world->SPIKELENGTH)*sin(agent.angle+M_PI/8),0);
					glColor4f(1,0.25,0.25,0.75);
					glVertex3f((r+agent.spikeLength*world->SPIKELENGTH)*cos(agent.angle),
							   (r+agent.spikeLength*world->SPIKELENGTH)*sin(agent.angle),0);
					glColor4f(1,0,0,0.25);
					glVertex3f((r+agent.spikeLength*world->SPIKELENGTH)*cos(agent.angle-M_PI/8),
							   (r+agent.spikeLength*world->SPIKELENGTH)*sin(agent.angle-M_PI/8),0);
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
					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle+M_PI/4),
							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle+M_PI/4),0);
					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle+M_PI/8),
							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle+M_PI/8),0);
					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle),
							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle),0);
					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle-M_PI/8),
							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle-M_PI/8),0);
					glVertex3f((r+world->GRABBING_DISTANCE)*cos(agent.angle+agent.grabangle-M_PI/4),
							   (r+world->GRABBING_DISTANCE)*sin(agent.angle+agent.grabangle-M_PI/4),0);
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

				//debug eyesight FOV's
				for(int q=0;q<NUMEYES;q++) {
					if(agent.isTiny() && !agent.isTinyEye(q)) break;
					float aa= agent.angle+agent.eyedir[q]+agent.eyefov[q]/2;
					glBegin(GL_POLYGON);

					glColor4f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3],0.15);

					glVertex3f(world->DIST*cos(aa), world->DIST*sin(aa), 0);
					aa-= agent.eyefov[q]/2;
					glVertex3f(world->DIST*cos(aa), world->DIST*sin(aa), 0);
					aa-= agent.eyefov[q]/2;
					glVertex3f(world->DIST*cos(aa), world->DIST*sin(aa), 0);
					glColor4f(0.5,0.5,0.5,0.0);
					glVertex3f(0,0,0);
					glEnd();
				}
			}

			glTranslatef(-90,24+0.5*r,0);

			if(scalemult > .2){
				//draw profile(s)
				float col;
				float yy=15;
				float xx=15;
				float ss=16;

				if(live_profilevis==Profile::INOUT || live_profilevis==Profile::BRAIN){
					//Draw inputs and outputs in in/out mode AND brain mode
					glBegin(GL_QUADS);
					for (int j=0;j<Input::INPUT_SIZE;j++) {
						col= agent.in[j];
						glColor3f(col,col,col);
						glVertex3f(0+ss*j, 0, 0.0f);
						glVertex3f(xx+ss*j, 0, 0.0f);
						glVertex3f(xx+ss*j, yy, 0.0f);
						glVertex3f(0+ss*j, yy, 0.0f);
						if(scalemult > .7){
							glEnd();
							if(j<=Input::xEYES && j%3==0){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "R", 1.0f, 0.0f, 0.0f);
							} else if(j<=Input::xEYES && j%3==1){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "G", 0.0f, 1.0f, 0.0f);
							} else if(j<=Input::xEYES && j%3==2){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "B", 0.0f, 0.0f, 1.0f);
							} else if(j==Input::CLOCK1 || j==Input::CLOCK2 || j==Input::CLOCK3){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "Q", 0.0f, 0.0f, 0.0f);
							} else if(j==Input::TEMP){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "T", col,(2-col)/2,(1-col));
							} else if(j==Input::HEARING1 || j==Input::HEARING2){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "E", 1.0f, 1.0f, 1.0f);
							} else if(j==Input::HEALTH){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "H", healthcolor.red, healthcolor.gre, healthcolor.blu);
							} else if(j==Input::BLOOD){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "B", 0.6f, 0.0, 0.0f);
							} else if(j==Input::FRUIT_SMELL){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "S", 1.0f, 1.0f, 0.0f);
							} else if(j==Input::HAZARD_SMELL){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "S", 0.9f, 0.0f, 0.81f);
							} else if(j==Input::MEAT_SMELL){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "S", 1.0f, 0.0f, 0.1f);
							} else if(j==Input::WATER_SMELL){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "S", 0.3f, 0.3f, 0.9f);
							} else if(j==Input::BOT_SMELL){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "S", 1.0f, 1.0f, 1.0f);
							} else if(j==Input::PLAYER){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "+", 1.0f, 1.0f, 1.0f);
							}
							glBegin(GL_QUADS);
						}
					}
					yy+=5;
					for (int j=0;j<Output::OUTPUT_SIZE;j++) {
						col= agent.out[j];
						if(j==Output::RED) glColor3f(col,0,0);
						else if (j==Output::GRE) glColor3f(0,col,0);
						else if (j==Output::BLU) glColor3f(0,0,col);
						else if (j==Output::JUMP) glColor3f(col,col,0);
						else glColor3f(col,col,col);
						glVertex3f(0+ss*j, yy, 0.0f);
						glVertex3f(xx+ss*j, yy, 0.0f);
						glVertex3f(xx+ss*j, yy+ss, 0.0f);
						glVertex3f(0+ss*j, yy+ss, 0.0f);
						if(scalemult > .7){
							glEnd();
							if(j==Output::LEFT_WHEEL_B || j==Output::LEFT_WHEEL_F){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "!", 1.0f, 0.0f, 1.0f);
							} else if(j==Output::RIGHT_WHEEL_B || j==Output::RIGHT_WHEEL_F){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "!", 0.0f, 1.0f, 0.0f);
							} else if(j==Output::VOLUME){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "V", 1.0f, 1.0f, 1.0f);
							} else if(j==Output::TONE){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "T", tonecolor.red, tonecolor.gre, tonecolor.blu);
							} else if(j==Output::CLOCKF3){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "Q", 0.0f, 0.0f, 0.0f);
							} else if(j==Output::SPIKE){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "S", 1.0f, 0.0f, 0.0f);
							} else if(j==Output::PROJECT){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "P", 0.5f, 0.0f, 0.5f);
							} else if(j==Output::JAW){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, ">", 1.0f, 1.0f, 0.0f);
							} else if(j==Output::GIVE){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "G", 0.0f, 0.3f, 0.0f);
							} else if(j==Output::GRAB){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "G", 0.0f, 0.6f, 0.6f);
							} else if(j==Output::GRAB_ANGLE){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "<", 0.0f, 0.6f, 0.6f);
							} else if(j==Output::JUMP){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "J", 1.0f, 1.0f, 0.0f);
							} else if(j==Output::BOOST){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "B", 0.6f, 0.6f, 0.6f);
							} else if(j==Output::WASTE_RATE){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "W", 0.9f, 0.0f, 0.81f);
							}
							glBegin(GL_QUADS);
						}

					}
					yy+=ss*2;
					glEnd();
				}

				//Draw one of the profilers based on the mode we have selected
				if(live_profilevis==Profile::BRAIN){
					//draw brain in brain profile mode, complements the i/o as drawn above
					glBegin(GL_QUADS);
					float offx=0;
					ss=8;
					xx=ss;
					for (int j=0;j<agent.brain.boxes.size();j++) {
						col = agent.brain.boxes[j].out;
						glColor3f(col,col,col);
						
						glVertex3f(offx+0+ss*j, yy, 0.0f);
						glVertex3f(offx+xx+ss*j, yy, 0.0f);
						glVertex3f(offx+xx+ss*j, yy+ss, 0.0f);
						glVertex3f(offx+ss*j, yy+ss, 0.0f);
						
						if ((j+1)%30==0) {
							yy+=ss;
							offx-=ss*30;
						}
					}
					glEnd();

				} else if (live_profilevis==Profile::EYES){
					//eyesight profile. Draw a box with colored disks
					glBegin(GL_QUADS);
					glColor3f(0,0,0.1);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 40, 0);
					glVertex3f(180, 40, 0);
					glVertex3f(180, 0, 0);
					glEnd();

					for(int q=0;q<NUMEYES;q++){
						glBegin(GL_POLYGON);
						glColor3f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3]);
						drawCircle(165-agent.eyedir[q]/2/M_PI*150,20,agent.eyefov[q]/M_PI*20);
						glEnd();
					}

					glBegin(GL_LINES);
					glColor3f(0.5,0.5,0.5);
					glVertex3f(0, 40, 0);
					glVertex3f(180, 40, 0);
					glVertex3f(180, 0, 0);
					glVertex3f(0, 0, 0);

					glColor3f(0.7,0,0);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 40, 0);
					glVertex3f(180, 40, 0);
					glVertex3f(180, 0, 0);
					
					glEnd();

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
						glColor4f(1-(q/(NUMEARS-1)),q/(NUMEARS-1),0,0.10+0.10*(1-q/(NUMEARS-1)));
						if(agent.hearlow[q]+world->SOUNDPITCHRANGE*2<agent.hearhigh[q]){
							glVertex3f(2+176*cap(agent.hearlow[q]+world->SOUNDPITCHRANGE), 2, 0);
							glVertex3f(2+176*cap(agent.hearlow[q]+world->SOUNDPITCHRANGE), 78, 0);
							glVertex3f(2+176*cap(agent.hearhigh[q]-world->SOUNDPITCHRANGE), 78, 0);
							glVertex3f(2+176*cap(agent.hearhigh[q]-world->SOUNDPITCHRANGE), 2, 0);
						}

						glVertex3f(2+176*agent.hearlow[q], 2, 0);
						glVertex3f(2+176*agent.hearlow[q], 78, 0);
						glVertex3f(2+176*agent.hearhigh[q], 78, 0);
						glVertex3f(2+176*agent.hearhigh[q], 2, 0);
					}
					glEnd();

					glBegin(GL_LINES);
					for(int e=0;e<world->selectedSounds.size();e++) {
						//unpackage each sound entry
						float volume= (int)world->selectedSounds[e];
						float tone= world->selectedSounds[e]-volume;
						volume/= 100;

						float fiz= 1;
						if(volume>=1) volume= cap(volume-1.0);
						else fiz= 0.4;

						if(tone==0.25) glColor4f(0.0,0.8,0.0,fiz); //this is the wheel sounds of other agents
						else glColor4f(0.7,0.7,0.7,fiz);

						glVertex3f(2+176*tone, 78, 0);
						glVertex3f(2+176*tone, 78-76*volume, 0);

						if(tone==0.25){ //display half-magenta line for the wheel sounds of others
							glColor4f(0.8,0.0,0.8,fiz);
							glVertex3f(2+176*tone, 78, 0);
							glVertex3f(2+176*tone, 78-38*volume, 0);
						}
					}
					glEnd();

					//now show our own sound, colored by tone
					glLineWidth(3);
					glBegin(GL_LINES);
					glColor4f(tonecolor.red, tonecolor.gre, tonecolor.blu, 0.5);
					glVertex3f(2+176*agent.tone, 78, 0);
					glVertex3f(2+176*agent.tone, 78-76*agent.volume, 0);
					glEnd();
					glLineWidth(1);

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
			xx=ss;
			for (int j=0;j<BRAINSIZE;j++) {
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
			float mag= (live_agentsvis==Visual::HEALTH) ? 3 : 1;//draw sharing as a thick green or red outline, bigger if viewing health
			glColor3f(generocitycolor.red, generocitycolor.gre, generocitycolor.blu);
			drawCircle(0, 0, rp*mag);
			glEnd();
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
			glVertex3f(xo,yo+21*(2-agent.health),0);
			glVertex3f(xo+5,yo+21*(2-agent.health),0);
			glColor3f(0,0.6,0);
			glVertex3f(xo+5,yo+42,0);
			glVertex3f(xo,yo+42,0);

			//repcounter/energy
			xo+= 7;
			glBegin(GL_QUADS);
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
			glEnd();
			
			if(world->isDebug()) {
				glTranslatef(8+agent.radius, -32, 0);
				//print hidden stats & values
				if(agent.near) RenderString(0, 0, GLUT_BITMAP_HELVETICA_12, "near!", 0.8f, 1.0f, 1.0f);

				//wheel speeds
				sprintf(buf2, "w1: %.3f", agent.w1);
				RenderString(0, -12, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 0.0f, 1.0f);

				sprintf(buf2, "w2: %.3f", agent.w2);
				RenderString(0, -24, GLUT_BITMAP_HELVETICA_12, buf2, 0.0f, 1.0f, 0.0f);

				//exhaustion readout
				sprintf(buf2, "exh: %.3f", agent.exhaustion);
				RenderString(0, -36, GLUT_BITMAP_HELVETICA_12, buf2, 1.0f, 1.0f, 0.0f);
			}
		}

		//end local coordinate stuff
		glPopMatrix();

		if(agent.id==world->getSelection() && !ghost) {//extra info for selected agent
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
				
				int minx, maxx, miny, maxy;
				int scx= (int) (agent.pos.x/conf::CZ);
				int scy= (int) (agent.pos.y/conf::CZ);

				minx= (scx-world->DIST/conf::CZ/2) > 0 ? (scx-world->DIST/conf::CZ/2)*conf::CZ : 0;
				maxx= (scx+1+world->DIST/conf::CZ/2) < conf::WIDTH/conf::CZ ? (scx+1+world->DIST/conf::CZ/2)*conf::CZ : conf::WIDTH;
				miny= (scy-world->DIST/conf::CZ/2) > 0 ? (scy-world->DIST/conf::CZ/2)*conf::CZ : 0;
				maxy= (scy+1+world->DIST/conf::CZ/2) < conf::HEIGHT/conf::CZ ? (scy+1+world->DIST/conf::CZ/2)*conf::CZ : conf::HEIGHT;

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

	if ((live_agentsvis!=Visual::NONE && (live_hidedead==0 || agent.health>0)) || ghost) {
		// don't render if visual set to NONE, or if agent is dead and we're hiding dead

		//agent body color assignment
		float red= 0,gre= 0,blu= 0;

		//first, calculate colors
		Color3f healthcolor= setColorHealth(agent.health);
		Color3f stomachcolor= setColorStomach(agent.stomach);
		Color3f discomfortcolor= setColorTempPref(agent.discomfort);
		Color3f metabcolor= setColorMetabolism(agent.metabolism);
		Color3f tonecolor= setColorTone(agent.tone);
		Color3f lungcolor= setColorLungs(agent.lungs);
		Color3f speciescolor= setColorSpecies(agent.species);
		Color3f crossablecolor= setColorCrossable(agent.species);
		Color3f generocitycolor= setColorGenerocity(agent.dhealth);
//		Color3f sexualitycolor= setColorSexuality(agent);

		//now colorize agents and other things
		if (live_agentsvis==Visual::RGB){ //real rgb values
			red= agent.real_red; gre= agent.real_gre; blu= agent.real_blu;

		} else if (live_agentsvis==Visual::STOMACH){
			red= stomachcolor.red;
			gre= stomachcolor.gre;
			blu= stomachcolor.blu;
		
		} else if (live_agentsvis==Visual::DISCOMFORT){
			red= discomfortcolor.red;
			gre= discomfortcolor.gre;
			blu= discomfortcolor.blu;

		} else if (live_agentsvis==Visual::VOLUME) {
			red= agent.volume;
			gre= agent.volume;
			blu= agent.volume;

		} else if (live_agentsvis==Visual::SPECIES){ 
			red= speciescolor.red;
			gre= speciescolor.gre;
			blu= speciescolor.blu;
		
		} else if (live_agentsvis==Visual::CROSSABLE){ //crossover-compatable to selection
			red= crossablecolor.red;
			gre= crossablecolor.gre;
			blu= crossablecolor.blu;

		} else if (live_agentsvis==Visual::HEALTH) {
			red= healthcolor.red;
			gre= healthcolor.gre;
			//blu= healthcolor.blu;

		} else if (live_agentsvis==Visual::METABOLISM){
			red= metabcolor.red;
			gre= metabcolor.gre;
			blu= metabcolor.blu;
		
		} else if (live_agentsvis==Visual::LUNGS){
			red= lungcolor.red;
			gre= lungcolor.gre;
			blu= lungcolor.blu;

//		} else if (live_agentsvis==Visual::REPMODE){
//			red= lungcolor.red;
//			gre= lungcolor.gre;
//			blu= lungcolor.blu;
		}

		float rad= r;
		float centeralpha= cap(agent.centerrender);
		float centercmult= agent.centerrender>1.0 ? -0.5*agent.centerrender+1.25 : 0.5*agent.centerrender+0.25;
		// when we're zoomed out far away, do some things differently, but not to the ghost (selected agent display)
		if( scalemult <= 0.1 && !ghost) {
			rad= 17; //this makes agents a standard size when zoomed out REALLY far, but not on the ghost

		}
		if (agent.health<=0 || (scalemult <= 0.3 && !ghost)) {
			//when zoomed out too far to want to know agent details, or if agent is dead (either ghost render or not), draw center as solid
			centeralpha= 1; 
			centercmult= 1;
		}
		
		float dead= agent.health==0 ? conf::DEADTRANSPARENCY : 1; //mult for dead agents, displayed with more transparent parts



		//we are now ready to start drawing
		glPushMatrix(); //switch to local position coordinates
		glTranslatef(x,y,0);


		//handle selected agent
		if (agent.id==world->getSelection() && !ghost) {
			//draw selection outline
			glLineWidth(2);
			glBegin(GL_LINES);
			glColor3f(1,1,1);
			drawOutlineRes(0, 0, rad+4/scalemult, ceil(scalemult+r/10)), 
			glEnd();
			glLineWidth(1);
		}

		if (scalemult > .3 || ghost) { //dont render eyes, ears, or boost if zoomed out, but always render them on ghosts
			//draw eyes
			for(int q=0;q<NUMEYES;q++) {
				if(agent.isTiny() && !agent.isTinyEye(q)) break; //skip extra eyes for tinies
				float aa= agent.angle+agent.eyedir[q];
				
				glBegin(GL_LINES);
				if (live_profilevis==Profile::EYES) {
					//color eye lines based on actual input if we're on the eyesight profile
					glColor4f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3],0.75*dead);
				} else glColor4f(0.5,0.5,0.5,0.75*dead);

				glVertex3f(r*cos(aa), r*sin(aa),0);
				glVertex3f((2*r+30)*cos(aa), (2*r+30)*sin(aa), 0); //this draws whiskers. Don't get rid of or change these <3

				glEnd();
			}

			//ears
			for(int q=0;q<NUMEARS;q++) {
				glBegin(GL_POLYGON);
				float aa= agent.angle + agent.eardir[q];
				//the centers of ears are black
				glColor4f(0,0,0,0.5);
				glVertex3f(r*cos(aa), r*sin(aa),0);
				//color ears differently if we are set on the sound profile or debug
				if(live_profilevis==Profile::SOUND || world->isDebug()) glColor4f(1-(q/(NUMEARS-1)),q/(NUMEARS-1),0,0.75*dead);
				else glColor4f(0.6,0.6,0,0.5*dead);				
				drawCircle(r*cos(aa), r*sin(aa), 2*agent.hear_mod);
				glColor4f(0,0,0,0.5);
				glVertex3f(r*cos(aa), r*sin(aa),0);
				glEnd();
			}

			//boost blur
			if (agent.boost){
				float direction= (agent.w1>0 || agent.w2>0) ? 1 : -1;
				for(int q=1;q<4;q++){
					Vector2f displace(r/3*q*direction, 0);
					displace.rotate(agent.angle+M_PI);

					glBegin(GL_POLYGON); 
					glColor4f(red,gre,blu,0.25);
					drawCircle(displace.x, displace.y, r);
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
					float aa= agent.angle+agent.grabangle+M_PI/8*mult;
					float ab= agent.angle+agent.grabangle-M_PI/8*mult;
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
		glColor4f(red*centercmult,gre*centercmult,blu*centercmult,dead*centeralpha);
		glVertex3f(0,0,0);
		glColor4f(red,gre,blu,dead);
		drawCircleRes(0, 0, rad, ceil(scalemult*0.5)+1);
		glColor4f(red*centercmult,gre*centercmult,blu*centercmult,dead*centeralpha);
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
			float mult= 1-powf(abs(agent.jawPosition),0.5);
			glVertex3f(rad*cos(agent.angle), rad*sin(agent.angle), 0);
			glVertex3f((10+rad)*cos(agent.angle+M_PI/8*mult), (10+rad)*sin(agent.angle+M_PI/8*mult), 0);
			glVertex3f(rad*cos(agent.angle), rad*sin(agent.angle), 0);
			glVertex3f((10+rad)*cos(agent.angle-M_PI/8*mult), (10+rad)*sin(agent.angle-M_PI/8*mult), 0);
		}

		//outline
		float out_red= 0,out_gre= 0,out_blu= 0;
		if (agent.isTiny()){
			out_red= 1.0;
			out_gre= 1.0;
			out_blu= 1.0;
		}
		if (agent.jump>0) { //draw jumping as a tan outline no matter what
			out_red= 0.7;
			out_gre= 0.7;
		}

		if (live_agentsvis!=Visual::HEALTH && agent.health<=0){ //draw outline as grey if dead, unless visualizing health
			glColor4f(0.7,0.7,0.7,dead);		
		//otherwise, color as agent's body if zoomed out, color as above if normal zoomed
		} else glColor4f(cap(out_red*blur + (1-blur)*red), cap(out_gre*blur + (1-blur)*gre), cap(out_blu*blur + (1-blur)*blu), dead);
		drawOutlineRes(0, 0, rad, ceil(scalemult*0.5)+1);
		glEnd();


		//sound waves!
		glBegin(GL_LINES);
		if(live_agentsvis==Visual::VOLUME && !ghost && agent.volume>0){
			float volume= agent.volume;
			float count= agent.tone*11+1;
			for (int l=0; l<=(int)count; l++){
				float dist= world->DIST*(l/count)+2*(world->modcounter%(int)((world->DIST)/2));
				if (dist>world->DIST) dist-= world->DIST;
				glColor4f(tonecolor.red, tonecolor.gre, tonecolor.blu, cap((1-dist/world->DIST)*(volume+0.1)));
				drawOutlineRes(0, 0, dist, ceil(scalemult)+2);
			}
		}
		glEnd();

		//some final stuff to render over top of the body but only when zoomed in or on the ghost
		if(scalemult > .1 || ghost){

			//spike, but only if sticking out
			if (agent.isSpikey(world->SPIKELENGTH)) {
				glBegin(GL_LINES);
				glColor4f(0.7,0,0,blur);
				glVertex3f(0,0,0);
				glVertex3f((world->SPIKELENGTH*agent.spikeLength)*cos(agent.angle),
						   (world->SPIKELENGTH*agent.spikeLength)*sin(agent.angle),
						   0);
				glEnd();
			}
			
			//brainsize (boxes with weights != 0) is indicated by a pair of circles: one for the "max brain size"
/* this turned out to not be too helpful, as it seems most agents by generation ~100 have had all their weights adjusted
			//added a new brain mutation to set weights to zero sometimes
			glBegin(GL_POLYGON); 
			glColor4f(0,0,0,dead*0.5);
			glVertex3f(x,y,0);
			glColor4f(agent.real_red,agent.real_gre,agent.real_blu,dead*0.5);
			drawCircleRes(x, y, rad*0.35, ceil(scalemult)+1);
			glColor4f(0,0,0,dead*0.5);
			glVertex3f(x,y,0);
			glEnd();

			glPushMatrix(); //switch to local position coordinates
			glTranslatef(x,y,0);
			glRotatef(agent.angle*180/M_PI,0,0,1);

			glBegin(GL_POLYGON); 
			glColor4f(1.0,0.8,0.8,0.5);
			drawCircleRes(0, 0, rad*0.35*agent.brain.getNonZeroWRatio(), 0);
			glEnd();

			glPopMatrix();*/

			//blood sense patch
			if (agent.blood_mod>0.25){
				float count= floorf(agent.blood_mod*4);
				float aa= 0.015;
				float ca= agent.angle - aa*count/2;
				//color coresponds to intensity of the input detected, opacity scaled to level of blood_mod over 0.25
				float alpha= cap(0.5*agent.blood_mod-0.25);
				Color3f bloodcolor= setColorMeat(agent.in[Input::BLOOD]+0.5);
				
				//the blood sense patch is drawn as multiple circles overlapping. Higher the blood_mult trait, the wider the patch
				for(int q=0;q<count;q++) {
					glBegin(GL_POLYGON);
					glColor4f(bloodcolor.red, bloodcolor.gre, bloodcolor.blu, alpha);
					drawCircle((r*7/8-1)*cos(ca), (r*7/8-1)*sin(ca), 0.3);
					glEnd();
					ca+= aa;
				}
			}

			//draw cute little dots for eyes
			for(int q=0;q<NUMEYES;q++) {
				if(agent.isTiny() && !agent.isTinyEye(q)) break;
				if((world->modcounter+agent.id)%85>=q*3 && (world->modcounter+agent.id)%85<q*3+conf::BLINKTIME && agent.health>0) continue; //blink eyes ocasionally... DAWWWWWW

				glBegin(GL_POLYGON);
				glColor4f(0,0,0,1.0);
				float ca= agent.angle+agent.eyedir[q];
				//the eyes are small and tiny if the agent has a low eye mod. otherwise they are big and buggie
				float eyesize= capm(agent.eye_see_agent_mod/2,0.1,1);
				drawCircle((r-2+eyesize)*cos(ca), (r-2+eyesize)*sin(ca), eyesize);
				glEnd();
			}
		}

		if (scalemult > .3 || ghost) { //render some extra stuff when we're really close
			//render grab target line if we have one
			if(world->GRAB_PRESSURE!=0 && agent.isGrabbing() && agent.health>0 && agent.grabID!=-1 && !ghost){
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

		//some final final debug stuff that is shown even on ghosts:
		if(world->isDebug() || ghost){
			//wheels and wheel speeds
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
		}

		glPopMatrix(); //restore world coords
	}
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ END DRAW AGENT ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//


void GLView::drawData()
{
	//draw misc info
	float mm = 3;

	//start graphs of aquatics, amphibians, and terrestrials
	Color3f graphcolor= setColorLungs(0.0);
	glBegin(GL_QUADS); 

	//aquatic count
	glColor3f(graphcolor.red,graphcolor.gre,graphcolor.blu);
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1){
			glColor3f((1+graphcolor.red)/2,(1+graphcolor.gre)/2,(1+graphcolor.blu)/2);
			continue;
		}
		glVertex3f(-2010 + q*10, conf::HEIGHT - mm*world->numTotal[q],0);
		glVertex3f(-2010 +(q+1)*10, conf::HEIGHT - mm*world->numTotal[q+1],0);
		glVertex3f(-2010 +(q+1)*10, conf::HEIGHT, 0);
		glVertex3f(-2010 + q*10, conf::HEIGHT, 0);
	}

	//amphibian count
	graphcolor= setColorLungs(0.4);
	glColor3f(graphcolor.red,graphcolor.gre,graphcolor.blu);
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1){
			glColor3f((1+graphcolor.red)/2,(1+graphcolor.gre)/2,(1+graphcolor.blu)/2);
			continue;
		}
		glVertex3f(-2010 + q*10, conf::HEIGHT - mm*world->numAmphibious[q] - mm*world->numTerrestrial[q],0);
		glVertex3f(-2010 +(q+1)*10, conf::HEIGHT - mm*world->numAmphibious[q+1] - mm*world->numTerrestrial[q+1],0);
		glVertex3f(-2010 +(q+1)*10, conf::HEIGHT, 0);
		glVertex3f(-2010 + q*10, conf::HEIGHT, 0);
	}

	//terrestrial count
	graphcolor= setColorLungs(1.0);
	glColor3f(graphcolor.red,graphcolor.gre,graphcolor.blu);
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1){
			glColor3f((1+graphcolor.red)/2,(1+graphcolor.gre)/2,(1+graphcolor.blu)/2);
			continue;
		}
		glVertex3f(-2010 + q*10, conf::HEIGHT - mm*world->numTerrestrial[q],0);
		glVertex3f(-2010 +(q+1)*10, conf::HEIGHT - mm*world->numTerrestrial[q+1],0);
		glVertex3f(-2010 +(q+1)*10, conf::HEIGHT, 0);
		glVertex3f(-2010 + q*10, conf::HEIGHT, 0);
	}
	glEnd();

	glBegin(GL_LINES);
	glColor3f(0,0,0); //border around graphs and feedback

	glVertex3f(0,0,0);
	glVertex3f(-2020,0,0);

	glVertex3f(-2020,0,0);
	glVertex3f(-2020,conf::HEIGHT,0);

	glVertex3f(-2020,conf::HEIGHT,0);
	glVertex3f(0,conf::HEIGHT,0);

	glColor3f(0,0,0.5); //hybrid count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2010 + q*10, conf::HEIGHT - mm*world->numHybrid[q],0);
		glVertex3f(-2010 +(q+1)*10, conf::HEIGHT - mm*world->numHybrid[q+1],0);
	}
	glColor3f(0,1,0); //herbivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2010 + q*10,conf::HEIGHT - mm*world->numHerbivore[q],0);
		glVertex3f(-2010 +(q+1)*10,conf::HEIGHT - mm*world->numHerbivore[q+1],0);
	}
	glColor3f(1,0,0); //carnivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2010 + q*10,conf::HEIGHT - mm*world->numCarnivore[q],0);
		glVertex3f(-2010 +(q+1)*10,conf::HEIGHT - mm*world->numCarnivore[q+1],0);
	}
	glColor3f(0.9,0.9,0); //frugivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2010 + q*10,conf::HEIGHT - mm*world->numFrugivore[q],0);
		glVertex3f(-2010 +(q+1)*10,conf::HEIGHT - mm*world->numFrugivore[q+1],0);
	}
	glColor3f(0,0,0); //total count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2010 + q*10,conf::HEIGHT - mm*world->numTotal[q],0);
		glVertex3f(-2010 +(q+1)*10,conf::HEIGHT - mm*world->numTotal[q+1],0);
	}
	glColor3f(0.8,0.8,0.6); //dead count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2010 + q*10,conf::HEIGHT - mm*(world->numDead[q]+world->numTotal[q]),0);
		glVertex3f(-2010 +(q+1)*10,conf::HEIGHT - mm*(world->numDead[q+1]+world->numTotal[q+1]),0);
	}

	//current population vertical bars
	glVertex3f(-2010 + world->ptr*10,conf::HEIGHT,0);
	glVertex3f(-2010 + world->ptr*10,conf::HEIGHT - mm*world->getAgents(),0);
	glColor3f(0,0,0);
	glVertex3f(-2010 + world->ptr*10,conf::HEIGHT,0);
	glVertex3f(-2010 + world->ptr*10,conf::HEIGHT - mm*world->getAlive(),0);
	glEnd();

	//labels for current population bars
	int movetext= 0;
	if (world->ptr>=conf::RECORD_SIZE*0.75) movetext= -700;
	sprintf(buf2, "%i dead", world->getDead());
	RenderString(-2006 + movetext + world->ptr*10,conf::HEIGHT - mm*world->getAgents(),
		GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 0.8f, 0.6f);
	sprintf(buf2, "%i agents", world->getAlive());
	RenderString(-2006 + movetext + world->ptr*10,conf::HEIGHT - mm*world->getAlive(),
		GLUT_BITMAP_HELVETICA_12, buf2, 0.0f, 0.0f, 0.0f);

	if(getLayerDisplayCount()>0){
		glBegin(GL_QUADS);
		glColor4f(0,0,0.1,1);
		glVertex3f(0,0,0);
		glVertex3f(conf::WIDTH,0,0);
		glVertex3f(conf::WIDTH,conf::HEIGHT,0);
		glVertex3f(0,conf::HEIGHT,0);
		glEnd();
	}
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

		if(ti==MainTiles::SAD){
			//SAD: selected agent display

			Agent selected= world->agents[world->getSelectedAgent()];

			//main box
			glBegin(GL_POLYGON);
			glColor4f(0,0.4,0.6,0.45);
			glVertex3f(tbx,tby,0); //top-left
			glColor4f(0,0.4,0.6,1);
			glVertex3f(tbx2,tby,0); //top-right
			glVertex3f(tbx2,tby2,0); //bottom-right
			glColor4f(0,0.4,0.6,0.25);
			glVertex3f(tbx,tby2,0); //bottom-left
			glEnd();

			//trapezoids
			glBegin(GL_POLYGON); //top
			glColor4f(cap(world->SUN_RED*0.5),cap((0.4+world->SUN_GRE)*0.5),cap((0.6+world->SUN_BLU)*0.5),1);
			glVertex3f(tbx,tby,0);
			glVertex3f(tx,ty,0);
			glVertex3f(tx2,ty,0);
			glVertex3f(tbx2,tby,0);
			glEnd();

			glBegin(GL_POLYGON); //right
			glColor4f(0,0.3,0.45,1);
			glVertex3f(tbx2,tby,0);
			glVertex3f(tx2,ty,0);
			glVertex3f(tx2,ty2,0);
			glVertex3f(tbx2,tby2,0);
			glEnd();

			glBegin(GL_POLYGON); //bottom
			glColor4f(0,0.25,0.35,1);
			glVertex3f(tbx2,tby2,0);
			glVertex3f(tx2,ty2,0);
			glColor4f(0,0.25,0.35,0.25);
			glVertex3f(tx,ty2,0);
			glVertex3f(tbx,tby2,0);
			glEnd();

			glBegin(GL_POLYGON); //left
			glColor4f(cap(world->SUN_RED*0.25),cap(0.4+world->SUN_GRE*0.25),cap(0.6+world->SUN_BLU*0.25),0.25);
			glVertex3f(tbx,tby2,0);
			glVertex3f(tx,ty2,0);
			glVertex3f(tx,ty,0);
			glVertex3f(tbx,tby,0);
			glEnd();


			//draw Ghost Agent
			float margin= (float)(th)/2;
			drawPreAgent(selected, tx+margin, 5+margin, true);
			drawAgent(selected, tx+margin, 5+margin, true);

			//Agent ID
			glBegin(GL_QUADS);
			glColor4f(0,0,0,0.7);
			float idlength= 19+UID::CHARWIDTH*floorf(1.0+log10((float)selected.id+1.0));
			float idboxx= tx+margin-idlength/2-5;
			float idboxy= ty+11;
			float idboxx2= tx+margin+idlength/2+5;
			float idboxy2= idboxy+14;

			glVertex3f(idboxx,idboxy2,0); 
			glVertex3f(idboxx2,idboxy2,0); 
			Color3f specie= setColorSpecies(selected.species);
			glColor4f(specie.red,specie.gre,specie.blu,1);
			glVertex3f(idboxx2,idboxy,0); 
			glVertex3f(idboxx,idboxy,0); 
			glEnd();

			sprintf(buf, "ID: %d", selected.id);
			RenderString(tx+margin-idlength/2, idboxy2-3, GLUT_BITMAP_HELVETICA_12, buf, 0.8f, 1.0f, 1.0f);

			for(int u=0; u<Hud::HUDS; u++) {
				bool drawbox= false;

				int ux= tx2-5-(UID::BUFFER+UID::HUDSWIDTH)*(3-(int)(u%3));
				int uy= ty+15+UID::BUFFER+((int)(u/3))*(UID::CHARHEIGHT+UID::BUFFER);
				//(ux,uy)= lower left corner of element
				//must + ul to ux, must - uw to uy, for other corners

				//Draw graphs
				if(u==Hud::HEALTH || u==Hud::REPCOUNTER || u==Hud::STOMACH || u==Hud::EXHAUSTION){
					//all graphs get a trasparent black box put behind them first
					glBegin(GL_QUADS);
					glColor4f(0,0,0,0.7);
					glVertex3f(ux-1,uy+1,0);
					glVertex3f(ux-1,uy-1-UID::CHARHEIGHT,0);
					glVertex3f(ux+1+UID::HUDSWIDTH,uy-1-UID::CHARHEIGHT,0);
					glVertex3f(ux+1+UID::HUDSWIDTH,uy+1,0);

					if(u==Hud::STOMACH){ //stomach indicators, ux= ww-100, uy= 40
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
					} else if (u==Hud::REPCOUNTER){ //repcounter indicator, ux=ww-200, uy=25
						glColor3f(0,0.7,0.7);
						glVertex3f(ux,uy,0);
						glColor3f(0,0.5,0.6);
						glVertex3f(ux,uy-UID::CHARHEIGHT,0);
						glColor3f(0,0.7,0.7);
						glVertex3f(cap(selected.repcounter/selected.maxrepcounter)*-UID::HUDSWIDTH+ux+UID::HUDSWIDTH,uy-UID::CHARHEIGHT,0);
						glColor3f(0,0.5,0.6);
						glVertex3f(cap(selected.repcounter/selected.maxrepcounter)*-UID::HUDSWIDTH+ux+UID::HUDSWIDTH,uy,0);
					} else if (u==Hud::HEALTH){ //health indicator, ux=ww-300, uy=25
						glColor3f(0,0.8,0);
						glVertex3f(ux,uy-UID::CHARHEIGHT,0);
						glVertex3f(selected.health/2.0*UID::HUDSWIDTH+ux,uy-UID::CHARHEIGHT,0);
						glColor3f(0,0.6,0);
						glVertex3f(selected.health/2.0*UID::HUDSWIDTH+ux,uy,0);
						glVertex3f(ux,uy,0);
					} else if (u==Hud::EXHAUSTION){ //Exhaustion/energy indicator ux=ww-100, uy=25
						float exh= 2/(1+exp(world->EXHAUSTION_MULT*selected.exhaustion));
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

				//write text and values
				if(u==Hud::HEALTH){
					if(live_agentsvis==Visual::HEALTH) drawbox= true;
					sprintf(buf, "Health: %.2f/2", selected.health);

				} else if(u==Hud::REPCOUNTER){
					sprintf(buf, "Child: %.2f/%.0f", selected.repcounter, selected.maxrepcounter);
				
				} else if(u==Hud::EXHAUSTION){
					if(world->EXHAUSTION_MULT>0 && selected.exhaustion>(5*world->EXHAUSTION_MULT)) sprintf(buf, "Exhausted!");
					else if (world->EXHAUSTION_MULT>0 && selected.exhaustion<(2*world->EXHAUSTION_MULT)) sprintf(buf, "Energetic!");
					else sprintf(buf, "Tired.");

				} else if(u==Hud::STOMACH){
					if(live_agentsvis==Visual::STOMACH) drawbox= true;
					if(selected.isHerbivore()) sprintf(buf, "\"Herbivore\"");
					else if(selected.isFrugivore()) sprintf(buf, "\"Frugivore\"");
					else if(selected.isCarnivore()) sprintf(buf, "\"Carnivore\"");
					else sprintf(buf, "\"Dead\"...");

				} else if(u==Hud::AGE){
					sprintf(buf, "Age: %.1f days", (float)selected.age/10);

				} else if(u==Hud::GENERATION){
					sprintf(buf, "Gen: %d", selected.gencount);

				} else if(u==Hud::TEMPPREF){
					if(live_agentsvis==Visual::DISCOMFORT) drawbox= true;
					if(selected.temperature_preference<0.3) sprintf(buf, "Tropical(%.3f)", selected.temperature_preference);
					else if (selected.temperature_preference>0.7) sprintf(buf, "Arctic(%.3f)", selected.temperature_preference);
					else sprintf(buf, "Temperate(%.2f)", selected.temperature_preference);

				} else if(u==Hud::LUNGS){
					if(live_agentsvis==Visual::LUNGS) drawbox= true;
					if(selected.isAquatic()) sprintf(buf, "Aquatic(%.3f)", selected.lungs);
					else if (selected.isTerrestrial()) sprintf(buf, "Terran(%.3f)", selected.lungs);
					else sprintf(buf, "Amphibian(%.2f)", selected.lungs);

				} else if(u==Hud::SIZE){
					sprintf(buf, "Radius: %.2f", selected.radius);						 

				} else if(u==Hud::MUTRATE1){
					sprintf(buf, "Mutrate1: %.3f", selected.MUTRATE1);
				} else if(u==Hud::MUTRATE2){ 
					sprintf(buf, "Mutrate2: %.3f", selected.MUTRATE2);

				} else if(u==Hud::CHAMOVID){
					sprintf(buf, "Camo: %.3f", selected.chamovid);
				
				} else if(u==Hud::METABOLISM){
					if(live_agentsvis==Visual::METABOLISM) drawbox= true;
					sprintf(buf, "Metab: %.2f", selected.metabolism);

				} else if(u==Hud::HYBRID){
					if(selected.hybrid) sprintf(buf, "Is Hybrid");
					else if(selected.gencount==0) sprintf(buf, "Was Spawned");
					else sprintf(buf, "Was Budded");

				} else if(u==Hud::GIVING){
					if(selected.isGiving()) sprintf(buf, "Generous");
					else if(selected.give>conf::MAXSELFISH) sprintf(buf, "Autocentric");
					else sprintf(buf, "Selfish");

				} else if(u==Hud::SPIKE){
					if(selected.health>0 && selected.isSpikey(world->SPIKELENGTH)){
						float mw= selected.w1>selected.w2 ? selected.w1 : selected.w2;
						if(mw<0) mw= 0;
						float val= world->DAMAGE_FULLSPIKE*selected.spikeLength*mw;
						if(val>2) sprintf(buf, "Spike: Deadly!");//health
						else if(val==0) sprintf(buf, "Spike: 0 h");
						else sprintf(buf, "Spike: ~%.2f h", val);
					} else sprintf(buf, "Not Spikey");

				} else if(u==Hud::STRENGTH){
					if(selected.strength>0.7) sprintf(buf, "Strong!");
					else if(selected.strength>0.3) sprintf(buf, "Not Weak");
					else sprintf(buf, "Weak!");

				} else if(u==Hud::NUMBABIES){
					sprintf(buf, "Num Babies: %d", selected.numbabies);

				} else if(u==Hud::SEXPROJECT){
					if(selected.isMale()) sprintf(buf, "Sexting (M)");
					else if(selected.isAsexual()) sprintf(buf, "Is Asexual");
					else sprintf(buf, "Sexting (F)");

				} else if(u==Hud::SPECIESID){
					if(live_agentsvis==Visual::STOMACH) drawbox= true;
					sprintf(buf, "Gene: %d", selected.species);

				} else if(u==Hud::MOTION){
					if(selected.jump>0) sprintf(buf, "Airborne!");
					else if(selected.encumbered) sprintf(buf, "Encumbered");
					else if(selected.boost) sprintf(buf, "Boosting");
					else if(abs(selected.w1)>0.1 || abs(selected.w2)>0.1) sprintf(buf, "Sprinting");
					else if(abs(selected.w1)>0.03 || abs(selected.w2)>0.03) sprintf(buf, "Moving");
					else if(selected.w1*selected.w2<-0.0009) sprintf(buf, "Spinning...");
					else sprintf(buf, "Idle");

				} else if(u==Hud::GRAB){
					if(selected.isGrabbing()){
						if(selected.grabID==-1) sprintf(buf, "Grabbing");
						else sprintf(buf, "Hold %d (%.2f)", selected.grabID, selected.grabbing);
					} else sprintf(buf, "Not Grabbing");

				} else if(u==Hud::BITE){
					if(selected.jawrend!=0) sprintf(buf, "Bite: %.2f h", abs(selected.jawPosition*world->DAMAGE_JAWSNAP));
					else sprintf(buf, "Not Biting");

				} else if(u==Hud::WASTE){
					if(selected.out[Output::WASTE_RATE]<0.05) sprintf(buf, "Has the Runs");
					else if(selected.out[Output::WASTE_RATE]<0.3) sprintf(buf, "Incontenent");
					else if(selected.out[Output::WASTE_RATE]>0.7) sprintf(buf, "Waste Prudent");
					else sprintf(buf, "Avg Waste Rate");

				} else if(u==Hud::VOLUME){
					if(live_agentsvis==Visual::VOLUME) drawbox= true;
					if(selected.volume>0.8) sprintf(buf, "Loud! (%.3f)", selected.volume);
					else if(selected.volume<=0.001) sprintf(buf, "Silent.");
					else if(selected.volume<0.2) sprintf(buf, "Quiet (%.3f)", selected.volume);
					else sprintf(buf, "Volume: %.3f", selected.volume);

				} else if(u==Hud::TONE){
					sprintf(buf, "Tone: %.3f", selected.tone);

				//Stats
				} else if(u==Hud::STAT_CHILDREN){
					sprintf(buf, "Children: %d", selected.children);

				} else if(u==Hud::STAT_KILLED){
					sprintf(buf, "Has Killed: %d", selected.killed);

				} else if(u==Hud::DEATH && selected.health==0){
					sprintf(buf, selected.death.c_str());

				} else sprintf(buf, "");

				//render box around our stat that we're visualizing to help user follow along

				//Render text
				RenderString(ux, uy, GLUT_BITMAP_HELVETICA_12, buf, 0.8f, 1.0f, 1.0f);
			}

		} else {
			//all other controls
			renderTile(maintiles[ti]);
		}

		//also render any children
		for(int ct=0; ct<maintiles[ti].children.size(); ct++){
			renderTile(maintiles[ti].children[ct]);
		}
	}
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
	// move the origin from the bottom left corner
	// to the upper left corner
	glTranslatef(0, -wh, 0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadIdentity();
	/*end setup*/

	//begin things that we actually want to draw staticly
	//First up, the status displays in the top left
	int currentline= 1;
	int spaceperline= 16;

	int linecount= StaticDisplay::STATICDISPLAYS; //this is not used directly in for loop because we modify it if Debug is on (NOT CURRENTLY USED)

	for(int line=0; line<linecount; line++){
		if(line==StaticDisplay::PAUSED)	{
			if(live_paused){
				float redness= 0.5*abs(sin((float)(currentTime)/250));
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Paused", 0.5f+redness, 0.55f-redness, 0.55f-redness);
				currentline++; //this appears in every if statement because we only move to next line if we added a line
			}
		} else if(line==StaticDisplay::FOLLOW) {
			if(live_follow!=0) {
				if(world->getSelectedAgent()>=0) RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Following", 0.75f, 0.75f, 0.75f);
				else RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "No Follow Target", 0.75f, 0.75f, 0.75f);
				currentline++;
			}
		} else if(line==StaticDisplay::AUTOSELECT) {
			if(live_selection!=Select::NONE && live_selection!=Select::MANUAL) {
				if(live_selection==Select::RELATIVE) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Relatives Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::AGGRESSIVE) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Aggression Stat Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::BEST_GEN) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Highest Gen. Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::HEALTHY) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Healthiest Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::ENERGETIC) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Energetic Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::OLDEST) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Oldest Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::PRODUCTIVE) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Children Stat Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::BEST_CARNIVORE) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Best Carni. Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::BEST_FRUGIVORE) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Best Frugi. Autoselect", 0.5f, 0.8f, 0.5f);
				} else if(live_selection==Select::BEST_HERBIVORE) {
					RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Best Herbi. Autoselect", 0.5f, 0.8f, 0.5f);
				}
				currentline++;
			}
		} else if(line==StaticDisplay::CLOSED) {
			if(world->isClosed()) {
				float blueness= 0.5*abs(sin((float)(currentTime)/250));
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Closed World", 0.55f-blueness, 0.75f-blueness, 0.5f+blueness);
				currentline++;
			}
		} else if(line==StaticDisplay::DROUGHT) {
			if(world->isDrought()) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Current Drought", 1.0f, 1.0f, 0.0f);
				currentline++;
			} else if(world->isOvergrowth()) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "Current Overgrowth", 0.0f, 0.65f, 0.25f);
				currentline++;
			}
		} else if (line==StaticDisplay::MUTATIONS) {
			if(world->MUTEVENTMULT>1) {
				sprintf(buf, "%ix Mutation Rate", world->MUTEVENTMULT);
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.75f, 0.0f, 1.0f);
				currentline++;
			}
		}  else if (line==StaticDisplay::USERCONTROL) {
			if(world->pcontrol) {
				RenderStringBlack(10, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, "User Control Active", 0.0f, 0.2f, 0.8f);
				currentline++;
			}
		}
		
		//we do not include debug here yet. see below 
	}
	

	if(world->isDebug()) { //no matter what, render these at the end
		currentline++;
		sprintf(buf, "Plant-Haz Supp: %i agents", (int)(world->getFoodSupp()-world->getHazardSupp()));
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		currentline++;
		sprintf(buf, "Fruit-Haz Supp: %i agents", (int)(world->getFruitSupp()-world->getHazardSupp()));
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		currentline++;
		sprintf(buf, "Meat-Haz Supp: %i agents", (int)(world->getMeatSupp()-world->getHazardSupp()));
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		currentline++;
		sprintf(buf, "Haz Impact: %i agents", (int)(-world->getHazardSupp()));
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		currentline++;
		sprintf(buf, "modcounter: %i", world->modcounter);
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		currentline++;
		sprintf(buf, "GL modcounter: %i", modcounter);
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		currentline++;
		sprintf(buf, "GL scalemult: %.4f", scalemult);
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		currentline++;
		sprintf(buf, "%% Land: %.3f", world->getLandRatio());
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		currentline++;
		sprintf(buf, "Avg Oxy Dmg: %f", world->HEALTHLOSS_NOOXYGEN*world->agents.size());
		RenderString(5, currentline*spaceperline, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
	}

	//center axis markers
	glBegin(GL_LINES);
	glColor4f(0,1,0,0.35); //green y-axis
	glVertex3f(ww/2,wh/2,0);
	glVertex3f(ww/2,wh/2+15,0);

	glColor4f(1,0,0,0.35); //red x-axis
	glVertex3f(ww/2,wh/2,0);
	glVertex3f(ww/2+15,wh/2,0);
	glEnd();

	int ux, uy; //sub-ui element x, y, length, width, barrier

	//event display y coord, based on Selected Agent Hud to avoid overlap
	int euy= 50 + 10+2*UID::BUFFER+((int)ceil((float)Hud::HUDS/3))*(UID::CHARHEIGHT+UID::BUFFER);

	//event log display
	int toastwidth= 200;
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
			case 0: 	red= 0.8; gre= 0.8; blu= 0.8;	break; //type 0: white
			case 1: 	red= 0.8; gre= 0.1; blu= 0.1;	break; //type 1: red
			case 2: 	red= 0.1; gre= 0.6;				break; //type 2: green
			case 3: 	gre= 0.3; blu= 0.9;				break; //type 3: blue
			case 4:		red= 0.8; gre= 0.8; blu= 0.1;	break; //type 4: yellow
			case 5:		gre= 0.9; blu= 0.9;				break; //type 5: cyan
			case 6:		red= 0.6; blu= 0.6;				break; //type 6: purple
			case 7:		red= 0.5; gre= 0.3; blu= 0.1;	break; //type 7: brown
			case 8:		gre= 1.0;						break; //type 8: neon
			case 9:		gre= 0.4; blu= 0.6;				break; //type 9: mint
			case 10:	red= 1/(1+world->modcounter%31); gre= 1/(1+world->modcounter%23); blu= 1/(1+world->modcounter%14); break; //type 10: all the colors!
			case 11:	red= 1.0; gre= 0.5; blu= 0.5;	break; //type 11: pink
			case 12:	red= 0.9; gre= 0.6;				break; //type 12: orange
			case 13:	red= 0.5;						break; //type 13: bloodred
			case 14:	red= 0.8; gre= 0.6;	blu= 1.0;	break; //type 14: lavender
			case 15:	red= 0.6; gre= 1.0;	blu= 0.4;	break; //type 15: lime
		}

		glBegin(GL_QUADS);
		glColor4f(red,gre,blu,0.5*fade);
		glVertex3f(ww-UID::BUFFER-toastwidth, euy+5+(2+eo)*ss+toastbase+move,0);
		glVertex3f(ww-UID::BUFFER, euy+5+(2+eo)*ss+toastbase+move,0);
		glColor4f(red*0.75,gre*0.75,blu*0.75,0.5*fade);
		glVertex3f(ww-UID::BUFFER, euy+5+(3+eo)*ss+toastbase+move,0);
		glVertex3f(ww-UID::BUFFER-toastwidth, euy+5+(3+eo)*ss+toastbase+move,0);
		glEnd();

		RenderStringBlack(ww-toastwidth, euy+10+2.5*ss+eo*ss+toastbase+move, GLUT_BITMAP_HELVETICA_12, world->events[eo].first, 1.0f, 1.0f, 1.0f, fade);
	}

	if(world->events.size()>conf::EVENTS_DISP) {
		//display a blinking graphic indicating there are more notifications
		float blink= 0.5*abs(sin((float)(currentTime)/400));

		glLineWidth(3);
		glBegin(GL_LINES);
		glColor4f(0.55-blink,0.5+blink,0.5+blink,0.75);
		glVertex3f(ww-UID::BUFFER-toastwidth/2-5, euy+10+2*ss+conf::EVENTS_DISP*ss+5*blink,0);
		glVertex3f(ww-UID::BUFFER-toastwidth/2, euy+5+10+2*ss+conf::EVENTS_DISP*ss+5*blink,0);
		glVertex3f(ww-UID::BUFFER-toastwidth/2, euy+5+10+2*ss+conf::EVENTS_DISP*ss+5*blink,0);
		glVertex3f(ww-UID::BUFFER-toastwidth/2+5, euy+10+2*ss+conf::EVENTS_DISP*ss+5*blink,0);
		glEnd();
		glLineWidth(1);
	}

	renderAllTiles();

	//draw the popup if available
	if(popupxy[0]>=0 && popupxy[1]>=0) {
		//first, split up our popup text and count the number of splits
		int linecount= popuptext.size();
		int maxlen= 1;
		
		for(int l=0; l<linecount; l++) {
			if(popuptext[l].size()>maxlen) maxlen= popuptext[l].size();
		}

		glBegin(GL_QUADS);
		glColor4f(0,0.4,0.6,0.65);
		glVertex3f(popupxy[0],popupxy[1],0);
		glVertex3f(popupxy[0],popupxy[1]+linecount*16,0);
		glVertex3f(popupxy[0]+(maxlen+maxlen*maxlen/49+1)*5,popupxy[1]+linecount*16,0);
		glVertex3f(popupxy[0]+(maxlen+maxlen*maxlen/49+1)*5,popupxy[1],0);
		glEnd();

		for(int l=0; l<popuptext.size(); l++) {
			RenderString(popupxy[0]+5, popupxy[1]+14*(l+1), GLUT_BITMAP_HELVETICA_12, popuptext[l].c_str(), 0.8f, 1.0f, 1.0f);
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


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ START DRAW CELLS ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//


void GLView::drawCell(int x, int y, const float values[Layer::LAYERS])
{
	int layers= getLayerDisplayCount();
	if (layers!=0) { //count = 0: off. display no layers
		Color3f cellcolor(0.0, 0.0, 0.0);

		//init color additives
		Color3f light= livearray_layersvis[DisplayLayers::LIGHT] ? setColorLight(values[Layer::LIGHT]) : setColorLight(1);

		Color3f terrain= livearray_layersvis[DisplayLayers::ELEVATION] ? setColorHeight(values[Layer::ELEVATION]) : setColorHeight(0);

		Color3f plant= livearray_layersvis[DisplayLayers::PLANTS] ? setColorPlant(values[Layer::PLANTS]) : setColorHeight(0);

		Color3f hazard= livearray_layersvis[DisplayLayers::HAZARDS] ? setColorHazard(values[Layer::HAZARDS]) : setColorHeight(0);

		Color3f fruit= livearray_layersvis[DisplayLayers::FRUITS] ? setColorFruit(values[Layer::FRUITS]) : setColorHeight(0);

		Color3f meat= livearray_layersvis[DisplayLayers::MEATS] ? setColorMeat(values[Layer::MEATS]) : setColorHeight(0);

		Color3f temp= livearray_layersvis[DisplayLayers::TEMP] ? setColorTemp((float)y) : setColorHeight(0); //special for temp: until cell-based, convert y coord and process

		if(layers>1) {
			terrain= livearray_layersvis[DisplayLayers::ELEVATION] ? setColorTerrain(values[Layer::ELEVATION]) : setColorHeight(0);
			if(livearray_layersvis[DisplayLayers::PLANTS] || livearray_layersvis[DisplayLayers::ELEVATION]) fruit= setColorHeight(0); //these are handled below if layer is selected + another
			meat= setColorHeight(0);
			temp.red*= 0.15; temp.gre*= 0.15; temp.blu*= 0.15;
		} else if(livearray_layersvis[DisplayLayers::LIGHT]) terrain= setColorHeight(1);

		if(livearray_layersvis[DisplayLayers::ELEVATION]){
			if(values[Layer::ELEVATION]>Elevation::BEACH_MID*0.1){ //cell is land
				if (livearray_layersvis[DisplayLayers::PLANTS] && values[Layer::ELEVATION]==Elevation::MOUNTAIN_HIGH*0.1) plant= setColorMountPlant(values[Layer::PLANTS]); //make mountain peaks

			} else { //cell is water
				//if(values[Layer::HAZARDS]>0.9){ light.red=0.0; light.gre=0.0; light.blu=0.0; } //danger from the deep

				//change plant and hazard colors in water
				plant= livearray_layersvis[DisplayLayers::PLANTS] ? setColorWaterPlant(values[Layer::PLANTS]) : setColorHeight(0);
				hazard= livearray_layersvis[DisplayLayers::HAZARDS] ? setColorWaterHazard(values[Layer::HAZARDS]) : setColorHeight(0);
			}
		}
		if(livearray_layersvis[DisplayLayers::HAZARDS] && values[Layer::HAZARDS]>0.9){ light.red=1.0; light.gre=1.0; light.blu=1.0; } //lightning from above

		cellcolor.red= cap((terrain.red + plant.red + hazard.red + fruit.red + meat.red)*light.red + temp.red);
		cellcolor.gre= cap((terrain.gre + plant.gre + hazard.gre + fruit.gre + meat.gre)*light.gre + temp.gre);
		cellcolor.blu= cap((terrain.blu + plant.blu + hazard.blu + fruit.blu + meat.blu)*light.blu + temp.blu);


		glBegin(GL_QUADS);
		glColor4f(cellcolor.red, cellcolor.gre, cellcolor.blu, 1);

		//code below makes cells into divided boxes when zoomed close up
		float gadjust= 0;
		if(scalemult>0.8 || live_grid) gadjust= scalemult<=0 ? 0 : 0.5/scalemult;

		glVertex3f(x*conf::CZ+gadjust,y*conf::CZ+gadjust,0);
		glVertex3f(x*conf::CZ+conf::CZ-gadjust,y*conf::CZ+gadjust,0);
		if (layers>1 && livearray_layersvis[DisplayLayers::ELEVATION]){ //if we're displaying reality, switch colors for bottom half for water!
			float waterwave= (values[Layer::ELEVATION]>=Elevation::BEACH_MID*0.1) ? 0 : 
				0.03*sin((float)(currentTime-590*x*(x+y)+350*y*y*x)/250);
			glColor4f(cellcolor.red + waterwave*light.red, cellcolor.gre + waterwave*light.gre, cellcolor.blu + waterwave*light.blu, 1);
		}
		glVertex3f(x*conf::CZ+conf::CZ-gadjust,y*conf::CZ+conf::CZ-gadjust,0);
		glVertex3f(x*conf::CZ+gadjust,y*conf::CZ+conf::CZ-gadjust,0);

		//if Land/All draw mode, draw fruit and meat as little diamonds
		if (layers>1) {
			if(livearray_layersvis[DisplayLayers::MEATS] && values[Layer::MEATS]>0.03 && scalemult>0.1){
				float meat= values[Layer::MEATS];
				cellcolor= setColorMeat(1.0); //meat on this layer is always bright red, but translucence is applied
				glColor4f(cellcolor.red*light.red, cellcolor.gre*light.gre, cellcolor.blu*light.blu, meat*0.8);
				float meatsz= 0.25*meat+0.15;

				glVertex3f(x*conf::CZ+0.5*conf::CZ,y*conf::CZ+0.5*conf::CZ+meatsz*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ+meatsz*conf::CZ,y*conf::CZ+0.5*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ,y*conf::CZ+0.5*conf::CZ-meatsz*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ-meatsz*conf::CZ,y*conf::CZ+0.5*conf::CZ,0);
				
				//make a smaller, solid center
				glColor4f(cellcolor.red*values[Layer::LIGHT], cellcolor.gre*values[Layer::LIGHT], cellcolor.blu*values[Layer::LIGHT], 1);
				meatsz-= 0.15;

				glVertex3f(x*conf::CZ+0.5*conf::CZ,y*conf::CZ+0.5*conf::CZ+meatsz*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ+meatsz*conf::CZ,y*conf::CZ+0.5*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ,y*conf::CZ+0.5*conf::CZ-meatsz*conf::CZ,0);
				glVertex3f(x*conf::CZ+0.5*conf::CZ-meatsz*conf::CZ,y*conf::CZ+0.5*conf::CZ,0);
			}

			if(livearray_layersvis[DisplayLayers::FRUITS] && values[Layer::FRUITS]>0.1 && scalemult>0.1){
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
					glVertex3f(fruitposx,fruitposy+1,0);
					glVertex3f(fruitposx+1,fruitposy,0);
					glVertex3f(fruitposx,fruitposy-1,0);
					glVertex3f(fruitposx-1,fruitposy,0);
				}
			}
		}

		glEnd();
	}
}

void GLView::setWorld(World* w)
{
	world = w;
}
