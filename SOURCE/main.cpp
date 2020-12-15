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
#include <irrKlang.h>


GLView* GLVIEW = new GLView(0);

int main(int argc, char **argv) {
	unsigned int seed= time(0);

	printf("Evagents ");
	#if defined(_DEBUG)
	printf("DEBUG ");
	srand(0);
	#else
	printf("Release ");
	srand(seed);
	#endif

	printf( "v%3.2f\n\n", conf::VERSION );
	printf( "OpenGL+GLUI, version: %3.2f\n", GLUI_Master.get_version() );
	printf( "irrKlang Audio, version: %s\n", IRR_KLANG_VERSION );
	printf( "\n" );

	printf("Music Credits via FreeSound.org: Andrewkn, ERH, Erokia, Evanjones4, Kjartan-abel, Newagesoup, and psovod.\n");
	printf( "\n" );

	//WORLD SETUP
	if (conf::WIDTH%conf::CZ!=0 || conf::HEIGHT%conf::CZ!=0) 
		printf("CAREFUL! The cell size variable conf::CZ should divide evenly into both conf::WIDTH and conf::HEIGHT! It doesn't right now!\n");
	//printf( "Seed: %d\n", seed );

	World* world = new World();
	GLVIEW->setWorld(world);
	world->setSeed(seed);


	//AUDIO SETUP
	// start the sound engine with default parameters
	irrklang::ISoundEngine* audioengine = irrklang::createIrrKlangDevice(ESOD_AUTO_DETECT, ESEO_MULTI_THREADED); //only using Multithreaded option; more are available
	if (!audioengine) return 0; // error starting up the engine
	world->setAudioEngine(audioengine);


	//GLUT SETUP
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE | GLUT_RGBA);
	glutInitWindowPosition(250,20);
	glutInitWindowSize(conf::WWIDTH,conf::WHEIGHT);

	GLVIEW->win1= glutCreateWindow("Evagents");

	//Anti-aliasing
//	glEnable(GL_POINT_SMOOTH);
	glEnable(GL_LINE_SMOOTH);
	glEnable(GL_POLYGON_SMOOTH);
	glPolygonMode(GL_FRONT, GL_FILL);

	//Alpha channel stuff
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

	//GLUI handles
	GLUI_Master.set_glutDisplayFunc(gl_renderScene);
	GLUI_Master.set_glutIdleFunc(gl_handleIdle);
	GLUI_Master.set_glutReshapeFunc(gl_changeSize);

	//Normal GL callbacks for interfaces
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


	//CLOSING THE PROGRAM
	GLUI_Master.close_all();
	audioengine->drop(); // delete audio engine
	SetThreadExecutionState(ES_CONTINUOUS); //return sleep state by clearing previous flags
	return 0;
}
