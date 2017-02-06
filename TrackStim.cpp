#include "stdafx.h"
#include "TrackStim.h"

/////////////// functions to set and write stimulus stuff from fly measurements //////////////////////////////

TrackStim::TrackStim()
{

	PI=3.14159;
	E = 2.718281828;
	ScreenUpdateRate=120; // throw up at Hz
	

	monitor_stimulus_file = NULL;   // file to write for stimulus info
	paramfilename[0] = NULL;

	TrackStim::setZeroTime();
	
	// initialize FMeas
	Sfull.x = Sfull.y = Sfull.theta = Sfull.xold = Sfull.yold = Sfull.thetaold = 0.0f;
	Sfull.vx = Sfull.vy = Sfull.vtheta = 0.0f;
	Sfull.tcurr = Sfull.told = Sfull.dt = 0.0f;
	Sfull.dx = Sfull.dy = Sfull.dtheta = 0.0f;

	Saccumulate=Sfull;

	// initialize Scurr, Sfull
	//Scurr.x = Scurr.y = Scurr.theta = 0.0f;    // all we keep track of here
	//Sfull.x = Sfull.y = Sfull.theta = 0.0f;    // and here

	// initialize stimulus stuff in case not read in
	numepochs=1;
	epochchoose=0;
	TrackStim::Stimulus=TrackStim::initializeStimulus();

	TrackStim::ViewPorts=TrackStim::initializeViewPositions();
	
	
	// set up random dot stuff...
	Rdots.tau = Stimulus.tau;
	Rdots.blobSize = Stimulus.spacing;  // in degrees
	Rdots.windowHeight = Stimulus.arenasize*3;
	Rdots.windowWidth = Stimulus.arenasize*3;
	Rdots.Zheight = Stimulus.arenaheight;
	Rdots.blobCount = (int)(400*Stimulus.density);  // assign based on density
	Rdots.CreateBlobs();

	framenumber = 0;

	perspELEVATION = asin(2.3/8)*180/3.1416;
	perspAZIMUTH = asin(1.6/8)*180/3.1416; 
	perspHEAD = 45;
} ;

TrackStim::~TrackStim()
{
	// close files in use
	if (!(monitor_stimulus_file == NULL))
		fclose(monitor_stimulus_file); 

	// make a time-stamped copy of output files
	char dateStr [9];
    char timeStr [9];
    _strdate(dateStr);
	_strtime( timeStr );
	char myDateTime [30];
	char cmd1 [150];
	char cmd2 [150];
	sprintf(myDateTime,"20%c%c_%c%c_%c%c_%c%c_%c%c_%c%c",dateStr[6],dateStr[7],dateStr[0],dateStr[1],dateStr[3],dateStr[4],
			timeStr[0],timeStr[1],timeStr[3],timeStr[4],timeStr[6],timeStr[7]);
	sprintf(cmd1,"copy .\\_stimulus_output.txt .\\_stimulus_output_%s.txt",myDateTime);
	sprintf(cmd2,"copy .\\_main_booleans.txt .\\_main_booleans_%s.txt",myDateTime);
	system(cmd1);
	system(cmd2);
	sprintf(cmd1,"copy .\\_stimulus_output.txt C:\\code\\backup_data\\_stimulus_output_%s.txt",myDateTime);
	sprintf(cmd2,"copy .\\_main_booleans.txt C:\\code\\backup_data\\_main_booleans_%s.txt",myDateTime);
	system(cmd1);
	system(cmd2);
};

TrackStim::STIMPARAMS TrackStim::initializeStimulus()
{
	TrackStim::STIMPARAMS Stim;   // declare it here...
	Stim.stimtype = 20;
	Stim.lum = .5;
	Stim.contrast = 1;
	Stim.duration = 1000;   // in seconds
	Stim.transgain = 1;
	Stim.rotgain = 1;
	Stim.trans.mean = 0;
	Stim.trans.amp = 0;
	Stim.trans.per = 1;
	Stim.trans.phase = 0;
	Stim.rot.mean = 0;
	Stim.rot.amp = 0;
	Stim.rot.per = 1; 
	Stim.rot.phase = 0;
	Stim.stimtrans.mean = 360;
	Stim.stimtrans.amp = 0;
	Stim.stimtrans.per = 1;
	Stim.stimtrans.phase = 0;
	Stim.stimrot.mean = 0;
	Stim.stimrot.amp = 0;
	Stim.stimrot.per = 1; 
	Stim.stimrot.phase = 30;
	Stim.spacing = 30; //.010; //.1 or 10
	Stim.spacing2 = 5;
	Stim.density = .5;
	Stim.tau = .1;  // in seconds
	Stim.arenasize = 100; // in mm
	Stim.arenaheight = 30; // in mm
	Stim.tau2 = 0.3; // in seconds...
	Stim.randomize = 1;
	Stim.x_center = 0;
	Stim.y_center = 0;
	Stim.x_center2 = 0;
	Stim.y_center2 = 0;
	Stim.base_radius = 200;
	Stim.base_extent = 5000;
	Stim.USEFRUSTUM = 0;
	
	return Stim;   // pass it back.
}

TrackStim::VIEWPOSITIONS TrackStim::initializeViewPositions()
{
	TrackStim::VIEWPOSITIONS temp;
	temp.x[0] = 0; temp.x[1] = 250; 
	temp.y[0] = 0; temp.y[1] = 300; 
	temp.w[0] = 200; temp.w[1] = 200; 
	temp.h[0] = 200; temp.h[1] = 200; 

	return temp;
};
void TrackStim::incrementFrameNumber()
{
	framenumber++;
};

void TrackStim::setZeroTime()
{
	QueryPerformanceCounter(&CounterBeginTime);  // time == 0
	QueryPerformanceFrequency(&CounterFreq);     // frequency of counter, needed to calculate time
};

float TrackStim::queryCurrTime()
{
	LARGE_INTEGER TimeNow;
	QueryPerformanceCounter(&TimeNow);
	return (float)(TimeNow.QuadPart - CounterBeginTime.QuadPart)/(float)CounterFreq.QuadPart;
};


float TrackStim::sineWave(float period)  // freq in Hz
{
	float tc = TrackStim::queryCurrTime() - epochtimezero;
	if (period > 0)   // ignore all if period is 0
		return sin(2*PI*tc/period);
	else
		return 0;
};

void TrackStim::writeStim(TaskHandle taskHandle)
{
	uInt32 data;
	int         error=0;
	char        errBuff[2048]={'\0'};

	if (monitor_stimulus_file == NULL)
	{
		monitor_stimulus_file=fopen("_stimulus_output.txt","w");
		fprintf(monitor_stimulus_file,"%s \n",paramfilename);
	}
	else
	{	
		//fprintf(monitor_stimulus_file,"%8d %8.3f %2d %5.4f %5.4f %4.3f \n",framenumber,Sfull.tcurr,epochchoose,Sfull.x,Sfull.y,Sfull.theta);
		DAQmxErrChk (DAQmxReadCounterScalarU32(taskHandle,1.0,&data,NULL));
		//char tmp[500];
		//sprintf(tmp,"data: %s\n",data);
		//MessageBox(NULL,tmp,"End of program",MB_YESNO|MB_ICONQUESTION)==IDNO;
		fprintf(monitor_stimulus_file,"%8d %8.3f %2d %5.4f %5.4f %4.3f %d\n",framenumber,Sfull.tcurr,epochchoose,Sfull.x,Sfull.y,Sfull.theta,data);
		//fprintf(monitor_stimulus_file,"%8d %8.3f %2d %5.4f %5.4f %4.3f \n",framenumber,Sfull.tcurr,epochchoose,Sfull.x,Sfull.y,Sfull.theta);
		//fprintf(monitor_stimulus_file,"%8d %8.3f %2d %5.4f %5.4f %3.2f \n",framenumber,Sfull.tcurr,epochchoose,Scurr.x,Scurr.dx,Scurr.dt);
	};
	Error:
	{
		puts("");
		if( DAQmxFailed(error) )
			DAQmxGetExtendedErrorInfo(errBuff,2048);
		//if( taskHandle!=0 ) {
			/*********************************************/
			// DAQmx Stop Code
			/*********************************************/
		//	DAQmxStopTask(taskHandle);
		//	DAQmxClearTask(taskHandle);
		//}
		if( DAQmxFailed(error) )
		{
			char tmp[500];
			printf("DAQmx Error: %s\n",errBuff);
			sprintf(tmp,"DAQmx Error: %s\n",errBuff);
			fprintf(monitor_stimulus_file,"%8d %8.3f %2d %5.4f %5.4f %4.3f \n",framenumber,Sfull.tcurr,epochchoose,Sfull.x,Sfull.y,Sfull.theta);
			//MessageBox(NULL,tmp,"End of program",MB_YESNO|MB_ICONQUESTION)==IDNO;
			getchar();
			//return;
		}
	}
};


float TrackStim::mod(float x,float y)
{
	if (x>=0)
		return x - y*(int)(x/y);
	else
		return x - y*(int)((x/y)-1);
}



//////////////////making real stimuli//////////////////////////


void TrackStim::forceWait()
{
	while (queryCurrTime() < float(framenumber)/ScreenUpdateRate)
	{ ;}; // just wait for it to catch up...

};
float TrackStim::getDLPColor(float DLPintensity, char channel)
{
	// returns the digital value to get that intensity in that channel; single gamma for both
	//const static float gamma_bl = 2.16; // before 131119
	//const static float gamma_gr = 2.18;
	//const static float scale_bl = 1.12;
	//const static float scale_gr = 1.47;
	
	//const static float gamma_bl = 2.0244; //new DLP bulb measurements 131119
	//const static float gamma_gr = 2.0811;
	//const static float scale_bl = 1.0991;
	//const static float scale_gr = 1.0468;

	//const static float gamma_bl = 2.0691; //new DLP bulb and chassis, measurements 131211
	//const static float gamma_gr = 2.0783;
	//const static float scale_bl = 1.0147;
	//const static float scale_gr = 1.0155;

	//const static float gamma_bl = 2.1191; //new DLP bulb, measurements 150619
	//const static float gamma_gr = 2.1311;
	//const static float scale_bl = 1.0083;
	//const static float scale_gr = 1.0069;

	const static float gamma_bl = 2.6859; //new DLP, 160622
	const static float gamma_gr = 2.6718;
	const static float scale_bl = 1.8525;
	const static float scale_gr = 1.8519;

	float temp = 0;
	
	if (channel == 'G')
	{
		temp = pow(DLPintensity/scale_gr , 1/gamma_gr);
	};

	if (channel == 'B')
	{
		temp = pow(DLPintensity/scale_bl, 1/gamma_bl);
	};

	if (temp < 1)
		return temp;
	else
		return 1.0f;
};

void TrackStim::setColorTriples()
{
	float bgcol = Stimulus.lum * (1 - Stimulus.contrast);
	float fgcol = Stimulus.lum * (1 + Stimulus.contrast);
	backgroundtriple[0]=0.0f;
	backgroundtriple[1]=getDLPColor(bgcol,'G');
	backgroundtriple[2]=getDLPColor(bgcol,'B');
	foregroundtriple[0]=0.0f;
	foregroundtriple[1]=getDLPColor(fgcol,'G');
	foregroundtriple[2]=getDLPColor(fgcol,'B');
	blacktriple[0]=blacktriple[1]=blacktriple[2]=0.0f;
	whitetriple[0]=whitetriple[1]=whitetriple[2]=1.0f;
};


