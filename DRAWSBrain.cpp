#include "DRAWSBrain.h"
using namespace std;


DRAWSBox::DRAWSBox(int numboxes)
{

	w.resize(CONNS,0);
	id.resize(CONNS,0);
	type.resize(CONNS,0);

	//constructor
	for (int i=0;i<CONNS;i++) {
		w[i]= randf(-10,10);
		if(randf(0,1)<conf::BRAIN_DEADCONNS) w[i]=0; //we might want to simulate brain development over time, so set some conns to zero weight
		
		id[i]= randi(0,numboxes);
		if (randf(0,1)<conf::BRAIN_DIRECTINPUT) id[i]= randi(0,Input::INPUT_SIZE); //connect a portion of the brain directly to input.
		
		type[i]= 0;
		if(randf(0,1)<conf::BRAIN_CHANGECONNS) type[i] = 1; //some conns can be change sensitive synapses
		if(randf(0,1)<conf::BRAIN_MEMCONN) type[i] = 2; //some conns can be memory synapses
	}

	seed= 0;
	kp= randf(0.01,1);
	gw= randf(-2,2);
	bias= randf(-1,1);

	out= 0;
	oldout= 0;
	target= 0;
}

DRAWSBrain::DRAWSBrain(int numboxes)
{
	//constructor
	// do not OMP!!!
	for (int i=0;i<numboxes;i++) {
		DRAWSBox a = DRAWSBox(numboxes); //make a random box and copy it over
		boxes.push_back(a);
		}

	//do other initializations
	init();
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


void DRAWSBrain::init()
{

}

void DRAWSBrain::tick(vector< float >& in, vector< float >& out)
{
	//do a single tick of the brain
	for (int j=0; j<(int)boxes.size(); j++){
		DRAWSBox* abox= &boxes[j];
		
		if (j<Input::INPUT_SIZE) { //take first few boxes and set their out to in[]. (no need to do these separately, since thay are first)
			abox->out= in[j];
		} else { //then do a dynamics tick and set all targets
			float acc=abox->bias;

			for (int k=0;k<CONNS;k++) {
				int idx=abox->id[k];
				int type = abox->type[k];
				float val= boxes[idx].out;

				if(type==2){ //switch conn
					if(val>0.5){
						break;
						continue;
					}
					continue;
				}
				
				if(type==1){ //change sensitive conn
					val-= boxes[idx].oldout;
					val*=10;
				}

				acc+= val*abox->w[k];
			}
			
			acc*= abox->gw;
			
			//put through sigmoid
			acc= 1.0/(1.0+exp(-acc));
			
			abox->target= cap(acc);
		}
	}
	

	for (int j=0; j<(int)boxes.size(); j++){
		DRAWSBox* abox= &boxes[j];

		//back up current out for each box
		abox->oldout = abox->out;

		//make all boxes go a bit toward target
		if (j>=Input::INPUT_SIZE) abox->out+= (abox->target-abox->out)*abox->kp;
	}

	//finally set out[] to the last few boxes output
	for (int j=0;j<Output::OUTPUT_SIZE;j++) {
		//jump has different responce because we've made it into a change sensitive output
		if (j==Output::JUMP) out[j]= cap(10*(boxes[boxes.size()-1-j].out-boxes[boxes.size()-1-j].oldout));
		else out[j]= boxes[boxes.size()-1-j].out;
	}
}

float DRAWSBrain::getActivity()
{
	float sum= 0;
	for (int j=0; j<(int)boxes.size(); j++){
		DRAWSBox* abox= &boxes[j];
		sum+= fabs(abox->out - abox->oldout);
	}
	return sum/boxes.size();
}

void DRAWSBrain::initMutate(float MR, float MR2)
{
	//for mutations which may occur at conception
	for (int j=0; j<(int)boxes.size(); j++){
		DRAWSBox* abox= &boxes[j];
		if (randf(0,1)<MR/50) {
			//randomize synapse type
			int rc= randi(0, CONNS);
			abox->type[rc] = randi(0,2);
//		  a2.mutations.push_back("synapse type randomized\n");
			abox->seed= 0;
		}

		if (randf(0,1)<MR/40) {
			//copy box
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

		if (randf(0,1)<MR/20) {
			//randomize connection
			int rc= randi(0, CONNS);
			int ri= randi(0,boxes.size());
			abox->id[rc]= ri;
//		  a2.mutations.push_back("connection randomized\n");
			abox->seed= 0;
		}

		if (randf(0,1)<MR/10) {
			//swap two input sources
			int rc1= randi(0, CONNS);
			int rc2= randi(0, CONNS);
			int temp= abox->id[rc1];
			abox->id[rc1]= abox->id[rc2];
			abox->id[rc2]= temp;
//			 a2.mutations.push_back("inputs swapped\n");
			abox->seed= 0;
		}

		// more likely changes here
		if (randf(0,1)<MR/2) {
			//jiggle global weight
			abox->gw+= randn(0, MR2);
		}

		if (randf(0,1)<MR) {
			//jiggle bias
			abox->bias+= randn(0, MR2*5);
		}

		if (randf(0,1)<MR) {
			//jiggle dampening
			abox->kp+= randn(0, MR2);
			if (abox->kp<0.01) abox->kp=0.01;
			if (abox->kp>1) abox->kp=1;
		}

		if (randf(0,1)<MR) {
			//jiggle weight
			int rc= randi(0, CONNS);
			abox->w[rc]+= randn(0, MR2*5);
		}
	}
}

void DRAWSBrain::liveMutate(float MR, float MR2, vector<float>& out)
{
	//for mutations which may occur while the bot is live
	int j= randi(0,boxes.size());
	DRAWSBox* abox= &boxes[j];

	if (randf(0,1)<MR) {
		//"neurons that fire together, wire together". Hebb process
		int rc= randi(0, CONNS);
		int b= -1;
		while (b<=-1){
			b-= 1;
			int rb= randi(0,boxes.size());
			if (abs(boxes[rb].oldout-abox->out)<=0.01) b= rb;
			if (b<=-100) break;
		}
		if (b>=0){
			abox->id[rc]= b;
			abox->seed= 0;
		}
	}

	if (randf(0,1)<MR) {
		//stimulate box weight
		float stim= out[Output::STIMULANT];
		if(stim>0.5){
			for (int k=0;k<CONNS;k++) {
				//modify weights based on matching old output and new input, if stimulant is active
				float val= boxes[abox->id[k]].out;
				abox->w[k]+= conf::LEARNRATE*stim*(abox->oldout-(1-val));
			}
		}
//		abox->seed= 0;
	}

	if (randf(0,1)<MR*5) {
		//jiggle bias
		abox->bias+= randn(0, MR2*10);
	}

	if (randf(0,1)<MR*5) {
		//jiggle dampening
		abox->kp+= randn(0, MR2*10);
		if (abox->kp<0.01) abox->kp=0.01;
		if (abox->kp>1) abox->kp=1;
	}

	if (randf(0,1)<MR*10) {
		//jiggle weight
		int rc= randi(0, CONNS);
		abox->w[rc]+= randn(0, MR2*10);
	}
}

DRAWSBrain DRAWSBrain::crossover(const DRAWSBrain& other)
{
	DRAWSBrain newbrain(*this);
	
	#pragma omp parallel for
	for (int i=0; i<(int)newbrain.boxes.size(); i++){
		if(i>=other.boxes.size()) continue; //hack: skip all the other brain's boxes if newbrain is smaller
		//this brings me to an important question regarding brain mutations. How should we manage mis-matching brains?
		//should we add new boxes from the larger, or forget them? If we add them, should they be added to the front? to the back?
		//somewhere in the middle? There are issues with each "solution" that I'm not comfortable with.
		int s1= this->boxes[i].seed;
		int s2= other.boxes[i].seed;
		//function which offers pobability of which parent to use, based on relative seed counters
		float threshold= ((s1-s2)/(1+abs(s1-s2))+1)/2;

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

