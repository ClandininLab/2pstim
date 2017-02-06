#include "stdafx.h"
#include "EnforcedCorr1D.h"

/////////////// functions to set and write stimulus stuff from fly measurements //////////////////////////////

EnforcedCorr1D::EnforcedCorr1D()
{

	Pwhite = 0.5f;
	numberofpixels=360;
	for (int ii=0; ii<360; ii++)
	{
		pixvals[ii]=0;
		pixvalsnew[ii]=0;
		parityvals[0][ii]=-1;
		parityvals[1][ii]=2*(rand() % 2)-1;
		parityvals[2][ii]=2*(rand() % 2)-1;
		parityvals[3][ii]=2*(rand() % 2)-1;
	}
	updatetype = 0;
	circularBC = FALSE;

	direction = 1;
	parity = 1;
	parityNUM = 2;

} ;

EnforcedCorr1D::~EnforcedCorr1D()
{
};

void EnforcedCorr1D::updateparityvals()
{
	for (int ii=0; ii<numberofpixels; ii++)
	{
		parityvals[3][ii]=parityvals[2][ii];
		parityvals[2][ii]=parityvals[1][ii];
		parityvals[1][ii]=parityvals[0][ii];
		if (pixvals[ii]==1)
			parityvals[0][ii]=1;
		else
			parityvals[0][ii]=-1;
	};
	PIXUPDATED=TRUE;
};

void EnforcedCorr1D::setnumberofpixels(int numin)
{
	numberofpixels=numin;
};

void EnforcedCorr1D::setPwhite(float Pin)
{
	Pwhite = Pin;
};

void EnforcedCorr1D::setupdatetype(int updatein)
{
	updatetype = updatein;
};

void EnforcedCorr1D::setBC(bool circBC)
{
	circularBC = circBC;
};

void EnforcedCorr1D::setparity(int parityin)
{
	parity = parityin;
};

void EnforcedCorr1D::setparityNUM(int parityNUMin)
{
	parityNUM = parityNUMin;
};

void EnforcedCorr1D::updatepixels()
{
	switch (updatetype)
	{
	case 0:
		randomizepixels();
		break;
	case 1:
		update1(1,1);
		break;
	case 2:
		update1(0,0);
		break;
	case 3:
		update1(1,0);
		break;
	case 4:
		update1(0,1);
		break;
	case 5:
		update1corr(1,1,0.5);
		break;
	case 6:
		update1corr(0,0,0.5);
		break;
	case 7:
		update1corr(1,0,0.5);
		break;
	case 8:
		update1corr(0,1,0.5);
		break;
	case 9:
		updateparitypixvals(parity,parityNUM);
		break;
	};
};