void TrackStim::setBackground() // sets background color
{
	// float color = pow((Stimulus.lum * (1 - Stimulus.contrast/2)) , (1/GammaCorrect));
	// glClearColor(color, color, color, 0.0f);		
	//
	//float bgcol = Stimulus.lum * (1 - Stimulus.contrast/2);
	//glClearColor(0.0f,getDLPColor(bgcol,'G'),getDLPColor(bgcol,'B'),0.0f);
	glClearColor(backgroundtriple[0],backgroundtriple[1],backgroundtriple[2],0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

};  


void TrackStim::setBackgroundColor()
{
	// float color = pow((Stimulus.lum * (1 - Stimulus.contrast/2)) , (1/GammaCorrect));
	// glColor3f(color, color, color);				// 
	
	//float bgcol = Stimulus.lum * (1 - Stimulus.contrast/2);
	//glColor3f(0.0f,getDLPColor(bgcol,'G'),getDLPColor(bgcol,'B'));
	glColor3f(backgroundtriple[0],backgroundtriple[1],backgroundtriple[2]);
};

void TrackStim::setForeground() // sets foreground color
{
	// float color = pow((Stimulus.lum * (1 + Stimulus.contrast/2)) , (1/GammaCorrect));
	// glColor3f(color, color, color);
	
	//float col = Stimulus.lum * (1 + Stimulus.contrast/2);
	//glColor3f(0.0f,getDLPColor(col,'G'),getDLPColor(col,'B'));
	glColor3f(foregroundtriple[0],foregroundtriple[1],foregroundtriple[2]);};  


void TrackStim::readstring(FILE *f,char *string)
{
	do
	{
		fgets(string, 255, f);
	} while ((string[0] == '/') || (string[0] == '\n'));
	return;
}

void TrackStim::readNoise()   // arena size from elsewhere -- spacing in stimulus
{
	/* to get this to work, use code:
	paramfilename = GetFileName() ;
	for (int ii=0; ii<260; ii++) paramfilekeep[ii]=paramfilename[ii]; // seems very klugey, and below
	 -- and pass this paramfilekeep to this function
	*/

	float x;
	int numpoints;
	FILE *filein;
	char oneline[255];
	
	// add in dialog box here, use global variable to keep world name
	filein = fopen("E:/Code/trackball_final/Trackball/Debug/Data/noise.txt", "rt");				// File To Load World Data From

	readstring(filein,oneline);
	sscanf(oneline, "NUMPOINTS %d\n", &numpoints);

	for (int loop = 0; loop < min(numpoints,14400); loop++)
	{
		readstring(filein,oneline);
		sscanf(oneline, "%f", &x);
		noiseValues[loop]=x;
	}
	fclose(filein);

};   

float TrackStim::retrieveNextNoiseValue()
{
	static int count=-1;
	count++;
	return noiseValues[count/2 % 14400]; // noise is sampled at 120Hz, 2 minutes
};


void TrackStim::readViewPositions()
{
	FILE *filein;
	char oneline[255];
	
	// add in dialog box here, use global variable to keep world name
	if (GetFileAttributes("viewpositions.txt") != INVALID_FILE_ATTRIBUTES)
	{
		filein = fopen("viewpositions.txt", "rt");				// File To Load World Data From

		readstring(filein,oneline);
		sscanf(oneline, "%i %i", &ViewPorts.x[0], &ViewPorts.x[1]);
		readstring(filein,oneline);
		sscanf(oneline, "%i %i", &ViewPorts.y[0], &ViewPorts.y[1]);
		readstring(filein,oneline);
		sscanf(oneline, "%i %i", &ViewPorts.w[0], &ViewPorts.w[1]);
		readstring(filein,oneline);
		sscanf(oneline, "%i %i", &ViewPorts.h[0], &ViewPorts.h[1]);
		fclose(filein);
	}
	else
		ViewPorts = initializeViewPositions();
};

void TrackStim::writeViewPositions()
{
	FILE *fileout;
	
	fileout = fopen("viewpositions.txt", "w");
	fprintf ( fileout, "%i %i \n", ViewPorts.x[0], ViewPorts.x[1] );
	fprintf ( fileout, "%i %i \n", ViewPorts.y[0], ViewPorts.y[1] );
	fprintf ( fileout, "%i %i \n", ViewPorts.w[0], ViewPorts.w[1] );
	fprintf ( fileout, "%i %i \n", ViewPorts.h[0], ViewPorts.h[1] );
	fclose ( fileout );
};


void TrackStim::distancePulse()
{
	float time = Sfull.tcurr - epochtimezero;
	if (time < Stimulus.tau)
		Saccumulate.x += Stimulus.stimtrans.mean/ScreenUpdateRate/2;
};


void TrackStim::rotationPulse()
{
	float time = Sfull.tcurr - epochtimezero;
	if (time < Stimulus.tau)
		Saccumulate.theta += PI/180*Stimulus.stimrot.mean/ScreenUpdateRate/2; // Sfull is reset each time stim is updated, so this is NOT adding cumulatively...
	
};

void TrackStim::rotationNoiseUpdate()
{
	// noise acts like velocity here, not position
	Saccumulate.theta += retrieveNextNoiseValue()*PI/180*Stimulus.stimrot.mean/240; // assumes these come at 240 Hz
};

void TrackStim::rotationDoublePulsePlus()
{
	float time = Sfull.tcurr - epochtimezero;
	if (time < Stimulus.tau)
		Saccumulate.theta += PI/180*Stimulus.stimrot.mean/ScreenUpdateRate/2; // Sfull is reset each time stim is updated, so this is NOT adding cumulatively...
	if (time > Stimulus.tau2)
		Saccumulate.theta += PI/180*Stimulus.stimrot.mean/ScreenUpdateRate/2;

};

void TrackStim::rotationDoublePulseMinus()
{
	float time = Sfull.tcurr - epochtimezero;
	if (time < Stimulus.tau)
		Saccumulate.theta += PI/180*Stimulus.stimrot.mean/ScreenUpdateRate/2; // Sfull is reset each time stim is updated, so this is NOT adding cumulatively...
	if (time > Stimulus.tau2)
		Saccumulate.theta -= PI/180*Stimulus.stimrot.mean/ScreenUpdateRate/2;
};
bool TrackStim::drawScene() // executes the drawing set by setStimType
{
	setColorTriples(); // do once for each draw scene
	
	glEnable(GL_SCISSOR_TEST);
	glViewport(0,0,800,600); // whole window
	glScissor(0,0,800,600);
	glClearColor(0.0f,0.0f,0.0f,0.0f); // set whole screen to be black; scissor testing makes backgrounds good in viewports
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
	glLoadIdentity();									// Reset The View
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // still on whole window

	for (int jj=0; jj<2; jj++)
	{
		switch (Stimulus.stimtype)
		{
			case 5:  // full field gaussian intensity...
				updateGaussianR();
				break;
			case 45:	// local gaussian intensity...
				updateGaussianR();
				break;
		};


		if (jj==0)
			Sfull.tcurr = queryCurrTime();
		else
			Sfull.tcurr = Sfull.tcurr + 1/2/ScreenUpdateRate;
		
		glViewport(ViewPorts.x[0],ViewPorts.y[0],ViewPorts.w[0],ViewPorts.h[0]);
		glScissor(ViewPorts.x[0],ViewPorts.y[0],ViewPorts.w[0],ViewPorts.h[0]);

		// new
		glClear(GL_DEPTH_BUFFER_BIT);
		glLoadIdentity();									// Reset The View

		switch (jj) // have to do this inside loop
		{
			case 0:  // t1 in blue --- blue first, then green in order of DLP
				glColorMask(GL_FALSE,GL_FALSE,GL_TRUE,GL_TRUE);
				break;
			case 1:  // t2 in green
				glColorMask(GL_FALSE,GL_TRUE,GL_FALSE,GL_TRUE);
				break;
		}

		setBackground();
		setForeground();

		glClear(GL_COLOR_BUFFER_BIT);
		glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
		glLoadIdentity();

		if (!Stimulus.USEFRUSTUM)
		{		
			gluPerspective(90.0f,(float)ViewPorts.w[0]/(float)ViewPorts.h[0],0.1f,Stimulus.arenasize); // peculiarity of the planes @ 45 degrees
		}
		else
		{
			//glFrustum(-80.0f*0.74f, 80.0f*0.26f, -83.0f, -3.0f, 50.0f, 1000.0f);
			glFrustum(-80.0f*0.74f, 80.0f*0.26f, 3.0f, 83.0f, 50.0f, 1000.0f);
		};

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();  // start at origin

		if (!Stimulus.USEFRUSTUM)
		{
			gluLookAt(0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f);
		}
		else
		{
			gluLookAt(0.0f,0.0f,0.0f,1.0f,0.0f,0.0f,0.0f,0.0f,1.0f);
		};


		//drawWindow(0,PI/4,.2); // for potential use later...
		
		// all set, now draw the stimulus
		switch (Stimulus.stimtype)
		{
			case 0:
				drawUniform();
				break;
			case 1:
				drawUniform();
				break;
			case 2:
				drawSquareGrid();
				break;
			case 3:
				drawMovingBars();
				break;
			case 4:
				drawMovingBar();
				break;
			case 5:
				drawGaussianFullfield();
				break;
			case 6:
				drawMovingBar_grid();
				break;
			case 7:
				drawBarPairsOffset();
				break;
			case 8:
				drawCorrGrid(FALSE); // no scramble
				break;
			case 9:
				drawCorrGrid(TRUE); // scramble -- temporal, no spatial correlations
				break;
			case 10:
				drawLocalSquareGrid();
				break;
			case 11:
				drawLocalCircle();
				break;
			case 12:
				drawLocalTorus();
				break;
			case 13:
				drawMovingBar_grid_randomspace();
				break;
			case 14:
				drawMovingBar_grid_replicateFFF();
				break;
			case 15:
				drawMovingBar_large();
				break;
			case 16:
				drawStaticGrid();
				break;
			case 17:
				drawMovingBar_grid_once();
				break;
			case 18:
				drawBar_grid_randomspace();
				break;
			case 19:
				drawUniform_sine();
				break;
			case 20:
				drawSine1D();
				break;
			case 21:
				drawBar_grid_randomspace_permute();
				break;
			case 22:
				drawSquareGridLB();
				break;
			case 23:
				drawStaticGridLB();
				break;
			case 24:
				drawStaticGridLB2();
				break;
			case 25:
				drawBar_grid_randomspace_permute_LB();
				break;
			case 26:
				drawUniform_LB();
				break;
			case 27:
				drawUniform_LB_varSize();
				break;
			case 28:
				drawMovingBar_grid_LB();
				break;
			case 29:
				drawCalibrationFFF();
				break;
			case 30:
				drawCalibrationFFF_withcorrection();
				break;
			case 31:
				drawSine1D_LB();
				break;
			case 32:
				drawUniform_sine_LB();
				break;
			case 33:
				drawSingleBar_local_LB();
				break;
			case 34:
				drawUniform_LB_varSizeExpInv();
				break;
			case 35:
				drawBarPair_local_LB();
				break;
			case 36:
				drawBarPairWide_local_LB();
				break;
			case 37:
				drawBarPair_local_LBn();
				break;
			case 38:
				drawLocalCircleTorusSimul();
				break;
			case 39:
				drawLocalCircleCR();
				break;
			case 40:
				drawLocalTorusCR();
				break;
			case 41:
				drawLocalCircleTorusSimulCR();
				break;
			case 42:
				drawLocalCircleCR2();
				break;
			case 43:
				drawLocalTorusCR2();
				break;
			case 44:
				drawLocalCircleTorusSimulCR2();
				break;
			case 45:
				drawGaussianLocalCircle();
				break;
			case 46:
				drawSineCylinder(Sfull.tcurr);
				break;
			case 47:
				drawSphere();
				break;
			case 48:
				drawSpherePrint(Sfull.tcurr-epochtimezero);
				break;
			case 49:
				drawSineCylinderIGray(Sfull.tcurr);
				break;
			case 50:
				drawCylinderStripe(Sfull.tcurr-epochtimezero);
				break;
			case 51:
				drawSquareWaveCylinder(Sfull.tcurr-epochtimezero);
				break;
			case 52:
				drawSquareWaveCylinderHS(Sfull.tcurr-epochtimezero);
				break;
			case 53:
				drawMovingBarsHS();
				break;
			case 54:
				drawUniform_EB();
				break;
			case 55:
				drawBar_grid_randomspace_permute_HY();
				break;
			case 56:
				drawLightBarCorrected_grid_randomspace_permute_HY();
				break;
			case 57:
				drawLightBarCorrected_grid_randomspace_permute_HY_test();
				break;
			case 58:
				drawBar_grid_randomspace_permute_HY_test();
			case 59:
				drawBar_grid_randomspace_permute_flyCord_onscreen_YF();
				break;
			case 60:
				drawBar_grid_randomspace_permute_flyCord_YF();
				break;
			case 61:
				drawMovingBar_grid_flyCord();
				break;
			case 62:
				drawMovingBar_grid_LB_onscreen();
				break;
		};


	};
	glClear(GL_DEPTH_BUFFER_BIT);
	glColorMask(GL_TRUE,GL_TRUE,GL_TRUE,GL_TRUE);
	TrackStim::drawStim();   // draw this last...

	glDisable(GL_SCISSOR_TEST); // for some reason, want to do this before swap buffer operation

	return TRUE;	
};  

void TrackStim::drawUniform()
{
	glBegin(GL_QUADS);
	glVertex3f(1,-5,-5);
	glVertex3f(1,-5,5);
	glVertex3f(1,5,5);
	glVertex3f(1,5,-5);
	glEnd();
};

void TrackStim::drawUniform_EB()
{	float grayI = 0.5;
	float time = Sfull.tcurr - epochtimezero;
	if (time > 2)
	{
		glBegin(GL_QUADS);
		glVertex3f(1,-5,-5);
		glVertex3f(1,-5,5);
		glVertex3f(1,5,5);
		glVertex3f(1,5,-5);
		glEnd();
	}
	else
	{
		glColor3f(0.0f,getDLPColor(grayI,'G'),getDLPColor(grayI,'B'));
		glBegin(GL_QUADS);
		glVertex3f(1,-5,-5);
		glVertex3f(1,-5,5);
		glVertex3f(1,5,5);
		glVertex3f(1,5,-5);
		glEnd();
	};
};

void TrackStim::drawCalibrationFFF()
{

	float fulltime = Sfull.tcurr - epochtimezero;
	int fullstep = int(fulltime/Stimulus.tau);
	//float loccont = fullstep*Stimulus.contrast; 
	float loccont = Stimulus.contrast; //ms: we used this line instead of the above for keeping one value for calibrations 131119
	Sfull.x = loccont;

	if (int(Stimulus.stimrot.phase)==1) 
		glColor3f(0.0f,loccont,0.0f); // RGB -- green
	else
		glColor3f(0.0f,0.0f,loccont); // RGB -- blue
		//glColor3f(0.0f,1.0f,1.0f); // RGB -- blue


	glBegin(GL_QUADS);
	glVertex3f(1,-5,-5);
	glVertex3f(1,-5,5);
	glVertex3f(1,5,5);
	glVertex3f(1,5,-5);
	glEnd();
};

void TrackStim::drawCalibrationFFF_withcorrection()
{

	float fulltime = Sfull.tcurr - epochtimezero;
	int fullstep = int(fulltime/Stimulus.tau);
	//float loccont = fullstep*Stimulus.contrast; 
	float loccont = Stimulus.contrast; //ms: we used this line instead of the above for keeping one value for calibrations 131119
	Sfull.x = loccont;

	if (int(Stimulus.stimrot.phase)==1)
		glColor3f(0.0f,getDLPColor(loccont,'G'),0.0f); // RGB -- green
	else
		glColor3f(0.0f,0.0f,getDLPColor(loccont,'B')); // RGB -- blue


	glBegin(GL_QUADS);
	glVertex3f(1,-5,-5);
	glVertex3f(1,-5,5);
	glVertex3f(1,5,5);
	glVertex3f(1,5,-5);
	glEnd();
};

void TrackStim::drawUniform_LB()
{
	static bool fff_first = TRUE;

	if(fff_first)
	{
		fff_first = FALSE;
		srand(time(NULL));
	}	

	glBegin(GL_QUADS);
	glVertex3f(1,-5,-5);
	glVertex3f(1,-5,5);
	glVertex3f(1,5,5);
	glVertex3f(1,5,-5);
	glEnd();
};

void TrackStim::drawUniform_LB_varSize()
{
	static bool fff_first = TRUE;
	float dsize = Stimulus.spacing;
	float dsize2 = Stimulus.spacing2;
	//float dsize = 1;

	if(fff_first)
	{
		fff_first = FALSE;
		srand(time(NULL));
	}	

	//glPushMatrix();
	//glRotatef(0,1,0,0);
	glBegin(GL_QUADS);
	glVertex3f(45,-45+dsize2,-45+dsize2);
	glVertex3f(45,-45+dsize2,-45+dsize);
	glVertex3f(45,-45+dsize,-45+dsize);
	glVertex3f(45,-45+dsize,-45+dsize2);
	glEnd();
	//glPopMatrix();
};

void TrackStim::drawUniform_LB_varSizeExpInv()
{
	static bool fff_first = TRUE;
	float dsize = Stimulus.spacing;
	float dsize2 = Stimulus.spacing2;
	float currlum = 0;
	//float dsize = 1;

	if(fff_first)
	{
		fff_first = FALSE;
		srand(time(NULL));
	}	

	if(Stimulus.trans.phase == -1)
	{
		if(Stimulus.contrast<0)
			currlum = 0;
		else
			currlum = 1;

		// External field
		glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		glBegin(GL_QUADS);
		glVertex3f(45,-45+dsize2,-45+dsize2);
		glVertex3f(45,-45+dsize2,-45+dsize2+15);
		glVertex3f(45,-45+dsize,-45+dsize2+15);
		glVertex3f(45,-45+dsize,-45+dsize2);
		glEnd();

		// External field
		glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		glBegin(GL_QUADS);
		glVertex3f(45,-45+dsize2,-45+dsize2);
		glVertex3f(45,-45+dsize2,-45+dsize);
		glVertex3f(45,-45+dsize2+15,-45+dsize);
		glVertex3f(45,-45+dsize2+15,-45+dsize2);
		glEnd();

		if(Stimulus.contrast<0)
			currlum = 1;
		else
			currlum = 0;
		
		// Internal field
		glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		glBegin(GL_QUADS);
		glVertex3f(45,-45+dsize2+15,-45+dsize2+15);
		glVertex3f(45,-45+dsize2+15,-45+dsize);
		glVertex3f(45,-45+dsize,-45+dsize);
		glVertex3f(45,-45+dsize,-45+dsize2+15);
		glEnd();
	}
	else
	{
		glBegin(GL_QUADS);
		glVertex3f(45,-45+dsize2,-45+dsize2);
		glVertex3f(45,-45+dsize2,-45+dsize);
		glVertex3f(45,-45+dsize,-45+dsize);
		glVertex3f(45,-45+dsize,-45+dsize2);
		glEnd();
	}
};

void TrackStim::drawUniform_sine()
{
	static float phase = 0;
	float PI = 3.14159f;
	float currlum = Stimulus.lum* (1 + Stimulus.contrast*sin(phase));

	phase += 1.0f/240.0f/Stimulus.tau*2.0f*PI;

	Sfull.x=currlum;
	
	glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));

	glBegin(GL_QUADS);
	glVertex3f(1,-5,-5);
	glVertex3f(1,-5,5);
	glVertex3f(1,5,5);
	glVertex3f(1,5,-5);
	glEnd();
};

void TrackStim::drawUniform_sine_LB()
{
	static float phase = 0;
	float PI = 3.14159f;
	float currlum = Stimulus.lum* (1 + Stimulus.contrast*sin(phase));

	float fulltime = Sfull.tcurr - epochtimezero;
	if (fulltime<2)
		currlum = 0.5;
		
	phase += 1.0f/240.0f/Stimulus.tau*2.0f*PI;
	Sfull.x=currlum;
	glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
	
	glBegin(GL_QUADS);
	glVertex3f(1,-5,-5);
	glVertex3f(1,-5,5);
	glVertex3f(1,5,5);
	glVertex3f(1,5,-5);
	glEnd();
};

void TrackStim::drawSineCylinder(float tc)
{
	static bool FIRST = TRUE;
	static float imagedata[128*3];
	if (FIRST)
	{
		for (int ii=0; ii<128; ii++)
		{
			imagedata[ii*3]=0.0f;
			imagedata[ii*3+1]=getDLPColor(Stimulus.lum *(1 + Stimulus.contrast*sin(ii*2*PI/128.0f)),'G');
			imagedata[ii*3+2]=getDLPColor(Stimulus.lum *(1 + Stimulus.contrast*sin(ii*2*PI/128.0f)),'B');
		}
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 128, 0, GL_RGB, GL_FLOAT, imagedata);
	};
	FIRST = FALSE;

	float wedgewidth = 20*PI/180; // width of each wedge
	float per = Stimulus.spacing;
	float radius = Stimulus.base_radius;
	float height = Stimulus.base_extent;
	float angle;

	float added = 0; // add the stimulus OL
	added += tc*Stimulus.stimtrans.mean*PI/180;  // put in translation mean here to get easy spinning. not ideal... bring amp back to 0 though!

	glPushMatrix(); // just to be sure...

	transformToScopeCoords();

	float rollang=Stimulus.stimrot.mean;
	// now, put it into the right pitch coordinates...
//	glRotatef(0.0f,0.0f,1.0f,0.0f); // rotates for head now draw however...
	glRotatef(-perspHEAD,0.0f,1.0f,0.0f); // rotates for head now draw however...
	// now, roll it however to get right perp angle...
//	glRotatef(rollang,-1.0f,0.0f,0.0f);
	glRotatef(rollang,1.0f,0.0f,0.0f);
	//glRotatef(0.0f,-cos(perspHEAD*180.0f/PI),0.0f,sin(perspHEAD*180.0f/PI));

	

	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, 1);
	glBegin(GL_QUADS);
	for (int ii=0; ii<18; ii++) // through each wedge...
	{
		angle = ii*wedgewidth + added;
		glTexCoord1f(ii*20.0f/per);
		glVertex3f(radius*cos(angle),radius*sin(angle), -height/2);
		glTexCoord1f((ii+1)*20.0f/per);
		glVertex3f(radius*cos(angle+wedgewidth),radius*sin(angle+wedgewidth), -height/2);
		glTexCoord1f((ii+1)*20.0f/per);
		glVertex3f(radius*cos(angle+wedgewidth),radius*sin(angle+wedgewidth), height/2);
		glTexCoord1f(ii*20.0f/per);
		glVertex3f(radius*cos(angle),radius*sin(angle), height/2);
	}
	glEnd();
	glDisable(GL_TEXTURE_1D);

	glPopMatrix();

};

void TrackStim::drawSineCylinderIGray(float tc)
{
	static bool FIRST = TRUE;
	static float imagedata[128*3];
	if (FIRST)
	{
		for (int ii=0; ii<128; ii++)
		{
			imagedata[ii*3]=0.0f;
			imagedata[ii*3+1]=getDLPColor(Stimulus.lum *(1 + Stimulus.contrast*sin(ii*2*PI/128.0f)),'G');
			imagedata[ii*3+2]=getDLPColor(Stimulus.lum *(1 + Stimulus.contrast*sin(ii*2*PI/128.0f)),'B');
		}
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 128, 0, GL_RGB, GL_FLOAT, imagedata);
	};
	FIRST = FALSE;

	float wedgewidth = 20*PI/180; // width of each wedge
	float per = Stimulus.spacing;
	float radius = Stimulus.base_radius;
	float height = Stimulus.base_extent;
	float angle;

	float added = 0; // add the stimulus OL
	added += tc*Stimulus.stimtrans.mean*PI/180;  // put in translation mean here to get easy spinning. not ideal... bring amp back to 0 though!

	glPushMatrix(); // just to be sure...

	transformToScopeCoords();

	float rollang=Stimulus.stimrot.mean;
	// now, put it into the right pitch coordinates...
//	glRotatef(0.0f,0.0f,1.0f,0.0f); // rotates for head now draw however...
	glRotatef(-perspHEAD,0.0f,1.0f,0.0f); // rotates for head now draw however...
	// now, roll it however to get right perp angle...
//	glRotatef(rollang,-1.0f,0.0f,0.0f);
	glRotatef(rollang,1.0f,0.0f,0.0f);
	//glRotatef(0.0f,-cos(perspHEAD*180.0f/PI),0.0f,sin(perspHEAD*180.0f/PI));

	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, 1);

	float time = Sfull.tcurr - epochtimezero;
	//if (time < float(epochchoose+1)/10)
	if (time > 2)
	{
		glBegin(GL_QUADS);
		for (int ii=0; ii<18; ii++) // through each wedge...
		{
			angle = ii*wedgewidth + added;
			glTexCoord1f(ii*20.0f/per);
			glVertex3f(radius*cos(angle),radius*sin(angle), -height/2);
			glTexCoord1f((ii+1)*20.0f/per);
			glVertex3f(radius*cos(angle+wedgewidth),radius*sin(angle+wedgewidth), -height/2);
			glTexCoord1f((ii+1)*20.0f/per);
			glVertex3f(radius*cos(angle+wedgewidth),radius*sin(angle+wedgewidth), height/2);
			glTexCoord1f(ii*20.0f/per);
			glVertex3f(radius*cos(angle),radius*sin(angle), height/2);
		}
		glEnd();
	}
	else
		Stimulus.contrast = 0;
	
	glDisable(GL_TEXTURE_1D);

	glPopMatrix();

};

void TrackStim::drawSquareWaveCylinder(float tc)
{

	float wedgewidth = Stimulus.spacing/2; // width of each wedge
	float per = Stimulus.spacing;
	float radius = Stimulus.base_radius;
	float height = Stimulus.base_extent;
	static float contrast = 0;
	float angle;

	float added = 0; // add the stimulus OL
	added += tc*Stimulus.stimtrans.mean;  // put in translation mean here to get easy spinning. not ideal... bring amp back to 0 though!

	static bool FIRST = TRUE;
	if(FIRST)
		contrast = Stimulus.contrast;
	FIRST = FALSE;

	glPushMatrix(); // just to be sure...

	transformToScopeCoords();

	float rollang=Stimulus.stimrot.mean;
	// now, put it into the right pitch coordinates...
	glRotatef(-perspHEAD,0.0f,1.0f,0.0f); // rotates for head now draw however...
	// now, roll it however to get right perp angle...
	glRotatef(rollang,1.0f,0.0f,0.0f);

	float time = Sfull.tcurr - epochtimezero;
	//if (time < float(epochchoose+1)/10)
	if (time > 2)
	{
		Stimulus.contrast = contrast;
		setForeground();
		for (int ii=0; ii<int(360.001/per); ii++)
		{
			drawCylinderWedge(added+ii*per,wedgewidth);
		};
	}
	else
		Stimulus.contrast = 0;

	glPopMatrix();

};

void TrackStim::drawSquareWaveCylinderHS(float tc)
{

	float wedgewidth = Stimulus.spacing/2; // width of each wedge
	float per = Stimulus.spacing;
	float radius = Stimulus.base_radius;
	float height = Stimulus.base_extent;
	static float contrast = 0;
	float angle;

	float added = 0; // add the stimulus OL
	float time = Sfull.tcurr - epochtimezero;
	if(time > Stimulus.spacing2)
	{
		added += (tc-Stimulus.spacing2)*Stimulus.stimtrans.mean;  // put in translation mean here to get easy spinning. not ideal... bring amp back to 0 though!
	}
	else
	{
		added = 0.01; 
	}

	static bool FIRST = TRUE;
	if(FIRST)
		contrast = Stimulus.contrast;
	FIRST = FALSE;

	glPushMatrix(); // just to be sure...

	transformToScopeCoords();

	float rollang=Stimulus.stimrot.mean;
	// now, put it into the right pitch coordinates...
	glRotatef(-perspHEAD,0.0f,1.0f,0.0f); // rotates for head now draw however...
	// now, roll it however to get right perp angle...
	glRotatef(rollang,1.0f,0.0f,0.0f);

	//if (time < float(epochchoose+1)/10)
	Stimulus.contrast = contrast;
	setForeground();
	for (int ii=0; ii<int(360.001/per); ii++)
	{
		drawCylinderWedge(added+ii*per,wedgewidth);
	};
	
	glPopMatrix();

};

