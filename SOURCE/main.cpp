#include "GLView.h"
#include "World.h"

#include <ctime>

#include "config.h"

#ifdef LOCAL_GLUT32
	#include "glut.h"
#else
	#include <GL/glut.h>
#endif

#include "glui.h"

#ifdef WIN32
#include <windows.h>
#endif

#include <stdio.h>


GLView* GLVIEW = new GLView(0);
int main(int argc, char **argv) {
	srand(time(0));

	printf("Evagents v%3.2f\n\n", conf::VERSION);
	if (conf::WIDTH%conf::CZ!=0 || conf::HEIGHT%conf::CZ!=0) printf("CAREFUL! The cell size variable conf::CZ should divide evenly into both conf::WIDTH and conf::HEIGHT! It doesn't right now!\n");
	
	printf( "GLUI version: %3.2f\n", GLUI_Master.get_version() );
	World* world = new World();
	GLVIEW->setWorld(world);

	//GLUT SETUP
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(250,20);
	glutInitWindowSize(conf::WWIDTH,conf::WHEIGHT);

	GLVIEW->win1= glutCreateWindow("Evagents");

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	GLUI_Master.set_glutDisplayFunc(gl_renderScene);
	GLUI_Master.set_glutIdleFunc(gl_handleIdle);
	GLUI_Master.set_glutReshapeFunc(gl_changeSize);

	glutKeyboardFunc(gl_processNormalKeys);
	glutSpecialFunc(gl_processSpecialKeys);
	glutKeyboardUpFunc(gl_processReleasedKeys);
	glutMouseFunc(gl_processMouse);
	glutMotionFunc(gl_processMouseActiveMotion);
	glutPassiveMotionFunc(gl_processMousePassiveMotion);

	//menu window.
	GLVIEW->gluiCreateMenu();

	//create right click context menu
	GLVIEW->glCreateMenu();

	if (CreateDirectory("saves", NULL)) printf("\"saves\" directory did not exist. Does now!\n");
	if (CreateDirectory("saved_agents", NULL)) printf("\"saved_agents\" directory did not exist. Does now!\n");

	if (SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED) == NULL){
		//these lines ensure windows doesn't hibernate or sleep if you use those features regularly
		SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
	}

	try{
		glutMainLoop();
	} catch( std::bad_alloc &){
		printf("Out of memory!\n");
	} catch( std::bad_exception &){
		printf("Severe error!\n");
	}

	SetThreadExecutionState(ES_CONTINUOUS); //return sleep state
	return 0;
}
