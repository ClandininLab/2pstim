#pragma once
#include <ctime>
#include <cmath>
#include <windows.h>		// Header File For Windows
#include <math.h>			// Math Library Header File
#include <stdio.h>			// Header File For Standard Input/Output
#include <string>
//#include <iostream>
using namespace std;
#include <gl\gl.h>			// Header File For The OpenGL32 Library
#include <gl\glu.h>			// Header File For The GLu32 Library
#include <gl\glaux.h>		// Header File For The Glaux Library
#include <NIDAQmx.h>
#include "Animation.h"      // header file for animation class to make random dot patterns.
#include "RandomC.h"
#include "EnforcedCorr1D.h"

#define DAQmxErrChk(functionCall) if( DAQmxFailed(error=(functionCall)) ) goto Error; else

class TrackStim
	/*
	Class for creating all the stimuli on the 200 Hz screen (can change to DLP)...
	*/
{

public:

	struct POSITIONINFO {  // for fly or stimulus
		float x,y,theta; //  current position, THETA IN RADIANS
		float vx,vy,vtheta; //  velocities
		float xold,yold,thetaold; // previous measured position
		float dx,dy,dtheta;
		float tcurr,told; // times of current and prev measurement
		float dt; // tcurr-told
	} ;

	bool PARAMSFROMFILE; // read parameters from file

	struct OLPARAMS {
		float mean, amp, per, phase;
	};

	struct STIMPARAMS { // everything attached to the stimulus; array of this to get stim sequence by index: "stim epoch"
		int stimtype;
		float lum;
		float contrast;
		float duration;
		float transgain,rotgain;
		// below: changes fly's frame with OL
		OLPARAMS trans;  // OL on fly translation & rotation -- OL on stimulus position and angle
		OLPARAMS rot;
		// below: in fly's frame
		OLPARAMS stimtrans;  // OL on stimulus translation, where applicable: halves, bars, inf corridor
		OLPARAMS stimrot;    // OL on stim rotation, where applicable: bars
		float spacing, spacing2;
		float density;
		float tau, tau2;
		float arenasize, arenaheight;  // arena parameters

		int randomize;

		float x_center, y_center, x_center2, y_center2;

		float base_radius, base_extent; // radius of 3d cylinders to be drawn, and their heights.
		float USEFRUSTUM;

	} Stimulus ;

	int numepochs; // number of epochs in stimulus
	int epochchoose; // control of which after reading them in...
	float epochtimezero;

	struct VIEWPOSITIONS
	{
		int x[2],y[2],h[2],w[2];  // (x,y,h,w) coords for all 4 view windows...
	} ViewPorts;

	EnforcedCorr1D RCorr2D[8];


private:
	
	// basic
	float PI;
	float E;
	float ScreenUpdateRate; // throw up at Hz

	POSITIONINFO Sfull; // contains open loop info, too
	POSITIONINFO Saccumulate; // for use in pulses -- adds to Sfull with updatestimulus

	// random dots class
	CAnimation Rdots;

	RandomC r1; // random number generator, with correlation time, etc.

	char paramfilename[260]; // stores param file anme to be written out


	// stimulus data
	long int framenumber;   // counts which frame is going up
	LARGE_INTEGER CounterBeginTime;
	LARGE_INTEGER CounterFreq;
	FILE * monitor_stimulus_file ;   // file to write for stimulus info


	void readstring(FILE *f,char *str);
	float mod(float x,float y);

	float getDLPColor(float DLPintensity, char channel);


	float backgroundtriple[3],foregroundtriple[3],blacktriple[3],whitetriple[3];
	void setColorTriples();
	float noiseValues[14400];  // get this on a workaround with periodic noise... can't make huge array!
	float retrieveNextNoiseValue();

	float perspELEVATION;
	float perspAZIMUTH; 
	float perspHEAD;

	void transformToScopeCoords();


public:
	
	//////////basic
	TrackStim();  // **
	virtual ~TrackStim(); // **

	//////////////set various things in the stimulus
	void setZeroTime();        // ** also done with class first defined...
	void writeStim(TaskHandle taskHandle);    // ** writes out stimulus data, do each color?
	float queryCurrTime(); // ** gets current time
	float sineWave(float freq); // ** makes sine wave
	void incrementFrameNumber();
	STIMPARAMS initializeStimulus();
	VIEWPOSITIONS initializeViewPositions();
	TrackStim::STIMPARAMS * readParams(char szFile[260]);

	void forceWait();

	void setBackground();  // sets background color
	void setBackgroundColor();
	void setForeground();  // sets foreground color

	/////////////making real stimuli
	void readNoise();
	void rotationNoiseUpdate();
	void readViewPositions();  //**
	void writeViewPositions();   //**
	void distancePulse();
	void rotationPulse();
	void rotationDoublePulsePlus();
	void rotationDoublePulseMinus();
	bool drawScene();  // ** executes the drawing set by setStimType
	void drawUniform();
	void drawUniform_LB();
	void drawUniform_LB_varSize();
	void drawUniform_LB_varSizeExpInv();
	void drawUniform_sine();
	void drawUniform_sine_LB();
	void drawUniform_EB();
	void drawStim();
	void drawSine1D();
	void drawSine1D_LB();
	void drawSineCylinder(float tc);
	void drawSquareWaveCylinder(float tc);
	void drawSquareWaveCylinderHS(float tc);
	void drawSineCylinderIGray(float tc);
	void drawCylinderStripe(float tc);
	void drawCylinderWedge(float angle, float w);
	void drawSphere();
	void drawSpherePrint(float tc);
	void drawStaticGrid();
	void drawStaticGridLB();
	void drawStaticGridLB2();
	void drawSquareGrid();
	void drawSquareGridLB();
	void drawMovingBars();
	void drawMovingBarsHS();
	void drawMovingBar_large();
	void drawMovingBar();
	void drawMovingBar_grid();
	void drawMovingBar_grid_LB();
	void drawMovingBar_grid_LB_onscreen();
	void drawMovingBar_grid_once();
	void drawMovingBar_grid_randomspace();
	void drawBar_grid_randomspace();
	void drawBar_grid_randomspace_permute();
	void drawBar_grid_randomspace_permute_LB();
	void drawBar_grid_randomspace_permute_HY();
	void drawLightBarCorrected_grid_randomspace_permute_HY();
	void drawLightBarCorrected_grid_randomspace_permute_HY_test();
	void drawBar_grid_randomspace_permute_HY_test();
	void drawBar_grid_randomspace_permute_flyCord_onscreen_YF();
	void drawBar_grid_randomspace_permute_flyCord_YF();
	void drawMovingBar_grid_flyCord();
	void drawMovingBar_grid_replicateFFF();
	void updateGaussianR();
	void drawGaussianFullfield();
	void drawGaussianLocalCircle();
	void drawBarPairsOffset();
	void drawSingleBar_local_LB();
	void drawBarPair_local_LB();
	void drawBarPair_local_LBn();
	void drawBarPairWide_local_LB();
	void drawCorrGrid(bool scramble);

	void drawLocalSquareGrid();
	void drawLocalCircle();
	void drawLocalCircleCR();
	void drawLocalCircleCR2();
	void drawLocalCircleTorusSimul();
	void drawLocalCircleTorusSimulCR();
	void drawLocalCircleTorusSimulCR2();
	void drawLocalTorus();
	void drawLocalTorusCR();
	void drawLocalTorusCR2();

	void drawCalibrationFFF();
	void drawCalibrationFFF_withcorrection();


	void drawBackgroundCube();
	void drawWindow(float azim, float zenith, float solid);
	void drawAlignmentGrid();
	void drawCeiling();
	void drawWedge(GLfloat angle, GLfloat width);
	void drawWedgeSquare(GLfloat angle, GLfloat width, GLfloat height);
	void drawColorTest();
	void drawBar();
	void drawCylinder();
	void drawCheckerboard();
	void drawCylinderHalves();
	void drawInfCorridor(); // realistic translation
	void drawBarCeiling();   // moving bar stimulus
	void drawLinearRDots(float dt); // random dots
	void drawRotRDots(float dt); // rotational random dots
	void drawRotRDotsHalves();
	void drawRotRDotsGrads();
	
} ;