void TrackStim::drawCylinderStripe(float tc)
{
	
	float wedgewidth = Stimulus.spacing; // width of each wedge

	float added = Stimulus.stimtrans.amp; // add the stimulus OL
	added += tc*Stimulus.stimtrans.mean;  // put in translation mean here to get easy spinning. not ideal... bring amp back to 0 though!

	glPushMatrix(); // just to be sure...

	transformToScopeCoords();

	float rollang=Stimulus.stimrot.mean;
	// now, put it into the right pitch coordinates...
	glRotatef(-perspHEAD,0.0f,1.0f,0.0f); // rotates for head now draw however...
	// now, roll it however to get right perp angle...
	glRotatef(rollang,1.0f,0.0f,0.0f);

	
	setForeground();
	drawCylinderWedge(added,wedgewidth);

	glPopMatrix();

};

void TrackStim::drawCylinderWedge(float angle, float w)
{
	// takes angles in degrees!
	float radius = Stimulus.base_radius;
	float height = Stimulus.base_extent;
	angle *= PI/180;
	w *= PI/180;

	glBegin(GL_QUADS);
	glVertex3f(radius*cos(angle),radius*sin(angle), -height/2);
	glVertex3f(radius*cos(angle+w),radius*sin(angle+w), -height/2);
	glVertex3f(radius*cos(angle+w),radius*sin(angle+w), height/2);
	glVertex3f(radius*cos(angle),radius*sin(angle), height/2);
	glEnd();
};

void TrackStim::transformToScopeCoords()
{
	// perspective always looks in +x axis direction, from origin!, z is up, in original coords.
	glRotatef(-perspAZIMUTH,0.0f,0.0f,1.0f); 
	glRotatef(perspELEVATION,0.0f,1.0f,0.0f); // signs? appear correct
	
	// i don't believe we want to translate in here...
	// coordinates now in microscope frame...

};

void TrackStim::drawSphere()
{
	
	setForeground();
	//glColor3f(0.0f,getDLPColor(1,'G'),getDLPColor(1,'B'));
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glPushMatrix();
	transformToScopeCoords();
	
	glRotatef(90.0f,0.0f,1.0f,0.0f); // draw it with pole along x axis? this is where we're looking, right?
	GLUquadricObj *cyl = gluNewQuadric();
	glPushMatrix();
	gluSphere(cyl,200.0f,32,32);
	glPopMatrix();

	glPopMatrix();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

};

void TrackStim::drawSpherePrint(float tc)
{
	static bool FILE_FIRST = FALSE;
	FILE *filein;

	setForeground();
	//glColor3f(0.0f,getDLPColor(1,'G'),getDLPColor(1,'B'));
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glPushMatrix();
	transformToScopeCoords();
	
	glRotatef(Stimulus.stimrot.amp,0.0f,0.0f,1.0f); // azimuth
	glRotatef(-Stimulus.stimrot.mean,0.0f,1.0f,0.0f); // elevation down
	glRotatef(90.0f,0.0f,1.0f,0.0f); // draw it with pole along x axis? this is where we're looking, right?
	GLUquadricObj *cyl = gluNewQuadric();
	glPushMatrix();
	gluSphere(cyl,200.0f,36,18);
	glPopMatrix();

	glPopMatrix();
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glReadBuffer(GL_FRONT_RIGHT);

	if ((tc<0.1) && (!FILE_FIRST))
		FILE_FIRST = TRUE;
	
	if (tc>0.5)
		FILE_FIRST = FALSE;

	if(FILE_FIRST)
	{
		//MessageBox(NULL,"in","INFO",MB_OK|MB_ICONSTOP);
		float data[155][155];
		//data = new float*[ViewPorts.h[0]];
		//for (int i = 0; i < ViewPorts.h[0]; i++)			
		//	data[i] = new float[ViewPorts.w[0]];

		glReadPixels(385,15,155,155,GL_GREEN,GL_FLOAT,&data);
		
		char str[100];
		sprintf(str,"pixel_vals_%d.txt",epochchoose);
		filein = fopen(str,"w");		
		for(int i = 0;i < 155;i++)
		{
			fprintf(filein,"\n");
			for(int j = 0;j < 155;j++)
				fprintf(filein,"%f \t",data[i][j]);
		}
		fclose(filein);
	}
		

};

void TrackStim::drawStim()
{
	// set up to look opposite direction
	glLoadIdentity();									// Reset The View
	glViewport(ViewPorts.x[1],ViewPorts.y[1],ViewPorts.w[1],ViewPorts.h[1]);
	glScissor(ViewPorts.x[1],ViewPorts.y[1],ViewPorts.w[1],ViewPorts.h[1]);
	glClearColor(0.0f,0.0f,0.0f,0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glMatrixMode(GL_PROJECTION);						// Select The Projection Matrix
	glLoadIdentity();
	gluPerspective(90.0f,(float)ViewPorts.w[1]/(float)ViewPorts.h[1],0.1f,100000.0f);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();  // start at origin
	gluLookAt(0.0f,0.0f,0.0f,-1.0f,0.0f,0.0f,0.0f,0.0f,1.0f);

	// draw that shit -- should be pretty quick

	glColor3f(whitetriple[0],whitetriple[1],whitetriple[2]);
	float time = Sfull.tcurr - epochtimezero;
	if (time < float(epochchoose+1)/10)
	{
		glBegin(GL_QUADS);
		glVertex3f(-1,-5,-5);
		glVertex3f(-1,-5,5);
		glVertex3f(-1,5,5);
		glVertex3f(-1,5,-5);
		glEnd();
	};
	// otherwise, don't do anything...
		
};

void TrackStim::drawSine1D()
{
	// set up texture
	static bool FIRST = TRUE;
	static float imagedata[128*3];
	float fulltime = Sfull.tcurr - epochtimezero;
	if ((FIRST) & (fulltime<0.05))
	{
		FIRST = FALSE;

		for (int ii=0; ii<128; ii++)
		{
			imagedata[ii*3]=0.0f;
			imagedata[ii*3+1]=getDLPColor(Stimulus.lum *(1 + Stimulus.contrast*sin(ii*2*PI/128.0f)),'G');
			imagedata[ii*3+2]=getDLPColor(Stimulus.lum *(1 + Stimulus.contrast*sin(ii*2*PI/128.0f)),'B');
		}
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 128, 0, GL_RGB, GL_FLOAT, imagedata);
	};
	if (fulltime>0.1)
		FIRST = TRUE;

	float d = Stimulus.spacing; //  period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;

	phase += v/240; // update rate of 240 hz
	phase = phase - int(phase/d)*d;
	Sfull.x = phase;
	Sfull.theta=rotation*PI/180;

	glPushMatrix();
	glRotatef(rotation,1,0,0);

	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, 1);
	glBegin(GL_QUADS);
	// motion is in second coordinate
	glTexCoord1f((-phase+2*side)/d);
	glVertex3f(Xoff,side,-side);
	glTexCoord1f(-phase/d);
	glVertex3f(Xoff,-side,-side);
	glTexCoord1f(-phase/d);
	glVertex3f(Xoff,-side,side);
	glTexCoord1f((-phase+2*side)/d);
	glVertex3f(Xoff,side,side);
	glEnd();
	glDisable(GL_TEXTURE_1D);
	glPopMatrix();

};


void TrackStim::drawSine1D_LB()
{
	// set up texture
	static bool FIRST = TRUE;
	static float imagedata[128*3];
	float fulltime = Sfull.tcurr - epochtimezero;
	if ((FIRST) & (fulltime<2))
	{
		FIRST = FALSE;
		int contrast = 0;

		for (int ii=0; ii<128; ii++)
		{
			imagedata[ii*3]=0.0f;
			imagedata[ii*3+1]=getDLPColor(Stimulus.lum *(1 + contrast*sin(ii*2*PI/128.0f)),'G');
			imagedata[ii*3+2]=getDLPColor(Stimulus.lum *(1 + contrast*sin(ii*2*PI/128.0f)),'B');
		}
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 128, 0, GL_RGB, GL_FLOAT, imagedata);
	};
	if (fulltime>0.1)
		FIRST = TRUE;

	if ((FIRST) & (fulltime>2) & (fulltime<2.05))
	{
		FIRST = FALSE;
		int contrast = 0;

		for (int ii=0; ii<128; ii++)
		{
			imagedata[ii*3]=0.0f;
			imagedata[ii*3+1]=getDLPColor(Stimulus.lum *(1 + Stimulus.contrast*sin(ii*2*PI/128.0f)),'G');
			imagedata[ii*3+2]=getDLPColor(Stimulus.lum *(1 + Stimulus.contrast*sin(ii*2*PI/128.0f)),'B');
		}
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 128, 0, GL_RGB, GL_FLOAT, imagedata);
	};

	float d = Stimulus.spacing; //  period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;

	phase += v/240; // update rate of 240 hz
	phase = phase - int(phase/d)*d;
	Sfull.x = phase;
	Sfull.theta=rotation*PI/180;

	glPushMatrix();
	glRotatef(rotation,1,0,0);

	glEnable(GL_TEXTURE_1D);
	glBindTexture(GL_TEXTURE_1D, 1);
	glBegin(GL_QUADS);
	// motion is in second coordinate
	glTexCoord1f((-phase+2*side)/d);
	glVertex3f(Xoff,side,-side);
	glTexCoord1f(-phase/d);
	glVertex3f(Xoff,-side,-side);
	glTexCoord1f(-phase/d);
	glVertex3f(Xoff,-side,side);
	glTexCoord1f((-phase+2*side)/d);
	glVertex3f(Xoff,side,side);
	glEnd();
	glDisable(GL_TEXTURE_1D);
	glPopMatrix();

};


void TrackStim::drawSquareGrid()
{
	static bool FIRST = TRUE;
	static bool WAITBUFFER = FALSE;
	float d = Stimulus.spacing;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nside = int(90/d);
	int Nsq = Nside * Nside;

	// modified 090810 to make each new call with a new random sequence...
	// before this date, multiple calls all had same sequence (first sequence)
	if (fulltime > 1)
		WAITBUFFER = TRUE;
	if ((!FIRST) & (WAITBUFFER))
	{
		FIRST = (fulltime < 0.002); // after first run, checks each time to see if epochtime has been rezeroed... if so, re-randomize.
		if (FIRST)
			WAITBUFFER = FALSE;
	}
	// this works b/c fulltime should be zero at some point, and tcurr updates at 240 Hz

	static int index[400];
	if (FIRST)
	{
		FIRST = FALSE;
		srand(time(NULL));
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}

	};

	// now, do the drawing... and record where drawn in Sfull
	int choosenum = int(fulltime / Stimulus.tau);
	float loctime = fulltime - choosenum*Stimulus.tau; // local time
	choosenum = choosenum % Nsq;
	float x = d*(index[choosenum] % Nside);
	float y = d*(int(index[choosenum]/Nside));
	x -= 45; y -= 45;
	Sfull.x=x; Sfull.y=y;
	float Xoff = 45;
	if (loctime<Stimulus.tau2)
	{
		glBegin(GL_QUADS);
		glVertex3f(Xoff,x,y);
		glVertex3f(Xoff,x+d,y);
		glVertex3f(Xoff,x+d,y+d);
		glVertex3f(Xoff,x,y+d);
		glEnd();
	}
	else
	{
		Sfull.x=0;
		Sfull.y=0;
	};

};




void TrackStim::drawSquareGridLB()
{
	static bool FIRST = TRUE;
	static bool WAITBUFFER = FALSE;
	float d = Stimulus.spacing;
	float fulltime = Sfull.tcurr - epochtimezero;
	int range = Stimulus.trans.mean;
	int Nside = int(range/d);
	int Nsq = Nside * Nside;
	int x_offset = Stimulus.stimtrans.mean;
	int y_offset = Stimulus.stimrot.mean;

	// modified 090810 to make each new call with a new random sequence...
	// before this date, multiple calls all had same sequence (first sequence)
	if (fulltime > 1)
		WAITBUFFER = TRUE;
	if ((!FIRST) & (WAITBUFFER))
	{
		FIRST = (fulltime < 0.002); // after first run, checks each time to see if epochtime has been rezeroed... if so, re-randomize.
		if (FIRST)
			WAITBUFFER = FALSE;
	}
	// this works b/c fulltime should be zero at some point, and tcurr updates at 240 Hz

	static int * index = new int[Nsq];
	if (FIRST)
	{
		FIRST = FALSE;
		srand(time(NULL));
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}

	};

	// now, do the drawing... and record where drawn in Sfull
	int choosenum = int(fulltime / Stimulus.tau);
	float loctime = fulltime - choosenum*Stimulus.tau; // local time
	choosenum = choosenum % Nsq;
	float x = d*(index[choosenum] % Nside);
	float y = d*(int(index[choosenum]/Nside));
	x -= 45; y -= 45;
	x += x_offset; y += y_offset;
	Sfull.x=x; Sfull.y=y;
	float Xoff = 45;
	if (loctime<Stimulus.tau2)
	{
		glBegin(GL_QUADS);
		glVertex3f(Xoff,x,y);
		glVertex3f(Xoff,x+d,y);
		glVertex3f(Xoff,x+d,y+d);
		glVertex3f(Xoff,x,y+d);
		glEnd();
	}
	else
	{
		Sfull.x=0;
		Sfull.y=0;
	};

};



void TrackStim::drawLocalSquareGrid()
{
	static bool FIRST = TRUE;
	static bool WAITBUFFER = FALSE;
	float dsize = Stimulus.spacing;
	float dmove = Stimulus.spacing2;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nside = int(20/dmove);
	int Nsq = Nside * Nside;

	// modified 090810 to make each new call with a new random sequence...
	// before this date, multiple calls all had same sequence (first sequence)
	if (fulltime > 1)
		WAITBUFFER = TRUE;
	if ((!FIRST) & (WAITBUFFER))
	{
		FIRST = (fulltime < 0.002); // after first run, checks each time to see if epochtime has been rezeroed... if so, re-randomize.
		if (FIRST)
			WAITBUFFER = FALSE;
	}
	// this works b/c fulltime should be zero at some point, and tcurr updates at 240 Hz

	static int index[400];
	if (FIRST)
	{
		FIRST = FALSE;
		srand(time(NULL));
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}

	};

	// now, do the drawing... and record where drawn in Sfull
	int choosenum = int(fulltime / Stimulus.tau);
	float loctime = fulltime - choosenum*Stimulus.tau; // local time
	choosenum = choosenum % Nsq;
	float x = dmove*(index[choosenum] % Nside) - 10 - dsize/2 + Stimulus.x_center;
	float y = dmove*(int(index[choosenum]/Nside)) - 10 - dsize/2 + Stimulus.y_center;
//	x -= 45; y -= 45;
	Sfull.x=x; Sfull.y=y;
	float Xoff = 45;
	if (loctime<Stimulus.tau2)
	{
		glBegin(GL_QUADS);
		glVertex3f(Xoff,x-dsize/2,y-dsize/2);
		glVertex3f(Xoff,x+dsize/2,y-dsize/2);
		glVertex3f(Xoff,x+dsize/2,y+dsize/2);
		glVertex3f(Xoff,x-dsize/2,y+dsize/2);
		glEnd();
	}
	else
	{
		Sfull.x=0;
		Sfull.y=0;
	};

};

void TrackStim::drawLocalCircle()
{
	static float Xoff=45.0f;
	float radius = Stimulus.spacing;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;
	
	if (fulltime < Stimulus.tau)
	{
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			glVertex3f(Xoff,Yoff,Zoff);
			glVertex3f(Xoff,Yoff+radius*cos(da*ii),Zoff+radius*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius*cos((ii+1)*da),Zoff+radius*sin((ii+1)*da));
			glVertex3f(Xoff,Yoff,Zoff);
		}
		glEnd();
	};

};

void TrackStim::drawLocalTorus()
{
	static float Xoff=45.0f;
	float radius = Stimulus.spacing;
	float radius_in = Stimulus.spacing2;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;
	
	if (fulltime < Stimulus.tau)
	{
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			glVertex3f(Xoff,Yoff+radius_in*cos(da*ii),Zoff+radius_in*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius*cos(da*ii),Zoff+radius*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius*cos((ii+1)*da),Zoff+radius*sin((ii+1)*da));
			glVertex3f(Xoff,Yoff+radius_in*cos(da*ii+da),Zoff+radius_in*sin(da*ii+da));
		}
		glEnd();
	};
};

void TrackStim::drawLocalCircleTorusSimul()
{
	static float Xoff=45.0f;
	float radius = Stimulus.spacing;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;
	float gain = Stimulus.transgain;
	float contrast = Stimulus.contrast;
	float currlum = 1;

	float radius_out = Stimulus.spacing2;

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;
	
	if (fulltime < Stimulus.tau)
	{
		
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			glVertex3f(Xoff,Yoff,Zoff);
			glVertex3f(Xoff,Yoff+radius*cos(da*ii),Zoff+radius*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius*cos((ii+1)*da),Zoff+radius*sin((ii+1)*da));
			glVertex3f(Xoff,Yoff,Zoff);
		}
		glEnd();

		if((gain == -1)&&(contrast>0))
		{
			currlum = 0;
			glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		}
		else if((gain == -1)&&(contrast<0))
		{
			currlum = 1;
			glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		}

		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			glVertex3f(Xoff,Yoff+radius*cos(da*ii),Zoff+radius*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius_out*cos(da*ii),Zoff+radius_out*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius_out*cos((ii+1)*da),Zoff+radius_out*sin((ii+1)*da));
			glVertex3f(Xoff,Yoff+radius*cos(da*ii+da),Zoff+radius*sin(da*ii+da));
		}
		glEnd();
		
	};

};

