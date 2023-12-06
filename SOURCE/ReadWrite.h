#pragma once

#include "World.h"
#include "settings.h"
#include "helpers.h"
#include <cstdio>
#include <iostream>

class ReadWrite
{

public:
    ReadWrite();

    void loadSettings(const char* filename); // Load text settings file

    void saveAgent(Agent* a, FILE* file); // Save a single agent
    void loadAgents(World* world, FILE* file, float fileversion, bool loadexact = true); // Load all agents from file and add them to the world

    void loadAgentFile(World* world, const char* address); // Load an agent from a text file

    void saveWorld(World* world, float xpos, float ypos, float scalemult, int autosaves, const char* filename); // Save the world to a text file
    void loadWorld(World* world, float& xtranslate, float& ytranslate, float& scalemult, int& autosaves, const char* filename); // Load the world from a text file

    const char* ourfile;
    const char* lastfile;
};