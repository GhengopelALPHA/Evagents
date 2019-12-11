//Read/Write module. Built by Julian Hershey, 2012. Use as you wish. Beware of human-made errors!

//The way this works is it includes agent.h, world.h, and glview.h, and is included by glview.cpp.
//I was unable to have this between world.h and glview.h. If you can find a better way to use this,
//please do so and let me know @ julian.hershey@gmail.com

//Currently, GLView has to create an instance of this class whenever saving or loading is needed (see GLView::menu)
//once that is done, the world and other data must be fed to the functions of ReadWrite in order to fully save all data
//It's not pretty, but it works

//Data can be saved in any file format, as the data is put into text form. I tried to emulate XML, but I did not
//strictly follow those rules. There must be a space between the equals sign and the data being loaded, or there
//will be failures. I've made use of a mode var to better control when certain objects and data should be expected.

//For loading agents: I've used <agent> and </agent> tags to outline individual agents. Upon loading, a fake agent
//is created, all attributes which have data to be retrieved are set, and the </agent> tag signals the copy of the
//fake agent's variables into a real agent placed in the world array.

//IN THE FILES: Order does not matter, as each line is read individually from the others. HOWEVER, cell data MUST
//be contained inside <cell>, and agent data MUST be included in its own <agent>. If something goes wrong and 
//two lines modify the same attribute, the latter one will be used.

//VERSION CHECKING: As of now, the system seems to deal with missing data quite harmlessly;
//in the case of agent vars missing, random init data values are assigned; in the case of
//world data, nothing can break down because those values are set by the program. Cells may break since it's more 
//complex, so beware. Even too much data (eg some old data that is no longer used by the program) isn't a
//big issue; simply remove the already removed variable and the line check for it.

#include "ReadWrite.h"

#include "settings.h"
#include "helpers.h"
#include <stdio.h>
#include <iostream>

using namespace std;

ReadWrite::ReadWrite()
{
	ourfile= "WORLD.SAV";
}

void ReadWrite::loadSettings(const char *filename)
{
	char line[64], *pos;
	char var[16];
	char dataval[16];

	//if no filename given, use default
	if(filename=="" || filename==0){
		filename= "settings.txt";
		printf("No filename given. Loading default settings.txt instead.\n");
	}

	//open the file
	FILE* sf = fopen(filename, "r");

	if(sf){
		printf("file exists! loading");
		while(!feof(sf)){
			fgets(line, sizeof(line), sf);
			pos= strtok(line,"\n");
			sscanf(line, "%s%s", var, dataval);
		
		}
	}
}


