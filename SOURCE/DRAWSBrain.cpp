#include "DRAWSBrain.h"
using namespace std;


DRAWSBox::DRAWSBox(int maxid)
{

	w.resize(CONNS,0);
	id.resize(CONNS,0);
	type.resize(CONNS,0);

	//constructor
	for (int i=0;i<CONNS;i++) {
		if(randf(0,1)<conf::BRAIN_DEADCONNS) w[i]=0; //we want to simulate brain development over time, so set some conns to zero weight
		else w[i]= randn(0,2);
		
		if (randf(0,1)<conf::BRAIN_DIRECTINPUTS) id[i]= randi(0,Input::INPUT_SIZE); //connect a portion of the brain directly to input.
		else id[i]= randi(0,maxid);

		type[i]= 0;
		if(randf(0,1)<conf::BRAIN_CHANGECONNS) type[i] = 1; //some conns can be change sensitive synapses
		if(randf(0,1)<conf::BRAIN_MEMCONNS) type[i] = 2; //some conns can be memory synapses
	}

	seed= 0;
	kp= randf(0,1);
	gw= randf(-2,2);
	bias= randf(-5,5);

	out= 0;
	oldout= 0;
	target= 0;
}

DRAWSBox::DRAWSBox(){
}

DRAWSBrain::DRAWSBrain(){
}

DRAWSBrain::DRAWSBrain(int numboxes)
{
	//constructor
	// do not OMP!!!
	for (int i=0;i<numboxes;i++) {
		DRAWSBox a = DRAWSBox(i+1); //make a random box and copy it over
		boxes.push_back(a);
	}
}

DRAWSBrain::DRAWSBrain(const DRAWSBrain& other)
{
	boxes= other.boxes;
}

DRAWSBrain& DRAWSBrain::operator=(const DRAWSBrain& other)
{
	if( this != &other )
		boxes = other.boxes;
	return *this;
}

void DRAWSBrain::tick(vector< float >& in, vector< float >& out)
{
	//do a single tick of the brain, for each box
	for (int j=0; j<(int)boxes.size(); j++){
		DRAWSBox* abox= &boxes[j];
		
		//fist, calculate the target values of the box
		if (j<Input::INPUT_SIZE) { //take first few boxes and set their out to in[] and don't do any more value calc
			abox->out= in[j];
		} else { //then do a dynamics tick
			float acc= abox->bias; //start with bias of box

			for (int k=0;k<CONNS;k++) { //for each possible connection...
				if(abox->w[k]==0) continue; //help out processing by skipping if weight is exactly 0

				int idx=abox->id[k];
				int type = abox->type[k];
				float val= boxes[idx].out;

				if(type==2){ //switch conn. If the input*w is >0.5, it freezes all later inputs to the box and finalizes sum, otherwise it's skipped
					if(val*abox->w[k]>0.5){
						break;
						continue;
					}
					continue;
				}
				
				if(type==1){ //change sensitive conn compares to old value, and gets magnified by *10 (arbitrary)
					//we do this AFTER type==2 because switch conn just checks the normal val
					val-= boxes[idx].oldout;
					val*=100;
				}

				//multiply by weight and add to the accumulation
				acc+= val*abox->w[k];
			}
			
			//multiply by global weight before sigmoiding it
			acc*= abox->gw;
			
			//put through sigmoid. very negative values -> 0, very positive values -> 1, values close to 0 -> near 0.5
			acc= 1.0/(1.0+exp(-acc));
			
			//apply as target
			abox->target= acc;
		}
	}
	

	for (int j=0; j<(int)boxes.size(); j++){
		DRAWSBox* abox= &boxes[j];

		//back up current out for each box
		abox->oldout = abox->out;

		//make all boxes go a bit toward target, with dampening applied
		if (j>=Input::INPUT_SIZE) abox->out+= (abox->target-abox->out)*abox->kp;
	}

	//finally set out[] to the last few boxes output
	for (int j=0;j<Output::OUTPUT_SIZE;j++) {
		//jump has different responce because we've made it into a change sensitive output
		int sourcebox= boxes.size()-1-j;

		if (j==Output::JUMP) out[j]= cap(100*(boxes[sourcebox].out-boxes[sourcebox].oldout));
		else out[j]= boxes[sourcebox].out;
	}
}

float DRAWSBrain::getActivityRatio() const
{
	float sum= 0;
	for (int j=Input::INPUT_SIZE; j<(int)boxes.size(); j++){
		sum+= fabs(boxes[j].out - boxes[j].oldout);
	}
	return sum/(boxes.size()-Input::INPUT_SIZE);
}