void TrackStim::drawLocalCircleCR()
{
	static bool FIRST = FALSE;
	static bool FILE_FIRST = TRUE;
	static float Xoff=45.0f;
	float radius = Stimulus.spacing;
	float radius1 = 0, radius2 = 0;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;

	float radius_h1 = 0, radius_h2 = 0, radius_v1 = 0, radius_v2 = 0;

	static float thetaH[90][90];
	static float thetaV[90][90];
	static float pos[90];
	static int x_ind, y_ind;
	static float min_x = 100, min_y = 100, cur_val;
	FILE *filein;

	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if(FIRST)
	{
		for(int i = -45; i < 45; i++)
		{
			cur_val = abs(i-Yoff);
			if(cur_val < min_x)
			{
				min_x = cur_val;
				x_ind = i+45;
			}
			cur_val = abs(i-Zoff);
			if(cur_val < min_y)
			{
				min_y = cur_val;
				y_ind = i+45;
			}
		}
		filein = fopen("horizontal_mags.txt", "r");				// File To Load World Data From
		for(int i = 0; i < 90; i++)
			for(int j = 0; j < 90; j++)
				fscanf(filein, "%f", &thetaH[i][j]);
		fclose(filein);
		filein = fopen("vertical_mags.txt", "r");				// File To Load World Data From
		for(int i = 0; i < 90; i++)
			for(int j = 0; j < 90; j++)
				fscanf(filein, "%f", &thetaV[i][j]);
		fclose(filein);

		radius_h1 = 0;
		for(int i = x_ind;i < x_ind+radius; i++)
			radius_h1 = radius_h1+(1/thetaH[i][y_ind]);
		
		radius_h2 = 0;
		for(int i = x_ind;i > x_ind-radius; i--)
			radius_h2 = radius_h2+(1/thetaH[i][y_ind]);
		
		radius_v1 = 0;
		for(int i = y_ind;i < y_ind+radius; i++)
			radius_v1 = radius_v1+(1/thetaV[x_ind][i]);
		
		radius_v2 = 0;
		for(int i = y_ind;i > y_ind-radius; i--)
			radius_v2 = radius_v2+(1/thetaV[x_ind][i]);

		// DEBUG

		//char str[100];
		//sprintf(str,"position = (%d,%d)",x_ind,y_ind);
		//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);

		//sprintf(str,"radii = (%f: %f,%f,%f,%f)",radius,radius_h1,radius_h2,radius_v1,radius_v2);
		//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);

		//char str[50];
		//sprintf(str,"element = %f",thetaH[3][1]);
		//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);

	}

	if (fulltime < Stimulus.tau)
	{
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			if(ii <= 4)
			{
				radius1 = (1-(ii/5.0))*radius_h1+(ii/5.0)*radius_v1;
				radius2 = (1-((ii+1)/5.0))*radius_h1+((ii+1)/5.0)*radius_v1;
				//char str[50];
				//sprintf(str,"radii = (%d,%f,%f)",ii,radius1,radius2);
				//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);
			}
			else if(ii <= 9)
			{
				radius1 = (1-((ii-5)/5.0))*radius_v1+((ii-5)/5.0)*radius_h2;
				radius2 = (1-((ii-4)/5.0))*radius_v1+((ii-4)/5.0)*radius_h2;
				//char str[50];
				//sprintf(str,"radii = (%d,%f,%f)",ii,radius1,radius2);
				//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);
			}
			else if(ii <= 14)
			{
				radius1 = (1-((ii-10)/5.0))*radius_h2+((ii-10)/5.0)*radius_v2;
				radius2 = (1-((ii-9)/5.0))*radius_h2+((ii-9)/5.0)*radius_v2;
				//char str[50];
				//sprintf(str,"radii = (%d,%f,%f)",ii,radius1,radius2);
				//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);
			}
			else if(ii <= 19)
			{
				radius1 = (1-((ii-15)/5.0))*radius_v2+((ii-15)/5.0)*radius_h1;
				radius2 = (1-((ii-14)/5.0))*radius_v2+((ii-14)/5.0)*radius_h1;
				//char str[50];
				//sprintf(str,"radii = (%d,%f,%f)",ii,radius1,radius2);
				//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);
			}
			glVertex3f(Xoff,Yoff,Zoff);
			glVertex3f(Xoff,Yoff+radius1*cos(da*ii),Zoff+radius1*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius2*cos((ii+1)*da),Zoff+radius2*sin((ii+1)*da));
			glVertex3f(Xoff,Yoff,Zoff);	
		}
		glEnd();

		/*
		glReadBuffer(GL_FRONT_RIGHT);
		FILE_FIRST = ((fulltime > 1) && (fulltime < 2));

		if(FILE_FIRST)
		{
			//MessageBox(NULL,"in","INFO",MB_OK|MB_ICONSTOP);
			float data[155][155];
			//data = new float*[ViewPorts.h[0]];
			//for (int i = 0; i < ViewPorts.h[0]; i++)			
			//	data[i] = new float[ViewPorts.w[0]];

			glReadPixels(385,15,155,155,GL_GREEN,GL_FLOAT,&data);
			filein = fopen("pixel_vals.txt", "w");		
			for(int i = 0;i < 155;i++)
			{
				fprintf(filein,"\n");
				for(int j = 0;j < 155;j++)
					fprintf(filein,"%f \t",data[i][j]);
			}
			fclose(filein);
			FILE_FIRST = FALSE;
		}
		*/
	};

};



void TrackStim::drawLocalCircleCR2()
{
	static bool FIRST = FALSE;
	static bool FILE_FIRST = TRUE;
	static float Xoff=45.0f;
	float radius = Stimulus.spacing;
	int rad = radius;
	int r;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;

	float Yoff = int(Stimulus.x_center);
	float Zoff = int(Stimulus.y_center);

	static float ang_rad_matx[21][8];
	static float ang_rad_maty[21][8];
	
	FILE *filein;

	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if(FIRST)
	{

		char str[200];
		char c1,c2;
		c1 = Yoff < 0 ? 'm' : 'p';
		c2 = Zoff < 0 ? 'm' : 'p';

		sprintf(str,"C:\\Users\\Clandininlab\\Desktop\\2pstim_wNI_LB\\Debug\\Data\\morph\\%c%.0f%c%.0f_x_mat.txt",c1,abs(Yoff),c2,abs(Zoff));
		//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);
		filein = fopen(str, "r");				// File To Load World Data From
		for(int i = 0; i < 21; i++)
			for(int j = 0; j < 8; j++)
				fscanf(filein, "%f", &ang_rad_matx[i][j]);
		fclose(filein);
		
		sprintf(str,"C:\\Users\\Clandininlab\\Desktop\\2pstim_wNI_LB\\Debug\\Data\\morph\\%c%.0f%c%.0f_y_mat.txt",c1,abs(Yoff),c2,abs(Zoff));
		filein = fopen(str, "r");
		// File To Load World Data From
		for(int i = 0; i < 21; i++)
			for(int j = 0; j < 8; j++)
				fscanf(filein, "%f", &ang_rad_maty[i][j]);
		fclose(filein);

		switch (rad)
		{
			// rads = [2 4 5 6 8 10 15 20];
			case 2:
				r = 0;
				break;
			case 4:
				r = 1;
				break;
			case 5:
				r = 2;
				break;
			case 6:
				r = 3;
				break;
			case 8:
				r = 4;
				break;
			case 10:
				r = 5;
				break;
			case 15:
				r = 6;
				break;
			case 20:
				r = 7;
				break;
		}
	}

	//char str[200];
	//sprintf(str,"r = %d",r);
	//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);

	if (fulltime < Stimulus.tau)
	{
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			//char str[200];
			//sprintf(str,"y_coord = %.2f",ang_rad_maty[ii][r]);
			//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);
			glVertex3f(Xoff,Yoff,Zoff);
			glVertex3f(Xoff,ang_rad_matx[ii][r],ang_rad_maty[ii][r]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r],ang_rad_maty[ii+1][r]);
			glVertex3f(Xoff,Yoff,Zoff);	
		}
		glEnd();

		/*
		glReadBuffer(GL_FRONT_RIGHT);
		FILE_FIRST = ((fulltime > 1) && (fulltime < 2));

		if(FILE_FIRST)
		{
			//MessageBox(NULL,"in","INFO",MB_OK|MB_ICONSTOP);
			float data[155][155];
			//data = new float*[ViewPorts.h[0]];
			//for (int i = 0; i < ViewPorts.h[0]; i++)			
			//	data[i] = new float[ViewPorts.w[0]];

			glReadPixels(385,15,155,155,GL_GREEN,GL_FLOAT,&data);
			filein = fopen("pixel_vals.txt", "w");		
			for(int i = 0;i < 155;i++)
			{
				fprintf(filein,"\n");
				for(int j = 0;j < 155;j++)
					fprintf(filein,"%f \t",data[i][j]);
			}
			fclose(filein);
			FILE_FIRST = FALSE;
		}
		*/
	};

};



void TrackStim::drawLocalTorusCR()
{
	static bool FIRST = FALSE;
	static float Xoff=45.0f;
	float radius = Stimulus.spacing;
	float radius_in = Stimulus.spacing2;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;

	float radius_h1 = 0, radius_h2 = 0, radius_v1 = 0, radius_v2 = 0;
	float radius_in_h1 = 0, radius_in_h2 = 0, radius_in_v1 = 0, radius_in_v2 = 0;
	float radius1, radius2, radius1_in, radius2_in;

	static float thetaH[90][90];
	static float thetaV[90][90];
	static float pos[90];
	static int x_ind, y_ind;
	static float min_x = 100, min_y = 100, cur_val;
	FILE *filein;

	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if(FIRST)
	{
		for(int i = -45; i < 45; i++)
		{
			cur_val = abs(i-Yoff);
			if(cur_val < min_x)
			{
				min_x = cur_val;
				x_ind = i+45;
			}
			cur_val = abs(i-Zoff);
			if(cur_val < min_y)
			{
				min_y = cur_val;
				y_ind = i+45;
			}
		}
		filein = fopen("horizontal_mags.txt", "r");				// File To Load World Data From
		for(int i = 0; i < 90; i++)
			for(int j = 0; j < 90; j++)
				fscanf(filein, "%f", &thetaH[i][j]);
		fclose(filein);

		filein = fopen("vertical_mags.txt", "r");				// File To Load World Data From
		for(int i = 0; i < 90; i++)
			for(int j = 0; j < 90; j++)
				fscanf(filein, "%f", &thetaV[i][j]);
		fclose(filein);

		radius_h1 = 0;
		radius_in_h1 = 0;
		for(int i = x_ind;i < x_ind+radius; i++)
		{
			radius_h1 = radius_h1+(1/thetaH[i][y_ind]);
			if(i < x_ind+radius_in)
				radius_in_h1 = radius_in_h1+(1/thetaH[i][y_ind]);
		}
		
		radius_h2 = 0;
		radius_in_h2 = 0;
		for(int i = x_ind;i > x_ind-radius; i--)
		{
			radius_h2 = radius_h2+(1/thetaH[i][y_ind]);
			if(i > x_ind-radius_in)
				radius_in_h2 = radius_in_h2+(1/thetaH[i][y_ind]);
		}
		
		radius_v1 = 0;
		radius_in_v1 = 0;
		for(int i = y_ind;i < y_ind+radius; i++)
		{
			radius_v1 = radius_v1+(1/thetaV[x_ind][i]);
			if(i < y_ind+radius_in)
				radius_in_v1 = radius_in_v1+(1/thetaV[x_ind][i]);
		}
		
		radius_v2 = 0;
		radius_in_v2 = 0;
		for(int i = y_ind;i > y_ind-radius; i--)
		{
			radius_v2 = radius_v2+(1/thetaV[x_ind][i]);
			if(i > y_ind-radius_in)
				radius_in_v2 = radius_in_v2+(1/thetaV[x_ind][i]);
		}

	}
	
	if (fulltime < Stimulus.tau)
	{
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{

			if(ii <= 4)
			{
				radius1 = (1-(ii/5.0))*radius_h1+(ii/5.0)*radius_v1;
				radius2 = (1-((ii+1)/5.0))*radius_h1+((ii+1)/5.0)*radius_v1;
				radius1_in = (1-(ii/5.0))*radius_in_h1+(ii/5.0)*radius_in_v1;
				radius2_in = (1-((ii+1)/5.0))*radius_in_h1+((ii+1)/5.0)*radius_in_v1;
			}
			else if(ii <= 9)
			{
				radius1 = (1-((ii-5)/5.0))*radius_v1+((ii-5)/5.0)*radius_h2;
				radius2 = (1-((ii-4)/5.0))*radius_v1+((ii-4)/5.0)*radius_h2;
				radius1_in = (1-((ii-5)/5.0))*radius_in_v1+((ii-5)/5.0)*radius_in_h2;
				radius2_in = (1-((ii-4)/5.0))*radius_in_v1+((ii-4)/5.0)*radius_in_h2;
			}
			else if(ii <= 14)
			{
				radius1 = (1-((ii-10)/5.0))*radius_h2+((ii-10)/5.0)*radius_v2;
				radius2 = (1-((ii-9)/5.0))*radius_h2+((ii-9)/5.0)*radius_v2;
				radius1_in = (1-((ii-10)/5.0))*radius_in_h2+((ii-10)/5.0)*radius_in_v2;
				radius2_in = (1-((ii-9)/5.0))*radius_in_h2+((ii-9)/5.0)*radius_in_v2;
			}
			else if(ii <= 19)
			{
				radius1 = (1-((ii-15)/5.0))*radius_v2+((ii-15)/5.0)*radius_h1;
				radius2 = (1-((ii-14)/5.0))*radius_v2+((ii-14)/5.0)*radius_h1;
				radius1_in = (1-((ii-15)/5.0))*radius_in_v2+((ii-15)/5.0)*radius_in_h1;
				radius2_in = (1-((ii-14)/5.0))*radius_in_v2+((ii-14)/5.0)*radius_in_h1;
			}

			glVertex3f(Xoff,Yoff+radius1_in*cos(da*ii),Zoff+radius1_in*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius1*cos(da*ii),Zoff+radius1*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius2*cos((ii+1)*da),Zoff+radius2*sin((ii+1)*da));
			glVertex3f(Xoff,Yoff+radius2_in*cos(da*ii+da),Zoff+radius2_in*sin(da*ii+da));
		}
		glEnd();
	};
};

void TrackStim::drawLocalTorusCR2()
{
	static bool FIRST = FALSE;
	static float Xoff=45.0f;
	float radius = Stimulus.spacing;
	float radius_in = Stimulus.spacing2;
	int rad = radius, rad_in = radius_in;
	int r, r_in;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;

	static float ang_rad_matx[21][8];
	static float ang_rad_maty[21][8];

	FILE *filein;

	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if(FIRST)
	{
		char str[200];
		char c1,c2;
		c1 = Yoff < 0 ? 'm' : 'p';
		c2 = Zoff < 0 ? 'm' : 'p';

		sprintf(str,"C:\\Users\\Clandininlab\\Desktop\\2pstim_wNI_LB\\Debug\\Data\\morph\\%c%.0f%c%.0f_x_mat.txt",c1,abs(Yoff),c2,abs(Zoff));
		//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);
		filein = fopen(str, "r");				// File To Load World Data From
		for(int i = 0; i < 21; i++)
			for(int j = 0; j < 8; j++)
				fscanf(filein, "%f", &ang_rad_matx[i][j]);
		fclose(filein);
		
		sprintf(str,"C:\\Users\\Clandininlab\\Desktop\\2pstim_wNI_LB\\Debug\\Data\\morph\\%c%.0f%c%.0f_y_mat.txt",c1,abs(Yoff),c2,abs(Zoff));
		filein = fopen(str, "r");
		// File To Load World Data From
		for(int i = 0; i < 21; i++)
			for(int j = 0; j < 8; j++)
				fscanf(filein, "%f", &ang_rad_maty[i][j]);
		fclose(filein);

		switch (rad)
		{
			// rads = [2 4 5 6 8 10 15 20];
			case 2:
				r = 0;
				break;
			case 4:
				r = 1;
				break;
			case 5:
				r = 2;
				break;
			case 6:
				r = 3;
				break;
			case 8:
				r = 4;
				break;
			case 10:
				r = 5;
				break;
			case 15:
				r = 6;
				break;
			case 20:
				r = 7;
				break;
		}

		switch (rad_in)
		{
			// rads = [2 4 5 6 8 10 15 20];
			case 2:
				r_in = 0;
				break;
			case 4:
				r_in = 1;
				break;
			case 5:
				r_in = 2;
				break;
			case 6:
				r_in = 3;
				break;
			case 8:
				r_in = 4;
				break;
			case 10:
				r_in = 5;
				break;
			case 15:
				r_in = 6;
				break;
			case 20:
				r_in = 7;
				break;
		}

		//char str[200];
		//sprintf(str,"r = %.2f, r_in = %.2f",r,r_in);
		//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);
	}
	
	if (fulltime < Stimulus.tau)
	{
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			//char str[200];
			//sprintf(str,"y_coord = %.2f",ang_rad_maty[ii][r]);
			//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);
			glVertex3f(Xoff,ang_rad_matx[ii][r_in],ang_rad_maty[ii][r_in]);
			glVertex3f(Xoff,ang_rad_matx[ii][r],ang_rad_maty[ii][r]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r],ang_rad_maty[ii+1][r]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r_in],ang_rad_maty[ii+1][r_in]);	
		}
		glEnd();
	};
};

void TrackStim::drawLocalCircleTorusSimulCR()
{
	static bool FIRST = FALSE;
	static float Xoff=45.0f;
	float radius = Stimulus.spacing;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;
	float gain = Stimulus.transgain;
	float contrast = Stimulus.contrast;
	float currlum = 1;

	float radius_out = Stimulus.spacing2;

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;

	float radius_h1 = 0, radius_h2 = 0, radius_v1 = 0, radius_v2 = 0;
	float radius_out_h1 = 0, radius_out_h2 = 0, radius_out_v1 = 0, radius_out_v2 = 0;
	float radius1, radius2, radius1_out, radius2_out;

	static float thetaH[90][90];
	static float thetaV[90][90];
	static float pos[90];
	static int x_ind, y_ind;
	static float min_x = 100, min_y = 100, cur_val;
	FILE *filein;

	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if(FIRST)
	{
		for(int i = -45; i < 45; i++)
		{
			cur_val = abs(i-Yoff);
			if(cur_val < min_x)
			{
				min_x = cur_val;
				x_ind = i+45;
			}
			cur_val = abs(i-Zoff);
			if(cur_val < min_y)
			{
				min_y = cur_val;
				y_ind = i+45;
			}
		}
		filein = fopen("horizontal_mags.txt", "r");				// File To Load World Data From
		for(int i = 0; i < 90; i++)
			for(int j = 0; j < 90; j++)
				fscanf(filein, "%f", &thetaH[i][j]);
		fclose(filein);

		filein = fopen("vertical_mags.txt", "r");				// File To Load World Data From
		for(int i = 0; i < 90; i++)
			for(int j = 0; j < 90; j++)
				fscanf(filein, "%f", &thetaV[i][j]);
		fclose(filein);

		radius_h1 = 0;
		radius_out_h1 = 0;
		for(int i = x_ind;i < x_ind+radius_out; i++)
		{
			radius_out_h1 = radius_out_h1+(1/thetaH[i][y_ind]);
			if(i < x_ind+radius)
				radius_h1 = radius_h1+(1/thetaH[i][y_ind]);
		}
		
		radius_h2 = 0;
		radius_out_h2 = 0;
		for(int i = x_ind;i > x_ind-radius_out; i--)
		{
			radius_out_h2 = radius_out_h2+(1/thetaH[i][y_ind]);
			if(i > x_ind-radius)
				radius_h2 = radius_h2+(1/thetaH[i][y_ind]);
		}
		
		radius_v1 = 0;
		radius_out_v1 = 0;
		for(int i = y_ind;i < y_ind+radius_out; i++)
		{
			radius_out_v1 = radius_out_v1+(1/thetaV[x_ind][i]);
			if(i < y_ind+radius)
				radius_v1 = radius_v1+(1/thetaV[x_ind][i]);
		}
		
		radius_out_v2 = 0;
		radius_v2 = 0;
		for(int i = y_ind;i > y_ind-radius_out; i--)
		{
			radius_out_v2 = radius_out_v2+(1/thetaV[x_ind][i]);
			if(i > y_ind-radius)
				radius_v2 = radius_v2+(1/thetaV[x_ind][i]);
		}

	}
	
	if (fulltime < Stimulus.tau)
	{
		
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{

			if(ii <= 4)
			{
				radius1_out = (1-(ii/5.0))*radius_out_h1+(ii/5.0)*radius_out_v1;
				radius2_out = (1-((ii+1)/5.0))*radius_out_h1+((ii+1)/5.0)*radius_out_v1;
				radius1 = (1-(ii/5.0))*radius_h1+(ii/5.0)*radius_v1;
				radius2 = (1-((ii+1)/5.0))*radius_h1+((ii+1)/5.0)*radius_v1;
			}
			else if(ii <= 9)
			{
				radius1_out = (1-((ii-5)/5.0))*radius_out_v1+((ii-5)/5.0)*radius_out_h2;
				radius2_out = (1-((ii-4)/5.0))*radius_out_v1+((ii-4)/5.0)*radius_out_h2;
				radius1 = (1-((ii-5)/5.0))*radius_v1+((ii-5)/5.0)*radius_h2;
				radius2 = (1-((ii-4)/5.0))*radius_v1+((ii-4)/5.0)*radius_h2;
			}
			else if(ii <= 14)
			{
				radius1_out = (1-((ii-10)/5.0))*radius_out_h2+((ii-10)/5.0)*radius_out_v2;
				radius2_out = (1-((ii-9)/5.0))*radius_out_h2+((ii-9)/5.0)*radius_out_v2;
				radius1 = (1-((ii-10)/5.0))*radius_h2+((ii-10)/5.0)*radius_v2;
				radius2 = (1-((ii-9)/5.0))*radius_h2+((ii-9)/5.0)*radius_v2;
			}
			else if(ii <= 19)
			{
				radius1_out = (1-((ii-15)/5.0))*radius_out_v2+((ii-15)/5.0)*radius_out_h1;
				radius2_out = (1-((ii-14)/5.0))*radius_out_v2+((ii-14)/5.0)*radius_out_h1;
				radius1 = (1-((ii-15)/5.0))*radius_v2+((ii-15)/5.0)*radius_h1;
				radius2 = (1-((ii-14)/5.0))*radius_v2+((ii-14)/5.0)*radius_h1;
			}

			glVertex3f(Xoff,Yoff,Zoff);
			glVertex3f(Xoff,Yoff+radius1*cos(da*ii),Zoff+radius1*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius2*cos((ii+1)*da),Zoff+radius2*sin((ii+1)*da));
			glVertex3f(Xoff,Yoff,Zoff);
		}
		glEnd();

		if((gain == -1)&&(contrast>0))
		{
			currlum = 0;
			glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		}
		else if((gain == -1)&&(contrast<0))
		{
			currlum = 1;
			glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		}

		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			if(ii <= 4)
			{
				radius1_out = (1-(ii/5.0))*radius_out_h1+(ii/5.0)*radius_out_v1;
				radius2_out = (1-((ii+1)/5.0))*radius_out_h1+((ii+1)/5.0)*radius_out_v1;
				radius1 = (1-(ii/5.0))*radius_h1+(ii/5.0)*radius_v1;
				radius2 = (1-((ii+1)/5.0))*radius_h1+((ii+1)/5.0)*radius_v1;
			}
			else if(ii <= 9)
			{
				radius1_out = (1-((ii-5)/5.0))*radius_out_v1+((ii-5)/5.0)*radius_out_h2;
				radius2_out = (1-((ii-4)/5.0))*radius_out_v1+((ii-4)/5.0)*radius_out_h2;
				radius1 = (1-((ii-5)/5.0))*radius_v1+((ii-5)/5.0)*radius_h2;
				radius2 = (1-((ii-4)/5.0))*radius_v1+((ii-4)/5.0)*radius_h2;
			}
			else if(ii <= 14)
			{
				radius1_out = (1-((ii-10)/5.0))*radius_out_h2+((ii-10)/5.0)*radius_out_v2;
				radius2_out = (1-((ii-9)/5.0))*radius_out_h2+((ii-9)/5.0)*radius_out_v2;
				radius1 = (1-((ii-10)/5.0))*radius_h2+((ii-10)/5.0)*radius_v2;
				radius2 = (1-((ii-9)/5.0))*radius_h2+((ii-9)/5.0)*radius_v2;
			}
			else if(ii <= 19)
			{
				radius1_out = (1-((ii-15)/5.0))*radius_out_v2+((ii-15)/5.0)*radius_out_h1;
				radius2_out = (1-((ii-14)/5.0))*radius_out_v2+((ii-14)/5.0)*radius_out_h1;
				radius1 = (1-((ii-15)/5.0))*radius_v2+((ii-15)/5.0)*radius_h1;
				radius2 = (1-((ii-14)/5.0))*radius_v2+((ii-14)/5.0)*radius_h1;
				//char str[50];
				//sprintf(str,"radii = (%d,%f,%f,%f,%f)",ii,radius1,radius2,radius1_out,radius2_out);
				//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);
			}

			glVertex3f(Xoff,Yoff+radius1*cos(da*ii),Zoff+radius1*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius1_out*cos(da*ii),Zoff+radius1_out*sin(da*ii));
			glVertex3f(Xoff,Yoff+radius2_out*cos((ii+1)*da),Zoff+radius2_out*sin((ii+1)*da));
			glVertex3f(Xoff,Yoff+radius2*cos(da*ii+da),Zoff+radius2*sin(da*ii+da));
		}
		glEnd();
		
	};

};