void ReadWrite::saveAgent(Agent *a, FILE *file)
{
	//NOTE: this method REQUIRES "file" to be opened and closed outside
	fprintf(file, "<a>\n"); //signals the writing of a new agent
//	fprintf(file, "id= %i\n", a->id); //id not loaded
	fprintf(file, "posx= %f\nposy= %f\n", a->pos.x, a->pos.y);
	fprintf(file, "angle= %f\n", a->angle);
	fprintf(file, "health= %f\n", a->health);
	fprintf(file, "gene_red= %f\ngene_gre= %f\ngene_blu= %f\n", a->gene_red, a->gene_gre, a->gene_blu);
	fprintf(file, "chamovid= %f\n", a->chamovid);
	fprintf(file, "sexprojectbias= %f\n", a->sexprojectbias);
	fprintf(file, "herb= %f\n", a->stomach[Stomach::PLANT]);
	fprintf(file, "carn= %f\n", a->stomach[Stomach::MEAT]);
	fprintf(file, "frug= %f\n", a->stomach[Stomach::FRUIT]);
	fprintf(file, "exhaustion= %f\n", a->exhaustion);
	fprintf(file, "species= %i\n", a->species);
	fprintf(file, "radius= %f\n", a->radius);
	fprintf(file, "spike= %f\n", a->spikeLength);
	fprintf(file, "jump= %f\n", a->jump);
	fprintf(file, "dhealth= %f\n", a->dhealth);
	fprintf(file, "age= %i\n", a->age);
	fprintf(file, "gen= %i\n", a->gencount);
	fprintf(file, "hybrid= %i\n", (int) a->hybrid);
	fprintf(file, "cl1= %f\ncl2= %f\n", a->clockf1, a->clockf2);
	fprintf(file, "smellmod= %f\n", a->smell_mod);
	fprintf(file, "hearmod= %f\n", a->hear_mod);
	fprintf(file, "bloodmod= %f\n", a->blood_mod);
	fprintf(file, "eyemod= %f\n", a->eye_see_agent_mod);
//		fprintf(file, "eyecellmod= %f\n", a->eye_see_cell_mod);
	for(int q=0;q<a->eyedir.size();q++) {
		fprintf(file, "<y>\n");
		fprintf(file, "eye#= %i\n", q);
		fprintf(file, "eyedir= %f\n", a->eyedir[q]);
		fprintf(file, "eyefov= %f\n", a->eyefov[q]);
		fprintf(file, "</y>\n");
	}
	for(int q=0;q<NUMEARS;q++) {
		fprintf(file, "<e>\n");
		fprintf(file, "ear#= %i\n", q);
		fprintf(file, "eardir= %f\n", a->eardir[q]);
		fprintf(file, "hearlow= %f\n", a->hearlow[q]);
		fprintf(file, "hearhigh= %f\n", a->hearhigh[q]);
		fprintf(file, "</e>\n");
	}
	fprintf(file, "numbabies= %i\n", a->numbabies);
	fprintf(file, "metab= %f\n", a->metabolism);
	fprintf(file, "repc= %f\n", a->repcounter);
	fprintf(file, "temppref= %f\n", a->temperature_preference);
	fprintf(file, "lungs= %f\n", a->lungs);
//	fprintf(file, "indicator= %f\n", a->indicator);
//	fprintf(file, "ir= %f\nig= %f\nib= %f\n", a->ir, a->ig, a->ib);
//	fprintf(file, "give= %f\n", a->give);
	fprintf(file, "mutrate1= %f\nmutrate2= %f\n", a->MUTRATE1, a->MUTRATE2);
	fprintf(file, "freshkill= %i\n", a->freshkill);
	fprintf(file, "strength= %f\n", a->strength);
	fprintf(file, "<b>\n"); //signals the writing of the brain (more for organization than proper loading)

	for(int b=0;b<a->brain.boxes.size();b++){
		fprintf(file, "<x>\n"); //signals the writing of a specific numbered box
		fprintf(file, "box#= %i\n", b);
		fprintf(file, "kp= %f\n", a->brain.boxes[b].kp);
		fprintf(file, "bias= %f\n", a->brain.boxes[b].bias);
		fprintf(file, "seed= %i\n", a->brain.boxes[b].seed);
		fprintf(file, "globalw= %f\n", a->brain.boxes[b].gw);
		fprintf(file, "target= %f\n", a->brain.boxes[b].target);
		fprintf(file, "out= %f\n", a->brain.boxes[b].out);
		fprintf(file, "oldout= %f\n", a->brain.boxes[b].oldout);
		for(int c=0;c<CONNS;c++){
			fprintf(file, "<n>\n"); //signals the writing of a specific connection for a specific box
			fprintf(file, "conn#= %i\n", c);
			fprintf(file, "type= %i\n", a->brain.boxes[b].type[c]);
			fprintf(file, "w= %f\n", a->brain.boxes[b].w[c]);
			fprintf(file, "cid= %i\n", a->brain.boxes[b].id[c]);
			fprintf(file, "</n>\n"); //end of connection
		}
		fprintf(file, "</x>\n"); //end of box
	}
	fprintf(file, "</b>\n"); //end of brain
	fprintf(file, "</a>\n"); //end of agent
}


