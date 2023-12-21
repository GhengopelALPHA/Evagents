#pragma once

#include "settings.h"
#include "helpers.h"

#include <algorithm>
#include <vector>
#include <cstdio>

class CPBox {
public:
	CPBox();
	CPBox(int numboxes);
	bool operator==(const CPBox& other) const;

	float bias;		//base number
    float gw;		//global weight
	float kp;		//damper, always in range [0,1]

	bool dead;		//t/f flag for if this box has downstream connections. If not, we don't visualize; it's not important. Output boxes are never dead
	int seed;		//number of successes (reproduction events) this box has experienced whilst unmodified

    //state variables
	float acc;		//accumulated value. This is used b/c we simulate conns instead of boxes, so we need to track each box's total before gweight and sigmoid
    float target;	//target value this node is going toward
    float out;		//current output
    float oldout;	//output a tick ago
};

class CPConn {
public:
	CPConn();
    CPConn(int boxes);
	bool operator==(const CPConn& other) const;

    float w;		//weight of each connecting box
	float bias;		//bias of the connection, aka the activation threshold
    int sid;		//source. +: box id in boxes[], -: input id in inputs list
	int tid;		//target box id in boxes[]
    int type;		//see ConnType for details

	bool dead;		//t/f flag for if this conn links to a dead target or not. Can also become dead if weight == 0
	int seed;		//number of successes (reproduction events) this conn has experienced whilst unmodified
};

/**
 * Connection-Positive brain
 */
class CPBrain {
public:
	CPBrain();
    CPBrain(int numboxes, int numconns);
	virtual CPBrain& operator=(const CPBrain& other);

	std::vector<CPBox> boxes;	// list of boxes
	std::vector<CPConn> conns;	// list of all connections. No limits on target conns, source conns, or even number of conns
	std::vector<CPConn> lives;	// list of simulated connections. genes affect conns, lives affect the brain (which affects the survival of the genes!)
    int mutations[Mutation::TYPES]; // list of integers that track the number of each mutation type (see Mutations enum for details)

	void setLives();
	void resetLives();
	void resetBrain();
	void healthCheck();

	int inRef(int id);
	int boxRef(int id);
	int connRef(int id);

    void tick(std::vector<float>& in, std::vector<float>& out);

	float getActivityRatio() const;
//	std::vector<int> traceBack(int outback);

    void initMutate(float MR, float MR2);
	void liveMutate(float MR, float MR2, std::vector<float>& out);
    CPBrain crossover(const CPBrain& other);


private:
    void init();
};