float DRAWSBrain::getNonZeroWRatio() const
{
	float sum= 0;
	for (int j=Input::INPUT_SIZE; j<(int)boxes.size(); j++){
		for (int k=0;k<CONNS;k++) {
			if(boxes[j].w[k]==0) continue;
			else sum++;
		}
	}
	return sum/(boxes.size()-Input::INPUT_SIZE)/CONNS;
}

void DRAWSBrain::initMutate(float MR, float MR2)
{
	//for mutations which may occur at conception
	for (int j=Input::INPUT_SIZE; j<(int)boxes.size(); j++){
		DRAWSBox* abox= &boxes[j];

		float seedfactor= 0.01*abox->seed+1;

		//start with rare mutations
		if (randf(0,1)*seedfactor<MR/10) {
			//randomize synapse type
			int rc= randi(0, CONNS);
			abox->type[rc] = randi(0,2);
//		  a2.mutations.push_back("synapse type randomized\n");
			abox->seed= 0;
		}

		if (randf(0,1)*seedfactor<MR/5) {
			//copy another box
			int k= randi(0,boxes.size());
			if(k!=j) {
				abox->type= boxes[k].type;
				abox->id= boxes[k].id;
				abox->bias= boxes[k].bias;
				abox->kp= boxes[k].kp;
				abox->type= boxes[k].type;
				abox->w= boxes[k].w;
//				a2.mutations.push_back("box coppied\n");
				abox->seed= 0;
			}
		}

		if (randf(0,1)*seedfactor<MR/5) {
			//branch box (sets a conn to reference a box which refers the same as another conn's box's references)
			//eg: [j]
			//    / \+create this conn
			//  [b1][bt]
			//    \ /
			//    [b2]
			//pick random conns
			int c1= randi(0,CONNS); //j's conn to b1
			int b1= abox->id[c1];
			int c2= randi(0,CONNS); //b1's conn to b2
			int b2= boxes[b1].id[c2];

			int bt= -1;
			//scan boxes...
			for (int b=0; b<(int)boxes.size(); b++){
				if(b==j || b==b1 || b==b2) continue;
				
				for (int k=0;k<CONNS;k++) {
					if(boxes[b].id[k]==b2){
						bt= b2;
						break;
						break;
					}
				}
			}
			if(bt>=0){
				int ct= c1;
				while(ct==c1) ct= randi(0, CONNS);
				abox->id[ct]= bt;
//				a2.mutations.push_back("box branched\n");
				abox->seed= 0;
			}
		}

		if (randf(0,1)*seedfactor<MR/2) {
			//randomize connection
			int rc= randi(0, CONNS);
			int ri= randi(0,boxes.size());
			abox->id[rc]= ri;
//		  a2.mutations.push_back("connection randomized\n");
			abox->seed= 0;
		}

		if (randf(0,1)*seedfactor<MR/2) {
			//swap two input sources
			int rc1= randi(0, CONNS);
			int rc2= randi(0, CONNS);
			int temp= abox->id[rc1];
			abox->id[rc1]= abox->id[rc2];
			abox->id[rc2]= temp;
//			 a2.mutations.push_back("inputs swapped\n");
			abox->seed= 0;
		}

		if (randf(0,1)*seedfactor<MR) {
			//mirror an input weight
			int rc1= randi(0, CONNS);
			int rc2= randi(0, CONNS);
			abox->w[rc1]= -abox->w[rc2];
//			 a2.mutations.push_back("input mirrored\n");
			abox->seed= 0;
		}

		// more likely changes here
		if (randf(0,1)*seedfactor<MR*2) {
			//jiggle global weight
			abox->gw+= randn(0, MR2);
		}

		if (randf(0,1)*seedfactor<MR*2) {
			//jiggle bias
			abox->bias+= randn(0, MR2*5);
		}

		if (randf(0,1)*seedfactor<MR*2) {
			//jiggle dampening
			abox->kp+= randn(0, MR2*0.5);
			abox->kp= cap(abs(abox->kp));
		}

		if (randf(0,1)*seedfactor<MR*3) {
			//wither connection
			int rc= randi(0, CONNS);
			if(randf(0,1)>fabs(abox->w[rc])) abox->w[rc]= 0; //the closer the weight is to 0, the more likely it withers
//		  a2.mutations.push_back("connection withered\n");
		}

		if (randf(0,1)*seedfactor<MR*5) {
			//jiggle weight
			int rc= randi(0, CONNS);
			abox->w[rc]+= randn(0, MR2*5);
		}
	}
}

