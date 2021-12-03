#ifndef CPBRAIN_H
#define CPBRAIN_H

#include "settings.h"
#include "helpers.h"

#include <algorithm>
#include <vector>
#include <stdio.h>

class CPBox {
public:
	CPBox();
	CPBox(int numboxes);

    float kp; //damper, always in range [0,1]
    float gw; //global weight
    float bias; //base number

	bool dead; //t/f flag for if this box has downstream connections. If not, we don't visualize; it's not important. Output boxes are never dead
	int seed; //number of successes (reproduction events) this box has experienced whilst unmodified

    //state variables
	float acc; //accumulated value. This is used b/c we simulate conns instead of boxes, so we need to track each box's total before gweight and sigmoid
    float target; //target value this node is going toward
    float out; //current output
    float oldout; //output a tick ago
};

class CPConn {
public:
	CPConn();
    CPConn(int boxes);

    float w; //weight of each connecting box
    int sid; //source. +: box id in boxes[], -: input id in inputs list
	int tid; //target box id in boxes[]
    int type; //0: regular synapse. 1: change-sensitive synapse. 2: NOT IMPLEMENTED memory trigger synapse

	int seed; //number of successes (reproduction events) this conn has experienced whilst unmodified
};

/**
 * Connection-Positive brain
 */
class CPBrain
{
public:

    std::vector<CPBox> boxes; //list of boxes
	std::vector<CPConn> conns; //list of simulated connections. No limits on target conns, source conns, or even number of conns

	CPBrain();
    CPBrain(int numboxes, int numconns);
    virtual CPBrain& operator=(const CPBrain& other);
	void setLiveBoxes();
	void resetLiveBoxes();
	void resetBrain();
	void healthCheck();

	int inRef(int id);
	int boxRef(int id);
	int connRef(int id);

    void tick(std::vector<float>& in, std::vector<float>& out);

	float getActivityRatio() const;
	float getNonZeroWRatio() const;
//	std::vector<int> traceBack(int outback);

    void initMutate(float MR, float MR2);
	void liveMutate(float MR, float MR2, std::vector<float>& out);
    CPBrain crossover( const CPBrain &other );


private:
    void init();
};

#endif
