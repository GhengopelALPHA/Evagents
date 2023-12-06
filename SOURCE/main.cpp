#include "GLView.h"
#include "World.h"
#include "config.h"
#include "glui.h"

#include <ctime>
#include <iostream>
#include <irrKlang.h>

#ifdef LOCAL_GLUT32
    #include "glut.h"
#else
    #include <GL/glut.h>
#endif

#ifdef WIN32
    #include <windows.h>
#endif

GLView* GLVIEW = new GLView(0);
unsigned int seed = time(0);

void printVersionInfo() {
    std::cout << "Evagents ";

    # if defined(_DEBUG)
        std::cout << "DEBUG ";
		seed = 0;
    # else
        std::cout << "Release ";
    # endif

    std::cout << "v" << conf::VERSION << "\n\n";
    std::cout << "OpenGL+GLUI, version: " << GLUI_Master.get_version() << "\n";
    std::cout << "irrKlang Audio, version: " << IRR_KLANG_VERSION << "\n\n";
	std::cout << "Music Credits via FreeSound.org: Andrewkn, ERH, Erokia, Evanjones4, Kjartan-abel, Newagesoup, and psovod.\n\n";
}

int main(int argc, char** argv) {
    printVersionInfo();

    // WORLD SETUP
	srand(seed);

	if (conf::WIDTH % conf::CZ != 0 || conf::HEIGHT % conf::CZ != 0) {
        std::cout << "CAREFUL! The cell size variable conf::CZ should divide evenly into both conf::WIDTH and conf::HEIGHT! Hit Enter to continue anyway.\n";
        getchar();
	}

    World* world = new World();
    GLVIEW->setWorld(world);
    world->setSeed(time(0));

    // AUDIO SETUP
    irrklang::ISoundEngine* audioengine = irrklang::createIrrKlangDevice(irrklang::ESOD_AUTO_DETECT, irrklang::ESEO_MULTI_THREADED);
    if (!audioengine) {
        std::cerr << "Error: init audio failed, Hit Enter to exit\n";
        getchar();
        return 0;
    }
    world->setAudioEngine(audioengine);

    // GLUT SETUP
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_MULTISAMPLE | GLUT_RGBA);
    glutInitWindowPosition(260, 20);
    glutInitWindowSize(conf::WWIDTH, conf::WHEIGHT);

    GLVIEW->win1 = glutCreateWindow("Evagents");

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
    glPolygonMode(GL_FRONT, GL_FILL);

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

    GLVIEW->gluiCreateMenu();
    GLVIEW->glCreateMenu();

    char zCurDir[257];
    GetCurrentDirectory(256, zCurDir);
    if (CreateDirectory("saves", NULL)) std::cout << "\"saves\" directory did not exist. Does now at " << zCurDir << "!\n";
    if (CreateDirectory("saved_agents", NULL)) std::cout << "\"saved_agents\" directory did not exist. Does now at " << zCurDir << "!\n";

    if (SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_AWAYMODE_REQUIRED) == NULL) {
        SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
    }

    try {
        glutMainLoop();
    } catch (const std::exception& e) {
		std::cout << "Exception caught during glutMainLoop(): " << e.what() << std::endl;
		std::cout << "Press Enter to exit..." << std::endl;
		getchar();
	} catch (...) {
		std::cout << "An unspecified error occurred during glutMainLoop()" << std::endl;
		std::cout << "Press Enter to exit..." << std::endl;
		getchar();
	}

    GLUI_Master.close_all();
    audioengine->drop();
    SetThreadExecutionState(ES_CONTINUOUS);

    return 0;
}