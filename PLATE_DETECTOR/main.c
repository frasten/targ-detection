#include <math.h>

#include <stdlib.h>
#include <string.h>

#include "cv.h"
#include "highgui.h"

/*
PERFEZIONAMENTI

calcolare i valori delle threshold

*/

void applyThreshold(IplImage * img , double th){
	int i,j;
	CvScalar px;
	for(i=0; i<img->width; i++){
		for(j=0; j<img->height;j++){
	
			px= cvGet2D(img,j,i );
			
			if(px.val[0]<th)
				cvSet2D(img,j,i,cvScalar(0,0,0,0));
		}
	}
}


void verticalEdgeDetection(IplImage * src, IplImage * dst){
	double th;
	th=200;
	
	
	cvSobel(src,dst,1,0,3);
	cvAbs(dst,dst);

	applyThreshold(dst,th);



}

void gaussianFilter(IplImage * src, IplImage * dst){
	
	cvSmooth(src,dst,CV_GAUSSIAN,11,11,6,0);

	cvConvertScale(dst,dst,1./255.,0);

}

IplImage * matchFilter(IplImage * img){
	IplImage *  tmpl32F, * tmpl8, * match;
	int matchDstWidth, matchDstHeigth;

	tmpl8= cvLoadImage("tmpl.png",0);
	assert(tmpl8->depth== IPL_DEPTH_8U);
	tmpl32F= cvCreateImage(cvGetSize(tmpl8),IPL_DEPTH_32F,1);
	cvConvertScale(tmpl8,tmpl32F,1./255.,0);

	matchDstWidth= img->width - tmpl32F->width + 1;
	matchDstHeigth = img->height - tmpl32F->height + 1;
	
	match = cvCreateImage( cvSize( matchDstWidth,matchDstHeigth ), IPL_DEPTH_32F, 1 );

	cvMatchTemplate( img, tmpl32F, match, CV_TM_CCORR );

	applyThreshold(match,150.0);
		

	cvConvertScale(match,match,1./500.,0);

	return match;
	

}

void morphoProcess(IplImage * img, CvRect roi){
	IplConvKernel * structuringElem;
	//cvSetImageROI(img,roi);
	structuringElem= cvCreateStructuringElementEx(18,6,5,1,CV_SHAPE_RECT,0);
	cvDilate(img,img,structuringElem,1);
	cvErode(img,img,structuringElem,1);

	cvErode(img,img,structuringElem,1);
	cvDilate(img,img,structuringElem,1);
	
//	cvResetImageROI(img);


}

CvRect findCandidate(IplImage * img){

}

void findPlate(IplImage * img){
	IplImage * gauss, * match, *edge;

	/**/
	double minval, maxval;
	CvPoint maxloc, minloc;
	/**/
	edge= cvCreateImage(cvGetSize(img),IPL_DEPTH_32F,img->nChannels);
	gauss= cvCreateImage(cvGetSize(img),IPL_DEPTH_32F,img->nChannels);
	
	verticalEdgeDetection(img,edge);
	cvShowImage("edge",edge);

	gaussianFilter(edge,gauss);
	cvShowImage("gauss",gauss);


	match=matchFilter(gauss);
	cvShowImage("matchFilter",match);	

	
	cvMinMaxLoc( match, &minval, &maxval, &minloc, &maxloc, 0 );

	cvRectangle( img, 
				 cvPoint( maxloc.x, maxloc.y ), 
				 cvPoint( maxloc.x+5, maxloc.y+5),
				 cvScalar( 0, 0, 0, 0 ), 1, 0, 0 );


/*
		cvRectangle( img, 
				 cvPoint( 80, 200 ), 
				 cvPoint( 190, 220),
				 cvScalar( 0, 0, 255, 0 ), 1, 0, 0 );
*/
	

		
morphoProcess(edge,cvRect(50,170,100,70));
		
	
	cvShowImage("edg",edge);
	cvShowImage("ed",img);
	cvWaitKey(0);

}


int main(){
IplImage * src;



src= cvLoadImage("img/leon.JPG",0);
	assert(src->depth== IPL_DEPTH_8U);

	findPlate(src);
}