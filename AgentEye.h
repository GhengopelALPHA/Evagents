#ifndef AGENTEYE_H
#define AGENTEYE_H

#include "vmath.h"

#include <vector>
#include <string>
#include <algorithm>

class AgentEye
{
public:
	AgentEye();
//	AgentEye();
		
	float eye_see_agent_mod; //sensitivity per eye
	float location; //radial location of the eye structure
	float direction; //radial direction the general eye structure points. in range [-pi/2, pi/2] with 0= pointing outward
	//values beyond +-pi/2 could be possible and could equate to eyes being located closer to center, pointed outward; consider pi= traditional eye
//	float eyesensors; //number of sensors this eye has UNUSED, USES GLOBAL DEFINES
	std::vector<float> sensor_fovs; //field of view for each eye sensor on this eye structure
	std::vector<float> sensor_angles; //direction angles of each eye sensor on this eye structure. in range [-pi/2, pi/2] with 0= co-linear with eyedirection
	//direction= 0 and sensor_angles[i]= 0 equivalent to traditional eye (but with radius locus)
	//
};

#endif // AGENTEYE_H
