#include "CPBrain.h"
#include "helpers.h"
using namespace std;


CPBox::CPBox(int numboxes)
{
	seed = 0;
	kp = cap(randf(0,1.5));
	gw = randf(-2,2);
	bias = randn(0,1);

	acc = 0;
	out = 0;
	oldout = 0;
	target = 0;
}

CPBox::CPBox(){
}

CPConn::CPConn(int numboxes)
{
	seed = 0;
	w = randn(0,2);
	if (randf(0,1) < conf::BRAIN_DIRECTINPUTS) sid = randi(-Input::INPUT_SIZE, 0); //connect a portion of the brain directly to input (negative sid).
	else sid = randi(-Input::INPUT_SIZE, numboxes);
	tid = randi(max(0, sid), numboxes); //always connect forward in new brains, to emulate layering
	type = 0;
	if(randf(0,1) < conf::BRAIN_CHANGECONNS) type = 1; //some conns can be change sensitive synapses
//	if(randf(0,1) < conf::BRAIN_MEMCONNS) type = 2; //some conns can be memory synapses NOT IMPLEMENTED
}

CPConn::CPConn(){
}

CPBrain::CPBrain(){
}

CPBrain::CPBrain(int numboxes, int numconns)
{
	// do not OMP!!!
	for (int i=0; i < numboxes; i++) {
		CPBox a = CPBox(numboxes);
		boxes.push_back(a);
	}
	// do not OMP!!!
	for (int i=0; i < numconns; i++) {
		CPConn a = CPConn(boxes.size());
		conns.push_back(a);
	}

	resetBrain();
}

CPBrain& CPBrain::operator=(const CPBrain& other)
{
	if( this != &other ) {
		boxes= other.boxes;
		conns= other.conns;
	}
	return *this;
}

void CPBrain::setLiveBoxes()
{
	//all lives are set at init to t/f based on if in INPUT or OUTPUT ranges already, true if between the ranges, false if inside the ranges (always simulate)
	//so all we need to do is set dead flag = false for all boxes used as a source
//	#pragma omp parallel for
	for (int i=0; i < (int)conns.size(); i++) {
		int sid= conns[i].sid;
		if(sid >= 0 && sid < max((int)(boxes.size() - Output::OUTPUT_SIZE), 0)) {
			boxes[sid].dead = false;
		}
	}
}

//reset ALL boxes and set dead flag
void CPBrain::resetLiveBoxes()
{
	int numboxes = (int)boxes.size();
	#pragma omp parallel for
	for (int i=0; i < numboxes; i++) {
		if(i >= numboxes - Output::OUTPUT_SIZE) boxes[i].dead = false;
		else boxes[i].dead = true;
	}
	setLiveBoxes();
}

//This method will update lives and conns to find and delete ALL dead conns (conns leading to dead boxes) RECURSIVELY. Use while creating new agents
void CPBrain::resetBrain()
{
	while(true) {
		bool erasedconn = false;

		//set live boxes' dead flag based on if in Inputs/Outputs, or if referenced as a source for a conn
		resetLiveBoxes();

		//clean the connections which lead to dead boxes after the reset
		vector<CPConn>::iterator iter= conns.begin();
		while (iter != conns.end() - 1) { //-1 so we never delete the last conn no matter what
			if (boxes[iter->tid].dead) {
				iter= conns.erase(iter);
				erasedconn = true;
			} else {
				++iter;
			}
		}
		
		if (!erasedconn) break;
		//repeat the process if we deleted a conn, there may be newly dead boxes now
	}
	healthCheck();
}

void CPBrain::healthCheck()
{
	for (int i=0; i < (int)conns.size(); i++){
		conns[i].tid = boxRef(conns[i].tid);
	}
}

int CPBrain::inRef(int id)
{
	if (id < 0) id = - id - 1;
	int refval = capm(id, 0, Input::INPUT_SIZE - 1);
	if (refval != id) {
		printf("BRAIN ERROR: an input ID passed to 'inRef()' was invalid. Please set a breakpoint\n");
	}
	return refval;
}

