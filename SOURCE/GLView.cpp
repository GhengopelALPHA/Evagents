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


void RenderString(float x, float y, void *font, const char* string, float r, float g, float b, float a= 1.0)
{
	glColor4f(r,g,b,a);
	glRasterPos2f(x, y);
	int len = (int) strlen(string);
	for (int i = 0; i < len; i++)
		glutBitmapCharacter(font, string[i]);
}

void drawCircle(float x, float y, float r) {
	float n;
	for (int k=0;k<17;k++) {
		n = k*(M_PI/8);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
	}
}

void drawCircleRes(float x, float y, float r, int res) {
	float n;
	if(res<1) res=1;
	for (int k=0;k<(8*res+1);k++) {
		n = k*(M_PI/4/res);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
	}
}


void drawOutlineRes(float x, float y, float r, int res) {
	float n;
	if(res<1) res=1;
	for (int k=0;k<(8*res+1);k++) {
		n = k*(M_PI/4/res);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
		n = (k+1)*(M_PI/4/res);
		glVertex3f(x+r*sin(n),y+r*cos(n),0);
	}
}


void drawQuadrant(float x, float y, float r, float a, float b) {
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
		mousedrag(false)
{

	xtranslate= 0.0;
	ytranslate= 0.0;
	scalemult= 0.2;
	downb[0]=0;downb[1]=0;downb[2]=0;
	mousex=0;mousey=0;
	popupReset();
	savehelper= new ReadWrite();
	lastsavedtime= 0;
}

GLView::~GLView()
{

}

void GLView::gotoDefaultZoom()
{
	float scaleA= 0;
	scaleA= (float)glutGet(GLUT_WINDOW_WIDTH);
	scaleA/= (conf::WIDTH+2200);
	float scaleB= (float)glutGet(GLUT_WINDOW_HEIGHT)/(conf::HEIGHT+150);
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
}

void GLView::processMouse(int button, int state, int x, int y)
{
	if(world->isDebug()) printf("MOUSE EVENT: button=%i state=%i x=%i y=%i, scale=%f, mousedrag=%i\n", button, state, x, y, scalemult, mousedrag);
	
	if(!mousedrag && state==1){ //dont let mouse click do anything if drag flag is raised
		//check our buttonlist! FUTURE!
		//THE FUTURE IS NOW?

		//have world deal with it. First translate to world coordinates though
		int wx= (int) ((x-glutGet(GLUT_WINDOW_WIDTH)/2)/scalemult-xtranslate);
		int wy= (int) ((y-glutGet(GLUT_WINDOW_HEIGHT)/2)/scalemult-ytranslate);

		if(world->processMouse(button, state, wx, wy, scalemult) && live_selection!=Select::RELATIVE) live_selection = Select::MANUAL;
	}

	mousex=x; mousey=y; //update tracked mouse position
	mousedrag= false;

	downb[button]=1-state; //state is backwards, ah well
}

void GLView::processMouseActiveMotion(int x, int y)
{
	if(world->isDebug()) printf("MOUSE MOTION x=%i y=%i, %i %i %i\n", x, y, downb[0], downb[1], downb[2]);
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

	if(abs(mousex-x)>4 || abs(mousey-y)>4) mousedrag= true; //mouse was clearly dragged, don't select agents after
	if(live_layersvis!=Display::REALITY) handlePopup(x, y); //make sure we handle any popups
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
	if(live_layersvis!=Display::REALITY && live_layersvis!=Display::NONE) {
		//convert mouse x,y to world x,y
		int worldcx= ((x-glutGet(GLUT_WINDOW_WIDTH)/2)/scalemult-xtranslate)/conf::CZ;
		int worldcy= ((y-glutGet(GLUT_WINDOW_HEIGHT)/2)/scalemult-ytranslate)/conf::CZ;

		if (worldcx>=0 && worldcx<conf::WIDTH && worldcy>=0 && worldcy<conf::HEIGHT) { //if somewhere in world...
			char line[128];
			int level= 0;

			popupReset(x+12, y); //clear and set popup position near mouse
			
			switch(live_layersvis){
				case Display::ELEVATION :	strcpy(line, "Height");		level= Layer::ELEVATION;	break;
				case Display::PLANTS :		strcpy(line, "Plant");		level= Layer::PLANTS;		break;
				case Display::MEATS :		strcpy(line, "Meat");		level= Layer::MEATS;		break;
				case Display::FRUITS :		strcpy(line, "Fruit");		level= Layer::FRUITS;		break;
				case Display::HAZARDS :		strcpy(line, "Waste");		level= Layer::HAZARDS;		break;
				case Display::TEMP :		strcpy(line, "Temperature");	level= -1;				break;
				case Display::LIGHT :		strcpy(line, "Light Level");	level= Layer::LIGHT;	break;
			}
			popupAddLine(line);

			sprintf(line, "x: %d, y: %d", worldcx, worldcy);
			popupAddLine(line);
			if(live_layersvis!=Display::TEMP) sprintf(line, "value: %.3f", world->cells[level][worldcx][worldcy]);
			//must handle temp layer different b/c not a real cell layer (for now)
			else sprintf(line, "value: %.3f", 2.0*abs((float)worldcy*conf::CZ/conf::HEIGHT - 0.5));
			popupAddLine(line);

			//Special extra infos
			if(live_layersvis==Display::ELEVATION) {
				int landtype= (int)ceilf(world->cells[level][worldcx][worldcy]*10);
				switch(landtype){
					case 0: case 1: case 2: case 3: case 4:	strcpy(line, "Ocean");	break;
					case 5 :	strcpy(line, "Beach");	break;
					case 6 :	strcpy(line, "Plains");	break;
					case 7 :	strcpy(line, "Steppe");	break;
					case 8 :	strcpy(line, "Hill");	break;
					case 9 :	strcpy(line, "Highland");	break;
					case 10 :	strcpy(line, "Mountain");	break;
				}
				popupAddLine(line);
			}
				
			if(live_layersvis==Display::HAZARDS && world->cells[level][worldcx][worldcy]>0.9) {
				strcpy(line, "LIGHTNING STRIKE!");
				popupAddLine(line);
			}
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
	if (key == 27) //[esc] quit
		exit(0);
	else if (key=='h') { //interface help
		//MAKE SURE ALL UI CHANGES CATELOGUED HERE
		world->addEvent("Left-click an agent to select it",5);
		world->addEvent("Press 'f' to follow selection",5);
		world->addEvent("Pan with left-mouse drag or ",5);
		world->addEvent("  use the arrow keys",5);
		world->addEvent("Zoom with middle-mouse drag or ",5);
		world->addEvent("  use the '<'/'>' keys",5);
		world->addEvent("Press 'p' to pause",5);
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
	} else if (key=='p') {
		//pause
		live_paused= !live_paused;
	} else if (key=='m') { //drawing
		live_fastmode= !live_fastmode;
		if (live_fastmode) world->dismissNextEvents(world->events.size());
	} else if (key==43 || key=='+') { //+
		live_skipdraw++;
	} else if (key==45 || key=='-') { //-
		live_skipdraw--;
	} else if (key=='l' || key=='k') { //layer switch; l= "next", k= "previous"
		if (key=='l') live_layersvis++;
		else live_layersvis--;
		if (live_layersvis>Display::DISPLAYS-1) live_layersvis= Display::NONE;
		if (live_layersvis<Display::NONE) live_layersvis= Display::DISPLAYS-1;
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
	}else if (key==127 && world->isDebug()) { //delete
		world->selectedKill();
	}else if (key==62) { //zoom+ >
		scalemult += 10*conf::ZOOM_SPEED;
	}else if (key==60) { //zoom- <
		scalemult -= 10*conf::ZOOM_SPEED;
		if(scalemult<0.01) scalemult=0.01;
	}else if (key==32) { //spacebar input [pressed]
		world->selectedInput(1);
	}else if (key==35) { //end key: print agent traits and pause world
		world->selectedPrint();
		live_paused= 1;
	}else if (key=='e') { // e: report selected's wheel inputs
		world->selectedTrace(1);
	}else if (key=='/' && world->isDebug()) { // / heal selected
		world->selectedHeal();
	}else if (key=='|' && world->isDebug()) { // | reproduce selected
		world->selectedBabys();
	}else if (key=='~' && world->isDebug()) { // ~ mutate selected
		world->selectedMutate();
	}else if (key=='`' && world->isDebug()) { // ` rotate selected stomach type
		world->selectedStomachMut();
	}else if (key==119) { //w (move faster)
		world->pcontrol= true;
		float newleft= capm(world->pleft + 0.04 + (world->pright-world->pleft)*0.35, -1, 1); //this extra code helps with turning out of tight circles
		world->pright= capm(world->pright + 0.04 + (world->pleft-world->pright)*0.35, -1, 1);
		world->pleft= newleft;
	}else if (key==97) { //a (turn left)
		world->pcontrol= true;
		world->pleft= capm(world->pleft - 0.02, -1, 1);
		world->pright= capm(world->pright + 0.02, -1, 1);
	}else if (key==115) { //s (move slower/reverse)
		world->pcontrol= true;
		float newleft= capm(world->pleft - 0.04 + (world->pright-world->pleft)*0.35, -1, 1);
		world->pright= capm(world->pright - 0.04 + (world->pleft-world->pright)*0.35, -1, 1);
		world->pleft= newleft;
	}else if (key==100) { //d (turn right)
		world->pcontrol= true;
		world->pleft= capm(world->pleft + 0.02, -1, 1);
		world->pright= capm(world->pright - 0.02, -1, 1);
	} else if (key==999) { //player control
		world->setControl(!world->pcontrol);
		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->pcontrol) glutChangeToMenuEntry(1, "Release Agent", 999);
		else glutChangeToMenuEntry(1, "Control Selected (w,a,s,d)", 999);
		glutSetMenu(m_id);

	// MENU ONLY OPTIONS:
	} else if (key==1001) { //add agents
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
		handleRW(2);
	} else if (key==1009) { //load world
		handleRW(1);
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
	}
}

void GLView::glCreateMenu()
{
	//right-click context menu
	sm1_id= glutCreateMenu(gl_menu); //configs & UI
	glutAddMenuEntry("Dismiss Visible Events (n)", 'n');
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Enter Debug Mode", 1005);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Re(over)write Config", 1006);
	glutAddMenuEntry("Load Config", 1010);

	sm2_id= glutCreateMenu(gl_menu); //spawn new
	glutAddMenuEntry("Agents", 1001);
	glutAddMenuEntry("Herbivores", 1002);
	glutAddMenuEntry("Carnivores", 1003);
	glutAddMenuEntry("Frugivores", 1004);

	sm3_id= glutCreateMenu(gl_menu); //selected agent
	glutAddMenuEntry("Control (w,a,s,d)", 999);
	glutAddMenuEntry("Heal (/)", '/');
	glutAddMenuEntry("Reproduce (|)", '|');
	glutAddMenuEntry("Mutate (~)", '~');
	glutAddMenuEntry("Delete (del)", 127);

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
	live_layersvis= Display::REALITY;
	live_profilevis= Profile::NONE;
	live_selection= Select::MANUAL;
	live_follow= 0;
	live_autosave= 0;
	live_debug= world->isDebug();
	live_grid= 0;
	live_landspawns= (int)world->DISABLE_LAND_SPAWN;
	live_moonlight= (int)world->MOONLIT;
	live_droughts= (int)world->DROUGHTS;
	live_droughtmult= (int)world->DROUGHTMULT;
	live_mutevents= (int)world->MUTEVENTS;

	//create GLUI and add the options, be sure to connect them all to their real vals later
	Menu = GLUI_Master.create_glui("Menu",0,1,1);
	Menu->add_checkbox("Pause",&live_paused);
	Menu->add_checkbox("Enable Autosaves",&live_autosave);

//	new GLUI_Button(rollout_world,"Load World",1, glui_handleRW); //change 1->5 for advanced loading window (needs work)
//	new GLUI_Button(rollout_world,"Save World",2, glui_handleRW);
//	new GLUI_Button(rollout_world,"New World",3, glui_handleRW);

	GLUI_Panel *panel_speed= new GLUI_Panel(Menu,"Speed Control");
	Menu->add_checkbox_to_panel(panel_speed,"Fast Mode",&live_fastmode);
	Menu->add_spinner_to_panel(panel_speed,"Speed:",GLUI_SPINNER_INT,&live_skipdraw);


	GLUI_Rollout *rollout_world= new GLUI_Rollout(Menu,"World Options",false);

	Menu->add_button_to_panel(rollout_world,"Load World", 1, glui_handleRW);
	Menu->add_button_to_panel(rollout_world,"Save World",2, glui_handleRW);
	Menu->add_button_to_panel(rollout_world,"New World",3, glui_handleRW);
	Menu->add_checkbox_to_panel(rollout_world,"Disable Spawns",&live_worldclosed);
	Menu->add_checkbox_to_panel(rollout_world,"Disable Land Spawns",&live_landspawns);
	Menu->add_checkbox_to_panel(rollout_world,"Moon Light",&live_moonlight);
	Menu->add_checkbox_to_panel(rollout_world,"Mutation Events",&live_mutevents);
	Menu->add_checkbox_to_panel(rollout_world,"Global Droughts",&live_droughts);
	GLUI_Panel *panel_drought= new GLUI_Panel(rollout_world,"",GLUI_PANEL_NONE);
	Menu->add_spinner_to_panel(panel_drought,"Drought Mod:",GLUI_SPINNER_FLOAT,&live_droughtmult);
	panel_drought->disable();

	GLUI_Rollout *rollout_vis= new GLUI_Rollout(Menu,"Visuals");

	GLUI_RadioGroup *group_layers= new GLUI_RadioGroup(rollout_vis,&live_layersvis);
	new GLUI_StaticText(rollout_vis,"Layer");
	for(int i=0; i<Display::DISPLAYS; i++){
		//this code allows the layers to be defined in any order
		char text[16]= "";
		if(i==Display::NONE) strcpy(text, "off");
		else if(i==Display::PLANTS) strcpy(text, "Plant");
		else if(i==Display::MEATS) strcpy(text, "Meat");
		else if(i==Display::HAZARDS) strcpy(text, "Hazard");
		else if(i==Display::FRUITS) strcpy(text, "Fruit");
		else if(i==Display::REALITY) strcpy(text, "Reality!");
		else if(i==Display::LIGHT) strcpy(text, "Light");
		else if(i==Display::TEMP) strcpy(text, "Temp");
		else if(i==Display::ELEVATION) strcpy(text, "Elevation");

		new GLUI_RadioButton(group_layers,text);
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


		new GLUI_RadioButton(group_agents,text);
	}
	Menu->add_checkbox_to_panel(rollout_vis, "Grid on", &live_grid);
	
	GLUI_Rollout *rollout_xyl= new GLUI_Rollout(Menu,"Selection");
	GLUI_RadioGroup *group_select= new GLUI_RadioGroup(rollout_xyl,&live_selection);
	for(int i=0; i<Select::SELECT_TYPES; i++){
		char text[16]= "";
		if(i==Select::OLDEST) strcpy(text, "Oldest");
		else if(i==Select::BEST_GEN) strcpy(text, "Best Gen.");
		else if(i==Select::HEALTHY) strcpy(text, "Healthiest");
		else if(i==Select::PRODUCTIVE) strcpy(text, "Productive");
		else if(i==Select::AGGRESSIVE) strcpy(text, "Aggressive");
		else if(i==Select::NONE) strcpy(text, "off");
		else if(i==Select::MANUAL) strcpy(text, "Manual");
		else if(i==Select::RELATIVE) strcpy(text, "Relative");

		new GLUI_RadioButton(group_select,text);
		if(i==Select::NONE) Menu->add_checkbox_to_panel(rollout_xyl, "Follow Selected", &live_follow);
	}
	new GLUI_Separator(rollout_xyl);
	Menu->add_button_to_panel(rollout_xyl, "Save Selected", 4, glui_handleRW);

	Menu->add_checkbox("DEBUG",&live_debug);

	//set to main graphics window
	Menu->set_main_gfx_window(win1);
}


//====================================== READ WRITE ===========================================//
void GLView::handleRW(int action) //glui callback for saving/loading worlds
{
	live_paused= 1;

	//action= 1: loading, action= 2: saving, action= 3: new (reset) world
	if (action==1){ //basic load option selected
		//Step 1 of loading: check if user really wants to reset world
		if(world->getEpoch()>0){
			//get time since last save
			int time= glutGet( GLUT_ELAPSED_TIME );
			char timetype= 'x';

			time-= lastsavedtime;
			if(time>=0 && time<60000) { timetype= 's'; time/=1000; }
			else if(time<3600000) { timetype= 'm'; time/=60000; }
			else if(time<86400000) { timetype= 'h'; time/=3600000; }
			else { timetype= 'd'; time/=86400000; }

			sprintf(buf,"Last saved %d %c ago", time, timetype);
			printf("%s", buf);

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert,"Are you sure? This will");
			new GLUI_StaticText(Alert,"erase the current world.");
			new GLUI_StaticText(Alert,buf); 
			new GLUI_Button(Alert,"Okay",0, glui_handleRW);
			new GLUI_Button(Alert,"Cancel",9, glui_handleCloses);

			Alert->set_main_gfx_window(win1);
		} else handleRW(0); //no need for alerts if epoch == 0

	} else if (action==0) {
		Loader = GLUI_Master.create_glui("Load World",0,50,50);

		Filename= new GLUI_EditText(Loader,"Enter Save Name (e.g, 'WORLD'):");
		Filename->set_w(300);
		new GLUI_Button(Loader,"Load",1, glui_handleCloses);
		new GLUI_Button(Loader,"Cancel",9, glui_handleCloses);

		Loader->set_main_gfx_window(win1);

	} else if (action==2) { //basic save option
		Saver = GLUI_Master.create_glui("Save World",0,50,50);

		Filename= new GLUI_EditText(Saver,"Type Save Name (e.g, 'WORLD'):");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save",2, glui_handleCloses);

		Saver->set_main_gfx_window(win1);
	} else if (action==3){ //new world (old reset)
		Alert = GLUI_Master.create_glui("Alert",0,50,50);
		Alert->show();
		new GLUI_StaticText(Alert,"Are you sure? This will");
		new GLUI_StaticText(Alert,"erase the current world."); 
		new GLUI_Button(Alert,"Okay",3, glui_handleCloses);
		new GLUI_Button(Alert,"Cancel",4, glui_handleCloses);

		Alert->set_main_gfx_window(win1);
	} else if (action==4) { //save selected agent
		Saver = GLUI_Master.create_glui("Save Agent",0,50,50);

		Filename= new GLUI_EditText(Saver,"Enter Save Name (e.g, 'AGENT'):");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save",6, glui_handleCloses);
	} else if (action==5) { //advanced loader
		Loader = GLUI_Master.create_glui("Load World",0,50,50);
		Browser= new GLUI_FileBrowser(Loader,"",false,8, glui_handleCloses);
	}
}