void ReadWrite::saveWorld(World *world, float xpos, float ypos, const char *filename)
{
	//Some Notes: When this method is called, it's assumed that filename is not blank or null
	char addressSAV[64];
	char addressREP[64];
	printf("Filename %s given. Saving...\n", filename);

	//set up our save file and report file addresses
	strcpy(addressSAV,"saves\\");
	strcat(addressSAV,filename);
	strcpy(addressREP, addressSAV);
	strcat(addressREP,"_report.txt");

	strcat(addressSAV,".SAV");

	//checking for the presence of the file has been relocated to GLView.cpp to allow GUI interactions

	//open save file, write over any previous
	FILE* fs = fopen(addressSAV, "w");
	
	//start with saving world settings to compare with current
	fprintf(fs,"<world>\n");
	fprintf(fs,"V= %.2f\n",conf::VERSION);
	fprintf(fs,"BRAINSIZE= %i\n", world->BRAINSIZE);
	fprintf(fs,"INPUTS= %i\n", Input::INPUT_SIZE);
	fprintf(fs,"OUTPUTS= %i\n", Output::OUTPUT_SIZE);
	fprintf(fs,"CONNECTIONS= %i\n", CONNS);
	fprintf(fs,"WIDTH= %i\n", conf::WIDTH);
	fprintf(fs,"HEIGHT= %i\n", conf::HEIGHT);
	fprintf(fs,"CELLSIZE= %i\n", conf::CZ); //these saved values up till now are mostly for version control for now
	//save settings which have GUI controls
	fprintf(fs,"MOONLIT= %i\n", world->MOONLIT);
	fprintf(fs,"DROUGHTS= %i\n", world->DROUGHTS);
	fprintf(fs,"DROUGHTMULT= %f\n", world->DROUGHTMULT);
	fprintf(fs,"MUTEVENTS= %i\n", world->MUTEVENTS);
	fprintf(fs,"MUTEVENTMULT= %i\n", world->MUTEVENTMULT);
	fprintf(fs,"CLOSED= %i\n", world->isClosed());
	//if creating more GUI settings to save, please be sure to add them to two sections in GLView. Search ".cfg/.sav"
	fprintf(fs,"epoch= %i\n", world->current_epoch);
	fprintf(fs,"mod= %i\n", world->modcounter);
	fprintf(fs,"xpos= %f\n", xpos);
	fprintf(fs,"ypos= %f\n", ypos); //GLView xtranslate and ytranslate
	for(int cx=0;cx<world->CW;cx++){ //start with the layers
		for(int cy=0;cy<world->CH;cy++){
			float food= world->cells[Layer::PLANTS][cx][cy];
			float meat= world->cells[Layer::MEATS][cx][cy];
			float hazard= world->cells[Layer::HAZARDS][cx][cy];
			float fruit= world->cells[Layer::FRUITS][cx][cy];
			float land= world->cells[Layer::ELEVATION][cx][cy];
			fprintf(fs, "<c>\n"); //signals the writting of a cell
			fprintf(fs, "cx= %i\n", cx);
			fprintf(fs, "cy= %i\n", cy);
			if (food>0) fprintf(fs, "food= %f\n", food); //to further save space, we needn't report a value of a layer for the cell if it's zero
			if (meat>0) fprintf(fs, "meat= %f\n", meat);
			if (hazard>0) fprintf(fs, "hazard= %f\n", hazard);
			if (fruit>0) fprintf(fs, "fruit= %f\n", fruit);
			if (land>0) fprintf(fs, "land= %f\n", land);
			fprintf(fs,"</c>\n");
		}
	}

	for(int i=0; i<(int)world->agents.size(); i++){
		// here we save all agents. All simulation-significant data must be stored, from the pos and angle, to the change in food the bot was experiencing
		Agent* a= &world->agents[i];

		saveAgent(a, fs);
	}
	fprintf(fs,"</world>");
	fclose(fs);


	//now copy over report.txt logs
	char line[1028], *pos;

	//first, check if the last epoch in the saved report matches or is one less than the first one in the current report
	FILE* fr = fopen("report.txt", "r"); 
	FILE* ft = fopen(addressREP, "r"); 
	bool append= false;
	char text[8]; //for checking
	int epoch= 0;
	int maxepoch= 0;

	if(ft){
		while(!feof(ft)){
			fgets(line, sizeof(line), ft);
			pos= strtok(line,"\n");
			sscanf(line, "%s%i%*s", &text, &epoch);
			if (strcmp(text, "Epoch:")==0) {
				if (epoch>maxepoch) maxepoch= epoch;
			}
		}
		fclose(ft);

		if(world->isDebug()) printf("Old report.txt found, and its last epoch was %i.\n", maxepoch);
		
		//now compare the max epoch from original with the first entry of the new
		if(fr){
			fgets(line, sizeof(line), fr);
			pos= strtok(line,"\n");
			sscanf(line, "%s%i%*s", &text, &epoch);
			if (strcmp(text, "Epoch:")==0) {
				//if it is, we append, because this is (very likely) a continuation of that save
				if(world->isDebug())printf("Our report.txt starts at epoch %i. ", epoch);
				if (epoch==maxepoch || epoch==maxepoch+1){
					append= true;
				} else printf("Old report.txt found. Replacing it.\n");
			}
			rewind(fr);
		}
		//otherwise, we overwrite!
	} else {
		if(world->isDebug()) printf("No old report.txt found. Continuing\n");
	}
	
	ft = append ? fopen(addressREP, "a") : fopen(addressREP, "w");
	if(fr){
		if(world->isDebug()) printf("Copying report.txt to save file\n");
		while(!feof(fr)){
			fgets(line, sizeof(line), fr);
			fprintf(ft, line);
		}
	} else {
		printf("report.txt didn\'t exist. That\'s... odd...\n");
	}
	fclose(fr); fclose(ft);
	
	printf("World Saved!\n");
}