void EnforcedCorr1D::randomizepixels()
{
	bool getswhite;
	for (int ii=0;ii<numberofpixels;ii++)
	{
		getswhite=(((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
		if (getswhite)
			pixvals[ii]=1;
		else
			pixvals[ii]=0;
	};
	updateparityvals();
};

void EnforcedCorr1D::updateparitypixvals(int P,int NUM)
{
	bool getswhite;
	int currpar;
	int startpos, endpos;
	
	// deal with edges
	if (circularBC)
	{
		startpos=0;
		endpos=numberofpixels;
	}
	else
	{
		if (direction == 1)
		{
			startpos=NUM-1;
			endpos=numberofpixels;

			for (int ii=0;ii<startpos;ii++)
			{
				getswhite=(((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
				if (getswhite)
					pixvals[ii]=1;
				else
					pixvals[ii]=0;
			};
		}
		else
		{
			startpos=0;
			endpos=numberofpixels-NUM+1;

			for (int ii=endpos;ii<numberofpixels;ii++)
			{
				getswhite=(((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
				if (getswhite)
					pixvals[ii]=1;
				else
					pixvals[ii]=0;
			};
		};
	};

	// deal with core values...
	for (int ii=startpos;ii<endpos;ii++)
	{
		currpar=computeparity(NUM,ii);

		if (currpar == P)
			pixvals[ii]=1;
		else
			pixvals[ii]=0;

	};

	updateparityvals();
};

int EnforcedCorr1D::computeparity(int NUM, int loc)
// assumes circularBC; update must take care of this...
{
	int parityout=1;

	for (int jj=0; jj<NUM-1; jj++)
	{
		int currloc;
		if (direction==1) // something wrong here?
			currloc = (loc-1-jj) % numberofpixels;
		else
			currloc = (loc+1+jj) % numberofpixels;

		if (currloc<0)
			currloc = numberofpixels + currloc;

		parityout *= parityvals [jj] [currloc];
	};
	return parityout;	
};

void EnforcedCorr1D::update1(int x1t1,int x2t2)
{
	bool getswhite;

	if (direction == 1)
	for (int ii=0;ii<numberofpixels;ii++)
	{
		if ((ii==0) && (!circularBC))
		{
			getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
			if (getswhite)
				pixvalsnew[ii]=1;
			else
				pixvalsnew[ii]=0;
		}
		else
		{
			if (ii>0)
				if (pixvals[ii-1]==x1t1)
				{
					pixvalsnew[ii]=x2t2;
				}
				else
				{
					getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
					if (getswhite)
						pixvalsnew[ii]=1;
					else
						pixvalsnew[ii]=0;
				}
			else
				if (pixvals[numberofpixels-1]==x1t1)
				{
					pixvalsnew[ii]=x2t2;
				}
				else
				{
					getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
					if (getswhite)
						pixvalsnew[ii]=1;
					else
						pixvalsnew[ii]=0;
				}
		};
		
	}
	else
	for (int ii=0;ii<numberofpixels;ii++)
	{
		if ((ii==numberofpixels-1) && (!circularBC))
		{
			getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
			if (getswhite)
				pixvalsnew[ii]=1;
			else
				pixvalsnew[ii]=0;
		}
		else
		{
			if (ii<numberofpixels-1)
				if (pixvals[ii+1]==x1t1)
				{
					pixvalsnew[ii]=x2t2;
				}
				else
				{
					getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
					if (getswhite)
						pixvalsnew[ii]=1;
					else
						pixvalsnew[ii]=0;
				}
			else
				if (pixvals[0]==x1t1)
				{
					pixvalsnew[ii]=x2t2;
				}
				else
				{
					getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
					if (getswhite)
						pixvalsnew[ii]=1;
					else
						pixvalsnew[ii]=0;
				}
		};
		
	}

	copypixvalsnewtopixvals();
	updateparityvals();

};

void EnforcedCorr1D::update1corr(int x1t1,int x2t2, float c)
{
	bool getswhite;
	bool updateit;

	if (direction == 1)
	for (int ii=0;ii<numberofpixels;ii++)
	{
		updateit = (((float)rand())/((float)RAND_MAX+1.0f) < c);
		getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
		if ((ii==0) && (!circularBC))
		{
			if (getswhite)
				pixvalsnew[ii]=1;
			else
				pixvalsnew[ii]=0;
		}
		else
		{
			if (ii>0)
				if ((pixvals[ii-1]==x1t1) && (updateit))
				{
					pixvalsnew[ii]=x2t2;
				}
				else
				{
					if (getswhite)
						pixvalsnew[ii]=1;
					else
						pixvalsnew[ii]=0;
				}
			else
				if ((pixvals[numberofpixels-1]==x1t1) && (updateit))
				{
					pixvalsnew[ii]=x2t2;
				}
				else
				{
					if (getswhite)
						pixvalsnew[ii]=1;
					else
						pixvalsnew[ii]=0;
				}
		};
		
	}
	else
	for (int ii=0;ii<numberofpixels;ii++)
	{
		updateit = (((float)rand())/((float)RAND_MAX+1.0f) < c);
		getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
		if ((ii==numberofpixels-1) && (!circularBC))
		{
			if (getswhite)
				pixvalsnew[ii]=1;
			else
				pixvalsnew[ii]=0;
		}
		else
		{
			if (ii<numberofpixels-1)
				if ((pixvals[ii+1]==x1t1) && (updateit))
				{
					pixvalsnew[ii]=x2t2;
				}
				else
				{
					if (getswhite)
						pixvalsnew[ii]=1;
					else
						pixvalsnew[ii]=0;
				}
			else
				if ((pixvals[0]==x1t1) && (updateit))
				{
					pixvalsnew[ii]=x2t2;
				}
				else
				{
					if (getswhite)
						pixvalsnew[ii]=1;
					else
						pixvalsnew[ii]=0;
				}
		};
		
	}

	copypixvalsnewtopixvals();
	updateparityvals();

};






void EnforcedCorr1D::update2(int parity)
{
	static int xtrip1[4][3]={{1,1,1},{1,0,0},{0,1,0},{0,0,1}}; // 2 different parities
	static int xtrip2[4][3]={{0,0,0},{0,1,1},{1,0,1},{1,1,0}};
	int (*xuse)[4][3];
	bool getswhite;
	bool changed;

	if (parity == 1)
		xuse=&xtrip1; // POINT to it
	else
		xuse=&xtrip2; // point to it

	//meat of loop
	for (int ii=0; ii<numberofpixels;ii++)
	{
		changed=FALSE;
		for (int jj=0;jj<4;jj++)
			if (test2((*xuse)[jj][0],(*xuse)[jj][1],ii))
			{
				pixvalsnew[ii]=(*xuse)[jj][2];
				changed=TRUE;
			};

		if (!changed)
		{
			getswhite = (((float)rand())/((float)RAND_MAX+1.0f) < Pwhite);
			if (getswhite)
				pixvalsnew[ii]=1;
			else
				pixvalsnew[ii]=0;
		}

	};
	copypixvalsnewtopixvals();
	updateparityvals();

};

bool EnforcedCorr1D::test2(int v1,int v2,int position)
{
	if (position>0)
		return ((pixvals[position-1]==v1) && (pixvals[position]==v2));
	else
	{ // first position...
		if (!circularBC)
			return (FALSE);
		else
			return ((pixvals[numberofpixels-1]==v1) && (pixvals[position]==v2));
	};
	
};

void EnforcedCorr1D::copypixvalstopixvalsnew()
{
	for (int ii=0; ii<numberofpixels;ii++)
		pixvalsnew[ii]=pixvals[ii];
};

void EnforcedCorr1D::copypixvalsnewtopixvals()
{
	for (int ii=0; ii<numberofpixels;ii++)
		pixvals[ii]=pixvalsnew[ii];
};

int EnforcedCorr1D::getpixelvalue(int pixnum)
{
	return pixvals[pixnum];
};

void EnforcedCorr1D::setDir(int dir)
{
	direction = dir;
};















