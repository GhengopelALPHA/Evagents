#ifndef READWRITE_H
#define READWRITE_H

#include "World.h"
#include <stdio.h>

#include "settings.h"
#include "helpers.h"
#include <stdio.h>
#include <iostream>

class ReadWrite
{

public:
	ReadWrite();

	void loadSettings(const char *filename); //load text settings file

	void saveAgent(Agent *a, FILE *file); //save a single agent
	void loadAgents(World *world, FILE *file, float fileversion, bool loadexact= true); //load all agents from file and add them to world

	void loadAgentFile(World *world, const char *address); //load agent from text file
	
	void saveWorld(World *world, float xpos, float ypos, float scalemult, const char *filename); //save world to text file
	void loadWorld(World *world, float &xtranslate, float &ytranslate, float &scalemult, const char *filename); //load world from text file
	
	const char *ourfile;
	const char *lastfile;
};

#endif // READWRITE_H