int CPBrain::boxRef(int id)
{
	int refval = capm(id, 0, boxes.size() - 1);
	if (refval != id) {
		printf("BRAIN ERROR: a box ID passed to 'boxRef()' was invalid. Please set a breakpoint\n");
	}
	return refval;
}

int CPBrain::connRef(int id)
{
	int refval = capm(id, 0, conns.size() - 1);
	if (refval != id) {
		printf("BRAIN ERROR: a connection ID passed to 'connRef()' was invalid. Please set a breakpoint\n");
	}
	return refval;
}

//do a single tick of the brain, for each conn. Note that all conns are assumed live and lead to live boxes per "resetBrain()"
void CPBrain::tick(vector< float >& in, vector< float >& out)
{	
	for (int i=0; i < (int)conns.size(); i++){
		//first, get the source value. if -, it's an input; otherwise if +, it's a brain box
		float value;
		if (conns[i].sid < 0) value = in[inRef(conns[i].sid)];
		else value = boxes[boxRef(conns[i].sid)].out;

		//next, process the value based on conn type
		/*if(type==2){ //switch conn. If the input*w is >0.5, it freezes all later inputs to the box and finalizes sum, otherwise it's skipped
			if(val*abox->w[k]>0.5){
				break;
				continue;
			}
			continue;
		} NOT IMPLEMENTED */
		
		if(conns[i].type == 1 && conns[i].sid >= 0){ //change sensitive conn compares to old value, and gets magnified by *10 (arbitrary)
			//we do this AFTER type==2 because switch conn just checks the normal val
			value -= boxes[boxRef(conns[i].sid)].oldout;
			value *= 100;
		}

		//get the target box
		CPBox* tbox = &boxes[boxRef(conns[i].tid)];

		//multiply by weight and add to the accumulation of the target box
		tbox->acc += value*conns[i].w;
	}

	//next, for all live boxes
	for (int i=0; i < (int)boxes.size(); i++) {
		if (boxes[i].dead) continue;

		CPBox* box = &boxes[i];

		box->acc += box->bias; //add bias
		
		//multiply by global weight before sigmoiding it
		box->acc *= box->gw;
		
		//put through sigmoid. very negative values -> 0, very positive values -> 1, values close to 0 -> near 0.5
		box->target = 1.0/(1.0 + exp(-box->acc));

		//done with acc, reset it
		box->acc = 0;

		//back up current out for each box
		box->oldout = box->out;

		//make all boxes go a bit toward target, with dampening applied
		box->out += (box->target - box->out) * box->kp;
	}

	//finally set out[] to the last few boxes to the output
	for (int i=0; i < Output::OUTPUT_SIZE; i++) {
		CPBox* sbox = &boxes[boxRef((int)boxes.size()-1-i)];

		//jump has different responce because we've made it into a change sensitive output
		if (i == Output::JUMP) out[i] = cap(100 * (sbox->out - sbox->oldout));
		else out[i] = sbox->out;
	}
}

float CPBrain::getActivityRatio() const
{
	float sum = 0;
	for (int i = 0; i < (int)boxes.size(); i++){
		sum += fabs(boxes[i].out - boxes[i].oldout);
	}
	return sum / boxes.size();
}

//depreciated?
float CPBrain::getNonZeroWRatio() const
{
	float sum= 0;
/*	for (int j=Input::INPUT_SIZE; j<(int)boxes.size(); j++){
		for (int k=0;k<CONNS;k++) {
			if(boxes[j].w[k]==0) continue;
			else sum++;
		}
	}*/
	return sum/(boxes.size()-Input::INPUT_SIZE)/CONNS;
}

