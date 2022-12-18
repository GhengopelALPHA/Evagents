#include "CPBrain.h"

using namespace std;

CPBox::CPBox(int numboxes)
{
	bias = randn(0,conf::BRAIN_BOX_BIAS_STD);
	gw = randf(-conf::BRAIN_BOX_GW_RANGE, conf::BRAIN_BOX_GW_RANGE);
	if (gw < 0 && randf(0,1) > conf::BRAIN_NEGATIVE_GW_BOXES) gw *= -1;
	kp = cap(randf(0, conf::BRAIN_BOX_KP_MAX));

	acc = 0;
	out = 0;
	oldout = 0;
	target = 0;
	seed = 0;
}

CPBox::CPBox(){
}

CPConn::CPConn(int numboxes)
{
	w = randn(0, conf::BRAIN_CONN_WEIGHT_STD);
	bias = randn(0, conf::BRAIN_CONN_BIAS_STD);
	relu = randn(0, conf::BRAIN_CONN_RELU_STD);

	tid = randi(0, numboxes);

	if (randf(0,1) < conf::BRAIN_DIRECTINPUTS) sid = randi(-Input::INPUT_SIZE, 0); //connect a portion of the brain directly to input (negative sid).
	else sid = randi(-Input::INPUT_SIZE, tid + 1); //always connect forward in new brains, to emulate layering

	type = ConnType::NORMAL;
	if(randf(0,1) < conf::BRAIN_INVERTCONNS) type = ConnType::INVERTED; //some conns have their inputs inverted (1-x)
	if(randf(0,1) < conf::BRAIN_CHANGECONNS) type = ConnType::DELTA; //some conns can be change sensitive synapses

	seed = 0;
	dead = true;
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
		if (randf(0,1) < conf::BRAIN_MIRRORCONNS && i > 0) {
			int randconnidx = randi(0, i);
			a.w = -conns[randconnidx].w + randn(0, 0.001);
			a.tid = conns[randconnidx].tid;
			a.type = conns[randconnidx].type;
		}
		conns.push_back(a);
	}

	resetBrain();
}

CPBrain& CPBrain::operator=(const CPBrain& other)
{
	if( this != &other ) {
		boxes= other.boxes;
		conns= other.conns;
		lives= other.lives;
	}
	return *this;
}

void CPBrain::setLives()
{
//	#pragma omp parallel for
	while (true) {
		bool enlivenedconn = false;

		//for each conn...
		for (int i=0; i < (int)conns.size(); i++) {
			if (!conns[i].dead) continue; //skip if we already checked this one and it's live

			int btid = boxRef(conns[i].tid); //get the target box id
			if (btid >= ((int)(boxes.size()) - Output::OUTPUT_SIZE) || !boxes[btid].dead) {
				// if the box id is in the Output range, or the box pointed at is not dead
				if (conns[i].sid>=0) 
					// and the source id is a box (not an input)
					boxes[boxRef(conns[i].sid)].dead = false; // disable the dead flag on that box
				conns[i].dead = false; // also mark this conn as a live conn
				enlivenedconn = true; //we did work this round. set exit condition to stay in loop
			}
		}

		if (!enlivenedconn) break; //repeat until we process a loop without enlivening any conn
	}

	//finally, commit all conns with dead=false to the lives list
	for (int i=0; i < (int)conns.size(); i++) {
		CPConn& conn = conns[i];
		if (!conn.dead) lives.push_back(conn);
	}
}

//reset ALL boxes and conns, and set dead flags
void CPBrain::resetLives()
{
	int numboxes = (int)boxes.size();
	#pragma omp parallel for
	for (int i=0; i < numboxes; i++) {
		if(i >= numboxes - Output::OUTPUT_SIZE) boxes[i].dead = false;
		else boxes[i].dead = true;
	}
	#pragma omp parallel for
	for (int i=0; i < conns.size(); i++) {
		conns[i].dead = true;
	}

	lives.clear();
	setLives();
}

//call this whenever changing conns or boxes, we need to check all of them and set lives
void CPBrain::resetBrain()
{
	//set live boxes' dead flag based on if in Inputs/Outputs, or if referenced as a source for a conn
	resetLives();

	#if defined(_DEBUG) 
		healthCheck();
	#endif 
}

void CPBrain::healthCheck()
{
	for (int i=0; i < (int)conns.size(); i++){
		conns[i].tid = boxRef(conns[i].tid);
	}
	for (int i=0; i < (int)lives.size(); i++){
		lives[i].tid = boxRef(lives[i].tid);
	}
//	for (int i=0; i < (int)boxes.size(); i++){
//		printf("%i, %i\n", i, (int)boxes[i].dead);
//	}
}