void TrackStim::drawLocalCircleTorusSimulCR2()
{
	static bool FIRST = FALSE;
	static float Xoff=45.0f;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;
	float gain = Stimulus.transgain;
	float contrast = Stimulus.contrast;
	float currlum = 1;

	float radius = Stimulus.spacing;
	float radius_out = Stimulus.spacing2;
	int rad = radius, rad_out = radius_out;
	int r, r_out;

	static float ang_rad_matx[21][8];
	static float ang_rad_maty[21][8];

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;

	FILE *filein;

	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if(FIRST)
	{
		char str[200];
		char c1,c2;
		c1 = Yoff < 0 ? 'm' : 'p';
		c2 = Zoff < 0 ? 'm' : 'p';

		sprintf(str,"C:\\Users\\Clandininlab\\Desktop\\2pstim_wNI_LB\\Debug\\Data\\morph\\%c%.0f%c%.0f_x_mat.txt",c1,abs(Yoff),c2,abs(Zoff));
		//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);
		filein = fopen(str, "r");				// File To Load World Data From
		for(int i = 0; i < 21; i++)
			for(int j = 0; j < 8; j++)
				fscanf(filein, "%f", &ang_rad_matx[i][j]);
		fclose(filein);
		
		sprintf(str,"C:\\Users\\Clandininlab\\Desktop\\2pstim_wNI_LB\\Debug\\Data\\morph\\%c%.0f%c%.0f_y_mat.txt",c1,abs(Yoff),c2,abs(Zoff));
		filein = fopen(str, "r");
		// File To Load World Data From
		for(int i = 0; i < 21; i++)
			for(int j = 0; j < 8; j++)
				fscanf(filein, "%f", &ang_rad_maty[i][j]);
		fclose(filein);

		switch (rad)
		{
			// rads = [2 4 5 6 8 10 15 20];
			case 2:
				r = 0;
				break;
			case 4:
				r = 1;
				break;
			case 5:
				r = 2;
				break;
			case 6:
				r = 3;
				break;
			case 8:
				r = 4;
				break;
			case 10:
				r = 5;
				break;
			case 15:
				r = 6;
				break;
			case 20:
				r = 7;
				break;
		}

		switch (rad_out)
		{
			// rads = [2 4 5 6 8 10 15 20];
			case 2:
				r_out = 0;
				break;
			case 4:
				r_out = 1;
				break;
			case 5:
				r_out = 2;
				break;
			case 6:
				r_out = 3;
				break;
			case 8:
				r_out = 4;
				break;
			case 10:
				r_out = 5;
				break;
			case 15:
				r_out = 6;
				break;
			case 20:
				r_out = 7;
				break;
		}
	}
	
	if (fulltime < Stimulus.tau)
	{
		
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			glVertex3f(Xoff,Yoff,Zoff);
			glVertex3f(Xoff,ang_rad_matx[ii][r],ang_rad_maty[ii][r]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r],ang_rad_maty[ii+1][r]);
			glVertex3f(Xoff,Yoff,Zoff);	
		}
		glEnd();

		if((gain == -1)&&(contrast>0))
		{
			currlum = 0;
			glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		}
		else if((gain == -1)&&(contrast<0))
		{
			currlum = 1;
			glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		}

		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{			
			glVertex3f(Xoff,ang_rad_matx[ii][r],ang_rad_maty[ii][r]);
			glVertex3f(Xoff,ang_rad_matx[ii][r_out],ang_rad_maty[ii][r_out]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r_out],ang_rad_maty[ii+1][r_out]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r],ang_rad_maty[ii+1][r]);	
		}
		glEnd();
		
	};

};





void TrackStim::drawMovingBars()
{
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	static float phase = 0;
	float Xoff = 45;
	float side = 360;

	phase += v/240; // update rate of 240 hz
	phase = mod(phase, 2*d);
	Sfull.x = phase;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);

	glBegin(GL_QUADS);
	for (int ii=-int(side/(2*d)); ii<int(side/2*d); ii+=2)
	{
		glVertex3f(Xoff,phase+d*ii,-side);
		glVertex3f(Xoff,phase+d*(ii+1),-side);
		glVertex3f(Xoff,phase+d*(ii+1),side);
		glVertex3f(Xoff,phase+d*ii,side);
	};
	glEnd();
	glPopMatrix();


};

void TrackStim::drawMovingBarsHS()
{
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	static float phase = 0;
	float Xoff = 45;
	float side = 360;

	float time = Sfull.tcurr - epochtimezero;
	if(time > Stimulus.spacing2)
	{
		phase += v/240; // update rate of 240 hz
		phase = mod(phase, 2*d);
	}
	else
	{
		phase += 0;
	}
	Sfull.x = phase;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);

	glBegin(GL_QUADS);
	for (int ii=-int(side/(2*d)); ii<int(side/2*d); ii+=2)
	{
		glVertex3f(Xoff,phase+d*ii,-side);
		glVertex3f(Xoff,phase+d*(ii+1),-side);
		glVertex3f(Xoff,phase+d*(ii+1),side);
		glVertex3f(Xoff,phase+d*ii,side);
	};
	glEnd();
	glPopMatrix();


};

void TrackStim::drawStaticGrid()
{
	float d = Stimulus.spacing;
	float rotation = Stimulus.stimrot.mean;
	float per = 30; // every 30 degrees
	float Xoff = 45;
	float side = 360;
	
	// could add in rotation here if we please -- check Limor's bar pair data first!
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	for (int ii=-60; ii<75; ii+=30)
	{
		glVertex3f(Xoff,ii-d/2,-side);
		glVertex3f(Xoff,ii+d/2,-side);
		glVertex3f(Xoff,ii+d/2,side);
		glVertex3f(Xoff,ii-d/2,side);
	};
	glEnd();
	glPopMatrix();


};


void TrackStim::drawStaticGridLB()
{
	float d = Stimulus.spacing;
	float rotation = Stimulus.stimrot.mean;
	float per = Stimulus.spacing2; // every 30 degrees
	float Xoff = 45;
	float side = 360;
	float offset = Stimulus.trans.mean;
	
	// could add in rotation here if we please -- check Limor's bar pair data first!
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	for (int ii=-60; ii<=60; ii+=per)
	{
		glVertex3f(Xoff,ii-d/2+offset,-side);
		glVertex3f(Xoff,ii+d/2+offset,-side);
		glVertex3f(Xoff,ii+d/2+offset,side);
		glVertex3f(Xoff,ii-d/2+offset,side);
	};
	glEnd();
	glPopMatrix();


};


void TrackStim::drawStaticGridLB2()
{
	float d = Stimulus.spacing;
	float rotation = Stimulus.stimrot.mean;
	float per = Stimulus.spacing2; // every 30 degrees
	float Xoff = 45;
	float side = 360;
	float offset = Stimulus.trans.mean;
	
	// could add in rotation here if we please -- check Limor's bar pair data first!
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	for (int ii=-45; ii<=45; ii+=per)
	{
		glVertex3f(Xoff,ii-d/2+offset,-side);
		glVertex3f(Xoff,ii+d/2+offset,-side);
		glVertex3f(Xoff,ii+d/2+offset,side);
		glVertex3f(Xoff,ii-d/2+offset,side);
	};
	glEnd();
	glPopMatrix();


};


void TrackStim::drawMovingBar()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = -side;
		FIRST=FALSE;
	};

	phase += v/240; // update rate of 240 hz
	phase = mod(phase+side, 2*side) - side;
	Sfull.x = phase;
	Sfull.theta=rotation*PI/180;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	glVertex3f(Xoff,phase-d/2,-side);
	glVertex3f(Xoff,phase+d/2,-side);
	glVertex3f(Xoff,phase+d/2,side);
	glVertex3f(Xoff,phase-d/2,side);
	glEnd();
	glPopMatrix();


};

void TrackStim::drawMovingBar_large()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d; // same period for all directions, otherwise awkward, i think
	static float phase = side;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = side;
		FIRST=FALSE;
	};

	phase += v/240; // update rate of 240 hz
	/*if (phase<0)
		phase = -mod(-phase, 2*d);
	else
		phase = mod(phase,2*d);
	*/
	Sfull.x = phase;
	Sfull.theta=rotation*PI/180;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	for (int ii=0; ii<1; ii++)
	//for (int ii=-int(side/(2*d)); ii<int(side/2*d); ii+=2)
	{
		glVertex3f(Xoff,phase+d*ii-d/2,-side);
		glVertex3f(Xoff,phase+d*(ii+1)-d/2,-side);
		glVertex3f(Xoff,phase+d*(ii+1)-d/2,side);
		glVertex3f(Xoff,phase+d*ii-d/2,side);
	};
	glEnd();
	glPopMatrix();

};

void TrackStim::drawMovingBar_grid()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = -side;
		FIRST=FALSE;
	};

	phase += v/240; // update rate of 240 hz
	phase = mod(phase+side, 2*side) - side;
	phgrid = floor(phase/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	glVertex3f(Xoff,phgrid-d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,side);
	glVertex3f(Xoff,phgrid-d/2,side);
	glEnd();
	glPopMatrix();


};

void TrackStim::drawMovingBar_grid_LB()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 90 + d/2; // same period for all directions, a long period - to avoid mixing responses
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = -side;
		FIRST=FALSE;
	};

	phase += v/240; // update rate of 240 hz
	phase = mod(phase+side, 2*side) - side;
	phgrid = floor(phase/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	glVertex3f(Xoff,phgrid-d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,side);
	glVertex3f(Xoff,phgrid-d/2,side);
	glEnd();
	glPopMatrix();


};

// reduces the amount of time the bar is not on the screen
void TrackStim::drawMovingBar_grid_LB_onscreen()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 60 + d/2; // same period for all directions, a long period - to avoid mixing responses
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = -side;
		FIRST=FALSE;
	};

	phase += v/240; // update rate of 240 hz
	phase = mod(phase+side, 2*side) - side;
	phgrid = floor(phase/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	glVertex3f(Xoff,phgrid-d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,side);
	glVertex3f(Xoff,phgrid-d/2,side);
	glEnd();
	glPopMatrix();


};

void TrackStim::drawMovingBar_grid_once()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = side;
		FIRST=FALSE;
	};

	phase += v/240; // update rate of 240 hz
	//phase = mod(phase+side, 2*side) - side;
	phgrid = floor(phase/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	glVertex3f(Xoff,phgrid-d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,side);
	glVertex3f(Xoff,phgrid-d/2,side);
	glEnd();
	glPopMatrix();


};

void TrackStim::drawMovingBar_grid_replicateFFF()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = -side;
		FIRST=FALSE;
	};

	phase += v/240; // update rate of 240 hz
	phase = mod(phase+side, 2*side) - side;
	phgrid = floor(phase/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.theta=rotation*PI/180;
	
	if (abs(phase)<5) // use 5 degree FOV for column? 
	{
		Sfull.x = phgrid;
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,phgrid-side,-side);
		glVertex3f(Xoff,phgrid+side,-side);
		glVertex3f(Xoff,phgrid+side,side);
		glVertex3f(Xoff,phgrid-side,side);
		glEnd();
		glPopMatrix();
	};


};

void TrackStim::drawMovingBar_grid_randomspace()
{
	static bool FIRST = TRUE;
	static bool MADEMAT = FALSE;
	static float index[200]={0.0f};
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = -side;
		FIRST=FALSE;
	};

	int Nsq = floor(2*side/Stimulus.spacing2);
	if (!MADEMAT)
	{
		MADEMAT = TRUE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii+1;
		// randomize it
 		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}
	};

	phase += v/240; // update rate of 240 hz
	phase = mod(phase+side, 2*side) - side;
	// put it in a random bin
	int stimnumber=int(floor((phase+side)/Stimulus.spacing2)) % Nsq;
	phgrid = floor((index[stimnumber]*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	glVertex3f(Xoff,phgrid-d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,side);
	glVertex3f(Xoff,phgrid-d/2,side);
	glEnd();
	glPopMatrix();


};

void TrackStim::drawBar_grid_randomspace_permute()
{
	static bool FIRST = TRUE;
	static bool SETUPPERM = TRUE;
	static bool WAITED = FALSE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = ceil(2*side/Stimulus.spacing2)+1;

	static int index[400];
	if (SETUPPERM)
	{
		SETUPPERM = FALSE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}
	};

	static int stimcount = 0;
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=index[stimcount % Nsq];
		stimcount++;
	};

	// put it in a random bin
	
	phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,phgrid-d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,2*side);
		glVertex3f(Xoff,phgrid-d/2,2*side);
		glEnd();
		glPopMatrix();
	};


};

void TrackStim::drawBar_grid_randomspace_permute_LB()
{
	static bool FIRST = TRUE;
	static bool SETUPPERM = TRUE;
	static bool WAITED = FALSE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = ceil(2*side/Stimulus.spacing2)+1;

	static int index[400];
	if (SETUPPERM)
	{
		srand(time(NULL));
		SETUPPERM = FALSE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}
	};

	static int stimcount = 0;
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=index[stimcount % Nsq];
		stimcount++;
	};

	// put it in a random bin
	
	phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,phgrid-d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,2*side);
		glVertex3f(Xoff,phgrid-d/2,2*side);
		glEnd();
		glPopMatrix();
	};


};

/*
bar at random position on grid - adapted from drawBar_grid_randomspace_permute_LB()
note: only use with Stimulus.stimrot.mean of 0, 90, 180, 270 (i.e. bar orthogonal to 1 side and perpendicular to other)
otherwise, bar won't span whole screen; LB version makes it such that bar is not always on the screen in this situation

*/
void TrackStim::drawBar_grid_randomspace_permute_HY()
{
	static bool FIRST = TRUE;
	static bool SETUPPERM = TRUE;
	static bool WAITED = FALSE;
	float d = Stimulus.spacing; // width of bar
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	//float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	float side = 45;// + d/2; // remove sqrt(2) so that bar always appears on the screen
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = ceil(2*side/Stimulus.spacing2)+1;

	static int index[400];
	if (SETUPPERM)
	{
		srand(time(NULL));
		SETUPPERM = FALSE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}
	};

	static int stimcount = 0;
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=index[stimcount % Nsq];
		stimcount++;
	};

	// put it in a random bin
	
	phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,phgrid-d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,2*side);
		glVertex3f(Xoff,phgrid-d/2,2*side);
		glEnd();
		glPopMatrix();
	};


};
/* Draws light bar but with correction for bleed through
	
	Notes:
		bar is necessarily 10 degrees wide
*/