//for mutations which may occur at conception
void CPBrain::initMutate(float MR, float MR2)
{
	//connection mutations.
	//Rare: new conn, boxify, random type, split conn, random source ID, random target ID, source ID bump, target ID bump. 
	//Common: random weight, weight wither, weight jiggle
	if (randf(0,1) < MR/100) {
		//Add conn
		CPConn a = CPConn((int)boxes.size());
		conns.push_back(a);
		boxes[boxRef(a.tid)].seed = 0;
	}

	for (int i=0; i < (int)conns.size(); i++){
		if (randf(0,1) < MR/200) {
			//boxify: a dead box is located, reset to allow simple passing of input, and then a new conn and original conn is attached
			//so that for [A]-a->[B], box [*C] and conn *b is created but without effecting the expected sum on [B]: [A]-a->[*C]-*b->[B]
			//locate a dead box
			int linkid = -1;
			for (int j=0; j < (int)boxes.size(); j++){
				if (boxes[j].dead) {
					linkid = j;
					break;
				}
			}
			if (linkid>=0) {
				int targetid = conns[i].tid; //back up the target box [B]
				conns[i].tid = linkid;

				//set box values to mock values to allow pass-through
				boxes[linkid].bias = 0;
				boxes[linkid].gw = 1;
				boxes[linkid].kp = 1;
				boxes[linkid].out = 0.5;
				boxes[linkid].oldout = 0.5;
				boxes[linkid].target = 0.5;

				CPConn a = CPConn((int)boxes.size()); //create new conn
				a.sid = linkid;
				a.tid = targetid;
				a.w = 1.0;
				a.type = 0;
				conns.push_back(a);
				boxes[boxRef(targetid)].seed = 0;
			}
		}

		if (randf(0,1) < MR/20) {
			//randomize type
			conns[i].type = randi(0,2); //remember randi is [a,b). Current options are: 0,1
			conns[i].seed = 0;
			boxes[boxRef(conns[i].tid)].seed = 0;
		}

		if (randf(0,1) < MR/15) {
			//randomize source ID
			conns[i].sid = capm(randi(0,(int)boxes.size()), -Input::INPUT_SIZE, boxes.size());
			conns[i].seed = 0;
			boxes[boxRef(conns[i].tid)].seed = 0;
		}

		if (randf(0,1) < MR/15) {
			//randomize target box ID
			boxes[boxRef(conns[i].tid)].seed = 0; //reset the tid's box before leaving it
			conns[i].tid = boxRef(randi(0,(int)boxes.size()));
			conns[i].seed = 0;
			boxes[boxRef(conns[i].tid)].seed = 0;

		}

		if (randf(0,1) < MR/5) {
			//split conn: new conn created from old conn, both get weight / 2
			conns[i].w /= 2;
			conns[i].seed = 0;
			CPConn copy = conns[i];
			conns.push_back(copy);
		}

		if (randf(0,1) < MR/4) {
			//source ID bump: +/- 1
			int oldid = conns[i].sid;
			int newid = capm(oldid + (int)(MR2*10*randi(-1,2)), -Input::INPUT_SIZE, boxes.size());
			conns[i].sid = newid;
			if (oldid != newid) {
				conns[i].seed = 0;
				boxes[boxRef(conns[i].tid)].seed = 0;
			}
		}
		
		if (randf(0,1) < MR/4) {
			//target ID bump: +/- 1
			int oldid = conns[i].tid;
			int newid = boxRef(oldid + (int)(MR2*10*randi(-1,2)));
			conns[i].tid = newid;
			if (oldid != newid) {
				conns[i].seed = 0;
				boxes[boxRef(oldid)].seed = 0;
				boxes[boxRef(newid)].seed = 0;
			}
		}

		//More common connection mutations:
		if (randf(0,1) < MR/5) {
			//randomize weight
			CPConn dummy = CPConn(conf::BRAINBOXES);
			conns[i].w = dummy.w;
			conns[i].seed = 0;
			boxes[boxRef(conns[i].tid)].seed = 0;
		}

		if (randf(0,1) < MR) {
			//weight wither
			if(randf(0,1) > fabs(conns[i].w)) {//the closer to 0 it already is, the higher the chance it gets set to 0
				conns[i].w = 0;
				conns[i].seed = 0;
				boxes[boxRef(conns[i].tid)].seed = 0;
			}
		}

		if (randf(0,1) < MR*2) {
			//weight jiggle
			conns[i].w += randn(0, MR2/2);
			//don't bother with seed = 0, this is too common and too low impact
		}
	}

	//box mutations
	//Rare: copy box, random bias, random global weight, random kp
	//Common: kp jiggle, global weight jiggle, bias jiggle
	for (int i=0; i < (int)boxes.size(); i++){
		CPBox* box= &boxes[i];

		if (randf(0,1)<MR/15) {
			//copy another box
			int j = randi(0,boxes.size());
			if(j != i) {
				box->bias = boxes[j].bias;
				box->kp = boxes[j].kp;
				box->gw = boxes[j].gw;
				box->seed = 0;
			}
		}

		if (randf(0,1)<MR/15) {
			//random bias
			CPBox dummy = CPBox(1);
			box->bias = dummy.bias;
			box->seed = 0;
		}

		if (randf(0,1)<MR/10) {
			//random global weight
			CPBox dummy = CPBox(1);
			box->gw = dummy.gw;
			box->seed = 0;
		}

		if (randf(0,1)<MR/3) {
			//random kp (dampening)
			CPBox dummy = CPBox(1);
			box->kp = dummy.kp;
			box->seed = 0;
		}

		//more common box mutations
		if (randf(0,1)<MR/5) {
			//global weight jiggle
			box->gw += randn(0, MR2/5);
		}

		if (randf(0,1)<MR/2) {
			//jiggle kp (dampening)
			box->kp = cap(randn(box->kp, MR2/10));
		}

		if (randf(0,1)<MR) {
			//jiggle bias
			box->bias += randn(0, MR2);
			//no need to reset seed for simple bias jiggle
		}
	}

	//clean up and reset brain conns and live boxes
	resetBrain();
}