void GLView::handleCloses(int action) //GLUI callback for handling window closing
{
	live_paused= 0;

	if (action==1) { //loading
		//Step 2: actual loading
		strcpy(filename,Filename->get_text());
		
		if (checkFile(filename)) {
			savehelper->loadWorld(world, xtranslate, ytranslate, filename);
			//.cfg/.sav make sure to put all saved world variables with GUI options here so they update
			live_worldclosed= (int)world->isClosed();
			live_landspawns= (int)world->DISABLE_LAND_SPAWN;
			live_moonlight= (int)world->MOONLIT;
			live_droughts= (int)world->DROUGHTS;
			live_droughtmult= (int)world->DROUGHTMULT;
			live_mutevents= (int)world->MUTEVENTS;
		}
		Loader->hide();

	} else if (action==2) { //saving
		trySaveWorld();
		Saver->hide();

	} else if (action==3) { //resetting
		world->reset();
		world->spawn();
		printf("WORLD RESET!\n");
		Alert->hide();
	} else if (action==4) { //Alert cancel/continue
		Alert->hide();
	} else if (action==5) { //Alert from above saving
		savehelper->saveWorld(world, xtranslate, ytranslate, filename);
		world->addEvent("Saved World (overwritten)",5);
		Alert->hide();
	} else if (action==6) { //saving agents
		const char *tempname= Filename->get_text();
		strcpy(filename, tempname);
		strcat(filename, ".AGT");

		if (checkFile(filename)) {
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
	} else if (action==8) { //advanced loader
		file_name= "";
		file_name= Browser->get_file();
		strcpy(filename,file_name.c_str());

		if (checkFile(filename)) {
			savehelper->loadWorld(world, xtranslate, ytranslate, filename);
		}
		Loader->hide();
	} else if(action==9) { //loader cancel from alert
		Loader->hide();
		Alert->hide();
	}
}


bool GLView::checkFile(char name[30]){

	if (debug) printf("File: '%s'\n",name);
	if (!name || name=="" || name==NULL || name[0]=='\0'){
		printf("ERROR: empty filename; returning to program.\n");

		Alert = GLUI_Master.create_glui("Alert",0,50,50);
		Alert->show();
		new GLUI_StaticText(Alert, "No file name given." );
		new GLUI_StaticText(Alert, "Returning to main program." );
		new GLUI_Button(Alert,"Okay",4, glui_handleCloses);
		return false;

	} else return true;
}

/*bool GLView::checkFile(std::string name){

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

void GLView::trySaveWorld(bool autosave)
{
	if(autosave) savehelper->saveWorld(world, xtranslate, ytranslate, "AUTOSAVE");
	else {
		strcpy(filename, Filename->get_text());

		if (checkFile(filename)) {
			//check the filename given to see if it exists yet
			char address[32];
			strcpy(address,"saves\\");
			strcat(address,filename);

			FILE* ck = fopen(address, "r");
			if(ck){
				if (debug) printf("WARNING: %s already exists!\n", filename);

				Alert = GLUI_Master.create_glui("Alert",0,50,50);
				Alert->show();
				new GLUI_StaticText(Alert, "The file name given already exists." );
				new GLUI_StaticText(Alert, "Would you like to overwrite?" );
				new GLUI_Button(Alert,"Okay",5, glui_handleCloses);
				new GLUI_Button(Alert,"Cancel",4, glui_handleCloses);
				live_paused= 1;

				fclose(ck);
			} else {
				savehelper->saveWorld(world, xtranslate, ytranslate, filename);
				lastsavedtime= glutGet( GLUT_ELAPSED_TIME );
				world->addEvent("Saved World",5);
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


void GLView::countdownEvents()
{
	//itterate event counters when drawing, once per frame regardless of skipdraw
	int count= world->events.size();
	if(conf::EVENTS_DISP<count) count= conf::EVENTS_DISP;
	for(int io= 0; io<count; io++){
		world->events[io].second.second--;
	}
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
//	world->DROUGHTMULT= live_droughtmult;
	world->MUTEVENTS= (bool)live_mutevents;
	world->setDebug((bool) live_debug);
	if(world->isDebug()) printf("/");

	if(world->modcounter==0 && world->getEpoch()==0){
		//do some spare actions if this is the first tick of the game (Main Menu here???)
		gotoDefaultZoom();
		past_agentsvis= live_agentsvis;
	}

	modcounter++;
	if (!live_paused) world->update();

	//autosave world periodically, based on world time
	if (live_autosave==1 && world->modcounter%(world->FRAMES_PER_DAY*10)==0){
		trySaveWorld(true);
	}

	//Do some recordkeeping and let's intelligently prevent users from simulating nothing
	if(world->getAgents()<=0 && live_worldclosed==1) {
		live_worldclosed= 0;
		if(live_autosave==1) live_autosave= 0; //I'm gonna guess you don't want to save the world anymore
		world->addEvent("Disabled Closed world, nobody was home!",7);
	}

	//show FPS and other stuff
	int currentTime = glutGet( GLUT_ELAPSED_TIME );
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

	glTranslatef(glutGet(GLUT_WINDOW_WIDTH)/2, glutGet(GLUT_WINDOW_HEIGHT)/2, 0.0f);	
	glScalef(scalemult, scalemult, 1.0f);

	//handle world agent selection interface
	world->setSelection(live_selection);
	if (world->getSelection()==-1 && live_selection!=Select::MANUAL && live_selection!=Select::NONE && world->modcounter%5==0) {
		live_selection= Select::NONE;
		world->addEvent("No valid Autoselect targets", 13);
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

	glTranslatef(xtranslate, ytranslate, 0.0f); //translate the view as needed
	
	world->draw(this, live_layersvis); //draw the world!

	glPopMatrix();
	glutSwapBuffers();

	countdownEvents();

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
	color.red= sqrt(discomfort);
	color.gre= discomfort==0 ? 0.75 : (2-discomfort)/4;
	color.blu= powf(1-discomfort,3);
	return color;
}

Color3f GLView::setColorMetabolism(float metabolism)
{
	Color3f color;
	color.red= metabolism;
	color.gre= metabolism*(1-metabolism);
	color.blu= 0.2+0.4*abs(metabolism*2-1);
	return color;
}

Color3f GLView::setColorTone(float tone)
{
	Color3f color;
	color.red= (1-tone)*(1-tone);
	color.gre= 1-fabs(tone-0.5)*2;
	color.blu= tone*tone;
	return color;
}

Color3f GLView::setColorLungs(float lungs)
{
	Color3f color;
	color.red= cap((0.2+lungs)*(0.8-lungs));
	color.gre= cap((0.05+lungs)*(1.4-lungs));
	color.blu= (1-lungs);
	return color;
}

Color3f GLView::setColorSpecies(float species)
{
	Color3f color;
	color.red= (cos((float)species/123*M_PI)+1.0)/2.0;
	color.gre= (sin((float)species/53*M_PI)+1.0)/2.0;
	color.blu= (cos((float)species/37*M_PI)+1.0)/2.0;
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
	Color3f color;
	bool is_land= (values[Layer::ELEVATION]>0.5) ? true : false;
	Color3f terrain= setColorTerrain(values[Layer::ELEVATION]);
	Color3f plant= is_land ? setColorPlant(values[Layer::PLANTS]) : setColorWaterPlant(values[Layer::PLANTS]);
	if (values[Layer::ELEVATION]==1) plant= setColorMountPlant(values[Layer::PLANTS]);
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
		color.red= 0.8;
		color.gre= 0.8;
		color.blu= 0.8;
	} else if(val==0.5){ //beach
		color.red= 0.9;
		color.gre= 0.9;
		color.blu= 0.6;
	} else if(val==0) { //water
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
	Color3f color(0.0,val/2*0.3,0.0);
	return color;
}

Color3f GLView::setColorMountPlant(float val)
{
	Color3f color(-val/2*0.2,val/2*0.2,-val/2*0.2);
	return color;
}

Color3f GLView::setColorFruit(float val)
{
	Color3f color(val/2*0.8,val/2*0.8,0.1);
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
		color.red= world->SUN_RED*val>0.4 ? cap(world->SUN_RED*val) : 0.4;
		color.gre= world->SUN_GRE*val>0.4 ? cap(world->SUN_GRE*val) : 0.4;
		color.blu= world->SUN_BLU*val+0.1>0.5 ? cap(world->SUN_BLU*val+0.1) : 0.5;
	} else {
		color.red= cap(world->SUN_RED*val);
		color.gre= cap(world->SUN_GRE*val);
		color.blu= cap(world->SUN_BLU*val+0.1);
	}
	return color;
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~DRAW AGENT~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//


void GLView::drawAgent(const Agent& agent, float x, float y, bool ghost)
{
	float n;
	float r= agent.radius;
	float rp= agent.radius+2.8;

	if (live_agentsvis!=Visual::NONE) {

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
		}


		//handle selected agent
		if (agent.id==world->getSelection() && !ghost) {
			//draw selection
			glLineWidth(2);
			glBegin(GL_LINES);
			glColor3f(1,1,1);
			drawOutlineRes(x, y, r+4/scalemult, ceil(scalemult)), 
			glEnd();
			glLineWidth(1);

			if(scalemult > .2){
				glPushMatrix();
				glTranslatef(x-90,y+24+0.5*agent.radius,0);

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

						if(tone==0.25) glColor4f(0.0,0.8,0.0,fiz);
						else glColor4f(0.7,0.7,0.7,fiz);

						glVertex3f(2+176*tone, 78, 0);
						glVertex3f(2+176*tone, 78-76*volume, 0);
					}
					glEnd();

					//now show our own sound, colored by tone
					glLineWidth(2);
					glBegin(GL_LINES);
					glColor4f(tonecolor.red, tonecolor.gre, tonecolor.blu, 0.5);
					glVertex3f(2+176*agent.tone, 78, 0);
					glVertex3f(2+176*agent.tone, 78-76*agent.volume, 0);
					glEnd();
					glLineWidth(1);

				}
				
				glPopMatrix();

				if(world->isDebug()){
					//draw DIST range on selected in DEBUG
					glBegin(GL_LINES);
					glColor3f(1,0,1);
					drawOutlineRes(x, y, world->DIST, ceil(scalemult)+6);
					glEnd();

					//now spike, share, and grab effective zones
					glBegin(GL_POLYGON);
					glColor4f(1,0,0,0.75);
					glVertex3f(x,y,0);
					glColor4f(1,0,0,0.25);
					glVertex3f(x+(r+agent.spikeLength*world->SPIKELENGTH)*cos(agent.angle+M_PI/8),
							   y+(r+agent.spikeLength*world->SPIKELENGTH)*sin(agent.angle+M_PI/8),0);
					glVertex3f(x+(r+agent.spikeLength*world->SPIKELENGTH)*cos(agent.angle),
							   y+(r+agent.spikeLength*world->SPIKELENGTH)*sin(agent.angle),0);
					glVertex3f(x+(r+agent.spikeLength*world->SPIKELENGTH)*cos(agent.angle-M_PI/8),
							   y+(r+agent.spikeLength*world->SPIKELENGTH)*sin(agent.angle-M_PI/8),0);
					glVertex3f(x,y,0);
					glEnd();

					//grab is currently a range-only thing, change?
					glBegin(GL_POLYGON);
					glColor4f(0,1,1,0.15);
					drawCircle(x,y,r+world->GRABBING_DISTANCE);
					glEnd();

					//health-sharing
					glBegin(GL_POLYGON);
					glColor4f(0,0.5,0,0.25);
					drawCircle(x,y,world->FOOD_SHARING_DISTANCE);
					glEnd();
				}
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
		}


		float rad= r;
		float centercmult= 0.75;
		// when we're zoomed out far away, do some things differently, but not to the ghost (selected agent display)
		if( scalemult <= 0.1 && !ghost) {
			rad= 15;
			centercmult= 1; //this keeps a solid color inside
		} else if (agent.health<=0) centercmult= 1; //this keeps dead agents from appearing asexual (and the award for strangest comment goes to...)


		//draw indicator of this agent... used for various events
		if (agent.indicator>0) {
			glBegin(GL_POLYGON);
			glColor4f(agent.ir,agent.ig,agent.ib,0.75);
			drawCircle(x, y, rad+((int)agent.indicator));
			glEnd();
		}

		//draw giving/receiving
		if(agent.dhealth!=0){
			glBegin(GL_POLYGON);
			float mag= (live_agentsvis==Visual::HEALTH) ? 3 : 1;//draw sharing as a thick green or red outline, bigger if viewing health
			glColor3f(generocitycolor.red, generocitycolor.gre, generocitycolor.blu);
			for (int k=0;k<17;k++){
				n = k*(M_PI/8);
				glVertex3f(x+rp*mag*sin(n),y+rp*mag*cos(n),0);
				n = (k+1)*(M_PI/8);
				glVertex3f(x+rp*mag*sin(n),y+rp*mag*cos(n),0);
			}
			glEnd();
		}
		
		float dead= agent.health==0 ? conf::DEADTRANSPARENCY : 1; //mult for dead agents, displayed with more transparent parts

		if (scalemult > .3 || ghost) { //dont render eyes, ears, or boost if zoomed out, but always render them on ghosts
			//draw eyes
			for(int q=0;q<NUMEYES;q++) {
				glBegin(GL_LINES);
				if (live_profilevis==Profile::EYES) {
					//color eyes based on actual input if we're on the eyesight profile
					glColor4f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3],0.75*dead);
				} else glColor4f(0.5,0.5,0.5,0.75*dead);
				glVertex3f(x,y,0);
				float aa= agent.angle+agent.eyedir[q];
				glVertex3f(x+(r+30)*cos(aa), y+(r+30)*sin(aa), 0);
				if(world->isDebug()){
					aa+= agent.eyefov[q]/2;
					glVertex3f(x+(r+30)*cos(aa), y+(r+30)*sin(aa), 0);
					aa-= agent.eyefov[q];
					glVertex3f(x+(r+30)*cos(aa), y+(r+30)*sin(aa), 0);
				}
				glEnd();
			}

			//ears
			for(int q=0;q<NUMEARS;q++) {
				glBegin(GL_POLYGON);
				//color ears differently if we are set on the sound profile or ghost
				if(live_profilevis==Profile::SOUND || ghost) glColor4f(1-(q/(NUMEARS-1)),q/(NUMEARS-1),0,0.75);
				else glColor4f(0.6,0.6,0,0.5);
				float aa= agent.angle + agent.eardir[q];
				drawCircle(x+r*cos(aa), y+r*sin(aa), 2.3);
				glEnd();
			}

			//boost blur
			if (agent.boost){
				for(int q=1;q<4;q++){
					Vector2f displace(r/3*q*(agent.w1+agent.w2), 0);
					displace.rotate(agent.angle+M_PI);

					glBegin(GL_POLYGON); 
					glColor4f(red,gre,blu,0.25);
					drawCircle(x+displace.x, y+displace.y, r);
					glEnd();
				}
			}

			//vis grabbing (if enabled)
			if(world->GRAB_PRESSURE!=0 && agent.grabbing>0.5){
				glLineWidth(2);
				glBegin(GL_LINES);
				
				glColor4f(0.0,0.7,0.7,0.75);
				glVertex3f(x,y,0);
				float mult= agent.grabID==-1 ? 1 : 0;
				float aa= agent.angle+agent.grabangle+M_PI/8*mult;
				float ab= agent.angle+agent.grabangle-M_PI/8*mult;
				glVertex3f(x+(world->GRABBING_DISTANCE+r)*cos(aa), y+(world->GRABBING_DISTANCE+r)*sin(aa), 0);
				glVertex3f(x,y,0);
				glVertex3f(x+(world->GRABBING_DISTANCE+r)*cos(ab), y+(world->GRABBING_DISTANCE+r)*sin(ab), 0);
				glEnd();
				glLineWidth(1);

/* agent-agent directed grab vis code. Works but coords are wrong from World.cpp
glLineWidth(2);
				glBegin(GL_LINES);
				
				glColor4f(0.0,0.7,0.7,0.75);
				glVertex3f(x,y,0);
		
				if(agent.grabID!=-1) glVertex3f(agent.grabx, agent.graby, 0);
				else {
					float aa= agent.angle+M_PI/8;
					float ab= agent.angle-M_PI/8;
					glVertex3f(x+(world->GRABBING_DISTANCE+r)*cos(aa), y+(world->GRABBING_DISTANCE+r)*sin(aa), 0);
					glVertex3f(x,y,0);
					glVertex3f(x+(world->GRABBING_DISTANCE+r)*cos(ab), y+(world->GRABBING_DISTANCE+r)*sin(ab), 0);
				}
				glEnd();
				glLineWidth(1);
*/
			}

		}
		
		glBegin(GL_POLYGON); 
		//body
		if(agent.isAsexual() && centercmult<1.0) glColor4f(red,gre,blu,0); //asexuals get a transparent center (they're cells!)
		else glColor4f(red*centercmult,gre*centercmult,blu*centercmult,dead); //sexuals get a darker center (they're masses!)
		glVertex3f(x,y,0);
		glColor4f(red,gre,blu,dead);
		drawCircleRes(x, y, rad, ceil(scalemult)+1);
		if(agent.isAsexual() && centercmult<1.0) glColor4f(red,gre,blu,0);
		else glColor4f(red*centercmult,gre*centercmult,blu*centercmult,dead);
		glVertex3f(x,y,0);
		glEnd();

		glBegin(GL_LINES);

		//outline and spikes are effected by the zoom magnitude
		float blur= cap(4.5*scalemult-0.5);
		if (ghost) blur= 1; //disable effect for static rendering

		//jaws
		if ((scalemult > .08 || ghost) && agent.jawrend>0) {
			//dont render jaws if zoomed too far out, but always render them on ghosts, and only if they've been active within the last few ticks
			glColor4f(0.9,0.9,0,blur);
			float mult= 1-powf(abs(agent.jawPosition),0.5);
			glVertex3f(x+rad*cos(agent.angle),y+rad*sin(agent.angle),0);
			glVertex3f(x+(10+rad)*cos(agent.angle+M_PI/8*mult), y+(10+rad)*sin(agent.angle+M_PI/8*mult), 0);
			glVertex3f(x+rad*cos(agent.angle),y+rad*sin(agent.angle),0);
			glVertex3f(x+(10+rad)*cos(agent.angle-M_PI/8*mult), y+(10+rad)*sin(agent.angle-M_PI/8*mult), 0);
		}

		//outline
		float out_red= 0,out_gre= 0,out_blu= 0;
		if (agent.jump>0) { //draw jumping as yellow outline
			out_red= 0.8;
			out_gre= 0.8;
		} else if (agent.radius<=5) {
			out_red= 1.0;
			out_gre= 1.0;
			out_blu= 1.0;
		}
		if (live_agentsvis!=Visual::HEALTH && agent.health<=0){ //draw outline as grey unless visualizing health and dead
			glColor4f(0.7,0.7,0.7,dead);		
		//otherwise, color as agent's body if zoomed out, color as above if normal zoomed
		} else glColor4f(cap(out_red*blur + (1-blur)*red), cap(out_gre*blur + (1-blur)*gre), cap(out_blu*blur + (1-blur)*blu), dead);
		drawOutlineRes(x, y, rad, ceil(scalemult)+1);

		//sound waves!
		if(live_agentsvis==Visual::VOLUME && !ghost && agent.volume>0){
			float volume= agent.volume;
			float count= agent.tone*11+1;
			for (int l=0; l<=(int)count; l++){
				float dist= world->DIST*(l/count)+4*(world->modcounter%(int)((world->DIST)/4));
				if (dist>world->DIST) dist-= world->DIST;
				glColor4f(tonecolor.red, tonecolor.gre, tonecolor.blu, cap((1-dist/world->DIST)*volume));

				for (int k=0;k<32;k++)
				{
					n = k*(M_PI/16);
					glVertex3f(x+dist*sin(n),y+dist*cos(n),0);
					n = (k+1)*(M_PI/16);
					glVertex3f(x+dist*sin(n),y+dist*cos(n),0);
				}
			}
		}

		//and spike, if harmful
		if ((scalemult > .08 && agent.isSpikey(world->SPIKELENGTH)) || ghost) {
			//dont render spike if zoomed too far out, but always render it on ghosts
			glColor4f(0.7,0,0,blur);
			glVertex3f(x,y,0);
			glVertex3f(x+(world->SPIKELENGTH*agent.spikeLength)*cos(agent.angle),
					   y+(world->SPIKELENGTH*agent.spikeLength)*sin(agent.angle),
					   0);
		}
		glEnd();

		//some final debug stuff that is shown even on ghosts:
		if(world->isDebug() || ghost){
			//wheels and wheel speeds
			float wheelangle= agent.angle+ M_PI/2;
			glBegin(GL_LINES);
			glColor3f(1,0,1);
			glVertex3f(x+agent.radius/2*cos(wheelangle),y+agent.radius/2*sin(wheelangle),0);
			glVertex3f(x+agent.radius/2*cos(wheelangle)+20*agent.w1*cos(agent.angle), y+agent.radius/2*sin(wheelangle)+20*agent.w1*sin(agent.angle), 0);
			wheelangle-= M_PI;
			glColor3f(0,1,0);
			glVertex3f(x+agent.radius/2*cos(wheelangle),y+agent.radius/2*sin(wheelangle),0);
			glVertex3f(x+agent.radius/2*cos(wheelangle)+20*agent.w2*cos(agent.angle), y+agent.radius/2*sin(wheelangle)+20*agent.w2*sin(agent.angle), 0);
			glEnd();

			glBegin(GL_POLYGON); 
			glColor3f(0,1,0);
			drawCircle(x+agent.radius/2*cos(wheelangle), y+agent.radius/2*sin(wheelangle), 1);
			glEnd();
			wheelangle+= M_PI;
			glColor3f(1,0,1);
			glBegin(GL_POLYGON); 
			drawCircle(x+agent.radius/2*cos(wheelangle), y+agent.radius/2*sin(wheelangle), 1);
			glEnd();
		}

		if(!ghost){ //only draw extra infos if not a ghost

			if(scalemult > .3) {//hide extra visual data if really far away

				int xo=8+agent.radius;
				int yo=-21;

				//health
				glBegin(GL_QUADS);
				glColor3f(0,0,0);
				glVertex3f(x+xo,y+yo,0);
				glVertex3f(x+xo+5,y+yo,0);
				glVertex3f(x+xo+5,y+yo+42,0);
				glVertex3f(x+xo,y+yo+42,0);

				glColor3f(0,0.8,0);
				glVertex3f(x+xo,y+yo+21*(2-agent.health),0);
				glVertex3f(x+xo+5,y+yo+21*(2-agent.health),0);
				glVertex3f(x+xo+5,y+yo+42,0);
				glVertex3f(x+xo,y+yo+42,0);

				//repcounter/energy
				xo+= 7;
				glBegin(GL_QUADS);
				glColor3f(0,0,0);
				glVertex3f(x+xo,y+yo,0);
				glVertex3f(x+xo+5,y+yo,0);
				glVertex3f(x+xo+5,y+yo+42,0);
				glVertex3f(x+xo,y+yo+42,0);

				glColor3f(0,0.7,0.7);
				glVertex3f(x+xo,y+yo+42*cap(agent.repcounter/agent.maxrepcounter),0);
				glVertex3f(x+xo+5,y+yo+42*cap(agent.repcounter/agent.maxrepcounter),0);
				glVertex3f(x+xo+5,y+yo+42,0);
				glVertex3f(x+xo,y+yo+42,0);

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
					glEnd();

					//debug cell smell box: outlines all cells the selected agent is "smelling"
					if(agent.id==world->getSelection()){
						int minx, maxx, miny, maxy;
						int scx= (int) (agent.pos.x/conf::CZ);
						int scy= (int) (agent.pos.y/conf::CZ);

						minx= (scx-world->DIST/conf::CZ/2) > 0 ? (scx-world->DIST/conf::CZ/2)*conf::CZ : 0;
						maxx= (scx+1+world->DIST/conf::CZ/2) < conf::WIDTH/conf::CZ ? (scx+1+world->DIST/conf::CZ/2)*conf::CZ : conf::WIDTH;
						miny= (scy-world->DIST/conf::CZ/2) > 0 ? (scy-world->DIST/conf::CZ/2)*conf::CZ : 0;
						maxy= (scy+1+world->DIST/conf::CZ/2) < conf::HEIGHT/conf::CZ ? (scy+1+world->DIST/conf::CZ/2)*conf::CZ : conf::HEIGHT;

						glBegin(GL_LINES);
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
				
					//tags: quick HUD of basic bot traits/stats
					int sep= 2;
					int le= 9;
					int wid= 5;
					int numtags= 8;

					xo+= 7+sep;
					for(int i=0;i<numtags;i++){
						int xmult= (int)floor((float)(i/4));
						int ymult= i%4;

						//different tag color schemes go here
						if (i==0){
							if(agent.hybrid) glColor3f(0,0,0.8); //hybrid?
							else if(!agent.isAsexual()) glColor4f(0,0.8,0.8,0.5);
							else continue;
						}
						else if (i==1) glColor3f(stomachcolor.red,stomachcolor.gre,stomachcolor.blu); //stomach type
						else if (i==2) glColor3f(agent.volume,agent.volume,agent.volume); //sound volume emitted
						else if (i==3) glColor3f(discomfortcolor.red,discomfortcolor.gre,discomfortcolor.blu); //temp discomfort
						else if (i==4) glColor3f(lungcolor.red,lungcolor.gre,lungcolor.blu); //land/water lungs requirement
						else if (i==7) glColor3f(metabcolor.red,metabcolor.gre,metabcolor.blu); //metabolism
						else if (i==5){ //ear 1 volume value heard
							float hear= agent.in[Input::HEARING1];
							if(hear==0) continue;
							else glColor3f(hear,hear,hear); //! =D
						}					
						else if (i==6){ //ear 2 volume value heard
							float hear= agent.in[Input::HEARING2];
							if(hear==0) continue;
							else glColor3f(hear,hear,hear);
						}
						
						glVertex3f(x+xo+(wid+sep)*xmult,y+yo+(le+sep)*ymult,0);
						glVertex3f(x+xo+(wid+sep)*xmult+wid,y+yo+(le+sep)*ymult,0);
						glVertex3f(x+xo+(wid+sep)*xmult+wid,y+yo+(le+sep)*ymult+le,0);
						glVertex3f(x+xo+(wid+sep)*xmult,y+yo+(le+sep)*ymult+le,0);
					}

					//print stats
					//generation count
					sprintf(buf2, "%i", agent.gencount);
					RenderString(x-rp*1.414, y-rp*1.414, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 1.0f, 1.0f);

					//age
					sprintf(buf2, "%.1f", (float)agent.age/10);
					float red= world->HEALTHLOSS_AGING==0 ? 0 : cap((float) agent.age/world->MAXAGE);
					//will be redder the closer it is to MAXAGE if health loss by aging is enabled
					RenderString(x-rp*1.414, y-rp*1.414-12, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 1.0-red, 1.0-red);

					//species id
					sprintf(buf2, "%i", agent.species);
					RenderString(x-rp*1.414, y-rp*1.414-24, GLUT_BITMAP_HELVETICA_12, buf2, speciescolor.red, speciescolor.gre, speciescolor.blu);
					
					//exhaustion readout
					sprintf(buf2, "%.2f", agent.exhaustion);
					RenderString(x-rp*1.414, y-rp*1.414-36, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 1.0f, 1.0f);
				}
				glEnd();
			}
		}
	}
}


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
	graphcolor= setColorLungs(0.8);
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

	if(live_layersvis!=0){
		glBegin(GL_QUADS);
		glColor4f(0,0,0.1,1);
		glVertex3f(0,0,0);
		glVertex3f(conf::WIDTH,0,0);
		glVertex3f(conf::WIDTH,conf::HEIGHT,0);
		glVertex3f(0,conf::HEIGHT,0);
		glEnd();
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

	int ww= glutGet(GLUT_WINDOW_WIDTH);
	int wh= glutGet(GLUT_WINDOW_HEIGHT);
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
	if(live_paused) RenderString(10, 20, GLUT_BITMAP_HELVETICA_12, "Paused", 0.5f, 0.5f, 0.5f);
	if(live_follow!=0) {
		if(world->getSelectedAgent()>=0) RenderString(10, 40, GLUT_BITMAP_HELVETICA_12, "Following", 0.5f, 0.5f, 0.5f);
		else RenderString(10, 40, GLUT_BITMAP_HELVETICA_12, "No Follow Target", 1.0f, 0.5f, 0.5f);
	}
	if(live_selection==Select::RELATIVE) RenderString(10, 60, GLUT_BITMAP_HELVETICA_12, "Relative Autoselect Mode", 0.5f, 0.8f, 0.5f);
	if(world->isClosed()) RenderString(10, 80, GLUT_BITMAP_HELVETICA_12, "Closed World", 0.5f, 0.5f, 0.5f);
	if(world->isDebug()) {
		sprintf(buf, "Plant-Haz Supp: %i agents", (int)(world->getFoodSupp()-world->getHazardSupp()));
		RenderString(5, 140, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "Fruit-Haz Supp: %i agents", (int)(world->getFruitSupp()-world->getHazardSupp()));
		RenderString(5, 160, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "Meat-Haz Supp: %i agents", (int)(world->getMeatSupp()-world->getHazardSupp()));
		RenderString(5, 180, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "-Haz 'Supp': %i agents", (int)(-world->getHazardSupp()));
		RenderString(5, 200, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "modcounter: %i", world->modcounter);
		RenderString(5, 240, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "GL modcounter: %i", modcounter);
		RenderString(5, 260, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "GL scalemult: %.4f", scalemult);
		RenderString(5, 280, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
		sprintf(buf, "%% Land: %.3f", world->getLandRatio());
		RenderString(5, 320, GLUT_BITMAP_HELVETICA_12, buf, 0.5f, 0.5f, 1.0f);
	}

	//center axis markers
	glBegin(GL_LINES);
	glColor4f(0,1,0,0.3); //green y-axis
	glVertex3f(ww/2,wh/2,0);
	glVertex3f(ww/2,wh/2+15,0);

	glColor4f(1,0,0,0.3); //red x-axis
	glVertex3f(ww/2,wh/2,0);
	glVertex3f(ww/2+15,wh/2,0);
	glEnd();

	int ux, uy, ul, uw, ub; //sub-ui element x, y, length, width, barrier
	ul= 88;
	uw= 9;
	ub= 6;

	//event display y coord, based on Selected Agent Hud to avoid overlap
	int euy= 10+2*ub+((int)ceil((float)Hud::HUDS/3))*(uw+ub);

	//event log display
	float ss= 18;
	float movezero= 0;
	int fadetime= conf::EVENTS_HALFLIFE-20;

	int count= world->events.size();
	if(conf::EVENTS_DISP<count) count= conf::EVENTS_DISP;

	for(int eo= 0; eo<count; eo++){
		int counter= world->events[eo].second.second;

		float fade= cap((-abs((float)counter)+conf::EVENTS_HALFLIFE)/20);

		float move= 0;


		if(counter>fadetime) move= powf(((float)counter-fadetime)/7.4,3);
		else if (counter<-fadetime) move= powf(((float)counter+fadetime)/7.4,3);

		if(eo==0){
			movezero= move;
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
			case 10:	red= 1/(1+world->modcounter%32); gre= 1/(1+world->modcounter%21); blu= 1/(1+world->modcounter%14); break; //type 10: all the colors!
			case 11:	red= 1.0; gre= 0.5; blu= 0.5;	break; //type 11: pink
			case 12:	red= 0.9; gre= 0.6;				break; //type 12: orange
		}

		glBegin(GL_QUADS);
		glColor4f(red,gre,blu,0.5*fade);
		glVertex3f(ww-202, euy+5+2.5*ss+eo*ss+movezero+move-0.5*ss,0);
		glVertex3f(ww-2, euy+5+2.5*ss+eo*ss+movezero+move-0.5*ss,0);
		glColor4f(red*0.75,gre*0.75,blu*0.75,0.5*fade);
		glVertex3f(ww-2, euy+5+2.5*ss+eo*ss+movezero+move+0.5*ss,0);
		glVertex3f(ww-202, euy+5+2.5*ss+eo*ss+movezero+move+0.5*ss,0);
		glEnd();

		RenderString(ww-200+1, euy+10+2.5*ss+eo*ss+movezero+move+1, GLUT_BITMAP_HELVETICA_12, world->events[eo].first, 0.0f, 0.0f, 0.0f, fade);
		RenderString(ww-200, euy+10+2.5*ss+eo*ss+movezero+move, GLUT_BITMAP_HELVETICA_12, world->events[eo].first, 1.0f, 1.0f, 1.0f, fade);
	}


	//selected agent overlay
	if(world->getSelectedAgent()>=0){
		//get agent
		Agent selected= world->agents[world->getSelectedAgent()];
		//slightly transparent background with a black stylized border
		glBegin(GL_POLYGON);
		glColor4f(0,0,0,0);
		glVertex3f(ww-440,10,0);
		glColor4f(0,0,0,1);
		glVertex3f(ww-2,2,0);
		glColor4f(0,0,0,0);
		glVertex3f(ww-10,euy,0);

		
		glEnd();

		glBegin(GL_POLYGON);
		glColor4f(0,0.4,0.6,0.45);
		glVertex3f(ww-440,10,0);
		glColor4f(0,0.4,0.6,1.0);
		glVertex3f(ww-440/2,10,0);
		glVertex3f(ww-10,10,0);
		glVertex3f(ww-10,euy,0);
		glColor4f(0,0.4,0.6,0.55);
		glVertex3f(ww-440/2,euy,0);
		glColor4f(0,0.4,0.6,0.25);
		glVertex3f(ww-440,euy,0);
		
		glEnd();

		//draw Ghost Agent
		drawAgent(selected, ww-370, 10+euy/2, true);

		//Agent ID
		glBegin(GL_QUADS);
		glColor4f(0,0,0,0.7);
		glVertex3f(ww-380+4+20+3*ceil(1.0+log((float)selected.id)),25+4,0);
		glVertex3f(ww-380-4,25+4,0);
		Color3f specie= setColorSpecies(selected.species);
		glColor4f(specie.red,specie.gre,specie.blu,1);
		glVertex3f(ww-380-4,25-4-uw,0);
		glVertex3f(ww-380+4+20+3*ceil(1.0+log((float)selected.id)),25-4-uw,0);
		glEnd();

		sprintf(buf, "ID: %d", selected.id);
		RenderString(ww-380,25, GLUT_BITMAP_HELVETICA_12,buf, 0.8f, 1.0f, 1.0f);

		for(int u=0; u<Hud::HUDS; u++) {
			ux= ww-10-(ub+ul)*(3-(int)(u%3));
			uy= 20+ub+((int)(u/3))*(uw+ub);
			//(ux,uy)= lower left corner of element
			//must + ul to ux, must - uw to uy, for other corners

			//Draw graphs
			if(u==Hud::HEALTH || u==Hud::REPCOUNTER || u==Hud::STOMACH || u==Hud::EXHAUSTION){
				//all graphs get a trasparent black box put behind them first
				glBegin(GL_QUADS);
				glColor4f(0,0,0,0.7);
				glVertex3f(ux-1,uy+1,0);
				glVertex3f(ux-1,uy-1-uw,0);
				glVertex3f(ux+1+ul,uy-1-uw,0);
				glVertex3f(ux+1+ul,uy+1,0);

				if(u==Hud::STOMACH){ //stomach indicators, ux= ww-100, uy= 40
					glColor3f(0,0.6,0);
					glVertex3f(ux,uy-uw,0);
					glVertex3f(ux,uy-(int)(uw*2/3),0);
					glVertex3f(selected.stomach[Stomach::PLANT]*ul+ux,uy-(int)(uw*2/3),0);
					glVertex3f(selected.stomach[Stomach::PLANT]*ul+ux,uy-uw,0);

					glColor3f(0.6,0.6,0);
					glVertex3f(ux,uy-(int)(uw*2/3),0);
					glVertex3f(ux,uy-(int)(uw/3),0);
					glVertex3f(selected.stomach[Stomach::FRUIT]*ul+ux,uy-(int)(uw/3),0);
					glVertex3f(selected.stomach[Stomach::FRUIT]*ul+ux,uy-(int)(uw*2/3),0);

					glColor3f(0.6,0,0);
					glVertex3f(ux,uy-(int)(uw/3),0);
					glVertex3f(ux,uy,0);
					glVertex3f(selected.stomach[Stomach::MEAT]*ul+ux,uy,0);
					glVertex3f(selected.stomach[Stomach::MEAT]*ul+ux,uy-(int)(uw/3),0);
				} else if (u==Hud::REPCOUNTER){ //repcounter indicator, ux=ww-200, uy=25
					glColor3f(0,0.7,0.7);
					glVertex3f(ux,uy,0);
					glVertex3f(ux,uy-uw,0);
					glVertex3f(cap(selected.repcounter/selected.maxrepcounter)*-ul+ux+ul,uy-uw,0);
					glVertex3f(cap(selected.repcounter/selected.maxrepcounter)*-ul+ux+ul,uy,0);
				} else if (u==Hud::HEALTH){ //health indicator, ux=ww-300, uy=25
					glColor3f(0,0.8,0);
					glVertex3f(ux,uy,0);
					glVertex3f(ux,uy-uw,0);
					glVertex3f(selected.health/2.0*ul+ux,uy-uw,0);
					glVertex3f(selected.health/2.0*ul+ux,uy,0);
				} else if (u==Hud::EXHAUSTION){ //Exhaustion/energy indicator ux=ww-100, uy=25
					glColor3f(0.8,0.8,0);
					glVertex3f(ux,uy,0);
					glVertex3f(ux,uy-uw,0);
					glColor3f(0.4,0.4,0);
					glVertex3f((2/(1+exp(-world->EXHAUSTION_MULT*selected.exhaustion))-1)*ul+ux,uy-uw,0);
					glVertex3f((2/(1+exp(-world->EXHAUSTION_MULT*selected.exhaustion))-1)*ul+ux,uy,0);
				}
				//end draw graphs
				glEnd();
			}

			//write text and values
			if(u==Hud::HEALTH){
				sprintf(buf, "Health: %.2f/2", selected.health);

			} else if(u==Hud::REPCOUNTER){
				sprintf(buf, "Child: %.2f/%.0f", selected.repcounter, selected.maxrepcounter);
			
			} else if(u==Hud::EXHAUSTION){
				if(world->EXHAUSTION_MULT>0 && selected.exhaustion>(5*world->EXHAUSTION_MULT)) sprintf(buf, "Exhausted!");
				else if (world->EXHAUSTION_MULT>0 && selected.exhaustion<(0.5*world->EXHAUSTION_MULT)) sprintf(buf, "Energetic!");
				else sprintf(buf, "Tired.");

			} else if(u==Hud::STOMACH){
				if(selected.isHerbivore()) sprintf(buf, "\"Herbivore\"");
				else if(selected.isFrugivore()) sprintf(buf, "\"Frugivore\"");
				else if(selected.isCarnivore()) sprintf(buf, "\"Carnivore\"");
				else sprintf(buf, "\"Dead\"...");

			} else if(u==Hud::AGE){
				sprintf(buf, "Age: %.1f", (float)selected.age/10);

			} else if(u==Hud::GENERATION){
				sprintf(buf, "Gen: %d", selected.gencount);

			} else if(u==Hud::TEMPPREF){
				if(selected.temperature_preference<0.3) sprintf(buf, "Tropical(%.3f)", selected.temperature_preference);
				else if (selected.temperature_preference>0.7) sprintf(buf, "Arctic(%.3f)", selected.temperature_preference);
				else sprintf(buf, "Temperate(%.2f)", selected.temperature_preference);

			} else if(u==Hud::LUNGS){
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
			
			} else if(u==Hud::METBOLISM){
				sprintf(buf, "Metab: %.2f", selected.metabolism);

			} else if(u==Hud::HYBRID){
				if(selected.hybrid) sprintf(buf, "Is Hybrid");
				else if(selected.gencount==0) sprintf(buf, "Was Spawned");
				else sprintf(buf, "Was Budded");

			} else if(u==Hud::GIVING){
				if(selected.give>0.5) sprintf(buf, "Generous");
				else if(selected.give>conf::MAXSELFISH) sprintf(buf, "Autocentric");
				else sprintf(buf, "Selfish");

			} else if(u==Hud::SPIKE){
				if(selected.isSpikey(world->SPIKELENGTH)){
					float mw= selected.w1>selected.w2 ? selected.w1 : selected.w2;
					if(mw<0) mw= 0;
					float val= world->DAMAGE_FULLSPIKE*selected.spikeLength*mw;
					if(val>2) sprintf(buf, "Spike: Deadly!");//health
					else sprintf(buf, "Spike: %.2f h", val);
				} else sprintf(buf, "Not Spikey");

			} else if(u==Hud::STRENGTH){
				if(selected.strength>0.7) sprintf(buf, "Strong!");
				else if(selected.strength>0.3) sprintf(buf, "Not Weak");
				else sprintf(buf, "Weak!");

			} else if(u==Hud::NUMBABIES){
				sprintf(buf, "Num Babies: %d", selected.numbabies);

			} else if(u==Hud::SEXPROJECT){
				if(selected.sexproject>1.0) sprintf(buf, "Sexting (M)");
				else if(!selected.isAsexual()) sprintf(buf, "Sexting (F)");
				else sprintf(buf, "Not Sexting");

			} else if(u==Hud::SPECIESID){
				sprintf(buf, "Gene: %d", selected.species);

			} else if(u==Hud::MOTION){
				if(selected.jump>0) sprintf(buf, "Airborne!");
				else if(selected.encumbered) sprintf(buf, "Encumbered");
				else if(selected.boost) sprintf(buf, "Sprinting");
				else if(abs(selected.w1)>0.03 || abs(selected.w2)>0.03) sprintf(buf, "Walking");
				else sprintf(buf, "Idle");

			} else if(u==Hud::GRAB){
				if(selected.grabbing>0.5){
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
				if(selected.volume>0.8) sprintf(buf, "Loud! (%.3f)", selected.volume);
				else if(selected.volume<=0.001) sprintf(buf, "Silent.");
				else if(selected.volume<0.2) sprintf(buf, "Quiet (%.3f)", selected.volume);
				else sprintf(buf, "Volume: %.3f", selected.volume);

			//Stats
			} else if(u==Hud::STAT_CHILDREN){
				sprintf(buf, "Children: %d", selected.children);

			} else if(u==Hud::STAT_KILLED){
				sprintf(buf, "Killed: %d", selected.killed);

			} else if(u==Hud::DEATH && selected.health==0){
				sprintf(buf, selected.death);

			} else sprintf(buf, "");

			//Render text
			RenderString(ux,uy, GLUT_BITMAP_HELVETICA_12,buf, 0.8f, 1.0f, 1.0f);
		}
	}

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
			RenderString(popupxy[0]+5,popupxy[1]+14*(l+1), GLUT_BITMAP_HELVETICA_12,popuptext[l].c_str(), 0.8f, 1.0f, 1.0f);
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


void GLView::drawCell(int x, int y, const float values[Layer::LAYERS])
{
	if (live_layersvis!=0) { //0: white
		Color3f cellcolor(1.0, 1.0, 1.0);
		glBegin(GL_QUADS);
		if (live_layersvis==Display::PLANTS) {
			cellcolor= setColorPlant(values[Layer::PLANTS]);
		} else if (live_layersvis==Display::MEATS) {
			cellcolor= setColorMeat(values[Layer::MEATS]);
		} else if (live_layersvis==Display::HAZARDS) {
			cellcolor= setColorHazard(values[Layer::HAZARDS]);
		} else if (live_layersvis==Display::FRUITS) {
			cellcolor= setColorFruit(values[Layer::FRUITS]);
		} else if (live_layersvis==Display::REALITY) {
			cellcolor= setColorCellsAll(values);//setColorLandWater(val);
		} else if (live_layersvis==Display::LIGHT) {
			cellcolor= setColorLight(values[Layer::LIGHT]);
		} else if (live_layersvis==Display::TEMP) {
			cellcolor= setColorTemp((float)y); //special for temp: until cell-based, convert y coord and process
		} else if (live_layersvis==Display::ELEVATION) {
			cellcolor= setColorHeight(values[Layer::ELEVATION]);
		}
		glColor4f(cellcolor.red, cellcolor.gre, cellcolor.blu, 1);
		//code below makes cells into divided boxes when zoomed close up
		float gadjust= 0;
		if(scalemult>0.8 || live_grid) gadjust= scalemult<=0 ? 0 : 0.5/scalemult;
		glVertex3f(x*conf::CZ+gadjust,y*conf::CZ+gadjust,0);
		glVertex3f(x*conf::CZ+conf::CZ-gadjust,y*conf::CZ+gadjust,0);
		glVertex3f(x*conf::CZ+conf::CZ-gadjust,y*conf::CZ+conf::CZ-gadjust,0);
		glVertex3f(x*conf::CZ+gadjust,y*conf::CZ+conf::CZ-gadjust,0);

		//if Land/All draw mode, draw fruit and meat as little squares
		if (live_layersvis==Display::REALITY) {
			if(values[Layer::MEATS]>0){
				float meat= values[Layer::MEATS];
				cellcolor= setColorMeat(1.0); //meat on this layer is always bright red, but translucence is applied
				glColor4f(cellcolor.red*values[Layer::LIGHT], cellcolor.gre*values[Layer::LIGHT], cellcolor.blu*values[Layer::LIGHT], meat);
				meat= 0.3*meat+0.5; //reuse the meat value for sizing

				glVertex3f(x*conf::CZ+meat*conf::CZ,y*conf::CZ+meat*conf::CZ,0);
				glVertex3f(x*conf::CZ+conf::CZ-meat*conf::CZ,y*conf::CZ+meat*conf::CZ,0);
				glVertex3f(x*conf::CZ+conf::CZ-meat*conf::CZ,y*conf::CZ+conf::CZ-meat*conf::CZ,0);
				glVertex3f(x*conf::CZ+meat*conf::CZ,y*conf::CZ+conf::CZ-meat*conf::CZ,0);
			}
			if(values[Layer::FRUITS]>0.1 && values[Layer::LIGHT]>0 && scalemult > 0.1){
				for(int i= 1; i<values[Layer::FRUITS]*10; i++){
					cellcolor= Color3f(0.8,0.8,0.2);
					//pseudo random number output system:
					//inputs: x, y of cell, the index of the fruit in range 0-10
					//method: take initial x and y into vector, and then rotate it again by golden ratio and increase distance by index
					float GR= 89.0/55;
					Vector2f point(sinf((float)y),cosf((float)x));
					point.rotate(2*M_PI*GR*i);
					point.normalize();
					
//					float adjustx= i%2==0 ? 10-i%3-i/2+i%4 : i%3+i*i/5;
//					float adjusty= i%3==1 ? 3+i%2+i : (i+1)%2+i*i/6
					glColor4f(cellcolor.red*values[Layer::LIGHT], cellcolor.gre*values[Layer::LIGHT], cellcolor.blu*values[Layer::LIGHT], 1);
					glVertex3f(x*conf::CZ+conf::CZ/2+point.x*i,y*conf::CZ+conf::CZ/2+point.y*i+1,0);
					glVertex3f(x*conf::CZ+conf::CZ/2+point.x*i+1,y*conf::CZ+conf::CZ/2+point.y*i,0);
					glVertex3f(x*conf::CZ+conf::CZ/2+point.x*i,y*conf::CZ+conf::CZ/2+point.y*i-1,0);
					glVertex3f(x*conf::CZ+conf::CZ/2+point.x*i-1,y*conf::CZ+conf::CZ/2+point.y*i,0);
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
