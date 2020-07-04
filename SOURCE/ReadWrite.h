#ifndef READWRITE_H
#define READWRITE_H

#include "World.h"
#include <stdio.h>

class ReadWrite
{

public:
	ReadWrite();

	void loadSettings(const char *filename); //load text settings file

	void saveAgent(Agent *a, FILE *file); //save a single agent
	void loadAgent(World *world, const char *filename); //load a single agent and add it to world

	void processAgentKeys(Agent *xa, char var[16], char dataval[16], int &mode); //process input stream through all the agent-reading keys (needed for loading single agents)

	void saveWorld(World *world, float xpos, float ypos, const char *filename); //save world to text
	void loadWorld(World *world, float &xtranslate, float &ytranslate, const char *filename); //load world from text
	
	const char *ourfile;
	const char *lastfile;
};

#endif // READWRITE_H