void TrackStim::drawLightBarCorrected_grid_randomspace_permute_HY()
{
	static bool FIRST = TRUE;
	static bool SETUPPERM = TRUE;
	static bool WAITED = FALSE;
	static float d = Stimulus.spacing; // width of bar
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	//float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	float side = 45;// + d/2; // remove sqrt(2) so that bar always appears on the screen
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = ceil(2*side/Stimulus.spacing2)+1;

	float bgcol = Stimulus.lum *(1 - Stimulus.contrast);
	float fgcol = Stimulus.lum *(1 + Stimulus.contrast);
	float corWidth = 10; // width on either side of bar to correct for
	float expo = 0.014; // exponent factor in correction
	//static int idSize = (corWidth*2+d)*3;
	static float imagedata[30*3]; // 1D texture definition - necessarily 10 deg bar with 10 on either side (will fix later, static requires constant)

	static int index[400];
	if (SETUPPERM)
	{
		srand(time(NULL));
		SETUPPERM = FALSE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}

		// generate texture
		for (int ii=0; ii<(corWidth*2+d); ii++)
		{
			if (ii < corWidth) // left of bar
			{
				imagedata[ii*3]=0.0f;
				imagedata[ii*3+1]=getDLPColor(bgcol-(bgcol*pow(E,(expo*(ii+1)))-bgcol),'G');
				imagedata[ii*3+2]=getDLPColor(bgcol-(bgcol*pow(E,(expo*(ii+1)))-bgcol),'B');
				// for testing
				//imagedata[ii*3+1]=getDLPColor(bgcol-(bgcol*pow(E,(0.05*(ii+1)))-bgcol),'G');
				//imagedata[ii*3+2]=getDLPColor(bgcol-(bgcol*pow(E,(0.05*(ii+1)))-bgcol),'B');
			}
			else if (ii < (d+corWidth)) // within bar
			{
				imagedata[ii*3]=0.0f;
				imagedata[ii*3+1]=getDLPColor(fgcol,'G');
				imagedata[ii*3+2]=getDLPColor(fgcol,'B');
			}
			else // right of bar
			{
				imagedata[ii*3]=0.0f;
				imagedata[ii*3+1]=getDLPColor(bgcol-(bgcol*pow(E,expo*((corWidth*2+d)-ii))-bgcol),'G');
				imagedata[ii*3+2]=getDLPColor(bgcol-(bgcol*pow(E,expo*((corWidth*2+d)-ii))-bgcol),'B');
				// for testing
				//imagedata[ii*3+1]=getDLPColor(bgcol-(bgcol*pow(E,0.05*((corWidth*2+d)-ii))-bgcol),'G');
				//imagedata[ii*3+2]=getDLPColor(bgcol-(bgcol*pow(E,0.05*((corWidth*2+d)-ii))-bgcol),'B');
			}
		}
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 30, 0, GL_RGB, GL_FLOAT, imagedata);
	};

	static int stimcount = 0;
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=index[stimcount % Nsq];
		stimcount++;
	};

	// put it in a random bin
	
	phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D,1);
		glBegin(GL_QUADS);
		glTexCoord1f(0.0);
		glVertex3f(Xoff,phgrid-d/2-corWidth,-2*side);
		glTexCoord1f(1.0);
		glVertex3f(Xoff,phgrid+d/2+corWidth,-2*side);
		glTexCoord1f(1.0);
		glVertex3f(Xoff,phgrid+d/2+corWidth,2*side);
		glTexCoord1f(0.0);
		glVertex3f(Xoff,phgrid-d/2-corWidth,2*side);
		glEnd();
		glDisable(GL_TEXTURE_1D);
		glPopMatrix();
	};


};
/* For measuring correction factor needed, only displays bar in one location on screen */
void TrackStim::drawLightBarCorrected_grid_randomspace_permute_HY_test()
{
	static bool FIRST = TRUE;
	static bool SETUPPERM = TRUE;
	static bool WAITED = FALSE;
	static float d = Stimulus.spacing; // width of bar
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	//float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	float side = 45;// + d/2; // remove sqrt(2) so that bar always appears on the screen
	static float phase = -side;
	float phgrid = 0;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = ceil(2*side/Stimulus.spacing2)+1;

	float bgcol = Stimulus.lum *(1 - Stimulus.contrast);
	float fgcol = Stimulus.lum *(1 + Stimulus.contrast);
	float corWidth = 10; // width on either side of bar to correct for
	float expo = 0.014; // exponent factor in correction
	//static int idSize = (corWidth*2+d)*3;
	static float imagedata[30*3]; // 1D texture definition - necessarily 10 deg bar with 10 on either side (will fix later, static requires constant)

	static int index[400];
	if (SETUPPERM)
	{
		srand(time(NULL));
		SETUPPERM = FALSE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}

		// generate texture
		for (int ii=0; ii<(corWidth*2+d); ii++)
		{
			if (ii < corWidth) // left of bar
			{
				imagedata[ii*3]=0.0f;
				imagedata[ii*3+1]=getDLPColor(bgcol-(bgcol*pow(E,(expo*(ii+1)))-bgcol),'G');
				imagedata[ii*3+2]=getDLPColor(bgcol-(bgcol*pow(E,(expo*(ii+1)))-bgcol),'B');
				// for testing
				//imagedata[ii*3+1]=getDLPColor(bgcol-(bgcol*pow(E,(0.05*(ii+1)))-bgcol),'G');
				//imagedata[ii*3+2]=getDLPColor(bgcol-(bgcol*pow(E,(0.05*(ii+1)))-bgcol),'B');
			}
			else if (ii < (d+corWidth)) // within bar
			{
				imagedata[ii*3]=0.0f;
				imagedata[ii*3+1]=getDLPColor(fgcol,'G');
				imagedata[ii*3+2]=getDLPColor(fgcol,'B');
			}
			else // right of bar
			{
				imagedata[ii*3]=0.0f;
				imagedata[ii*3+1]=getDLPColor(bgcol-(bgcol*pow(E,expo*((corWidth*2+d)-ii))-bgcol),'G');
				imagedata[ii*3+2]=getDLPColor(bgcol-(bgcol*pow(E,expo*((corWidth*2+d)-ii))-bgcol),'B');
				// for testing
				//imagedata[ii*3+1]=getDLPColor(bgcol-(bgcol*pow(E,0.05*((corWidth*2+d)-ii))-bgcol),'G');
				//imagedata[ii*3+2]=getDLPColor(bgcol-(bgcol*pow(E,0.05*((corWidth*2+d)-ii))-bgcol),'B');
			}
		}
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		//glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, 30, 0, GL_RGB, GL_FLOAT, imagedata);
	};

	static int stimcount = 0;
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=index[stimcount % Nsq];
		stimcount++;
	};

	// put it in a random bin
	
	//phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glEnable(GL_TEXTURE_1D);
		glBindTexture(GL_TEXTURE_1D,1);
		glBegin(GL_QUADS);
		glTexCoord1f(0.0);
		glVertex3f(Xoff,phgrid-d/2-corWidth,-2*side);
		glTexCoord1f(1.0);
		glVertex3f(Xoff,phgrid+d/2+corWidth,-2*side);
		glTexCoord1f(1.0);
		glVertex3f(Xoff,phgrid+d/2+corWidth,2*side);
		glTexCoord1f(0.0);
		glVertex3f(Xoff,phgrid-d/2-corWidth,2*side);
		glEnd();
		glDisable(GL_TEXTURE_1D);
		glPopMatrix();
	};


};

/* Same as above stimulus (displays bar only at the screen center), but without correction */
void TrackStim::drawBar_grid_randomspace_permute_HY_test()
{
	static bool FIRST = TRUE;
	static bool SETUPPERM = TRUE;
	static bool WAITED = FALSE;
	float d = Stimulus.spacing; // width of bar
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	//float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	float side = 45;// + d/2; // remove sqrt(2) so that bar always appears on the screen
	static float phase = -side;
	float phgrid = 0;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = ceil(2*side/Stimulus.spacing2)+1;

	static int index[400];
	if (SETUPPERM)
	{
		srand(time(NULL));
		SETUPPERM = FALSE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}
	};

	static int stimcount = 0;
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=index[stimcount % Nsq];
		stimcount++;
	};

	// put it in a random bin
	
	//phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,phgrid-d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,2*side);
		glVertex3f(Xoff,phgrid-d/2,2*side);
		glEnd();
		glPopMatrix();
	};


};
// the same as drawBar_grid_randomspace_permute_HY but ajusted for the fly eye cordinates-yf
// 
//drawBar_grid_randomspace_permute_flyCord_conscreen_YF()
void TrackStim::drawBar_grid_randomspace_permute_flyCord_onscreen_YF()
{
	static bool FIRST = TRUE;
	static bool SETUPPERM = TRUE;
	static bool WAITED = FALSE;
	float d = Stimulus.spacing; // width of bar
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	//float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	float side = 45;// + d/2; // remove sqrt(2) so that bar always appears on the screen
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = ceil(2*side/Stimulus.spacing2)+1;

	static int index[400];
	if (SETUPPERM)
	{
		srand(time(NULL));
		SETUPPERM = FALSE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}
	};

	static int stimcount = 0;
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=index[stimcount % Nsq];
		stimcount++;
	};

	// put it in a random bin
	
	phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
// putting the world into fly cordinates using code from other functions-yf
	transformToScopeCoords();

	//float rollang=Stimulus.stimrot.mean; // not sure if we need this?-yf
	// now, put it into the right pitch coordinates...
//	glRotatef(0.0f,0.0f,1.0f,0.0f); // rotates for head now draw however...
	glRotatef(-perspHEAD,0.0f,1.0f,0.0f); // rotates for head now draw however...
	// now, roll it however to get right perp angle...
//	glRotatef(rollang,-1.0f,0.0f,0.0f);
	//glRotatef(rollang,1.0f,0.0f,0.0f);// commented out-yf
	//glRotatef(0.0f,-cos(perspHEAD*180.0f/PI),0.0f,sin(perspHEAD*180.0f/PI));


	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,phgrid-d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,2*side);
		glVertex3f(Xoff,phgrid-d/2,2*side);
		glEnd();
		glPopMatrix();
	};


};

// the same as drawBar_grid_randomspace_permute_LB but ajusted for the fly eye cordinates-yf
// not all bars are on the screen....
void TrackStim::drawBar_grid_randomspace_permute_flyCord_YF()
{
	static bool FIRST = TRUE;
	static bool SETUPPERM = TRUE;
	static bool WAITED = FALSE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = ceil(2*side/Stimulus.spacing2)+1;

	static int index[400];
	if (SETUPPERM)
	{
		srand(time(NULL));
		SETUPPERM = FALSE;
		for (int ii = 0; ii< Nsq; ii++)
			index[ii]=ii;
		// randomize it
		for (int ii = 0; ii < Nsq; ii++)
		{
			int randNum, temporary;
			randNum = rand( ) % Nsq;
			temporary = index[ii];
			index[ii] = index[randNum];
			index[randNum] = temporary;
		}
	};

	static int stimcount = 0;
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=index[stimcount % Nsq];
		stimcount++;
	};

	// put it in a random bin
	
	phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
// putting the world into fly cordinates using code from other functions-yf
	transformToScopeCoords();

	float rollang=Stimulus.stimrot.mean; // not sure if we need this?-yf
	// now, put it into the right pitch coordinates...
//	glRotatef(0.0f,0.0f,1.0f,0.0f); // rotates for head now draw however...
	glRotatef(-perspHEAD,0.0f,1.0f,0.0f); // rotates for head now draw however...
	// now, roll it however to get right perp angle...
//	glRotatef(rollang,-1.0f,0.0f,0.0f);
	glRotatef(rollang,1.0f,0.0f,0.0f);// commented out-yf
	//glRotatef(0.0f,-cos(perspHEAD*180.0f/PI),0.0f,sin(perspHEAD*180.0f/PI));




	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		//glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,phgrid-d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,2*side);
		glVertex3f(Xoff,phgrid-d/2,2*side);
		glEnd();
		glPopMatrix();
	};


};

// same as drawMovingBar_grid_LB but changes to be in fly eye cordinates
void TrackStim::drawMovingBar_grid_flyCord()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 90 + d/2; // same period for all directions, a long period - to avoid mixing responses
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if (FIRST)
	{
		phase = -side;
		FIRST=FALSE;
	};

	phase += v/240; // update rate of 240 hz
	phase = mod(phase+side, 2*side) - side;
	phgrid = floor(phase/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	

	// putting the world into fly cordinates using code from other functions-yf
		transformToScopeCoords();

		//float rollang=Stimulus.stimrot.mean; // not sure if we need this?-yf
		// now, put it into the right pitch coordinates...
	//	glRotatef(0.0f,0.0f,1.0f,0.0f); // rotates for head now draw however...
		glRotatef(-perspHEAD,0.0f,1.0f,0.0f); // rotates for head now draw however...
		// now, roll it however to get right perp angle...
	//	glRotatef(rollang,-1.0f,0.0f,0.0f);
		//glRotatef(rollang,1.0f,0.0f,0.0f);// commented out-yf
		//glRotatef(0.0f,-cos(perspHEAD*180.0f/PI),0.0f,sin(perspHEAD*180.0f/PI));



	glPushMatrix();
	glRotatef(rotation,1,0,0);
	glBegin(GL_QUADS);
	glVertex3f(Xoff,phgrid-d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,-side);
	glVertex3f(Xoff,phgrid+d/2,side);
	glVertex3f(Xoff,phgrid-d/2,side);
	glEnd();
	glPopMatrix();


};

void TrackStim::drawBar_grid_randomspace()
{
	static bool FIRST = TRUE;
	static bool WAITED = FALSE;
	static float index[200]={0.0f};
	float d = Stimulus.spacing; // half period
	float v = Stimulus.stimtrans.mean; // degrees per second...
	float rotation = Stimulus.stimrot.mean;
	float Xoff = 45;
	float side = 45 + d/2; // same period for all directions, otherwise awkward, i think
	static float phase = -side;
	float phgrid;
	float fulltime = Sfull.tcurr - epochtimezero;
	int Nsq = floor(2*side/Stimulus.spacing2);
	static int stimnumber=0;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if (FIRST)
	{
		FIRST=FALSE;
		stimnumber=rand() % Nsq;
	};

	// put it in a random bin
	
	phgrid = floor((stimnumber*Stimulus.spacing2 - side)/Stimulus.spacing2)*Stimulus.spacing2;
	Sfull.x = phgrid;
	Sfull.theta=rotation*PI/180;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,phgrid-d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,-2*side);
		glVertex3f(Xoff,phgrid+d/2,2*side);
		glVertex3f(Xoff,phgrid-d/2,2*side);
		glEnd();
		glPopMatrix();
	};


};

void TrackStim::drawBarPairsOffset()
{
	static bool FIRST = TRUE;
	float d = Stimulus.spacing; // step size, bar width
	int rot = Stimulus.stimrot.mean;
	//float dur = Stimulus.duration/5.0f;
	float dur = Stimulus.duration/Stimulus.stimtrans.phase;
	float Xoff = 45;
	//float side = 45*sqrt(2.0f) + d/2; // same period for all directions, otherwise awkward, i think
	float side = 45; // Without diagonals, side can simply 45, i think
	float phase; 
	float offsets[5] = {0,0.05,0.1,0.2,0.5}; // Time offset between the 2 bars presentation
	float offD[5] = {2.5,2.45,2.4,2.3,2}; // both bars off duration
	float onD = Stimulus.tau; // bar on duration
	float fulltime = Sfull.tcurr - epochtimezero;
	float curOffset;
	//int Npairs = 3; // How many bar pairs to present simultaneously
	int Npairs = Stimulus.stimtrans.mean; // How many bar pairs to present simultaneously
	static float bar1_init[5];	// init times for bar 1
	static float bar2_init[5]; // init times for bar 2
	int Nrotations = 4; // How many different rotations we're using
	int rotations[4] = {0,90,180,270};

	if (!FIRST)
		FIRST = (fulltime < 0.01);
	
	static int index[4]; // Change 4 to Nrotations if different than 4
	if (FIRST)
	{
		phase = -side;
		FIRST=FALSE;
		for (int jj=0;jj<5;jj++)
		{
			bar1_init[jj] = jj*(dur/5);
			bar2_init[jj] = bar1_init[jj]+offsets[jj];
			//char str[50];
			//sprintf(str,"bar1_init = %f",bar1_init[jj]);
			//sprintf(str,"dur = %f",dur);
			// DEBUG
			//sprintf(str,"onD = %f",onD);
			//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);
			//sprintf(str,"bar2_init = %f",bar2_init[jj]);
			//MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);
		}
		if(Stimulus.randomize==1)
		{
			for (int ii = 0; ii< Nrotations; ii++)
				index[ii]=ii;
			// randomize it
			for (int ii = 0; ii < Nrotations; ii++)
			{
				int randNum, temporary;
				r1.resetSeed(time(NULL));
				randNum = rand( ) % Nrotations;
				temporary = index[ii];
				index[ii] = index[randNum];
				index[randNum] = temporary;
			}
			//MessageBox(NULL,"first","ERROR",MB_OK|MB_ICONSTOP);
		}
		//char str[50];
		//for (int ii=0; ii < Nrotations; ii++)
		//{
		//	sprintf(str,"rotation = %d",rotations[index[ii]]);
		//	MessageBox(NULL,str,"ERROR",MB_OK|MB_ICONSTOP);
		//}
	};

	//phase = d*(floor(fulltime/dur)+1)-side;
	phase = d*(floor(fulltime/dur))-side;
	Sfull.theta=rotations[index[rot]]*PI/180;
	
	for (int nn=0; nn<Npairs; nn++)
	{
		float curT = mod(fulltime,dur);
		if(((curT-bar1_init[0]<onD)&&(curT-bar1_init[0]>0))||((curT-bar1_init[1]<onD)&&(curT-bar1_init[1]>0))||((curT-bar1_init[2]<onD)&&(curT-bar1_init[2]>0))||((curT-bar1_init[3]<onD)&&(curT-bar1_init[3]>0))||((curT-bar1_init[4]<onD)&&(curT-bar1_init[4]>0)))
		{
			glPushMatrix();
			glRotatef(rotations[index[rot]],1,0,0);
			glBegin(GL_QUADS);
			glVertex3f(Xoff,phase+nn*side*2/Npairs,-side);
			glVertex3f(Xoff,phase+nn*side*2/Npairs+d,-side);
			glVertex3f(Xoff,phase+nn*side*2/Npairs+d,side);
			glVertex3f(Xoff,phase+nn*side*2/Npairs,side);
			glEnd();
			glPopMatrix();
			Sfull.x = phase;
		}
		else
			Sfull.x = side*5;
		
		if(((curT-bar2_init[0]<onD)&&(curT-bar2_init[0]>0))||((curT-bar2_init[1]<onD)&&(curT-bar2_init[1]>0))||((curT-bar2_init[2]<onD)&&(curT-bar2_init[2]>0))||((curT-bar2_init[3]<onD)&&(curT-bar2_init[3]>0))||((curT-bar2_init[4]<onD)&&(curT-bar2_init[4]>0)))
		{
			glPushMatrix();
			glRotatef(rotations[index[rot]],1,0,0);
			glBegin(GL_QUADS);
			glVertex3f(Xoff,phase+nn*side*2/Npairs+d,-side);
			glVertex3f(Xoff,phase+nn*side*2/Npairs+2*d,-side);
			glVertex3f(Xoff,phase+nn*side*2/Npairs+2*d,side);
			glVertex3f(Xoff,phase+nn*side*2/Npairs+d,side);
			glEnd();
			glPopMatrix();
			Sfull.y = phase+d;
		}
		else
			Sfull.y = side*5;
	}
};

void TrackStim::drawSingleBar_local_LB()
{
	float fulltime = Sfull.tcurr - epochtimezero;

	float d = Stimulus.spacing; // bar widthase
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2;
	float rotation = Stimulus.stimrot.mean;
	float center = 500;
	float offset = Stimulus.spacing2;
	
	static bool FIRST = TRUE;
	static bool WAITED = FALSE;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if(rotation==0)
		center = Stimulus.x_center;
	else
		center = Stimulus.y_center;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,center+offset,-2*side);
		glVertex3f(Xoff,center+offset+d,-2*side);
		glVertex3f(Xoff,center+offset+d,2*side);
		glVertex3f(Xoff,center+offset,2*side);
		glEnd();
		glPopMatrix();
	};


};


void TrackStim::drawBarPair_local_LB()
{
	float fulltime = Sfull.tcurr - epochtimezero;

	float d = Stimulus.spacing; // bar widthase
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2;
	float rotation = Stimulus.stimrot.mean;
	float center = 500;
	float center2 = 500;
	float offset = Stimulus.spacing2;
	float currlum = 0;
	
	static bool FIRST = TRUE;
	static bool WAITED = FALSE;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if(rotation==0)
		center = Stimulus.x_center;
	else
		center = Stimulus.y_center;

	if(rotation==0)
		center2 = Stimulus.x_center2;
	else
		center2 = Stimulus.y_center2;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,center+offset,-2*side);
		glVertex3f(Xoff,center+offset+d,-2*side);
		glVertex3f(Xoff,center+offset+d,2*side);
		glVertex3f(Xoff,center+offset,2*side);
		glEnd();
		glPopMatrix();
	}
	else if(fulltime<=Stimulus.tau2)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,center+offset,-2*side);
		glVertex3f(Xoff,center+offset+d,-2*side);
		glVertex3f(Xoff,center+offset+d,2*side);
		glVertex3f(Xoff,center+offset,2*side);
		glEnd();
		glPopMatrix();

		if((Stimulus.contrast<0)&&(Stimulus.trans.phase<0))
			currlum = 1;
		else if((Stimulus.contrast>0)&&(Stimulus.trans.phase<0))
			currlum = 0;
		else if((Stimulus.contrast>0)&&(Stimulus.trans.phase>0))
			currlum = 1;
		else
			currlum = 0;

		// External field
		glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,center2+offset,-2*side);
		glVertex3f(Xoff,center2+offset+d,-2*side);
		glVertex3f(Xoff,center2+offset+d,2*side);
		glVertex3f(Xoff,center2+offset,2*side);
		glEnd();
		glPopMatrix();
	}


};


