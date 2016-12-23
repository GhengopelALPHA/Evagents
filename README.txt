========================================================================================================================
Evagents
Authors: Julian Hershey, Andrej Karpathy, Casey Link
Contributors: David Coleman, 
Supporters: 
License: BSD (see end of file)

Project website and attached forum: 
https://sites.google.com/site/evagentsproject/

========================================================================================================================

WHAT IS EVAGENTS?
Evagents is a "natural" selection simulator. The program contains a world wherein agents can interact with that world 
and each other, reproduce, die, run around, eat, breed, communicate, and other things, using neural nets to calculate 
their responses to stimuli. Each agent has a huge number of features available to them that they can use in various ways 
to try and survive long enough to reproduce, extending their solution through time. Any time a baby is born its traits 
are copied from it's parent(s), but with chances of mutations. Thus a form of natural selection is applied to create 
populations of species that can best survive in the world. Competition and cooperation are important secondary 
characteristics that agents must develop, using their DRAWS Brains, a neural network with features of Dampening, 
Recurrence, Addition, Weighted, Sigmoid-outputs that takes inputs (in range 0-1) and feeds them to outputs (also 0-1) over
time and in an initially random structure. As time passes, however, the brains can mutate into more efficient forms

========================================================================================================================

BUILDING:
(for users that want to edit the source code)

To compile Evagents you will need:
+ Microsoft Windows
+ MS SDK for Windows Server 2008 and .NET Framework 3.5 (https://www.microsoft.com/en-us/download/details.aspx?id=11310)
+ OpenGL* (http://www.opengl.org/resources/libraries/glut/)

It will use OpenMP to speed up everything, in case you have multi-core CPU, and OpenGL + GLUI to draw a user interface.

Take the provided .vcproj file and edit the files as you desire.
When finished, build the solution in either Debug or Release configuration, depending on your needs

*OpenGL is typically included in Windows distributions, so a manual DL is not usually required.
========================================================================================================================

COMMON USAGE:
(for compiled builds and .exe users)

To run Evagents you will need:
+ Microsoft Windows

Open the Evagents.exe by double-clicking, as usual.

Once running, hit 'm' to activate "fast mode" (and not draw). Speed can also be controlled with + and -. Use mouse to 
pan (left-click and drag) and zoom (middle-click and drag). Agents can be selected and deselected by clicking on them.
Press 'f' to follow selected. 'o' to auto-select the oldest agent, 'g' the best generation, and 'h' for healthiest.
'wasd' controls the selected agent, '/' heals it, '|' (shift+'\') reproduces it, 'delete' kills it, 'spacebar' gives input

Agents will flash events with colors and sizes depending on what's happening to them. Here is a quick key:
Green event: Agent reproduced asexually. There should now be some babies around that look and behave like the parent
Grey: Agent was just born into the world.
Dark Blue: Agent sexually reproduced with another nearby agent, also flashing blue. The children nearby are a crossover
Purple: The agent just experienced a mutation event unrelated to reproduction mutations, and may behave differently
Light-Blue: Agent just collided and took damage while bouncing away from the other agent. Size proportional to damage
Yellow: Agent used its jaws to bite another agent. Size proportional to damage dealt
Orange: Agent used its spike to stab another agent. Size proportional to damage dealt
Red: Agent used either its spike or jaws to kill another agent.

========================================================================================================================

QUESTIONS, COMMENTS, ISSUES are best posted on the google group, available at the project site above, 
or contact me at: julian.hershey@gmail.com

========================================================================================================================

LICENSE CLAUSE:
Redistribution and use in source and binary forms are permitted provided that the above Authors list, project name, and this paragraph are duplicated with all distributions, and that any documentation, advertising materials, and other materials related to such distribution and use acknowledge that the software was developed by the Authors. The names of the Authors may not be used to endorse or promote products derived from this software without specific prior written permission. THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.

========================================================================================================================