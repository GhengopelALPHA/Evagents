#include "UIElement.h"



	//event log display
	float ss= 18;
	float movezero= 0;
	int fadetime= conf::EVENTS_HALFLIFE-20;

	int count= world->events.size();
	if(conf::EVENTS_DISP<count) count= conf::EVENTS_DISP;

	for(int io= 0; io<count; io++){
		int counter= world->events[io].second;

		float fade= cap((-abs((float)counter)+conf::EVENTS_HALFLIFE)/20);

		float move= 0;


		if(counter>fadetime) move= powf(((float)counter-fadetime)/7.4,3);
		else if (counter<-fadetime) move= powf(((float)counter+fadetime)/7.4,3);

		if(io==0){
			movezero= move;
			//move= 0;
		}
		glBegin(GL_QUADS);
		glColor4f(0.8,0.8,0.8,0.5*fade);
		glVertex3f(ww-202, 145+2.5*ss+io*ss+movezero+move-0.5*ss,0);
		glVertex3f(ww-2, 145+2.5*ss+io*ss+movezero+move-0.5*ss,0);
		glVertex3f(ww-2, 145+2.5*ss+io*ss+movezero+move+0.5*ss,0);
		glVertex3f(ww-202, 145+2.5*ss+io*ss+movezero+move+0.5*ss,0);
		glEnd();

		RenderString(ww-200, 150+2.5*ss+io*ss+movezero+move, GLUT_BITMAP_HELVETICA_12, world->events[io].first, 0.0f, 0.0f, 0.0f, fade);
	}