void DRAWSBrain::liveMutate(float MR, float MR2, vector<float>& out)
{
	//for mutations which may occur while the bot is live
	int j= randi(Input::INPUT_SIZE,boxes.size());
	DRAWSBox* abox= &boxes[j];

	float seedfactor= 0.01*abox->seed+1;

	if (randf(0,1)*seedfactor<MR/5) {
		//"neurons that fire together, wire together". Hebb process
		int rc= randi(0, CONNS);
		int b= -1;
		while (b<=-1){
			int rb= randi(0,boxes.size());
			if (abs(boxes[rb].oldout-abox->out)<=0.01) {
				b= rb;
				break;
			}
			if (b<-100) break;
			b-= 1;
		}
		if (b>=0){
			abox->id[rc]= b;
			abox->seed= 0;
		}
	}

	if (randf(0,1)*seedfactor<MR && conf::LEARNRATE>0) {
		//stimulate box weight
		float stim= out[Output::STIMULANT];
		if(stim>0.5){
			for (int k=0;k<CONNS;k++) {
				//modify weights based on matching old output and old input, if stimulant is active
				float oldin= boxes[abox->id[k]].oldout;
				abox->w[k]+= conf::LEARNRATE*stim*(abox->oldout-(1-oldin));
			}
		}
	}

	if (randf(0,1)*seedfactor<MR*5) {
		//jiggle bias
		abox->bias+= randn(0, MR2*10);
	}

	if (randf(0,1)*seedfactor<MR*5) {
		//jiggle dampening
		abox->kp+= randn(0, MR2*2);
		abox->kp= cap(abs(abox->kp));
	}

	if (randf(0,1)*seedfactor<MR*10) {
		//wither connection
		int rc= randi(0, CONNS);
		if(randf(0,1)>fabs(abox->w[rc])) abox->w[rc]= 0; //the closer the weight is to 0, the more likely it withers
	}

	if (randf(0,1)*seedfactor<MR*10) {
		//jiggle weight
		int rc= randi(0, CONNS);
		abox->w[rc]+= randn(0, MR2*10);
	}
}

DRAWSBrain DRAWSBrain::crossover(const DRAWSBrain& other)
{
	DRAWSBrain newbrain(*this);
	
	#pragma omp parallel for
	for (int i=Input::INPUT_SIZE; i<(int)newbrain.boxes.size(); i++){
		if(i>=other.boxes.size()) continue; //hack: skip all the other brain's boxes if newbrain is smaller
		//this brings me to an important question regarding brain mutations. How should we manage mis-matching brains?
		//should we add new boxes from the larger, or forget them? If we add them, should they be added to the front? to the back?
		//somewhere in the middle? There are issues with each "solution" that I'm not comfortable with.
		int s1= this->boxes[i].seed;
		int s2= other.boxes[i].seed;
		//function which offers pobability of which parent to use, based on relative seed counters
		float threshold= ((s1-s2)/(conf::BRAINSEEDHALFTOLERANCE+abs(s1-s2))+1)/2;

		if(randf(0,1)<threshold){
			newbrain.boxes[i].bias= this->boxes[i].bias;
			newbrain.boxes[i].gw= this->boxes[i].gw;
			newbrain.boxes[i].kp= this->boxes[i].kp;
			newbrain.boxes[i].seed= this->boxes[i].seed + 1;
//			this->boxes[i].seed += 1; //reward the copied box
			for (int j=0; j<CONNS; j++){
				newbrain.boxes[i].id[j] = this->boxes[i].id[j];
				newbrain.boxes[i].w[j] = this->boxes[i].w[j];
				newbrain.boxes[i].type[j] = this->boxes[i].type[j];
			}
		
		} else {
			newbrain.boxes[i].bias= other.boxes[i].bias;
			newbrain.boxes[i].gw= other.boxes[i].gw;
			newbrain.boxes[i].kp= other.boxes[i].kp;
			newbrain.boxes[i].seed= other.boxes[i].seed + 1;
//			other.boxes[i].seed += 1;
			for (int j=0; j<CONNS; j++){
				newbrain.boxes[i].id[j] = other.boxes[i].id[j];
				newbrain.boxes[i].w[j] = other.boxes[i].w[j];
				newbrain.boxes[i].type[j] = other.boxes[i].type[j];
			}
		}
	}
	return newbrain;
}

std::vector<int> DRAWSBrain::traceBack(int outback)
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
}