void TrackStim::drawBarPair_local_LBn()
{
	float fulltime = Sfull.tcurr - epochtimezero;

	float d = Stimulus.spacing; // bar widthase
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2;
	float rotation = Stimulus.stimrot.mean;
	float center = 500;
	float center2 = 500;
	float offset = Stimulus.spacing2;
	float currlum = 0;

	float da = PI/10;
	
	static bool FIRST = TRUE;
	static bool WAITED = FALSE;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	/* DEBUG
		glColor3f(0.0f,-getDLPColor(currlum,'G'),-getDLPColor(currlum,'B'));
		glPushMatrix();
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			glVertex3f(Xoff,Stimulus.x_center,Stimulus.y_center);
			glVertex3f(Xoff,Stimulus.x_center+5*cos(da*ii),Stimulus.y_center+5*sin(da*ii));
			glVertex3f(Xoff,Stimulus.x_center+5*cos((ii+1)*da),Stimulus.y_center+5*sin((ii+1)*da));
			glVertex3f(Xoff,Stimulus.x_center,Stimulus.y_center);
		}
		glEnd();
		glPopMatrix();

		glPushMatrix();
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			glVertex3f(Xoff,Stimulus.x_center2,Stimulus.y_center2);
			glVertex3f(Xoff,Stimulus.x_center2+5*cos(da*ii),Stimulus.y_center2+5*sin(da*ii));
			glVertex3f(Xoff,Stimulus.x_center2+5*cos((ii+1)*da),Stimulus.y_center2+5*sin((ii+1)*da));
			glVertex3f(Xoff,Stimulus.x_center2,Stimulus.y_center2);
		}
		glEnd();
		glPopMatrix();
		*/

	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glTranslatef(Xoff,Stimulus.x_center,Stimulus.y_center);
		glRotatef(rotation,1,0,0);
		//glTranslatef(Xoff,Stimulus.y_center+offset,0);
		//glRotatef(rotation-90,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(0,offset,-2*side);
		glVertex3f(0,offset+d,-2*side);
		glVertex3f(0,offset+d,2*side);
		glVertex3f(0,offset,2*side);
		glEnd();
		glPopMatrix();
	}
	else if(fulltime<=Stimulus.tau2)
	{
		glPushMatrix();
		glTranslatef(Xoff,Stimulus.x_center,Stimulus.y_center);
		glRotatef(rotation,1,0,0);
		//glTranslatef(Xoff,Stimulus.y_center+offset,0);
		//glRotatef(rotation-90,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(0,offset,-2*side);
		glVertex3f(0,offset+d,-2*side);
		glVertex3f(0,offset+d,2*side);
		glVertex3f(0,offset,2*side);
		glEnd();
		glPopMatrix();

		if((Stimulus.contrast<0)&&(Stimulus.trans.phase<0))
			currlum = 1;
		else if((Stimulus.contrast>0)&&(Stimulus.trans.phase<0))
			currlum = 0;
		else if((Stimulus.contrast>0)&&(Stimulus.trans.phase>0))
			currlum = 1;
		else
			currlum = 0;

		// External field
		glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		glPushMatrix();
		glTranslatef(Xoff,Stimulus.x_center2,Stimulus.y_center2);
		glRotatef(rotation,1,0,0);
		//glTranslatef(Xoff,Stimulus.y_center2+offset,0);
		//glRotatef(rotation-90,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(0,offset,-2*side);
		glVertex3f(0,offset+d,-2*side);
		glVertex3f(0,offset+d,2*side);
		glVertex3f(0,offset,2*side);
		glEnd();
		glPopMatrix();

	}


};


void TrackStim::drawBarPairWide_local_LB()
{
	float fulltime = Sfull.tcurr - epochtimezero;

	float d = Stimulus.spacing; // bar widths
	float Xoff = 45;
	float side = 45*sqrt(2.0f) + d/2;
	float rotation = Stimulus.stimrot.mean;
	float center = 500;
	float center2 = 500;
	float offset = Stimulus.spacing2;
	float currlum = 0;
	
	static bool FIRST = TRUE;
	static bool WAITED = FALSE;
	if ((!FIRST) & (WAITED))
		FIRST = (fulltime < 0.004);
	WAITED = (fulltime > 0.01); // fastest it can change

	if(rotation==0)
		center = Stimulus.x_center;
	else
		center = Stimulus.y_center;

	if(rotation==0)
		center2 = Stimulus.x_center2;
	else
		center2 = Stimulus.y_center2;
	
	if (fulltime<=Stimulus.tau)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,center+offset-d,-2*side);
		glVertex3f(Xoff,center+offset+d,-2*side);
		glVertex3f(Xoff,center+offset+d,2*side);
		glVertex3f(Xoff,center+offset-d,2*side);
		glEnd();
		glPopMatrix();
	}
	else if(fulltime<=Stimulus.tau2)
	{
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,center+offset-d,-2*side);
		glVertex3f(Xoff,center+offset+d,-2*side);
		glVertex3f(Xoff,center+offset+d,2*side);
		glVertex3f(Xoff,center+offset-d,2*side);
		glEnd();
		glPopMatrix();

		if((Stimulus.contrast<0)&&(Stimulus.trans.phase<0))
			currlum = 1;
		else if((Stimulus.contrast>0)&&(Stimulus.trans.phase<0))
			currlum = 0;
		else if((Stimulus.contrast>0)&&(Stimulus.trans.phase>0))
			currlum = 1;
		else
			currlum = 0;

		// External field
		glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		glPushMatrix();
		glRotatef(rotation,1,0,0);
		glBegin(GL_QUADS);
		glVertex3f(Xoff,center2+offset,-2*side);
		glVertex3f(Xoff,center2+offset+d,-2*side);
		glVertex3f(Xoff,center2+offset+d,2*side);
		glVertex3f(Xoff,center2+offset,2*side);
		glEnd();
		glPopMatrix();
	}


};


void TrackStim::drawCorrGrid(bool scramble)
{
	static int count=0;
	static int IMW=256;
	static int IMH=8;
	static float imagedata[256*8*3]={1.0f};
	float rotation = Stimulus.stimrot.mean;


	float mlum=Stimulus.lum;
	int N = 90*sqrt(2.0f)/int(Stimulus.spacing+0.01);
	int index[360], index_proper[360], index_scramble[360];
	// deal with RCorr2D -- update it every time...
	for (int ii=0; ii<8; ii++)
	{
		RCorr2D[ii].setPwhite(Stimulus.density); // start at 0.5...
		RCorr2D[ii].setBC(bool(int(Stimulus.stimtrans.phase+0.01))); // 0 or 1 on stimtrans phase
		RCorr2D[ii].setnumberofpixels(N);
		RCorr2D[ii].setupdatetype(int(Stimulus.stimrot.phase+0.01));
		RCorr2D[ii].setDir(int(Stimulus.rot.phase+0.01));
		RCorr2D[ii].setparity(int(Stimulus.rot.per+1.01)-1);
		RCorr2D[ii].setparityNUM(int(Stimulus.trans.per+0.01));
	};

	// update if count is properly set...
	int rem = count % int(Stimulus.tau*2+0.01); // tau in frames!
	if (rem == 0) // randomize to keep from getting too many!
		for (int ii=0; ii<8; ii++)
			RCorr2D[ii].randomizepixels();
	if (rem == int(Stimulus.tau+.01))
		for (int ii=0; ii<8; ii++)
			RCorr2D[ii].updatepixels();

	count++; // update count, once per 4 ms (or frame in this case -- one call per frame!)

	// output a few pixels for checking purposes -- base 2 in x column
	Sfull.x = 0;
	for (int ii=0;ii<12; ii++)
	{
		Sfull.x *= 2; // convert to base 2 to get it out...
		Sfull.x += RCorr2D[0].getpixelvalue(ii);
	};
	Sfull.theta = rotation*PI/180;

	if (RCorr2D[0].PIXUPDATED) // re-randomize ordering every update, just to be safe
	{
		for (int ii = 0; ii< N; ii++)
		{
			index_proper[ii]=ii;
			index_scramble[ii]=ii;
		};
		// randomize it
		int randNum, temporary;
		for (int ii = 0; ii < N; ii++)
		{
			randNum = rand( ) % N;
			temporary = index_scramble[ii];
			index_scramble[ii] = index_scramble[randNum];
			index_scramble[randNum] = temporary;
		}
	};

	if (RCorr2D[0].PIXUPDATED)
	{
		if (scramble)
			for (int ii = 0; ii < N; ii++)
				index[ii] = index_scramble[ii]; // assign these properly
		else
			for (int ii = 0; ii < N; ii++)
				index[ii] = index_proper[ii];

		// update all pixels...
		for (int ii=0; ii<N; ii++)
			for (int jj=0; jj<8; jj++)
			{
				if (RCorr2D[jj].getpixelvalue(ii)==0)
				{
					imagedata[3*(index[ii]+jj*IMW)+0]=backgroundtriple[0];
					imagedata[3*(index[ii]+jj*IMW)+1]=backgroundtriple[1];
					imagedata[3*(index[ii]+jj*IMW)+2]=backgroundtriple[2]; 
				}
				else
				{
					imagedata[3*(index[ii]+jj*IMW)+0]=foregroundtriple[0];
					imagedata[3*(index[ii]+jj*IMW)+1]=foregroundtriple[1];
					imagedata[3*(index[ii]+jj*IMW)+2]=foregroundtriple[2]; 
				};
			};
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 1);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, IMW, IMH, 0, GL_RGB, GL_FLOAT, imagedata);

		RCorr2D[0].PIXUPDATED=FALSE; // only do this ONCE!
	}

	float squaresize = 90.0f*sqrt(2.0f)/2;
	float Xoff = 45;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 1);

	glPushMatrix();
	glRotatef(rotation,1,0,0);

	glBegin(GL_QUADS);

	// draw the single square

	glTexCoord2f(0.0f,0.0f); // bot left
	glVertex3f(Xoff,-squaresize,-squaresize);

	glTexCoord2f(0.0f,float(N)/float(IMH)/2); // top left
	//glTexCoord2f(0.0f,float(1)/float(IMH)); // top left
	glVertex3f(Xoff,-squaresize,squaresize);

	glTexCoord2f(float(N)/float(IMW)/2,float(N)/float(IMH)/2);
	//glTexCoord2f(float(N)/float(IMW)/2,float(1)/float(IMH));
	glVertex3f(Xoff,squaresize,squaresize);
	
	glTexCoord2f(float(N)/float(IMW)/2,0.0f);
	glVertex3f(Xoff,squaresize,-squaresize);

	glEnd();

	glPopMatrix();

	glDisable(GL_TEXTURE_2D);


};



void TrackStim::updateGaussianR()
{
	static int counter = 0;
	if (counter==0)
	{
		r1.resetSeed(0);
		r1.setdt(1/240.0f); // this must get called twice per frame now...
		r1.setTau(Stimulus.tau);
	};
	r1.Rrandnt();

	counter++;

};

void TrackStim::drawGaussianFullfield()
{
	float lum1=Stimulus.lum*(1+Stimulus.contrast*r1.readLastValue());
	lum1=max(0.0,lum1); 
	Sfull.x=lum1*100;  // store values here? perhaps not wisest, but should work.

	glColor3f(0.0f,getDLPColor(lum1,'G'),getDLPColor(lum1,'B'));

	glBegin(GL_QUADS);
	glVertex3f(1,-5,-5);
	glVertex3f(1,-5,5);
	glVertex3f(1,5,5);
	glVertex3f(1,5,-5);
	glEnd();


};

void TrackStim::drawGaussianLocalCircle()
{
	static bool FIRST = FALSE;
	static float Xoff=45.0f;
	float da = PI/10;
	float fulltime = Sfull.tcurr - epochtimezero;
	float gain = Stimulus.transgain;
	float lum1=Stimulus.lum*(1+Stimulus.contrast*r1.readLastValue());
	float currlum = 0;

	float radius = Stimulus.spacing;
	float radius_out = Stimulus.spacing2;
	int rad = radius, rad_out = radius_out;
	int r, r_out;

	static float ang_rad_matx[21][8];
	static float ang_rad_maty[21][8];

	float Yoff = Stimulus.x_center;
	float Zoff = Stimulus.y_center;

	lum1=max(0.0,lum1); 
	Sfull.x=lum1*100;  // store values here? perhaps not wisest, but should work.

	// begin

	FILE *filein;

	if (!FIRST)
		FIRST = (fulltime < 0.01);

	if(FIRST)
	{
		char str[200];
		char c1,c2;
		c1 = Yoff < 0 ? 'm' : 'p';
		c2 = Zoff < 0 ? 'm' : 'p';

		sprintf(str,"C:\\Users\\Clandininlab\\Desktop\\2pstim_wNI_LB\\Debug\\Data\\morph\\%c%.0f%c%.0f_x_mat.txt",c1,abs(Yoff),c2,abs(Zoff));
		//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);
		filein = fopen(str, "r");				
		for(int i = 0; i < 21; i++)
			for(int j = 0; j < 8; j++)
				fscanf(filein, "%f", &ang_rad_matx[i][j]);
		fclose(filein);
		
		sprintf(str,"C:\\Users\\Clandininlab\\Desktop\\2pstim_wNI_LB\\Debug\\Data\\morph\\%c%.0f%c%.0f_y_mat.txt",c1,abs(Yoff),c2,abs(Zoff));
		filein = fopen(str, "r");
		for(int i = 0; i < 21; i++)
			for(int j = 0; j < 8; j++)
				fscanf(filein, "%f", &ang_rad_maty[i][j]);
		fclose(filein);

		switch (rad)
		{
			// rads = [2 4 5 6 8 10 15 20];
			case 2:
				r = 0;
				break;
			case 4:
				r = 1;
				break;
			case 5:
				r = 2;
				break;
			case 6:
				r = 3;
				break;
			case 8:
				r = 4;
				break;
			case 10:
				r = 5;
				break;
			case 15:
				r = 6;
				break;
			case 20:
				r = 7;
				break;
		}

		switch (rad_out)
		{
			// rads = [2 4 5 6 8 10 15 20];
			case 2:
				r_out = 0;
				break;
			case 4:
				r_out = 1;
				break;
			case 5:
				r_out = 2;
				break;
			case 6:
				r_out = 3;
				break;
			case 8:
				r_out = 4;
				break;
			case 10:
				r_out = 5;
				break;
			case 15:
				r_out = 6;
				break;
			case 20:
				r_out = 7;
				break;
		}
	}
	
	if (fulltime < Stimulus.tau2)
	{
		glColor3f(0.0f,getDLPColor(lum1,'G'),getDLPColor(lum1,'B'));
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{
			//char str[200];
			//sprintf(str,"r = %d, r out = %d, rad = %d, rad_out = %d",r,r_out,rad,rad_out);
			//MessageBox(NULL,str,"INFO",MB_OK|MB_ICONSTOP);
			glVertex3f(Xoff,Yoff,Zoff);
			glVertex3f(Xoff,ang_rad_matx[ii][r],ang_rad_maty[ii][r]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r],ang_rad_maty[ii+1][r]);
			glVertex3f(Xoff,Yoff,Zoff);	
		}
		glEnd();

		currlum = Stimulus.transgain;
		glColor3f(0.0f,getDLPColor(currlum,'G'),getDLPColor(currlum,'B'));
		
		glBegin(GL_QUADS);
		for (int ii=0; ii<20; ii++)
		{		
			glVertex3f(Xoff,ang_rad_matx[ii][r],ang_rad_maty[ii][r]);
			glVertex3f(Xoff,ang_rad_matx[ii][r_out],ang_rad_maty[ii][r_out]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r_out],ang_rad_maty[ii+1][r_out]);
			glVertex3f(Xoff,ang_rad_matx[ii+1][r],ang_rad_maty[ii+1][r]);	
		}
		glEnd();
	};
};


void TrackStim::drawBackgroundCube()
{
	// obsolete, now that we can get glclear to work within each viewport, black outside...
	float d = Stimulus.arenasize*.999;
	TrackStim::setBackgroundColor();  // set this up to draw in background color only...
	
	//glTranslatef(Sfull.x,Sfull.y,0.0f); // translate outside the function
	GLUquadricObj *cyl = gluNewQuadric();
	glPushMatrix();
	gluSphere(cyl,d,18,10);
	glPopMatrix();

	//glPushMatrix();
	//gluCylinder(cyl,d*.998,d*.998,Stimulus.arenaheight*2,18,1);
	//glPopMatrix();

	/*	float cx = Sfull.x; 
	float cy = Sfull.y; // center it here...
	glRotatef(Sfull.theta,0.0f,0.0f,1.0f);
	glBegin(GL_QUADS);
	// square 1 -- bottom
	glVertex3f(cx-d, cy-d,-d);
	glVertex3f(cx-d, cy+d,-d);
	glVertex3f(cx+d, cy+d,-d);
	glVertex3f(cx+d, cy-d,-d);
	// 2 -- top
	glVertex3f(cx-d, cy-d,d);
	glVertex3f(cx-d, cy+d,d);
	glVertex3f(cx+d, cy+d,d);
	glVertex3f(cx+d, cy-d,d);
	// 3 -- side one
	glVertex3f(cx+d, cy-d,-d);
	glVertex3f(cx+d, cy+d,-d);
	glVertex3f(cx+d, cy+d,d);
	glVertex3f(cx+d, cy-d,d);
	// 4 -- side one opposite
	glVertex3f(cx-d, cy-d,-d);
	glVertex3f(cx-d, cy+d,-d);
	glVertex3f(cx-d, cy+d,d);
	glVertex3f(cx-d, cy-d,d);
	// 5 -- side two
	glVertex3f(cx-d, cy+d,-d);
	glVertex3f(cx-d, cy+d,d);
	glVertex3f(cx+d, cy+d,d);
	glVertex3f(cx+d, cy+d,-d);
	// 6
	glVertex3f(cx-d, cy-d,-d);
	glVertex3f(cx-d, cy-d,d);
	glVertex3f(cx+d, cy-d,d);
	glVertex3f(cx+d, cy-d,-d);

	glEnd();*/

};

void TrackStim::drawWindow(float azim, float zenith, float solid)
{
	//TrackStim::setBackground();
	glColor3f(blacktriple[0],blacktriple[1],blacktriple[2]);
	float d = 10.0f;
	glPushMatrix();
	glTranslatef(Sfull.x,Sfull.y,0.0f);
	glRotatef(Sfull.theta*180/PI,0,0,1);
	glRotatef(azim*180/PI,0,0,1);
	glRotatef(zenith*180/PI,0,1,0); // rotate so that z is in correct direction

	GLUquadricObj *cyl = gluNewQuadric();

	glPushMatrix();
	glTranslatef(0.0f,0.0f,-d);
	gluCylinder(cyl,d,d,2*d,10,1);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f,0.0f,-d);
	gluDisk(cyl,0,d,10,1);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(0.0f,0.0f,d);
	gluDisk(cyl,solid*d,d,10,1);
	glPopMatrix();

	glPopMatrix();
	TrackStim::setForeground();
};

