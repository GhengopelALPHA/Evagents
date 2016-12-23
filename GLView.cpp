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

}

GLView::~GLView()
{

}
void GLView::changeSize(int w, int h)
{
	//Tell GLUT that were changing the projection, not the actual view
	glMatrixMode(GL_PROJECTION);
	//Reset the coordinate system
	glLoadIdentity();
	//resize viewport to new size
	glViewport(0, 0, w, h);
	//reconcile projection (required to keep everything that's visible, visible
	glOrtho(0,w,h,0,0,1);
	//revert to normal view opperations
	glMatrixMode(GL_MODELVIEW);
}

void GLView::processMouse(int button, int state, int x, int y)
{
	if(world->isDebug()) printf("MOUSE EVENT: button=%i state=%i x=%i y=%i, scale=%f, mousedrag=%i\n", button, state, x, y, scalemult, mousedrag);
	
	if(!mousedrag && state==1){ //dont let mouse click do anything if drag flag is raised
		//have world deal with it. First translate to world coordinates though
		int wx= (int) ((x-glutGet(GLUT_WINDOW_WIDTH)/2)/scalemult-xtranslate);
		int wy= (int) ((y-glutGet(GLUT_WINDOW_HEIGHT)/2)/scalemult-ytranslate);

		if(world->processMouse(button, state, wx, wy, scalemult) && live_selection!=Select::RELATIVE) live_selection = Select::MANUAL;
	}

	mousex=x; mousey=y;
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
		//right mouse button.
	}*/
	if(abs(mousex-x)>4 || abs(mousey-y)>4) mousedrag= true; //mouse was clearly dragged, don't select agents after
	mousex=x; mousey=y;
}

void GLView::processMousePassiveMotion(int x, int y)
{
	//for mouse scrolling. [DISABLED]
/*	if(y<=30) ytranslate += 2*(30-y);
	if(y>=glutGet(GLUT_WINDOW_HEIGHT)-30) ytranslate -= 2*(y-(glutGet(GLUT_WINDOW_HEIGHT)-30));
	if(x<=30) xtranslate += 2*(30-x);
	if(x>=glutGet(GLUT_WINDOW_WIDTH)-30) xtranslate -= 2*(x-(glutGet(GLUT_WINDOW_WIDTH)-30));*/
}

void GLView::processNormalKeys(unsigned char key, int x, int y)
{
	menu(key);	
}

void GLView::processSpecialKeys(int key, int x, int y)
{
	menuS(key);	
}

void GLView::processReleasedKeys(unsigned char key, int x, int y)
{
	if (key==32) {//spacebar input [released]
			world->pinput1= 0;
	}
}

void GLView::menu(int key)
{
	ReadWrite* savehelper= new ReadWrite(); //for loading/saving
	if (key == 27) //[esc] quit
		exit(0);
	else if (key=='p') {
		//pause
		live_paused= !live_paused;
	} else if (key=='m') { //drawing
		live_fastmode= !live_fastmode;
	} else if (key==43) { //+
		live_skipdraw++;
	} else if (key==45) { //-
		live_skipdraw--;
	} else if (key=='l' || key=='k') { //layer switch; l= "next", k= "previous"
		if (key=='l') live_layersvis++;
		else live_layersvis--;
		if (live_layersvis>Layer::LAYERS) live_layersvis= 0;
		if (live_layersvis<0) live_layersvis= Layer::LAYERS;
	} else if (key=='z' || key=='x') { //change agent visual scheme; x= "next", z= "previous"
		if (key=='x') live_agentsvis++;
		else live_agentsvis--;
		if (live_agentsvis>Visual::VISUALS-1) live_agentsvis= Visual::NONE;
		if (live_agentsvis<Visual::NONE) live_agentsvis= Visual::VISUALS-1;
	} else if (key==1001) { //add agents
		world->addAgents(5);
	} else if (key==1002) { //add Herbivore agents
		world->addAgents(5, Stomach::PLANT);
	} else if (key==1003) { //add Carnivore agents
		world->addAgents(5, Stomach::MEAT);
	} else if (key==1004) { //add Frugivore agents
		world->addAgents(5, Stomach::FRUIT);
	} else if (key=='c') {
		world->setClosed( !world->isClosed() );
		live_worldclosed= (int) world->isClosed();
/*		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->isClosed()) glutChangeToMenuEntry(4, "Open World", 'c');
		else glutChangeToMenuEntry(4, "Close World", 'c');
		glutSetMenu(m_id);*/
	} else if (key=='f') {
		live_follow= !live_follow; //toggle follow selected agent
	} else if(key=='o') {
		if(live_selection!=Select::OLDEST) live_selection= Select::OLDEST; //select oldest agent
		else live_selection= Select::NONE;
	} else if(key=='q') {
		//zoom and translocate to instantly see the whole world
		float scaleA= (float)glutGet(GLUT_WINDOW_WIDTH)/(conf::WIDTH+2200);
		float scaleB= (float)glutGet(GLUT_WINDOW_HEIGHT)/(conf::HEIGHT+150);
		if(scaleA>scaleB) scalemult= scaleB;
		else scalemult= scaleA;
		xtranslate= -(conf::WIDTH-2020)/2;
		ytranslate= -(conf::HEIGHT-80)/2;
		live_follow= 0;
	} else if(key=='g') {
		if(live_selection!=Select::BEST_GEN) live_selection= Select::BEST_GEN; //select most advanced generation agent
		else live_selection= Select::NONE;
	} else if(key=='h') {
		if(live_selection!=Select::HEALTHY) live_selection= Select::HEALTHY; //select healthiest
		else live_selection= Select::NONE;
	} else if(key=='R') {
		world->setSelectedAgent(randi(0,world->agents.size())); //select random agent, among all
		live_selection= Select::MANUAL;
	} else if(key=='r') {
		while(true){ //select random agent, among alive
			int idx= randi(0,world->agents.size());
			if (world->agents[idx].health>0.1) {
				world->setSelectedAgent(idx);
				live_selection= Select::MANUAL;
				break;
			}
		}
	}else if (key==127) { //delete
		world->selectedKill();
	}else if (key==62) { //zoom+ >
		scalemult += 10*conf::ZOOM_SPEED;
	}else if (key==60) { //zoom- <
		scalemult -= 10*conf::ZOOM_SPEED;
		if(scalemult<0.01) scalemult=0.01;
	}else if (key==32) { //spacebar input [pressed]
		world->pinput1= 1;
	}else if (key=='/') { // / heal selected
		world->selectedHeal();
	}else if (key=='|') { // | reproduce selected
		world->selectedBabys();
	}else if (key=='~') { // ~ mutate selected
		world->selectedMutate();
	}else if (key==119) { //w (move faster)
		world->pcontrol= true;
		world->pleft= capm(world->pleft + 0.08, -1, 1);
		world->pright= capm(world->pright + 0.08, -1, 1);
	}else if (key==97) { //a (turn left)
		world->pcontrol= true;
		world->pleft= capm(world->pleft - 0.05 + (world->pright-world->pleft)*0.05, -1, 1); //this extra code helps with turning out of tight circles
		world->pright= capm(world->pright + 0.05 + (world->pleft-world->pright)*0.05, -1, 1);
	}else if (key==115) { //s (move slower)
		world->pcontrol= true;
		world->pleft= capm(world->pleft - 0.08, -1, 1);
		world->pright= capm(world->pright - 0.08, -1, 1);
	}else if (key==100) { //d (turn right)
		world->pcontrol= true;
		world->pleft= capm(world->pleft + 0.05 + (world->pright-world->pleft)*0.05, -1, 1);
		world->pright= capm(world->pright - 0.05 + (world->pleft-world->pright)*0.05, -1, 1);
	} else if (key==999) { //player control
		world->setControl(!world->pcontrol);
		glutGet(GLUT_MENU_NUM_ITEMS);
		if (world->pcontrol) glutChangeToMenuEntry(1, "Release Agent", 999);
		else glutChangeToMenuEntry(1, "Control Selected (w,a,s,d)", 999);
		glutSetMenu(m_id);
	}else if (key==1005) { //menu only, debug mode
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
	} else if (world->isDebug()) {
		printf("Unknown key pressed: %i\n", key);
	}
	//other keys: '1':49, '2':50, ..., '0':48
}