void CPBrain::liveMutate(float MR, float MR2, vector<float>& out)
{
	//for mutations which may occur while the bot is live
	//live mutations are applied to the boxes and conns as normal, but only jiggle changes are allowed
	int randconn = randi(0, conns.size());

	if (conf::LEARNRATE>0 && conns[randconn].sid >= 0 && randf(0,1)<MR/4) {
		//stimulate box weight: boxes that fire together, strengthen
		float stim = out[Output::STIMULANT];
		if (stim > 0.5) {
			//modify weights based on matching old output and old input, if stimulant is active
			float sourceoldout = boxes[boxRef(conns[randconn].sid)].oldout;
			float targetoldout = boxes[boxRef(conns[randconn].tid)].oldout;
			conns[randconn].w += conf::LEARNRATE * (2*stim - 1) * (1 - abs(targetoldout - sourceoldout));
		}
	}

	if (randf(0,1) < MR) {
		//weight wither
		if(randf(0,1) > fabs(conns[randconn].w)) {//the closer to 0 it already is, the higher the chance it gets set to 0
			conns[randconn].w = 0;
			conns[randconn].seed = 0;
			boxes[boxRef(conns[randconn].tid)].seed = 0;
		}
	}

	if (randf(0,1) < MR*2) {
		//weight jiggle
		conns[randconn].w += randn(0, MR2);
		//don't bother with seed = 0, this is too common and too low impact
	}
	
	//box mutation
	CPBox* box= &boxes[randi(0, boxes.size())];

	if (randf(0,1)<MR/5) {
		//global weight jiggle
		box->gw += randn(0, MR2/5);
	}

	if (randf(0,1)<MR/2) {
		//jiggle kp (dampening)
		box->kp = cap(randn(box->kp, MR2/10));
	}

	if (randf(0,1)<MR) {
		//jiggle bias
		box->bias += randn(0, MR2);
		//no need to reset seed for simple bias jiggle
	}
}