void ReadWrite::loadWorld(World *world, float &xtranslate, float &ytranslate, const char *filename)
{
	//Some Notes: When this method is called, it's assumed that filename is not blank or null
	char address[32];
	char line[64], *pos;
	char var[16];
	char dataval[16];
	int cxl= 0;
	int cyl= 0;
	int mode= -1;//loading mode: -1= off, 0= world, 1= cell, 2= agent, 3= box, 4= connection, 5= eyes, 6= ears

	Agent xa(world->BRAINSIZE, world->MEANRADIUS, world->REP_PER_BABY, world->MUTCHANCE, world->MUTSIZE); //mock agent. gets moved and deleted after loading
	bool t1= false, t2= false; //triggers for keeping track of where exactly we are

	int eyenum= -1; //counters
	int earnum= -1;
	int boxnum= -1;
	int connnum= -1;
	int cellmult= 1; //version 0.04 upgrade tool
	int i; //integer buffer
	float f; //float buffer

	strcpy(address,"saves\\");
	strcat(address,filename);
	strcat(address,".SAV");

	FILE *fl;
	fl= fopen(address, "r");
	if(fl){
		printf("file '%s' exists! loading.\n", address);
		while(!feof(fl)){
			fgets(line, sizeof(line), fl);
			pos= strtok(line,"\n");
			sscanf(line, "%s%s", var, dataval);

			if(mode==-1){ //mode @ -1 = off
				if(strcmp(var, "<world>")==0){ //strcmp= 0 when the arguements equal
					//if we find a <world> tag, enable world loading and reset. simple
					mode= 0;
					world->reset();
					printf("discovered world.\n"); //report status
				}else if(strcmp(var, "<a>")==0){
					//if we find an <agent> tag, jump right to agent loading (for spawning saved agents)
					mode= 2;
				}
			}else if(mode==0){ //mode @ 0 = world
				if(strcmp(var, "V=")==0){
					//version number
					sscanf(dataval, "%f", &f);
					if(f!=conf::VERSION) {
						printf("ALERT: Version Number different! Issues may occur!\n");
						if(f<=0.03) {
							printf("ALERT: version number < 0.04 detected! Multiplying all cell values by 2!\n");
							cellmult= 2;
						}
					}
				}else if(strcmp(var, "BRAINSIZE=")==0){
					sscanf(dataval, "%i", &i);
					world->BRAINSIZE= i;
				}else if(strcmp(var, "INPUTS=")==0){
					sscanf(dataval, "%i", &i);
					if(i!=Input::INPUT_SIZE) {
						printf("ALERT: Brain Input size different! Issues may occur!\n");
					}
				}else if(strcmp(var, "OUTPUTS=")==0){
					sscanf(dataval, "%i", &i);
					if(i!=Output::OUTPUT_SIZE) {
						printf("ALERT: Brain Output size different! Issues may occur!\n");
					}
				}else if(strcmp(var, "CONNECTIONS=")==0){
					sscanf(dataval, "%i", &i);
					if(i!=CONNS) {
						printf("ALERT: Brain CONNS per brain box different! Issues may occur!\n");
					}
				}else if(strcmp(var, "WIDTH=")==0){
					//this WILL be loaded soon
					sscanf(dataval, "%i", &i);
					if(i!=conf::WIDTH) {
						printf("ALERT: World Width different! Issues may occur!\n");
					}
				}else if(strcmp(var, "HEIGHT=")==0){
					//this WILL be loaded soon
					sscanf(dataval, "%i", &i);
					if(i!=conf::HEIGHT) {
						printf("ALERT: World Height different! Issues may occur!\n");
					}
				}else if(strcmp(var, "CELLSIZE=")==0){
					//this may be loaded soon
					sscanf(dataval, "%i", &i);
					if(i!=conf::CZ) {
						printf("ALERT: Cell Size different! Issues may occur!\n");
					}
				}else if(strcmp(var, "MOONLIT=")==0){
					sscanf(dataval, "%i", &i);
					if(i==1) world->MOONLIT= true;
					else world->MOONLIT= false;
				}else if(strcmp(var, "DROUGHTS=")==0){
					sscanf(dataval, "%i", &i);
					if(i>=1) world->DROUGHTS= true;
					else world->DROUGHTS= false;
				}else if(strcmp(var, "DROUGHTMULT=")==0){
					sscanf(dataval, "%f", &f);
					world->DROUGHTMULT= f;
					if(i>9000) world->DROUGHTMULT= randf(0,2);
				}else if(strcmp(var, "MUTEVENTS=")==0){
					sscanf(dataval, "%i", &i);
					if(i>=1) world->MUTEVENTS= true;
					else world->MUTEVENTS= false;
				}else if(strcmp(var, "MUTEVENTMULT=")==0){
					sscanf(dataval, "%i", &i);
					world->MUTEVENTMULT= i;
//				}else if(strcmp(var, "connections=")==0){
					//conns count; NOT CURRENTLY USED
//				}else if(strcmp(var, "width=")==0){
					//world width; NOT CURRENTLY USED
//					sscanf(dataval, "%i", &i);
//					conf::WIDTH= i;
//				}else if(strcmp(var, "height=")==0){
					//world height; NOT CURRENTLY USED
//					sscanf(dataval, "%i", &i);
//					conf::HEIGHT= i;
//				}else if(strcmp(var, "cellsize=")==0){
					//CZ; NOT CURRENTLY USED
//					sscanf(dataval, "%i", &i);
//					conf::CZ= i;
				}else if(strcmp(var, "epoch=")==0){
					//epoch
					sscanf(dataval, "%i", &i);
					world->current_epoch= i;
				}else if(strcmp(var, "mod=")==0){
					//mod count
					sscanf(dataval, "%i", &i);
					world->modcounter= i;
				}else if(strcmp(var, "CLOSED=")==0){
					//closed state
					sscanf(dataval, "%i", &i);
					if (i==1) world->setClosed(true);
					else world->setClosed(false);
//				}else if(strcmp(var, "PAUSED=")==0){
//					//Paused state (always saves as false)
//					sscanf(dataval, "%i", &i);
//					if (i==1) ;
//					else ;
				}else if(strcmp(var, "xpos=")==0){
					//veiw screen location x
					sscanf(dataval, "%f", &f);
					xtranslate= f;
				}else if(strcmp(var, "ypos=")==0){
					//veiw screen location y
					sscanf(dataval, "%f", &f);
					ytranslate= f;
				}else if(strcmp(var, "<c>")==0){
					//cells tag activates cell reading mode
					mode= 1;
				}else if(strcmp(var, "<a>")==0){
					//agent tag activates agent reading mode
					mode= 2;
					if (!t1) printf("loaded cells.\n"); //report status
					t1= true;
				}
			}else if(mode==1){ //mode @ 1 = cell
				if(strcmp(var, "</c>")==0){
					//end_cell tag is checked for first, because of else condition
					mode= 0;
				}else if(strcmp(var, "cx=")==0){
					sscanf(dataval, "%i", &i);
					cxl= i;
				}else if(strcmp(var, "cy=")==0){
					sscanf(dataval, "%i", &i);
					cyl= i;
				}else if(strcmp(var, "food=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[Layer::PLANTS][cxl][cyl]= f*cellmult; //version 0.04 upgrade tool
				}else if(strcmp(var, "meat=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[Layer::MEATS][cxl][cyl]= f*cellmult; //version 0.04 upgrade tool
				}else if(strcmp(var, "hazard=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[Layer::HAZARDS][cxl][cyl]= f*cellmult; //version 0.04 upgrade tool
				}else if(strcmp(var, "fruit=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[Layer::FRUITS][cxl][cyl]= f*cellmult; //version 0.04 upgrade tool
				}else if(strcmp(var, "land=")==0){
					sscanf(dataval, "%f", &f);
					world->cells[Layer::ELEVATION][cxl][cyl]= f;
				}			
			}else if(mode==2){ //mode @ 2 = agent
				if(strcmp(var, "</a>")==0){
					//end_agent tag is checked for, and when found, copies agent xa
					mode= 0;
					Agent loadee= xa;
					loadee.id= world->idcounter;
					world->idcounter++;
					world->agents.push_back(loadee);
					if(!t2 && randf(0,1)>world->NUMBOTS/world->agents.size()){
						printf("halfway there.\n");
						t2= true;
					}
				}else if(strcmp(var, "posx=")==0){
					sscanf(dataval, "%f", &f);
					xa.pos.x= f;
				}else if(strcmp(var, "posy=")==0){
					sscanf(dataval, "%f", &f);
					xa.pos.y= f;
				}else if(strcmp(var, "angle=")==0){
					sscanf(dataval, "%f", &f);
					xa.angle= f;
				}else if(strcmp(var, "health=")==0){
					sscanf(dataval, "%f", &f);
					xa.health= f;
				}else if(strcmp(var, "gene_red=")==0){
					sscanf(dataval, "%f", &f);
					xa.gene_red= f;
				}else if(strcmp(var, "gene_gre=")==0){
					sscanf(dataval, "%f", &f);
					xa.gene_gre= f;
				}else if(strcmp(var, "gene_blu=")==0){
					sscanf(dataval, "%f", &f);
					xa.gene_blu= f;
				}else if(strcmp(var, "chamovid=")==0){
					sscanf(dataval, "%f", &f);
					xa.chamovid= f;
				}else if(strcmp(var, "sexprojectbias=")==0){
					sscanf(dataval, "%f", &f);
					xa.sexprojectbias= f;
				}else if(strcmp(var, "herb=")==0){
					sscanf(dataval, "%f", &f);
					xa.stomach[Stomach::PLANT]= f;
				}else if(strcmp(var, "carn=")==0){
					sscanf(dataval, "%f", &f);
					xa.stomach[Stomach::MEAT]= f;
				}else if(strcmp(var, "frug=")==0){
					sscanf(dataval, "%f", &f);
					xa.stomach[Stomach::FRUIT]= f;
				}else if(strcmp(var, "exhaustion=")==0){
					sscanf(dataval, "%f", &f);
					xa.exhaustion= f;
				}else if(strcmp(var, "species=")==0){
					sscanf(dataval, "%i", &i);
					xa.species= i;
				}else if(strcmp(var, "radius=")==0){
					sscanf(dataval, "%f", &f);
					xa.radius= f;
				}else if(strcmp(var, "spike=")==0){
					sscanf(dataval, "%f", &f);
					xa.spikeLength= f;
				}else if(strcmp(var, "jump=")==0){
					sscanf(dataval, "%f", &f);
					xa.jump= f;
				}else if(strcmp(var, "dfood=")==0 || strcmp(var, "dhealth=")==0){
					sscanf(dataval, "%f", &f);
					xa.dhealth= f;
				}else if(strcmp(var, "age=")==0){
					sscanf(dataval, "%i", &i);
					xa.age= i;
				}else if(strcmp(var, "gen=")==0){
					sscanf(dataval, "%i", &i);
					xa.gencount= i;
				}else if(strcmp(var, "hybrid=")==0){
					sscanf(dataval, "%i", &i);
					if(i==1) xa.hybrid= true;
					else xa.hybrid= false;
				}else if(strcmp(var, "cl1=")==0){
					sscanf(dataval, "%f", &f);
					xa.clockf1= f;
				}else if(strcmp(var, "cl2=")==0){
					sscanf(dataval, "%f", &f);
					xa.clockf2= f;
				}else if(strcmp(var, "smellmod=")==0){
					sscanf(dataval, "%f", &f);
					xa.smell_mod= f;
				}else if(strcmp(var, "hearmod=")==0){
					sscanf(dataval, "%f", &f);
					xa.hear_mod= f;
				}else if(strcmp(var, "bloodmod=")==0){
					sscanf(dataval, "%f", &f);
					xa.blood_mod= f;
				}else if(strcmp(var, "eyemod=")==0){
					sscanf(dataval, "%f", &f);
					xa.eye_see_agent_mod= f;
//				}else if(strcmp(var, "eyecellmod=")==0){
//					sscanf(dataval, "%f", &f);
//					xa.eye_see_cell_mod= f;
				}else if(strcmp(var, "numbabies=")==0){
					sscanf(dataval, "%i", &i);
					xa.numbabies= i;
				}else if(strcmp(var, "metab=")==0){
					sscanf(dataval, "%f", &f);
					xa.metabolism= f;
				}else if(strcmp(var, "repc=")==0){
					sscanf(dataval, "%f", &f);
					xa.repcounter= f;
				}else if(strcmp(var, "temppref=")==0){
					sscanf(dataval, "%f", &f);
					xa.temperature_preference= f;
				}else if(strcmp(var, "lungs=")==0){
					sscanf(dataval, "%f", &f);
					xa.lungs= f;
				}else if(strcmp(var, "mutrate1=")==0){
					sscanf(dataval, "%f", &f);
					xa.MUTRATE1= f;
				}else if(strcmp(var, "mutrate2=")==0){
					sscanf(dataval, "%f", &f);
					xa.MUTRATE2= f;
				}else if(strcmp(var, "freshkill=")==0){
					sscanf(dataval, "%i", &i);
					xa.freshkill= i;
				}else if(strcmp(var, "strength=")==0){
					sscanf(dataval, "%f", &f);
					xa.strength= f;
				}else if(strcmp(var, "<y>")==0){
					mode= 5; //eye mode
				}else if(strcmp(var, "<x>")==0){
					mode= 3; //brain mode
				}else if(strcmp(var, "<e>")==0){
					mode= 6; //ear mode
				}
			}else if(mode==5){ //mode @ 5 = eye (of agent)
				if(strcmp(var, "</y>")==0){
					mode= 2;
				}else if(strcmp(var, "eye#=")==0){
					sscanf(dataval, "%i", &i);
					eyenum= i;
				}else if(strcmp(var, "eyedir=")==0){
					sscanf(dataval, "%f", &f);
					xa.eyedir[eyenum]= f;
				}else if(strcmp(var, "eyefov=")==0){
					sscanf(dataval, "%f", &f);
					xa.eyefov[eyenum]= f;
				}
			}else if(mode==6){ //mode @ 6 = ear (of agent)
				if(strcmp(var, "</e>")==0){
					mode= 2;
				}else if(strcmp(var, "ear#=")==0){
					sscanf(dataval, "%i", &i);
					earnum= i;
				}else if(strcmp(var, "eardir=")==0){
					sscanf(dataval, "%f", &f);
					xa.eardir[earnum]= f;
				}else if(strcmp(var, "hearlow=")==0){
					sscanf(dataval, "%f", &f);
					xa.hearlow[earnum]= f;
				}else if(strcmp(var, "hearhigh=")==0){
					sscanf(dataval, "%f", &f);
					xa.hearhigh[earnum]= f;
				}
			}else if(mode==3){ //mode @ 3 = brain box (of agent)
				if(strcmp(var, "</x>")==0){
					mode= 2;
				}else if(strcmp(var, "box#=")==0){
					sscanf(dataval, "%i", &i);
					boxnum= i;
				}else if(strcmp(var, "seed=")==0){
					sscanf(dataval, "%i", &i);
					xa.brain.boxes[boxnum].seed= i;
				}else if(strcmp(var, "kp=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].kp= f;
				}else if(strcmp(var, "bias=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].bias= f;
				}else if(strcmp(var, "globalw=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].gw= f;
				}else if(strcmp(var, "target=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].target= f;
				}else if(strcmp(var, "out=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].out= f;
				}else if(strcmp(var, "oldout=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].oldout= f;
				}else if(strcmp(var, "<n>")==0){
					mode= 4;
				}
			}else if(mode==4){ //mode @ 4 = connection (of brain box of agent)
				if(strcmp(var, "</n>")==0){
					mode= 3;
				}else if(strcmp(var, "conn#=")==0){
					sscanf(dataval, "%i", &i);
					connnum= i;
				}else if(strcmp(var, "type=")==0){
					sscanf(dataval, "%i", &i);
					xa.brain.boxes[boxnum].type[connnum]= i;
				}else if(strcmp(var, "w=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].w[connnum]= f;
				}else if(strcmp(var, "cid=")==0){
					sscanf(dataval, "%f", &f);
					xa.brain.boxes[boxnum].id[connnum]= f;
				}
			}
		}
		printf("loaded agents.\n"); //report status
		fclose(fl);

		printf("WORLD LOADED!\n");

		world->setInputs();
		world->brainsTick();
		world->processOutputs(true);
		world->processOutputs(true);
		world->processOutputs(true);
		world->processOutputs(true);
		world->processOutputs(true);

	} else { //DOH! the file doesn't exist!
		printf("ERROR: Save file specified, '%s' doesn't exist!\n", address);
	}
}