void GLView::menuS(int key) // movement control
{
	if (key == GLUT_KEY_UP) {
	   ytranslate += 20/scalemult;
	} else if (key == GLUT_KEY_LEFT) {
		xtranslate += 20/scalemult;
	} else if (key == GLUT_KEY_DOWN) {
		ytranslate -= 20/scalemult;
	} else if (key == GLUT_KEY_RIGHT) {
		xtranslate -= 20/scalemult;
	}
}

void GLView::glCreateMenu()
{
	m_id = glutCreateMenu(gl_menu); //right-click context menu
	glutAddMenuEntry("Control Selected (w,a,s,d)", 999); //line contains mode-specific text, see menu function above
	glutAddMenuEntry("Heal Agent (/)", '/');
	glutAddMenuEntry("Reproduce Agent (|)", '|');
	glutAddMenuEntry("Mutate Agent (~)", '~');
	glutAddMenuEntry("Delete Agent (del)", 127);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Spawn Agents", 1001);
	glutAddMenuEntry("Spawn Herbivores", 1002);
	glutAddMenuEntry("Spawn Carnivores", 1003);
	glutAddMenuEntry("Spawn Frugivores", 1004);
	glutAddMenuEntry("Toggle Closed (c)", 'c');
	glutAddMenuEntry("Save World",1008);
	glutAddMenuEntry("-------------------",-1);
	glutAddMenuEntry("Load World",1009);
	glutAddMenuEntry("Enter Debug Mode", 1005);
	glutAddMenuEntry("Reset Agents", 1007);
	glutAddMenuEntry("Rewrite Config", 1006);
	glutAddMenuEntry("Reload Config", 1010);
	glutAddMenuEntry("Exit (esc)", 27);
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
	live_layersvis= Layer::LAND+1; //+1 because "off" is not a real layer
	live_profilevis= Profile::INOUT;
	live_selection= Select::MANUAL;
	live_follow= 0;
	live_autosave= 0;
	live_debug= 0;
	live_grid= 0;

	//create GLUI and add the options, be sure to connect them all to their real vals later
	Menu = GLUI_Master.create_glui("Menu",0,1,1);
	Menu->add_checkbox("Closed world",&live_worldclosed);
	Menu->add_checkbox("Pause",&live_paused);
	Menu->add_checkbox("Allow Autosaves",&live_autosave);

	new GLUI_Button(Menu,"Load World",1, glui_handleRW); //change 1->5 for advanced loading window (needs work)
	new GLUI_Button(Menu,"Save World",2, glui_handleRW);
	new GLUI_Button(Menu,"New World",3, glui_handleRW);

	GLUI_Panel *panel_speed= new GLUI_Panel(Menu,"Speed Control");
	Menu->add_checkbox_to_panel(panel_speed,"Fast Mode",&live_fastmode);
	Menu->add_spinner_to_panel(panel_speed,"Speed",GLUI_SPINNER_INT,&live_skipdraw);

	GLUI_Rollout *rollout_vis= new GLUI_Rollout(Menu,"Visuals");

	GLUI_RadioGroup *group_layers= new GLUI_RadioGroup(rollout_vis,&live_layersvis);
	new GLUI_StaticText(rollout_vis,"Layer");
	new GLUI_RadioButton(group_layers,"off");
	for(int i=0; i<Layer::LAYERS; i++){
		//this code allows the layers to be defined in any order
		char text[16]= "";
		if(i==Layer::PLANTS) strcpy(text, "Plant");
		else if(i==Layer::MEATS) strcpy(text, "Meat");
		else if(i==Layer::HAZARDS) strcpy(text, "Hazard");
		else if(i==Layer::FRUITS) strcpy(text, "Fruit");
		else if(i==Layer::LAND) strcpy(text, "Map");
		else if(i==Layer::LIGHT) strcpy(text, "Light");
		else if(i==Layer::TEMP) strcpy(text, "Temp");

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
		else if(i==Visual::LUNGS) strcpy(text, "Land/Water");


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

void GLView::handleRW(int action) //glui callback for saving/loading worlds
{
	live_paused= 1;

	//action= 1: loading, action= 2: saving, action= 3: new (reset) world
	if (action==1){ //basic load option selected
		Loader = GLUI_Master.create_glui("Load World",0,50,50);

		Filename= new GLUI_EditText(Loader,"File Name (e.g, 'WORLD.SCB'):");
		Filename->set_w(300);
		new GLUI_Button(Loader,"Load",1, glui_handleCloses);

		Loader->set_main_gfx_window(win1);

	} else if (action==2){ //basic save option
		Saver = GLUI_Master.create_glui("Save World",0,50,50);

		Filename= new GLUI_EditText(Saver,"File Name (e.g, 'WORLD.SCB'):");
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
	} else if (action==4){ //save selected agent
		Saver = GLUI_Master.create_glui("Save Agent",0,50,50);

		Filename= new GLUI_EditText(Saver,"File Name (e.g, 'AGENT.BOT'):");
		Filename->set_w(300);
		new GLUI_Button(Saver,"Save",6, glui_handleCloses);
	} else if (action==5){ //advanced loader
		Loader = GLUI_Master.create_glui("Load World",0,50,50);
		Browser= new GLUI_FileBrowser(Loader,"",false,8, glui_handleCloses);
	}
}

void GLView::handleCloses(int action) //GLUI callback for handling window closing
{
	live_paused= 0;

	ReadWrite* savehelper= new ReadWrite(); //for loading/saving

	if (action==1){ //loading
		strcpy(filename,Filename->get_text());
		if (debug) printf("File: '\\saves\\%s'\n",filename);
		if (!filename || filename=="" || filename==NULL || filename[0]=='\0'){
			printf("ERROR: empty filename; returning to program.\n");

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert,"No file name given.");
			new GLUI_StaticText(Alert,"Returning to main program.");
			new GLUI_Button(Alert,"Okay",4, glui_handleCloses);

		} else {
			savehelper->loadWorld(world, xtranslate, ytranslate, filename);
		}
		Loader->hide();

	} else if (action==2){ //saving
		trySaveWorld();
		Saver->hide();

	} else if (action==3){ //resetting
		world->reset();
		world->spawn();
		printf("WORLD RESET!\n");
		Alert->hide();
	} else if (action==4){ //Alert cancel/continue
		Alert->hide();
	} else if (action==5){ //Alert from above saving
		savehelper->saveWorld(world, xtranslate, ytranslate, filename);
		Alert->hide();
	} else if (action==6){ //saving agents
		const char *tempname= Filename->get_text();
		strcpy(filename, tempname);
		if (debug) printf("File: '\\saved_agents\\%s'",filename);
		if (!filename || filename=="" || filename==NULL || filename[0]=='\0'){
			printf("ERROR: empty filename; returning to program.\n");

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert, "No file name given." );
			new GLUI_StaticText(Alert, "Returning to main program." );
			new GLUI_Button(Alert,"Okay");

		} else {
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
	} else if (action==8){ //advanced loader
		file_name= "";
		file_name= Browser->get_file();
		strcpy(filename,file_name.c_str());

		if (debug) printf("File: '\\saves\\%s'\n",filename);
		if (!filename || filename=="" || filename==NULL || filename[0]=='\0'){
			printf("ERROR: empty filename; returning to program.\n");

			Alert = GLUI_Master.create_glui("Alert",0,50,50);
			Alert->show();
			new GLUI_StaticText(Alert,"No file name given.");
			new GLUI_StaticText(Alert,"Returning to main program.");
			new GLUI_Button(Alert,"Okay",4, glui_handleCloses);

		} else {
			savehelper->loadWorld(world, xtranslate, ytranslate, filename);
		}
		Loader->hide();
	}
}

void GLView::trySaveWorld(bool autosave)
{
	const char *tempname;
	if(autosave) tempname= "AUTOSAVE.SCB";
	else tempname= Filename->get_text();
	strcpy(filename, tempname);
	if (debug) printf("File: '\\saves\\%s'\n",filename);
	if (!filename || filename=="" || filename==NULL || filename[0]=='\0'){
		printf("ERROR: empty filename; returning to program.\n");

		Alert = GLUI_Master.create_glui("Alert",0,50,50);
		Alert->show();
		new GLUI_StaticText(Alert, "No file name given." );
		new GLUI_StaticText(Alert, "Returning to main program." );
		new GLUI_Button(Alert,"Okay");

	} else {
		char address[32];
		strcpy(address,"saves\\");
		strcat(address,tempname);

		if(autosave){ //autosave overwrites
			ReadWrite* savehelper= new ReadWrite();
			savehelper->saveWorld(world, xtranslate, ytranslate, filename);
		} else {
			//check the filename given to see if it exists yet
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
				ReadWrite* savehelper= new ReadWrite();
				savehelper->saveWorld(world, xtranslate, ytranslate, filename);
			}
		}
	}
}

