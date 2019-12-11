#ifndef DRAWSBRAIN_H
#define DRAWSBRAIN_H

#include "settings.h"
#include "helpers.h"

#include <algorithm>
#include <vector>
#include <stdio.h>

class DRAWSBox {
public:

    DRAWSBox(int maxid);

    std::vector<float> w; //weight of each connecting box
    std::vector<int> id; //id in boxes[] of the connecting box
    std::vector<int> type; //0: regular synapse. 1: change-sensitive synapse. 2: memory trigger synapse

	int seed; //number of successes (reproduction events) this box has experienced whilst unmodified
    float kp; //damper
    float gw; //global w
    float bias; //base number

    //state variables
    float target; //target value this node is going toward
    float out; //current output
    float oldout; //output a tick ago
};

/**
 * Dampened, Recurrent, Additive, Weighted Sigmoid brain
 */
class DRAWSBrain
{
public:

    std::vector<DRAWSBox> boxes;

    DRAWSBrain(int numboxes);
    DRAWSBrain(const DRAWSBrain &other);
    virtual DRAWSBrain& operator=(const DRAWSBrain& other);

    void tick(std::vector<float>& in, std::vector<float>& out);
	float getActivity() const;
    void initMutate(float MR, float MR2);
	void liveMutate(float MR, float MR2, std::vector<float>& out);
    DRAWSBrain crossover( const DRAWSBrain &other );
	std::vector<int> traceBack(int outback);


private:
    void init();
};

#endif