CPBrain CPBrain::crossover(const CPBrain& other)
{
	CPBrain newbrain = *this;
	if (this->conns.size() < other.conns.size()) {
		newbrain = other;
	}

	for (int i=0; i < (int)newbrain.conns.size(); i++){
		if(i >= other.conns.size() || i >= this->conns.size()) continue; //allow the rest of the conns to be whichever they were inherited from

		int s1= this->conns[i].seed;
		int s2= other.conns[i].seed;
		//function which offers probability of which parent to use, based on relative seed counters
		float threshold= ((s1 - s2) / (conf::BRAINSEEDHALFTOLERANCE + abs(s1 - s2)) + 1) / 2;

		if(randf(0,1)<threshold){
			newbrain.conns[i] = this->conns[i];
		} else {
			newbrain.conns[i] = other.conns[i];
		}
		newbrain.conns[i].seed++;
	}

	for (int i = 0; i < (int)newbrain.boxes.size(); i++){
		if(i >= other.boxes.size() || i >= this->boxes.size()) continue; //allowing for variable box counts (NOT IMPLEMENTED)

		int s1= this->boxes[i].seed;
		int s2= other.boxes[i].seed;
		//function which offers probability of which parent to use, based on relative seed counters
		float threshold= ((s1 - s2) / (conf::BRAINSEEDHALFTOLERANCE + abs(s1 - s2)) + 1) / 2;

		if(randf(0,1)<threshold){
			newbrain.boxes[i] = this->boxes[i];
		} else {
			newbrain.boxes[i] = other.boxes[i];
		}
		newbrain.boxes[i].seed++;
	}
	return newbrain;
}

/*std::vector<int> CPBrain::traceBack(int outback)
{
	std::vector<int> inputs; //list of boxes which effect our Output of Interest
	std::vector<int> todos; //list of boxes still need to check
	std::vector<int> diddos; //list of boxes done. Continually sorted

	//if bad data, break
	if(outback<0 || outback>=Output::OUTPUT_SIZE) {
		printf("bad trace value passed\n");
		return inputs;
	}

	int nowtrace= boxes.size()-1-outback;
	int tries= 0;
	//give us a natural stopping point with # tries
	while(tries<100) {
		tries++;

		//for every connection of the current tracing box,
		for (int k=0;k<CONNS;k++) {
			//make sure we didn't already check it earlier
			if (std::binary_search (diddos.begin(), diddos.end(), boxes[nowtrace].id[k])) continue;
			
			//check if the connection is strong enough to count
			if(fabs(boxes[nowtrace].w[k])>conf::BRAIN_TRACESTRENGTH) {
				
				//next, see if it references an input.
				if(boxes[nowtrace].id[k]<Input::INPUT_SIZE) {
					//if it does, need to make sure we didn't already find this one, and therefore skip list-making
					for (int i=0; i<inputs.size(); i++) {
						if(inputs[i]==boxes[nowtrace].id[k]) {
							break;
							continue;
						}
					}

					//if we didn't continue, we didn't find our input listed. Add it!
					inputs.push_back(boxes[nowtrace].id[k]);

					//check if we have put every input on the input list, end early (or right on time!)
					if(inputs.size()==Input::INPUT_SIZE) {
						break;
						break;
					}
				}

				//if we did not reference an input, add it to the to-do list
				else todos.push_back(boxes[nowtrace].id[k]);
				//regardless, we checked the box and can skip it later.
				diddos.push_back(boxes[nowtrace].id[k]);
				std::sort (diddos.begin(), diddos.end()); //sort our list so we can use binary_search above
			}
		}

		//after checking our current connections, we must check if we have anything on the to-do list
		if(todos.size()!=0) {
			//pick the next box to check, and remove from to-do
			nowtrace= todos[todos.size()-1];
			todos.pop_back();
		} else break;
	}

	if(todos.size()>0) printf("we had %d nodes left to try before giving up\n", todos.size());

	return inputs;
} NOT IMPLEMENTED*/