void GLView::countdownEvents()
{
	//itterate event counters when drawing, once per frame regardless of skipdraw
	int count= world->events.size();
	if(conf::EVENTS_DISP<count) count= conf::EVENTS_DISP;
	for(int io= 0; io<count; io++){
		world->events[io].second--;
	}
}

void GLView::handleIdle()
{
	//set proper window (we don't want to draw on nothing, now do we?!)
	if (glutGetWindow() != win1) glutSetWindow(win1); 

	GLUI_Master.sync_live_all();

	//after syncing all the live vars with GLUI_Master, set the vars they represent to their proper values.
	world->setClosed(live_worldclosed);
	world->setDebug((bool) live_debug);

	modcounter++;
	if (!live_paused) world->update();

	//autosave world periodically, based on world time
	if (live_autosave==1 && world->modcounter%(world->FRAMES_PER_EPOCH)==0){
		trySaveWorld(true);
		world->addEvent("Auto-saved World!");
	}

	//show FPS and other stuff
	int currentTime = glutGet( GLUT_ELAPSED_TIME );
	frames++;
	if ((currentTime - lastUpdate) >= 1000) {
		sprintf( buf, "FPS: %d speed: %d Alive Agents: %d Herbivores: %d Carnivores: %d Frugivores: %d Dead: %d Epoch: %d",
			frames, live_skipdraw, world->getAgents()-world->getDead(), world->getHerbivores(), world->getCarnivores(), world->getFrugivores(), world->getDead(), world->epoch() );
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
			float mult=-0.005*(live_skipdraw-1); //ugly, ah well
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();

	glTranslatef(glutGet(GLUT_WINDOW_WIDTH)/2, glutGet(GLUT_WINDOW_HEIGHT)/2, 0.0f);	
	glScalef(scalemult, scalemult, 1.0f);

	//handle world agent selection interface
	world->setSelection(live_selection);
	if (world->getSelection()==-1 && live_selection!=Select::MANUAL && modcounter%5==0) live_selection= Select::NONE;

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

	glTranslatef(xtranslate, ytranslate, 0.0f);
	
	world->draw(this, live_layersvis);

	glPopMatrix();
	glutSwapBuffers();

	countdownEvents();
}

void GLView::drawAgent(const Agent& agent, float x, float y, bool ghost)
{
	float n;
	float r= agent.radius;
	float rp= agent.radius+2.5;

	if (live_agentsvis!=Visual::NONE) {

		//color assignment
		float red= 0,gre= 0,blu= 0;
		float discomfort= 0;

		//first, calculate colors which also have indicator boxes
		discomfort= cap(sqrt(abs(2.0*abs(agent.pos.y/conf::HEIGHT - 0.5) - agent.temperature_preference)));
		if (discomfort<0.005) discomfort= 0;

		float stomach_red= cap(pow(agent.stomach[Stomach::MEAT],2)+pow(agent.stomach[Stomach::FRUIT],2)-pow(agent.stomach[Stomach::PLANT],2));
		float stomach_gre= cap(pow(agent.stomach[Stomach::PLANT],2)/2+pow(agent.stomach[Stomach::FRUIT],2)-pow(agent.stomach[Stomach::MEAT],2));

		float species_red= (cos((float)agent.species/97*M_PI)+1.0)/2.0;
		float species_gre= (sin((float)agent.species/47*M_PI)+1.0)/2.0;
		float species_blu= (cos((float)agent.species/21*M_PI)+1.0)/2.0;

		float metab_red= agent.metabolism;
		float metab_gre= 0.6*agent.metabolism;
		float metab_blu= 0.2+0.4*abs(agent.metabolism*2-1);

		float lung_red= 0.2+2*agent.lungs*(1-agent.lungs);
		float lung_gre= 0.4*agent.lungs+0.3;
		float lung_blu= 0.6*(1-agent.lungs)+0.2;
		//land (0.2,0.7,0.2), amph (0.2,0.5,0.5), water (0.2,0.3,0.8)


		//now colorize agents and other things
		if (live_agentsvis==Visual::RGB){ //real rgb values
			red= agent.red; gre= agent.gre; blu= agent.blu;

		} else if (live_agentsvis==Visual::STOMACH){ //stomach
			red= stomach_red;
			gre= stomach_gre;
		
		} else if (live_agentsvis==Visual::DISCOMFORT){ //temp discomfort
			red= discomfort; gre= (2-discomfort)/2; blu= 1-discomfort;

		} else if (live_agentsvis==Visual::VOLUME) { //volume
			red= agent.volume;
			gre= agent.volume;
			blu= agent.volume;

		} else if (live_agentsvis==Visual::SPECIES){ //species 
			red= species_red;
			gre= species_gre;
			blu= species_blu;
		
		} else if (live_agentsvis==Visual::CROSSABLE){ //crossover-compatable to selection
			//all other agents look grey if unrelated or if none is selected, b/c then we don't have a reference
			red= 0.8;
			gre= 0.8;
			blu= 0.8;

			if(world->getSelectedAgent()>=0){
				float deviation= abs(agent.species - world->agents[world->getSelectedAgent()].species); //species deviation check
				if (deviation==0) { //exact copies
					red= 0.2;
				} else if (deviation<=world->MAXDEVIATION) {
					//reproducable relatives
					red= 0;
					gre= 0;
				} else if (deviation<=3*world->MAXDEVIATION) {
					//un-reproducable relatives
					red= 0.6;
					gre= 0.4;
				}
			}

		} else if (live_agentsvis==Visual::HEALTH) { //health
			gre= agent.health<=0.1 ? agent.health : powf(agent.health/2,0.5);
			red= ((int)(agent.health*1000)%2==0) ? (1.0-agent.health/2)*gre : (1.0-agent.health/2);

		} else if (live_agentsvis==Visual::METABOLISM){
			red= metab_red;
			gre= metab_gre;
			blu= metab_blu;
		
		} else if (live_agentsvis==Visual::LUNGS){
			red= lung_red;
			gre= lung_gre;
			blu= lung_blu;
		}


		//handle selected agent
		if (agent.id==world->getSelection() && !ghost) {
			//draw selection
			glLineWidth(2);
			glBegin(GL_LINES);
			glColor3f(1,1,1);
			for (int k=0;k<17;k++)
			{
				n = k*(M_PI/8);
				glVertex3f(x+(agent.radius+4/scalemult)*sin(n),y+(agent.radius+4/scalemult)*cos(n),0);
				n = (k+1)*(M_PI/8);
				glVertex3f(x+(agent.radius+4/scalemult)*sin(n),y+(agent.radius+4/scalemult)*cos(n),0);
			}
			glEnd();
			glLineWidth(1);

			if(scalemult > .2){
				glPushMatrix();
				glTranslatef(x-90,y+23,0);

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
						if(j==Input::TEMP) glColor3f(col,(2-col)/2,(1-col));
						glColor3f(col,col,col);
						glVertex3f(0+ss*j, 0, 0.0f);
						glVertex3f(xx+ss*j, 0, 0.0f);
						glVertex3f(xx+ss*j, yy, 0.0f);
						glVertex3f(0+ss*j, yy, 0.0f);
						if(scalemult > .7){
							glEnd();
							if(j==Input::CLOCK1 || j==Input::CLOCK2 || j==Input::CLOCK3){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "Q", 0.0f, 0.0f, 0.0f);
							} else if(j==Input::TEMP){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "T", 0.8f, 0.5f, 0.0f);
							} else if(j==Input::HEARING1 || j==Input::HEARING2){
								RenderString(xx/3+ss*j, yy*2/3, GLUT_BITMAP_HELVETICA_12, "E", 1.0f, 1.0f, 1.0f);
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
		//				else if (j==Output::GRAB) glColor3f(0,col,col);
						else if (j==Output::TONE) glColor3f((1-col)*(1-col),1-fabs(col-0.5)*2,col*col);
						else glColor3f(col,col,col);
						glVertex3f(0+ss*j, yy, 0.0f);
						glVertex3f(xx+ss*j, yy, 0.0f);
						glVertex3f(xx+ss*j, yy+ss, 0.0f);
						glVertex3f(0+ss*j, yy+ss, 0.0f);
						if(scalemult > .7){
							glEnd();
							if(j==Output::LEFT_WHEEL_B || j==Output::LEFT_WHEEL_F || j==Output::RIGHT_WHEEL_B || j==Output::RIGHT_WHEEL_F){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "!", 0.0f, 1.0f, 0.0f);
							} else if(j==Output::VOLUME){
								RenderString(xx/3+ss*j, yy+ss*2/3, GLUT_BITMAP_HELVETICA_12, "V", 1.0f, 1.0f, 1.0f);
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
					glVertex3f(0, 20, 0);
					glVertex3f(90, 20, 0);
					glVertex3f(90, 0, 0);
					glEnd();

					for(int q=0;q<NUMEYES;q++){
						glBegin(GL_POLYGON);
						glColor3f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3]);
						drawCircle(15+agent.eyedir[q]/2/M_PI*60,10,agent.eyefov[q]/M_PI*10);
						glEnd();
					}

					glBegin(GL_LINES);
					glColor3f(0.5,0.5,0.5);
					glVertex3f(0, 20, 0);
					glVertex3f(90, 20, 0);
					glVertex3f(90, 0, 0);
					glVertex3f(0, 0, 0);

					glColor3f(0.7,0,0);
					glVertex3f(0, 0, 0);
					glVertex3f(0, 20, 0);
					glVertex3f(90, 20, 0);
					glVertex3f(90, 0, 0);
					
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
					glColor4f((1-agent.tone)*(1-agent.tone), 1-fabs(agent.tone-0.5)*2, agent.tone*agent.tone,0.5);
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
					for (int k=0;k<17;k++)
					{
						n = k*(M_PI/8);
						glVertex3f(x+world->DIST*sin(n),y+world->DIST*cos(n),0);
						n = (k+1)*(M_PI/8);
						glVertex3f(x+world->DIST*sin(n),y+world->DIST*cos(n),0);
					}
					glEnd();

					//now spike, share, and grab effective zones
					glBegin(GL_POLYGON);
					glColor4f(1,0,0,0.35);
					glVertex3f(x,y,0);
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

		//draw indicator of this agent... used for various events
		if (agent.indicator>0) {
			glBegin(GL_POLYGON);
			glColor4f(agent.ir,agent.ig,agent.ib,0.75);
			drawCircle(x, y, r+((int)agent.indicator));
			glEnd();
		}

		//draw giving/receiving
		if(agent.dfood!=0){
			glBegin(GL_POLYGON);
			float mag=cap(abs(agent.dfood)/world->FOODTRANSFER)*2/3;
			if(agent.dfood>0) glColor3f(0,mag,0);
			else glColor3f(mag,0,0); //draw sharing as a thick green or red outline
			for (int k=0;k<17;k++){
				n = k*(M_PI/8);
				glVertex3f(x+rp*sin(n),y+rp*cos(n),0);
				n = (k+1)*(M_PI/8);
				glVertex3f(x+rp*sin(n),y+rp*cos(n),0);
			}
			glEnd();
		}
		
		float tra= agent.health==0 ? 0.4 : 1; //mult for dead agents, displayed with more transparent parts

		if (scalemult > .1 || ghost) { //dont render eyes, ears, or boost if zoomed too far out, but always render them on ghosts
			//draw eyes
			for(int q=0;q<NUMEYES;q++) {
				glBegin(GL_LINES);
				if (live_profilevis==Profile::EYES) {
					//color eyes based on actual input if we're on the eyesight profile
					glColor4f(agent.in[Input::EYES+q*3],agent.in[Input::EYES+1+q*3],agent.in[Input::EYES+2+q*3],0.75*tra);
				} else glColor4f(0.5,0.5,0.5,0.75*tra);
				glVertex3f(x,y,0);
				float aa= agent.angle+agent.eyedir[q];
				glVertex3f(x+(r+30)*cos(aa), y+(r+30)*sin(aa), 0);
				glEnd();
			}

			//ears
			for(int q=0;q<NUMEARS;q++) {
				glBegin(GL_POLYGON);
				//color ears differently if we are set on the sound profile
				if(live_profilevis==Profile::SOUND) glColor4f(1-(q/(NUMEARS-1)),q/(NUMEARS-1),0,0.75);
				else glColor4f(0.6,0.6,0,0.5);
				float aa= agent.angle + agent.eardir[q];
				drawCircle(x+r*cos(aa), y+r*sin(aa), 2.3);
				glEnd();
			}

			//boost blur
			if (agent.boost){
				for(int q=1;q<4;q++){
					Vector2f displace(r/4*q*(agent.w1+agent.w2), 0);
					displace.rotate(agent.angle+M_PI);

					glBegin(GL_POLYGON); 
					glColor4f(red,gre,blu,0.25);
					drawCircle(x+displace.x, y+displace.y, r);
					glEnd();
				}
			}

			//vis grabbing
			if((scalemult > .3 || ghost) && agent.grabbing>0.5){
				glLineWidth(2);
				glBegin(GL_LINES);
				
				glColor4f(0.0,0.7,0.7,0.75);
				glVertex3f(x,y,0);
				float mult= agent.grabID==-1 ? 1 : 0;
				float aa= agent.angle+M_PI/8*mult;
				float ab= agent.angle-M_PI/8*mult;
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
		glColor4f(red,gre,blu,tra);
		drawCircle(x, y, r);
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
			glVertex3f(x+r*cos(agent.angle),y+r*sin(agent.angle),0);
			glVertex3f(x+(10+r)*cos(agent.angle+M_PI/8*mult), y+(10+r)*sin(agent.angle+M_PI/8*mult), 0);
			glVertex3f(x+r*cos(agent.angle),y+r*sin(agent.angle),0);
			glVertex3f(x+(10+r)*cos(agent.angle-M_PI/8*mult), y+(10+r)*sin(agent.angle-M_PI/8*mult), 0);
		}

		//outline
		float out_red= 0,out_gre= 0,out_blu= 0;
		if (agent.jump>0) { //draw jumping as yellow outline
			out_red= 0.8;
			out_gre= 0.8;
		}
		if (agent.health<=0){ //draw dead as grey outline
			glColor4f(0.7,0.7,0.7,tra);
		} else glColor3f(cap(out_red*blur + (1-blur)*red), cap(out_gre*blur + (1-blur)*gre), cap(out_blu*blur + (1-blur)*blu));
		for (int k=0;k<17;k++)
		{
			n = k*(M_PI/8);
			glVertex3f(x+r*sin(n),y+r*cos(n),0);
			n = (k+1)*(M_PI/8);
			glVertex3f(x+r*sin(n),y+r*cos(n),0);
		}

		//sound waves!
		if(live_agentsvis==Visual::VOLUME && !ghost && agent.volume>0){
			float volume= agent.volume;
			float count= agent.tone*11+1;
			for (int l=0; l<=(int)count; l++){
				float dist= world->DIST*(l/count)+4*(world->modcounter%(int)((world->DIST)/4));
				if (dist>world->DIST) dist-= world->DIST;
				glColor4f((1-agent.tone)*(1-agent.tone), 1-fabs(agent.tone-0.5)*2, agent.tone*agent.tone, cap((1-dist/world->DIST)*volume));

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
			glColor3f(0,1,0);
			glVertex3f(x+agent.radius/2*cos(wheelangle),y+agent.radius/2*sin(wheelangle),0);
			glVertex3f(x+agent.radius/2*cos(wheelangle)+20*agent.w1*cos(agent.angle), y+agent.radius/2*sin(wheelangle)+20*agent.w1*sin(agent.angle), 0);
			wheelangle-= M_PI;
			glVertex3f(x+agent.radius/2*cos(wheelangle),y+agent.radius/2*sin(wheelangle),0);
			glVertex3f(x+agent.radius/2*cos(wheelangle)+20*agent.w2*cos(agent.angle), y+agent.radius/2*sin(wheelangle)+20*agent.w2*sin(agent.angle), 0);
			glEnd();

			glBegin(GL_POLYGON); 
			glColor3f(0,1,0);
			drawCircle(x+agent.radius/2*cos(wheelangle), y+agent.radius/2*sin(wheelangle), 1);
			glEnd();
			wheelangle+= M_PI;
			glBegin(GL_POLYGON); 
			drawCircle(x+agent.radius/2*cos(wheelangle), y+agent.radius/2*sin(wheelangle), 1);
			glEnd();
		}

		if(!ghost){ //only draw extra infos if not a ghost

			if(scalemult > .3) {//hide extra visual data if really far away

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
				}

				//tags: quick HUD of basic bot traits/stats
				int xo=8+agent.radius;
				int yo=-21;
				int sep= 2;
				int le= 9;
				int wid= 5;
				int numtags= 8;

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
				glVertex3f(x+xo,y+yo+42*cap(agent.repcounter/world->REPRATE),0);
				glVertex3f(x+xo+5,y+yo+42*cap(agent.repcounter/world->REPRATE),0);
				glVertex3f(x+xo+5,y+yo+42,0);
				glVertex3f(x+xo,y+yo+42,0);

				//indicator tags
				xo+= 7+sep;
				for(int i=0;i<numtags;i++){
					int xmult= (int)floor((float)(i/4));
					int ymult= i%4;

					//different tag color schemes go here
					if (i==0){
						if(agent.hybrid) glColor3f(0,0,0.8); //hybrid?
						else if(agent.sexproject) glColor4f(0,0.8,0.8,0.5);
						else continue;
					}
					else if (i==1) glColor3f(stomach_red,stomach_gre,0); //stomach type
					else if (i==2) glColor3f(agent.volume,agent.volume,agent.volume); //sound volume emitted
					else if (i==3) glColor3f(discomfort,(2-discomfort)/2,(1-discomfort)); //temp discomfort
					else if (i==4) glColor3f(lung_red,lung_gre,lung_blu); //land/water lungs requirement
					else if (i==7) glColor3f(metab_red,metab_gre,metab_blu); //metabolism
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
				glEnd();
			}

			//print stats
			if(scalemult > .5) { // hide the number stats when zoomed out
				//generation count
				sprintf(buf2, "%i", agent.gencount);
				RenderString(x-rp*1.414, y-rp*1.414, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 1.0f, 1.0f);

				//age
				sprintf(buf2, "%i", agent.age);
				float red= world->HEALTHLOSS_AGING==0 ? 0 : cap((float) agent.age/world->MAXAGE);
				//will be redder the closer it is to MAXAGE if health loss by aging is enabled
				RenderString(x-rp*1.414, y-rp*1.414-12, GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 1.0-red, 1.0-red);

				//species id
				sprintf(buf2, "%i", agent.species);
				RenderString(x-rp*1.414, y-rp*1.414-24, GLUT_BITMAP_HELVETICA_12, buf2, species_red, species_gre, species_blu);
			}
		}
	}
}


void GLView::drawData()
{
	float mm = 3;
	//draw misc info
	glBegin(GL_LINES);
	glColor3f(0,0,0); //border around graphs and feedback

	glVertex3f(0,0,0);
	glVertex3f(-2020,0,0);

	glVertex3f(-2020,0,0);
	glVertex3f(-2020,conf::HEIGHT,0);

	glVertex3f(-2020,conf::HEIGHT,0);
	glVertex3f(0,conf::HEIGHT,0);

	glColor3f(0,0,0.8); //hybrid count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10, conf::HEIGHT - mm*world->numHybrid[q],0);
		glVertex3f(-2020 +(q+1)*10, conf::HEIGHT - mm*world->numHybrid[q+1],0);
	}
	glColor3f(0,1,0); //herbivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*world->numHerbivore[q],0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*world->numHerbivore[q+1],0);
	}
	glColor3f(1,0,0); //carnivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*world->numCarnivore[q],0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*world->numCarnivore[q+1],0);
	}
	glColor3f(0.7,0.7,0); //frugivore count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*world->numFrugivore[q],0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*world->numFrugivore[q+1],0);
	}
	glColor3f(0,0,0); //total count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*world->numTotal[q],0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*world->numTotal[q+1],0);
	}
	glColor3f(0.8,0.8,0.6); //dead count
	for(int q=0;q<conf::RECORD_SIZE-1;q++) {
		if(q==world->ptr-1) continue;
		glVertex3f(-2020 + q*10,conf::HEIGHT -mm*(world->numDead[q]+world->numTotal[q]),0);
		glVertex3f(-2020 +(q+1)*10,conf::HEIGHT -mm*(world->numDead[q+1]+world->numTotal[q+1]),0);
	}

	//current population vertical bars
	glVertex3f(-2020 + world->ptr*10,conf::HEIGHT,0);
	glVertex3f(-2020 + world->ptr*10,conf::HEIGHT -mm*world->getAgents(),0);
	glColor3f(0,0,0);
	glVertex3f(-2020 + world->ptr*10,conf::HEIGHT,0);
	glVertex3f(-2020 + world->ptr*10,conf::HEIGHT -mm*world->getAlive(),0);
	glEnd();

	//labels for current population bars
	sprintf(buf2, "%i dead", world->getDead());
	RenderString(-2016 + world->ptr*10,conf::HEIGHT -mm*world->getAgents(),
		GLUT_BITMAP_HELVETICA_12, buf2, 0.8f, 0.8f, 0.6f);
	sprintf(buf2, "%i agents", world->getAlive());
	RenderString(-2016 + world->ptr*10,conf::HEIGHT -mm*world->getAlive(),
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

	//event log display
	float ss= 18;
	float movezero= 0;
	int fadetime= conf::EVENTS_HALFLIFE-20;

	int count= world->events.size();
	if(conf::EVENTS_DISP<count) count= conf::EVENTS_DISP;

	for(int io= 0; io<count; io++){
		int counter= world->events[io].second;

		float fade= cap((-abs((float)counter)+conf::EVENTS_HALFLIFE)/20);

		float move= 0;


		if(counter>fadetime) move= powf(((float)counter-fadetime)/7.4,3);
		else if (counter<-fadetime) move= powf(((float)counter+fadetime)/7.4,3);

		if(io==0){
			movezero= move;
			//move= 0;
		}
		glBegin(GL_QUADS);
		glColor4f(0.8,0.8,0.8,0.5*fade);
		glVertex3f(ww-202, 145+2.5*ss+io*ss+movezero+move-0.5*ss,0);
		glVertex3f(ww-2, 145+2.5*ss+io*ss+movezero+move-0.5*ss,0);
		glVertex3f(ww-2, 145+2.5*ss+io*ss+movezero+move+0.5*ss,0);
		glVertex3f(ww-202, 145+2.5*ss+io*ss+movezero+move+0.5*ss,0);
		glEnd();

		RenderString(ww-200, 150+2.5*ss+io*ss+movezero+move, GLUT_BITMAP_HELVETICA_12, world->events[io].first, 0.0f, 0.0f, 0.0f, fade);
	}


	//selected agent overlay
	if(world->getSelectedAgent()>=0){
		//get agent
		Agent selected= world->agents[world->getSelectedAgent()];
		//slightly transparent background
		glBegin(GL_QUADS);
		glColor4f(0,0.4,0.5,0.55);
		glVertex3f(ww-10,10,0);
		glVertex3f(ww-10,150,0);
		glVertex3f(ww-400,150,0);
		glVertex3f(ww-400,10,0);

		//stomach indicators
		glColor4f(0,0,0,0.7);
		glVertex3f(ww-101,15,0);
		glVertex3f(ww-101,26,0);
		glVertex3f(ww-16,26,0);
		glVertex3f(ww-16,15,0);

		glColor3f(0,0.6,0);
		glVertex3f(ww-100,16,0);
		glVertex3f(ww-100,19,0);
		glVertex3f(selected.stomach[Stomach::PLANT]*83+(ww-100),19,0);
		glVertex3f(selected.stomach[Stomach::PLANT]*83+(ww-100),16,0);

		glColor3f(0.6,0.6,0);
		glVertex3f(ww-100,19,0);
		glVertex3f(ww-100,22,0);
		glVertex3f(selected.stomach[Stomach::FRUIT]*83+(ww-100),22,0);
		glVertex3f(selected.stomach[Stomach::FRUIT]*83+(ww-100),19,0);

		glColor3f(0.6,0,0);
		glVertex3f(ww-100,22,0);
		glVertex3f(ww-100,25,0);
		glVertex3f(selected.stomach[Stomach::MEAT]*83+(ww-100),25,0);
		glVertex3f(selected.stomach[Stomach::MEAT]*83+(ww-100),22,0);

		//repcounter indicator
		glColor4f(0,0,0,0.7);
		glVertex3f(ww-201,15,0);
		glVertex3f(ww-201,26,0);
		glVertex3f(ww-106,26,0);
		glVertex3f(ww-106,15,0);
		glColor3f(0,0.7,0.7);
		glVertex3f(ww-200,16,0);
		glVertex3f(ww-200,25,0);
		glVertex3f(cap(selected.repcounter/world->REPRATE)*-93+(ww-107),25,0);
		glVertex3f(cap(selected.repcounter/world->REPRATE)*-93+(ww-107),16,0);

		//health indicator
		glColor4f(0,0,0,0.7);
		glVertex3f(ww-301,15,0);
		glVertex3f(ww-301,26,0);
		glVertex3f(ww-206,26,0);
		glVertex3f(ww-206,15,0);
		glColor3f(0,0.8,0);
		glVertex3f(ww-300,16,0);
		glVertex3f(ww-300,25,0);
		glVertex3f(selected.health/2.0*93+(ww-300),25,0);
		glVertex3f(selected.health/2.0*93+(ww-300),16,0);
		glEnd();

		//draw Ghost Agent
		drawAgent(selected, ww-350, 80, true);

		//write text and values
		//Agent ID
		sprintf(buf, "ID: %d", selected.id);
		RenderString(ww-380,
					 25, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Health
		sprintf(buf, "Health: %.2f/2", selected.health);
		RenderString(ww-300,
					 25, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Repcounter
		sprintf(buf, "Child: %.2f/%.0f", selected.repcounter, world->REPRATE);
		RenderString(ww-200,
					 25, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stomach
//		sprintf(buf, "H%.1f F%.1f C%.1f", selected.stomach[Stomach::PLANT], selected.stomach[Stomach::FRUIT], 
//			selected.stomach[Stomach::MEAT]);
//		RenderString(ww-100,
//					 25, GLUT_BITMAP_HELVETICA_12,
//					 buf, 0.8f, 1.0f, 1.0f);

		if(selected.isHerbivore()) sprintf(buf, "\"Herbivore\"");
		else if(selected.isFrugivore()) sprintf(buf, "\"Frugivore\"");
		else if(selected.isCarnivore()) sprintf(buf, "\"Carnivore\"");
		else sprintf(buf, "\"Dead\"...");
		RenderString(ww-100,
					 40, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//age
		sprintf(buf, "Age: %d", selected.age);
		RenderString(ww-300,
					 40, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Generation
		sprintf(buf, "Gen: %d", selected.gencount);
		RenderString(ww-200,
					 40, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Temperature Preference
		if(selected.temperature_preference<0.3) sprintf(buf, "Heat-loving(%.3f)", selected.temperature_preference);
		else if (selected.temperature_preference>0.7) sprintf(buf, "Cold-loving(%.3f)", selected.temperature_preference);
		else sprintf(buf, "Temperate(%.3f)", selected.temperature_preference);
		RenderString(ww-300,
					 55, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Lung-type
		if(selected.lungs<0.3) sprintf(buf, "Aquatic(%.3f)", selected.lungs);
		else if (selected.lungs>0.7) sprintf(buf, "Terrestrial(%.3f)", selected.lungs);
		else sprintf(buf, "Amphibious(%.3f)", selected.lungs);
		RenderString(ww-200,
					 55, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Radius
		sprintf(buf, "Radius: %.2f", selected.radius);
		RenderString(ww-100,
					 55, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);
					 

		//Mutrates
		sprintf(buf, "Mutrate1: %.3f", selected.MUTRATE1);
		RenderString(ww-300,
					 70, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		sprintf(buf, "Mutrate2: %.3f", selected.MUTRATE2);
		RenderString(ww-200,
					 70, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Metabolism
		sprintf(buf, "Metab: %.2f", selected.metabolism);
		RenderString(ww-100,
					 70, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Hybrid
		if(selected.hybrid) sprintf(buf, "Hybrid");
		else sprintf(buf, "Budded");
		RenderString(ww-300,
					 85, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Giving
		if(selected.give>0.5) sprintf(buf, "Generous");
		else if(selected.give>0.01) sprintf(buf, "Autocentric");
		else sprintf(buf, "Selfish");
		RenderString(ww-200,
					 85, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Spike
		if(selected.isSpikey(world->SPIKELENGTH)){
			float mw= selected.w1>selected.w2 ? selected.w1 : selected.w2;
			if(mw<0) mw= 0;
			float val= world->SPIKEMULT*selected.spikeLength*mw;
			sprintf(buf, "Spike: %.2f h", val);
		} else sprintf(buf, "Not Spikey");
		RenderString(ww-100,
					 85, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Children
		sprintf(buf, "Children: %d", selected.children);
		RenderString(ww-300,
					 100, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Trait: Strength
		if(selected.strength>0.7) sprintf(buf, "Strong!");
		else if(selected.strength>0.3) sprintf(buf, "Not Weak");
		else sprintf(buf, "Weak!");
		RenderString(ww-200,
					 100, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Killed
		sprintf(buf, "Killed: %d", selected.killed);
		RenderString(ww-100,
					 100, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Trait: Num Babies
		sprintf(buf, "Num Babies: %d", selected.numbabies);
		RenderString(ww-300,
					 115, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Output: Sexual projection
		if(selected.sexproject) sprintf(buf, "Sexting");
		else sprintf(buf, "Not Sexting");
		RenderString(ww-200,
					 115, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Species ID (Genome)
		sprintf(buf, "Gene: %d", selected.species);
		RenderString(ww-100,
					 115, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Jumping status
		if(selected.jump<=0) sprintf(buf, "Grounded");
		else sprintf(buf, "Airborne!");
		RenderString(ww-300,
					 130, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Grab status
		if(selected.grabbing>0.5){
			if(selected.grabID==-1) sprintf(buf, "Seeking");
			else sprintf(buf, "Hold: %d", selected.grabID);
		} else sprintf(buf, "Isolated");
		RenderString(ww-200,
					 130, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

		//Stats: Biting
		if(selected.jawrend!=0) sprintf(buf, "Bite: %.2f h", abs(selected.jawPosition*world->HEALTHLOSS_JAWSNAP));
		else sprintf(buf, "Not Biting");
		RenderString(ww-100,
					 130, GLUT_BITMAP_HELVETICA_12,
					 buf, 0.8f, 1.0f, 1.0f);

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


void GLView::drawCell(int x, int y, float quantity)
{
	if (live_layersvis!=0) { //0: white
		glBegin(GL_QUADS);
		if (live_layersvis==Layer::PLANTS+1) { //plant food: green w/ navy blue background
			glColor4f(0.0,quantity,0.1,1);
		} else if (live_layersvis==Layer::MEATS+1) { //meat food: red/burgundy w/ navy blue bg
			glColor4f(quantity,0.0,0.1,1);
		} else if (live_layersvis==Layer::HAZARDS+1) { //hazards: purple/magenta w/ navy blue bg
			glColor4f(quantity,0.0,quantity*0.9+0.1,1);
		} else if (live_layersvis==Layer::FRUITS+1) { //fruit: yellow w/ navy blue bg
			glColor4f(quantity*0.8,quantity*0.8,0.1,1);
		} else if (live_layersvis==Layer::LAND+1) { //land: green if 1, blue if 0, navy blue otherwise (debug)
			if (quantity==1) glColor4f(0.3,0.7,0.3,1);
			else if (quantity==0) glColor4f(0.3,0.3,0.9,1);
			else glColor4f(0,0,0.1,1);
		} else if (live_layersvis==Layer::LIGHT+1) { //light: bright white/yellow w/ navy blue bg
			//unless moonlight is a thing, then its a grey bg
			if(world->MOONLIT) glColor4f(0.5*quantity+0.5,0.5*quantity+0.5,quantity*0.3+0.5,1);
			else glColor4f(quantity,quantity,quantity*0.7+0.1,1);
		} else if (live_layersvis==Layer::TEMP+1) { //temp: yellow near the equator, blue at the poles
			float col= cap(2*abs((float)y*conf::CZ/conf::HEIGHT - 0.5)-0.02);
			glColor3f(1-col,1-col/2,col);
		}

		//code to make cells into divided boxes when zoomed close up
		float gadjust= 0;
		if(scalemult>0.8 || live_grid) gadjust= scalemult<=0 ? 0 : 0.5/scalemult;
		glVertex3f(x*conf::CZ+gadjust,y*conf::CZ+gadjust,0);
		glVertex3f(x*conf::CZ+conf::CZ-gadjust,y*conf::CZ+gadjust,0);
		glVertex3f(x*conf::CZ+conf::CZ-gadjust,y*conf::CZ+conf::CZ-gadjust,0);
		glVertex3f(x*conf::CZ+gadjust,y*conf::CZ+conf::CZ-gadjust,0);
		glEnd();
	}
}

void GLView::setWorld(World* w)
{
	world = w;
}