void TrackStim::drawAlignmentGrid()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);  
	GLUquadricObj *cyl = gluNewQuadric();
	glPushMatrix();
	gluSphere(cyl,20,18,10);
	glPopMatrix();

};
void TrackStim::drawCeiling()
{
	int ii,jj,kk,ll;
	
	for (ii=-1; ii<2; ii++)               // 2 outer loops to draw extra worlds around boundaries...
	{
		for (jj=-1; jj<2; jj++)
		{	
			for (kk=0; kk<Stimulus.arenasize/Stimulus.spacing/2; kk++)
			{
				for (ll=0; ll<Stimulus.arenasize/Stimulus.spacing/2; ll++)
				{
					glPushMatrix(); // save (0,0,0);
					glTranslatef(ii*Stimulus.arenasize + kk*Stimulus.spacing*2,
						jj*Stimulus.arenasize + ll*Stimulus.spacing*2,10.0f);
					glBegin(GL_QUADS);
					glVertex3f(0.0f,0.0f,0.0f);
					glVertex3f(0.0f,Stimulus.spacing,0.0f);
					glVertex3f(Stimulus.spacing,Stimulus.spacing,0.0f);
					glVertex3f(Stimulus.spacing,0.0f,0.0f);
					glEnd();
					glBegin(GL_QUADS);
					glVertex3f(Stimulus.spacing,Stimulus.spacing,0.0f);
					glVertex3f(Stimulus.spacing,2*Stimulus.spacing,0.0f);
					glVertex3f(2*Stimulus.spacing,2*Stimulus.spacing,0.0f);
					glVertex3f(2*Stimulus.spacing,Stimulus.spacing,0.0f);
					glEnd();
					glPopMatrix(); // return to (0,0,0);
				};
			};
		};
	};
	
};
void TrackStim::drawWedge(GLfloat angle, GLfloat width)  // ** draws wedge: everything in degrees, till this point
{
	GLfloat radius=30.0f;
	GLfloat heightfactor = 10.0f;

	angle *= PI/180;
	width *= PI/180;

	glBegin(GL_QUADS);
		glVertex3f(radius*cos(angle),radius*sin(angle), 0.0f);
		glVertex3f(radius*cos(angle+width),radius*sin(angle+width), 0.0f);
		glVertex3f(radius*cos(angle+width),radius*sin(angle+width), Stimulus.arenaheight*heightfactor);
		glVertex3f(radius*cos(angle),radius*sin(angle), Stimulus.arenaheight*heightfactor);
	glEnd();

};

void TrackStim::drawWedgeSquare(GLfloat angle, GLfloat width, GLfloat height)  // ** draws wedge: everything in degrees, till this point
{
	GLfloat radius=30.0f;

	angle *= PI/180;
	width *= PI/180;

	glBegin(GL_QUADS);
		glVertex3f(radius*cos(angle),radius*sin(angle), height);
		glVertex3f(radius*cos(angle+width),radius*sin(angle+width), height);
		glVertex3f(radius*cos(angle+width),radius*sin(angle+width), height + radius*width/2);
		glVertex3f(radius*cos(angle),radius*sin(angle), height + radius*width/2);
	glEnd();

};
void TrackStim::drawBar()
{
	float added = Stimulus.stimrot.mean + Stimulus.stimrot.amp*TrackStim::sineWave(Stimulus.stimrot.per); // add the stimulus OL
	// glTranslatef(Sfull.x,Sfull.y,0.0f);
	// glRotatef(added,0.0f,0.0f,1.0f);
	TrackStim::drawWedge(0.0f + added,Stimulus.spacing);
};
void TrackStim::drawCylinder()   // pinwheel 
{
	int ii;
	float tc = TrackStim::queryCurrTime() - epochtimezero;
	float angle = Stimulus.spacing;  // in degrees
	float added = Stimulus.stimrot.mean + Stimulus.stimrot.amp*TrackStim::sineWave(Stimulus.stimrot.per); // add the stimulus OL
	added += tc*Stimulus.stimtrans.mean;  // put in translation mean here to get easy spinning. not ideal... bring amp back to 0 though!
	//glTranslatef(Sfull.x,Sfull.y,0.0f);
	//glRotatef(added + Sfull.theta*180/PI,0.0f,0.0f,1.0f); // should do this stuff outside this function for clarity and consistency
	for (ii=0; ii<360/angle/2; ii++)
	{
		TrackStim::drawWedge(ii*angle*2 + added,angle);
	}

};

void TrackStim::drawCheckerboard()   // checkerboard with varying phase 
{
	int ii, jj;
	float tc = TrackStim::queryCurrTime() - epochtimezero;
	float angle = Stimulus.spacing;  // in degrees
	float added = Stimulus.stimrot.mean + Stimulus.stimrot.amp*TrackStim::sineWave(Stimulus.stimrot.per); // add the stimulus OL
	added += tc*Stimulus.stimtrans.mean;  // put in translation mean here to get easy spinning. not ideal... bring amp back to 0 though!
	float squareheight=angle*PI/180*30/2;

	// cylinder radius is 30, height is 10x arena height; vertical extent is 1/2 of horizontal to get more squares in
	for (ii=0; ii<360/angle/2; ii++)
	{
		for (jj=0; jj<(Stimulus.arenaheight*10.0f/squareheight); jj++)
		{
			if ((jj % 2)==1)
				TrackStim::drawWedgeSquare(ii*angle*2 + added + Stimulus.stimrot.phase, angle, jj*squareheight); // all arguments in degrees here, except height
			else
				TrackStim::drawWedgeSquare(ii*angle*2 + added,angle, jj*squareheight);
		}
	}

};

void TrackStim::drawColorTest()
{
	float dist = 10;
	//static bool coloron = TRUE;

	if (framenumber % 2)
		setForeground();  // wouldn't normally want to change these colors here, but in this routine we do
	else
		setBackgroundColor();

	GLUquadricObj *cyl = gluNewQuadric();
	glPushMatrix();
	gluSphere(cyl,5,4,4);
	glPopMatrix();

	//coloron = !coloron; // switch each frame...

};

void TrackStim::drawCylinderHalves()
{
	int ii;
	float angle = Stimulus.spacing; // in degrees
	GLfloat Dscale=1;  // this scales teh distance: weird metric to 
	GLfloat startangle;
	float time = queryCurrTime() - epochtimezero;
	
	//startangle = (DistanceAccumulated * Dscale) 
	//	+ time*Stimulus.stimtrans.mean + Stimulus.stimtrans.amp*TrackStim::sineWave(Stimulus.stimtrans.per);
	startangle = (Saccumulate.theta * Dscale); 
	startangle -= 2*angle*(int)(startangle / 2/angle);  // stupid -- no mod function for double
	if (startangle < 0)
		startangle += 2*angle;

	glRotatef(Stimulus.stimrot.mean,0,0,1);
	
	// first half of it
	for (ii=0; ii<180/angle/2; ii++)  // this gets the correct number
	{
		if (ii*angle*2 + startangle < 180 - angle)
		{
			TrackStim::drawWedge(ii*angle*2+startangle, angle);
		}
		else
		{
			TrackStim::drawWedge(ii*angle*2+startangle, 2*angle - startangle);  // last wedge
			TrackStim::drawWedge(0, startangle - angle);
		};
	}
	// second half of it
	for (ii=0; ii<180/angle/2; ii++)  // this gets the correct number
	{
		if (ii*angle*2 + startangle < 180 - angle)
		{
			TrackStim::drawWedge(360 - (ii*angle*2+startangle), -angle);
		}
		else
		{
			TrackStim::drawWedge(360 - (ii*angle*2+startangle), -(2*angle - startangle));  // last wedge
			TrackStim::drawWedge(360 - 0, -(startangle - angle));
		};
	}
}; // halves that rotate

void TrackStim::drawInfCorridor()
{
	double Dscale = 1; 
	double extent=30*Stimulus.arenasize;
	double depth=30*Stimulus.arenaheight;
	double width=10;
	float spacing = Stimulus.spacing;
	int ii;
	float time=TrackStim::queryCurrTime();;
	float startpos;
	//float added = Stimulus.stimrot.mean + Stimulus.stimrot.amp*TrackStim::sineWave(Stimulus.stimrot.per); // add the stimulus OL
	
	
	startpos= (Saccumulate.x * Dscale) 
		+ time*Stimulus.stimtrans.mean + Stimulus.stimtrans.amp*TrackStim::sineWave(Stimulus.stimtrans.per);
	startpos -= spacing*(int)(startpos / spacing); // stupid -- no mod function for double

	glPushMatrix();

	//glRotatef(added,0.0f,0.0f,1.0f);
	glTranslatef(-extent-startpos,0,0);
	for (ii=-(int)(extent/spacing); ii<=(int)(extent/spacing); ii++)
	{
		glBegin(GL_QUADS);
		glVertex3f(0,width,0.0f);
		glVertex3f(0,width,depth);
		glVertex3f((0.5)*spacing,width,depth);
		glVertex3f((0.5)*spacing,width,0.0f);
		glEnd();
		glBegin(GL_QUADS);
		glVertex3f(0,-width,0.0f);
		glVertex3f(0,-width,depth);
		glVertex3f((0.5)*spacing,-width,depth);
		glVertex3f((0.5)*spacing,-width,0.0f);
		glEnd();
		glTranslatef(spacing,0,0);
	};
	glPopMatrix();
}; // realistic translation

void TrackStim::drawLinearRDots(float dt)
{
	//static float lasttime=TrackStim::queryCurrTime();
	//float currtime = TrackStim::queryCurrTime();
	Rdots.tau = Stimulus.tau;
	Rdots.Zheight = Stimulus.arenaheight; // height factor from cylinder drawing
	Rdots.windowWidth = Stimulus.arenasize;
	Rdots.windowHeight = Stimulus.arenasize;
	Rdots.blobSize = Stimulus.spacing*PI/180*Stimulus.arenaheight;  // in degrees now
	int temp,temp2;
	temp=(int)(Rdots.windowWidth/Rdots.blobSize);
	temp2=(int)(Rdots.windowWidth/Rdots.blobSize);
	Rdots.blobCount = (int)((float)temp*(float)temp2 * -1*log(1-Stimulus.density));  // roughly the number on it
	
	Rdots.speed = (Stimulus.stimtrans.mean + Stimulus.stimtrans.amp*TrackStim::sineWave(Stimulus.stimtrans.per)) * PI/180 * Stimulus.arenaheight; // give it speed to update with -- degrees/s @ nearest point
	double angle = Stimulus.stimrot.mean + Stimulus.stimrot.amp*TrackStim::sineWave(Stimulus.stimrot.per); // gets angle right
	glRotatef(angle,0,0,1.0f);
	Rdots.RenderScene(dt);
}; // random dots

void TrackStim::drawRotRDots(float dt)
{
	//static float lasttime=TrackStim::queryCurrTime();
	//float currtime = TrackStim::queryCurrTime();
	Rdots.tau = Stimulus.tau;
	Rdots.Zheight = Stimulus.arenaheight*10; // height factor from cylinder drawing
	int temp,temp2;
	temp=(int)(Stimulus.arenaheight*10/(Stimulus.spacing*PI/180*30));
	temp2=(int)(360/Stimulus.spacing);
	Rdots.blobCount = (int)((float)temp*(float)temp2 * -1*log(1-Stimulus.density));  // roughly the number on it
	Rdots.blobSize = Stimulus.spacing;  // in degrees
	Rdots.speed = Stimulus.stimrot.mean + Stimulus.stimrot.amp*TrackStim::sineWave(Stimulus.stimrot.per); // give it speed to update with
	Rdots.speed *= PI/180;
	Rdots.RenderScene_rotation(dt);
	//lasttime=currtime;
}; // rotational random dots

void TrackStim::drawRotRDotsHalves()
{
	static float lasttime=TrackStim::queryCurrTime();
	float currtime = TrackStim::queryCurrTime();
	Rdots.speed = Stimulus.stimtrans.mean + Stimulus.stimtrans.amp*TrackStim::sineWave(Stimulus.stimtrans.per); // give it speed to update with
	Rdots.RenderScene_rotation_halves(currtime - lasttime);
	lasttime=currtime;
};

void TrackStim::drawRotRDotsGrads()
{
	static float lasttime=TrackStim::queryCurrTime();
	float currtime = TrackStim::queryCurrTime();
	Rdots.speed = Stimulus.stimtrans.mean + Stimulus.stimtrans.amp*TrackStim::sineWave(Stimulus.stimtrans.per); // give it speed to update with
	Rdots.RenderScene_rotation_grad(currtime - lasttime);
	lasttime=currtime;
};

void TrackStim::drawBarCeiling() // moving bar stimulus
{
	double spacing=Stimulus.spacing;
	int ii;
	double extent=3*Stimulus.arenasize;
	double width=3*Stimulus.arenasize;
	float time;
	float startpos;
	
	time=TrackStim::queryCurrTime();
	startpos = time*Stimulus.stimtrans.mean + Stimulus.stimtrans.amp*TrackStim::sineWave(Stimulus.stimtrans.per);  // OL part
	startpos -= spacing*(int)(startpos / spacing);  // stupid -- no mod function for double
	double angle = Stimulus.stimrot.mean + Stimulus.stimrot.amp*TrackStim::sineWave(Stimulus.stimrot.per); // gets angle right

	glPushMatrix();
	glRotatef(angle,0,0,1.0f);
	
	glTranslatef(-extent/2-startpos,0,Stimulus.arenaheight);
	for (ii=-(int)(extent/spacing); ii<(int)(extent/spacing); ii++)
	{
		glBegin(GL_QUADS);
		glVertex3f(0,-width,0.0f);
		glVertex3f(spacing/2,-width,0.0f);
		glVertex3f(spacing/2,width,0.0f);
		glVertex3f(0,width,0.0f);
		glEnd();
		glTranslatef(spacing,0,0);

	};
	glPopMatrix();
};   


/// read in stimulus file, output from excel file
TrackStim::STIMPARAMS * TrackStim::readParams(char * szFile)
{
	FILE *filein;
	int numlines;
	char linename[255];
	char oneline[255];
	float pcurr[50];
	STIMPARAMS Stims[50];
	for (int jj=0; jj<50; jj++) 
	{
		Stims[jj] = TrackStim::initializeStimulus();
	}

	string currlinename=" "; // initialize string objects once
	string complinename=" ";
	string currline=" ";

	//filein = fopen("data/feedbackparams.txt", "rt");				// File To Load Data From
	filein = fopen(szFile, "rt");				// File To Load Data From
	for (int ii=0; ii<260; ii++) TrackStim::paramfilename[ii]=szFile[ii];
	
	// read in initial values about file
	TrackStim::readstring(filein,oneline);
	sscanf(oneline, "PARAMS %d", &numlines);
	TrackStim::readstring(filein,oneline);
	sscanf(oneline, "EPOCHS %d", &numepochs);
	
	for (int loop = 0; loop < numlines; loop++)   // loop on number of parameters
	{
		// go through remaining lines...
		fscanf(filein, "%s", &linename);
		currlinename=linename;

		for (int jj = 0; jj < numepochs; jj++)    // loop on number of parameter sets
		{
			fscanf(filein, "%f", &pcurr[jj]);   // this appears to work
		};
		
		for (int jj = 0; jj < numepochs; jj++)
		{
			complinename="Stimulus.stimtype";
			if ((currlinename == complinename))  // does this work, or do i have to declare a new one here?
				Stims[jj].stimtype = pcurr[jj];
			complinename="Stimulus.lum";
			if ((currlinename == complinename))
				Stims[jj].lum = pcurr[jj];
			complinename="Stimulus.contrast";
			if ((currlinename == complinename))
				Stims[jj].contrast = pcurr[jj];
			complinename="Stimulus.duration";
			if ((currlinename == complinename))
				Stims[jj].duration = pcurr[jj];

			complinename="Stimulus.transgain";
			if ((currlinename == complinename))
				Stims[jj].transgain = pcurr[jj];
			complinename="Stimulus.rotgain";
			if ((currlinename == complinename))
				Stims[jj].rotgain = pcurr[jj];

			complinename="Stimulus.trans.mean";
			if ((currlinename == complinename))
				Stims[jj].trans.mean = pcurr[jj];
			complinename="Stimulus.trans.amp";
			if ((currlinename == complinename))
				Stims[jj].trans.amp = pcurr[jj];
			complinename="Stimulus.trans.per";
			if ((currlinename == complinename))
				Stims[jj].trans.per = pcurr[jj];
			complinename="Stimulus.trans.phase";
			if ((currlinename == complinename))
				Stims[jj].trans.phase = pcurr[jj];

			complinename="Stimulus.rot.mean";
			if ((currlinename == complinename))
				Stims[jj].rot.mean = pcurr[jj];
			complinename="Stimulus.rot.amp";
			if ((currlinename == complinename))
				Stims[jj].rot.amp = pcurr[jj];
			complinename="Stimulus.rot.per";
			if ((currlinename == complinename))
				Stims[jj].rot.per = pcurr[jj];
			complinename="Stimulus.rot.phase";
			if ((currlinename == complinename))
				Stims[jj].rot.phase = pcurr[jj];

			complinename="Stimulus.stimrot.mean";
			if ((currlinename == complinename))
				Stims[jj].stimrot.mean = pcurr[jj];
			complinename="Stimulus.stimrot.amp";
			if ((currlinename == complinename))
				Stims[jj].stimrot.amp = pcurr[jj];
			complinename="Stimulus.stimrot.per";
			if ((currlinename == complinename))
				Stims[jj].stimrot.per = pcurr[jj];
			complinename="Stimulus.stimrot.phase";
			if ((currlinename == complinename))
				Stims[jj].stimrot.phase = pcurr[jj];

			complinename="Stimulus.stimtrans.mean";
			if ((currlinename == complinename))
				Stims[jj].stimtrans.mean = pcurr[jj];
			complinename="Stimulus.stimtrans.amp";
			if ((currlinename == complinename))
				Stims[jj].stimtrans.amp = pcurr[jj];
			complinename="Stimulus.stimtrans.per";
			if ((currlinename == complinename))
				Stims[jj].stimtrans.per = pcurr[jj];
			complinename="Stimulus.stimtrans.phase";
			if ((currlinename == complinename))
				Stims[jj].stimtrans.phase = pcurr[jj];

			complinename="Stimulus.spacing";
			if ((currlinename == complinename))
				Stims[jj].spacing = pcurr[jj];
			complinename="Stimulus.spacing2";
			if ((currlinename == complinename))
				Stims[jj].spacing2 = pcurr[jj];
			complinename="Stimulus.density";
			if ((currlinename == complinename))
				Stims[jj].density = pcurr[jj];
			complinename="Stimulus.tau";
			if ((currlinename == complinename))
				Stims[jj].tau = pcurr[jj];
			complinename="Stimulus.tau2";
			if ((currlinename == complinename))
				Stims[jj].tau2 = pcurr[jj];

			complinename="Stimulus.arenasize";
			if ((currlinename == complinename))
				Stims[jj].arenasize = pcurr[jj];
			complinename="Stimulus.arenaheight";
			if ((currlinename == complinename))
				Stims[jj].arenaheight = pcurr[jj];

			complinename="Stimulus.randomize";
			if ((currlinename == complinename))
				Stims[jj].randomize = pcurr[jj];

			complinename="Stimulus.x_center";
			if ((currlinename == complinename))
				Stims[jj].x_center = pcurr[jj];
			complinename="Stimulus.y_center";
			if ((currlinename == complinename))
				Stims[jj].y_center = pcurr[jj];

			complinename="Stimulus.x_center2";
			if ((currlinename == complinename))
				Stims[jj].x_center2 = pcurr[jj];
			complinename="Stimulus.y_center2";
			if ((currlinename == complinename))
				Stims[jj].y_center2 = pcurr[jj];

			complinename="Stimulus.base_radius";
			if ((currlinename == complinename))
				Stims[jj].base_radius = pcurr[jj];
			complinename="Stimulus.base_extent";
			if ((currlinename == complinename))
				Stims[jj].base_extent = pcurr[jj];


			complinename="Stimulus.USEFRUSTUM";
			if ((currlinename == complinename))
				Stims[jj].USEFRUSTUM = pcurr[jj];

		};

	}

	fclose(filein);

	return Stims;
};