int CPBrain::inRef(int id)
{
	if (id < 0) id = - id - 1;
	int refval = clamp(id, 0, Input::INPUT_SIZE - 1);
	if (refval != id) {
		printf("BRAIN ERROR: an input ID passed to 'inRef()' was invalid. Please set a breakpoint\n");
	}
	return refval;
}

int CPBrain::boxRef(int id)
{
	int refval = clamp(id, 0, boxes.size() - 1);
	if (refval != id) {
		printf("BRAIN ERROR: a box ID passed to 'boxRef()' was invalid. Please set a breakpoint\n");
	}
	return refval;
}

int CPBrain::connRef(int id)
{
	int refval = clamp(id, 0, conns.size() - 1);
	if (refval != id) {
		printf("BRAIN ERROR: a connection ID passed to 'connRef()' was invalid. Please set a breakpoint\n");
	}
	return refval;
}

//do a single tick of the brain, for each conn. Note that all conns are assumed live and lead to live boxes per "resetBrain()"
//DO NOT OMP anything, we handle that on the layer above, for each agent
void CPBrain::tick(vector< float >& in, vector< float >& out)
{	
	for (int i=0; i < (int)lives.size(); i++){
		//first, get the source value. if -, it's an input; otherwise if +, it's a brain box
		float value = lives[i].sid < 0 ? in[inRef(lives[i].sid)] : boxes[boxRef(lives[i].sid)].out;

		if (lives[i].type == ConnType::INVERTED) value = 1 - value;
		else if (lives[i].type == ConnType::DELTA && lives[i].sid >= 0){ //change sensitive conn compares to old value, and gets magnified by *10 (arbitrary)
			value -= boxes[boxRef(lives[i].sid)].oldout;
			value *= 10;
		}

		//add bias and apply a max with ReLU, the Rectified Linear Unit, as an activation function
		value = max((float)value*lives[i].w + lives[i].bias, lives[i].relu);

		//multiply by greater weight and add to the accumulation of the target box
		boxes[boxRef(lives[i].tid)].acc += value;
	}

	//next, for all live boxes...
	for (int i=0; i < (int)boxes.size(); i++) {
		if (boxes[i].dead) continue;

		CPBox* box = &boxes[i]; //DO NOT De-Pointer-ize! It causes glitches!

		float presig = (box->acc + box->bias)*box->gw;
		
		//put value through fast sigmoid. very negative values -> 0, very positive values -> 1, values close to 0 -> near 0.5
		box->target = 0.5 * (presig / ( 1 + abs(presig)) + 1);
		//box->target = 1.0 / (1.0 + exp( -(box->acc + box->bias)*box->gw )); //old sigmoid

		//done with acc, reset it, and back up current out for each box
		box->acc = 0;
		box->oldout = box->out;

		//make all boxes go a bit toward target, with dampening applied
		box->out += (box->target - box->out) * box->kp;

		//finally, set out[] to the last few boxes to the output
		if (i >= (int)boxes.size() - Output::OUTPUT_SIZE) {
			int outidx = i - ( (int)boxes.size() - Output::OUTPUT_SIZE );

			//jump has different response because we've made it into a change sensitive output
			if (outidx == Output::JUMP) out[outidx] = cap(10 * (box->out - box->oldout));
			else out[outidx] = box->out;
		}
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

//for mutations which may occur at conception
void CPBrain::initMutate(float MR, float MR2)
{
	//connection (conn) mutations.
	//Extraordinary (/100+): random type, random source ID, random target ID
	//Rare (/10+): random relu, new conn, invert type, mirror conn, target ID bump, split conn, random weight, random bias
	//Common: source ID bump, bias jiggle, relu jiggle, weight wither, weight jiggle

	for(int i= 0; i<randi(1,6); i++){ //possibly add between 0 and 5 new connections
		if (randf(0,1) < MR/10) {
			//Add conn
			CPConn a = CPConn((int)boxes.size());
			a.w*= conf::BRAIN_CONN_WEIGHT_MUTATION_DAMPEN; //new conns in a surviving brain get reduced weight
			conns.push_back(a);
			boxes[boxRef(a.tid)].seed = 0;
		}
	}

	for (int i=0; i < (int)conns.size(); i++){
		//Extraordinary:
		if (randf(0,1) < MR/100) {
			//randomize type
			int oldtype = conns[i].type;
			conns[i].type = randi(0, ConnType::CONN_TYPES); //remember randi is [a,b)
			if (oldtype != conns[i].type) {
				conns[i].seed = 0;
				boxes[boxRef(conns[i].tid)].seed = 0;
			}
		}

		if (randf(0,1) < MR/100) {
			//randomize source ID
			int oldid = conns[i].sid;
			int newid = clamp(randi(0,(int)boxes.size()), -Input::INPUT_SIZE, boxes.size()-1);
			if (oldid != newid) {
				conns[i].sid = newid;
				conns[i].seed = 0;
				boxes[boxRef(conns[i].tid)].seed = 0;
			}
		}

		if (randf(0,1) < MR/100) {
			//randomize target ID
			int oldid = conns[i].tid;
			int newid = clamp(randi(0,(int)boxes.size()), 0, (int)boxes.size()-1);
			if (oldid != newid) {
				conns[i].tid = newid;
				conns[i].seed = 0;
				boxes[boxRef(newid)].seed = 0;
				boxes[boxRef(oldid)].seed = 0;
			}
		}

		//Rare:
		if (randf(0,1) < MR/75) {
			//randomize relu
			CPConn dummy = CPConn(conf::BRAINBOXES);
			conns[i].relu = dummy.relu;
			conns[i].seed = 0;
			boxes[boxRef(conns[i].tid)].seed = 0;
		}

		if (randf(0,1) < MR/50) {
			//invert type: if type==normal or type==inverted, swap it
			if (conns[i].type == ConnType::NORMAL) {
				conns[i].type = ConnType::INVERTED;
			} else if (conns[i].type == ConnType::INVERTED) {
				conns[i].type = ConnType::NORMAL;
			}
		}

		if (randf(0,1) < MR/25) {
			//mirror conn: conn gets -w
			conns[i].w = -conns[i].w;
			conns[i].seed = 0;
		}

		if (randf(0,1) < MR/20) {
			//target ID bump: +/- 1 or more
			int oldid = conns[i].tid;
			int range = (int)ceil(MR2*100);
			int newid = clamp(oldid + randi(-range,range+1), 0, (int)boxes.size()-1);
			//+ (int)(MR2*100*randi(-1,2)) this is a jump mutation; implement later
			if (oldid != newid) {
				conns[i].tid = newid;
				conns[i].seed = 0;
				boxes[boxRef(oldid)].seed = 0;
				boxes[boxRef(newid)].seed = 0;
			}
		}

		if (randf(0,1) < MR/15) {
			//split conn: new conn created from old conn, both get weight / 2
			conns[i].w /= 2;
			CPConn copy = conns[i];
			conns.push_back(copy);
		}

		if (randf(0,1) < MR/15) {
			//randomize weight
			CPConn dummy = CPConn(conf::BRAINBOXES);
			conns[i].w = dummy.w*conf::BRAIN_CONN_WEIGHT_MUTATION_DAMPEN; //reduced weight on existing brain conns
			conns[i].seed = 0;
			boxes[boxRef(conns[i].tid)].seed = 0;
		}

		if (randf(0,1) < MR/12) {
			//randomize bias
			CPConn dummy = CPConn(conf::BRAINBOXES);
			conns[i].bias = dummy.bias;
			conns[i].seed = 0;
			boxes[boxRef(conns[i].tid)].seed = 0;
		}

		//Common:
		if (randf(0,1) < MR/10) {
			//source ID bump: +/- 1 or more
			int oldid = conns[i].sid;
			int range = (int)ceil(MR2*100);
			int newid = clamp(oldid + randi(-range,range+1), -Input::INPUT_SIZE, boxes.size()-1);
			//+ (int)(MR2*100*randi(-1,2))
			if (oldid != newid) {
				conns[i].sid = newid;
				conns[i].seed = 0;
				boxes[boxRef(conns[i].tid)].seed = 0;
			}
		}

		if (randf(0,1) < MR/8) {
			//relu jiggle
			conns[i].relu += randn(0, MR2);
		}

		if (randf(0,1) < MR/5) {
			//weight wither
			if(randf(0,1) > fabs(conns[i].w)*2) {
				//the closer to 0 it already is, the higher the chance it gets set to 0. *2 rescales it to range (-0.5,0.5), outside this the mutation is impossible
				conns[i].w = 0;
				conns[i].seed = 0;
				boxes[boxRef(conns[i].tid)].seed = 0;
			}
		}

		if (randf(0,1) < MR/2) {
			//bias jiggle
			conns[i].bias += randn(0, MR2);
		}

		if (randf(0,1) < MR) {
			//weight jiggle
			conns[i].w += randn(0, MR2);
		}
	}

	//box mutations
	//Extraordinary (/100+): copy box, random bias, random global weight
	//Rare (/10+): mirror global weight, mirror bias
	//Common: random kp, kp jiggle, global weight jiggle, bias jiggle
	for (int i=0; i < (int)boxes.size(); i++){
		CPBox* box= &boxes[i];

		//Extraordinary:
		if (randf(0,1)<MR/150) {
			//copy another box
			int j = randi(0,boxes.size());
			if(j != i) {
				box->bias = boxes[j].bias;
				box->kp = boxes[j].kp;
				box->gw = boxes[j].gw;
				box->seed = 0;
			}
		}

		if (randf(0,1)<MR/100) {
			//random bias
			CPBox dummy = CPBox(1);
			box->bias = dummy.bias;
			box->seed = 0;
		}

		if (randf(0,1)<MR/100) {
			//random global weight
			CPBox dummy = CPBox(1);
			box->gw = dummy.gw;
			box->seed = 0;
		}

		//Rare:
		if (randf(0,1)<MR/50) {
			//mirror global weight
			CPBox dummy = CPBox(1);
			box->gw = -box->gw;
			box->seed = 0;
		}

		if (randf(0,1)<MR/20) {
			//mirror bias
			box->bias = -box->bias;
			box->seed = 0;
		}

		//Common:
		if (randf(0,1)<MR/10) {
			//random kp (dampening)
			CPBox dummy = CPBox(1);
			box->kp = dummy.kp;
			box->seed = 0;
		}

		if (randf(0,1)<MR/5) {
			//global weight jiggle
			box->gw += randn(0, MR2/4);
		}

		if (randf(0,1)<MR/2) {
			//jiggle kp (dampening)
			box->kp = cap(randn(box->kp, 2*MR2));
		}

		if (randf(0,1)<MR) {
			//jiggle bias
			box->bias += randn(0, MR2);
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

	if (conf::LEARN_RATE>0 && conns[randconn].sid >= 0 && randf(0,1)<MR/8) {
		//stimulate box weight: boxes that fire together, strengthen
		float stim = out[Output::STIMULANT];
		if (stim > 0.5) {
			//modify weights based on matching old output and old input, if stimulant is active
			float sourceoldout = boxes[boxRef(conns[randconn].sid)].oldout;
			float targetoldout = boxes[boxRef(conns[randconn].tid)].oldout;
			conns[randconn].w += conf::LEARN_RATE * (2*stim - 1) * (1 - abs(targetoldout - sourceoldout));
		}
	}

	if (randf(0,1) < MR/5) {
		//weight wither
		if(randf(0,1) > fabs(conns[randconn].w)) {//the closer to 0 it already is, the higher the chance it gets set to 0
			conns[randconn].w = 0;
			conns[randconn].seed = 0;
			boxes[boxRef(conns[randconn].tid)].seed = 0;
		}
	}

	if (randf(0,1) < MR/8) {
		//relu jiggle
		conns[randconn].relu += randn(0, MR2/8);
	}

	if (randf(0,1) < MR/2) {
		//bias jiggle
		conns[randconn].bias += randn(0, MR2/8);
	}

	if (randf(0,1) < MR) {
		//weight jiggle
		conns[randconn].w += randn(0, MR2/8);
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
		box->bias += randn(0, MR2/4);
		//no need to reset seed for simple bias jiggle
	}

	//NOTE: we do not need to resetBrain here because we should NOT be changing any connections in a hard manner;
	// no deletions, additions, or source/target changes
}

CPBrain CPBrain::crossover(const CPBrain& other)
{
	CPBrain newbrain = *this;
	if (this->conns.size() < other.conns.size()) {
		newbrain = other;
	}

	for (int i = 0; i < (int)newbrain.conns.size(); i++){
		if (i >= other.conns.size() || i >= this->conns.size()) continue; //allow the rest of the conns to be whichever they were inherited from

		int s1 = this->conns[i].seed;
		int s2 = other.conns[i].seed;
		//function which offers probability of which parent to use, based on relative seed counters
		float threshold = ((s1 - s2) / (conf::BRAINSEEDHALFTOLERANCE + abs(s1 - s2)) + 1) / 2;

		if (randf(0,1) < threshold){
			newbrain.conns[i] = this->conns[i];
		} else {
			newbrain.conns[i] = other.conns[i];
		}
		newbrain.conns[i].seed++;
	}

	for (int i = 0; i < (int)newbrain.boxes.size(); i++){
		if (i >= other.boxes.size() || i >= this->boxes.size()) continue; //allowing for variable box counts (NOT IMPLEMENTED)

		int s1 = this->boxes[i].seed;
		int s2 = other.boxes[i].seed;
		//function which offers probability of which parent to use, based on relative seed counters
		float threshold = ((s1 - s2) / (conf::BRAINSEEDHALFTOLERANCE + abs(s1 - s2)) + 1) / 2;

		if(randf(0,1) < threshold){
			newbrain.boxes[i] = this->boxes[i];
		} else {
			newbrain.boxes[i] = other.boxes[i];
		}
		newbrain.boxes[i].seed++;
	}
	newbrain.resetBrain();
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

