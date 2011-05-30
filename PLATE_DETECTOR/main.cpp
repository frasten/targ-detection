#include <math.h>

#include <stdlib.h>
#include <string.h>

#include "cv.h"
#include "highgui.h"

/*
PERFEZIONAMENTI

calcolare i valori delle threshold


*/
double pendenzaRetta(CvPoint * p1, CvPoint * p2);
int MaxHorzStraightLineDetection(IplImage * src, CvPoint * p1, CvPoint * p2);
void elaboraImmagine(IplImage * img);
double angle( CvPoint* pt1, CvPoint* pt2, CvPoint* pt0 );
void threshold(IplImage * img, CvPoint * p);
void setDimension(IplImage * img, int * height, int * width);
void bonfa();



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
	cvSetImageROI(img,roi);
	structuringElem= cvCreateStructuringElementEx(14,10,7,5,CV_SHAPE_RECT,0);
	cvDilate(img,img,structuringElem,1);

	cvErode(img,img,structuringElem,1);
	cvDilate(img,img,structuringElem,1);
	
	cvResetImageROI(img);


}

//CvRect findCandidate(IplImage * img){
//
//}

void findPlate(IplImage * img){
	IplImage * gauss, * match, *edge;
	CvRect ROI;

	/**/
	double minval, maxval;
	CvPoint maxloc, minloc;
	int i,j;
	/**/
	edge= cvCreateImage(cvGetSize(img),IPL_DEPTH_32F,img->nChannels);
	gauss= cvCreateImage(cvGetSize(img),IPL_DEPTH_32F,img->nChannels);
	
	verticalEdgeDetection(img,edge);
//	cvShowImage("edge",edge);

	gaussianFilter(edge,gauss);
//	cvShowImage("gauss",gauss);


	match=matchFilter(gauss);
//	cvShowImage("matchFilter",match);	

	
	cvMinMaxLoc( match, &minval, &maxval, &minloc, &maxloc, 0 );

	cvRectangle( img, 
				 cvPoint( maxloc.x, maxloc.y ), 
				 cvPoint( maxloc.x+180, maxloc.y+48),
				 cvScalar( 0, 0, 0, 0 ), 1, 0, 0 );


/*
		cvRectangle( img, 
				 cvPoint( 80, 200 ), 
				 cvPoint( 190, 220),
				 cvScalar( 0, 0, 255, 0 ), 1, 0, 0 );
*/
	

		
	ROI=cvRect(maxloc.x,maxloc.y,180,48);
	morphoProcess(edge,ROI);
		
	
	cvShowImage("edg",edge);
//	cvShowImage("ed",img);
//	cvWaitKey(0);


//MASCHERA
	for(i=0; i<img->width; i++)
		for(j=0; j< img->height; j++)
			if(cvGet2D(edge,j,i).val[0]==0)
				cvSet2D(img,j,i,cvScalar(255,0,0,0));

		cvShowImage("edg",img);
		cvSetImageROI(img,ROI);
//	cvShowImage("ed",img);
	//cvWaitKey(0);

}


int main(){
	IplImage * img;

	char* immagineIntermedia = "saved.tif";


	//img= cvLoadImage("test/targa_grande2.jpg",0);
	//img= cvLoadImage("test/targa_media2.jpg",0);
	img= cvLoadImage("test/targa_piccola3.jpg",0);
	assert(img->depth== IPL_DEPTH_8U);
	findPlate(img);

	
	elaboraImmagine(img);
	cvShowImage("originale",img);
	cvSaveImage(immagineIntermedia,img,0);
	
	printf("Immagine salvata\n");
	printf("Passo l'immagine a Tesseract per l'ocr\n");
	//ocr(immagineIntermedia);  //decommentare una volta installato tesseract

	
	cvWaitKey(0);

	

}

/** Cerca la retta orizzontale più lunga.
	IplImage * src - l'immagine in cui cercare la retta
	cvPoint * p1 - Il primo punto che limita la retta
	cvPoint * p2 - Il secondo punto che limita la retta
*/
int MaxHorzStraightLineDetection(IplImage * src, CvPoint * p1, CvPoint * p2){
	CvSeq* lines;
	double maxLength=0;
	int countMaxLength=-1;

	IplImage *edge = cvCreateImage(cvGetSize(src),src->depth,src->nChannels);
	IplImage * horzEdge = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 1);
	CvMemStorage * storage = cvCreateMemStorage(0);
	
	cvCanny(src,edge,250,250,3);
	cvShowImage("edge",edge);
	
	IplImage * edgeU = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
	//cvConvertScale(horzEdge,edgeU,edge->depth/IPL_DEPTH_8U,0);
	//cvCopy(edge,edgeU);
	//cvShowImage("horzU",edgeU);

	//cvConvertScale(edge,horzEdge,1,0);
	//cvCopy(edge,horzEdge);

	cvSobel(edge, horzEdge, 0, 1, 3);
	cvShowImage("horzEdge",horzEdge);
		
	//IplImage * edgeU = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
	cvConvertScale(horzEdge,edgeU,IPL_DEPTH_32F/IPL_DEPTH_8U,0);
	cvShowImage("horzU",edgeU);
	

	IplImage * righe = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 4);
	lines = cvHoughLines2(edgeU, storage, CV_HOUGH_PROBABILISTIC , 8, CV_PI/1800, 10, 11,4);
	//lines = cvHoughLines2(edgeU, storage, CV_HOUGH_PROBABILISTIC , 0.1, CV_PI/1800, 10, 11,4);
	//lines = cvHoughLines2(edgeU, storage, CV_HOUGH_STANDARD , 0.01, 0.01, 1.5, 0,0);
	for(int i=0;i<lines->total;i++){
		CvPoint* line = (CvPoint*)cvGetSeqElem(lines,i);
		double length = sqrt(pow(double(line[0].y-line[1].y),2)+pow(double(line[0].x-line[1].x),2));
		if(length>maxLength){
			maxLength = length;
			countMaxLength = i;
		}
		cvLine(righe,line[0],line[1],cvScalar(255,255,255,255),1,8,0);
	}
	if (countMaxLength != -1){
		CvPoint* line = (CvPoint*)cvGetSeqElem(lines,countMaxLength);
		p1->x = line[0].x;
		p1->y = line[0].y;
		p2->x = line[1].x;
		p2->y = line[1].y;
		cvLine(righe,line[0],line[1],cvScalar(255,0,0,0),3,8,0);
		cvShowImage("righe",righe);
		//cvWaitKey(0);
		return 1;
	}
	cvShowImage("righe",righe);
	//cvWaitKey(0);
	return 0;
}

/**Effettua l'elaborazione che precede l'ocr*/
void elaboraImmagine(IplImage * img){
	int height,width;
	setDimension(img,&height,&width);
	CvPoint * minPoint, * maxPoint;
	minPoint = &cvPoint(-1,-1);
	maxPoint = &cvPoint(-1,-1);
	if (MaxHorzStraightLineDetection(img,minPoint,maxPoint)){
		double angoloRotazione = pendenzaRetta(minPoint,maxPoint);
		if(angoloRotazione != 0){
			CvMat *mapMatrix = cvCreateMat(2,3, CV_32F);
			//cv2DRotationMatrix(cvPoint2D32f(((double) img->height)/2.0,((double) img->width)/2.0),angoloRotazione*180/CV_PI,1.0,mapMatrix);
			cv2DRotationMatrix(cvPoint2D32f(((double) width)/2.0,((double) height)/2.0),angoloRotazione*180/CV_PI,1.0,mapMatrix);
			IplImage * rotated = cvCreateImage(cvGetSize(img),img->depth,img->nChannels);
			cvWarpAffine(img, rotated, mapMatrix);
			cvCopy(rotated,img,0);
			//cvShowImage("ruotata",img);
		}
	}

	//cvLine(img,*minPoint,*maxPoint,cvScalar(0,0,0,0),1,8,0);
	//isolaTarga(img);
}

/**Calcola la pendenza di tale retta*/
double pendenzaRetta(CvPoint * p1, CvPoint * p2){
	if((p1->x != p2->x))
		return atan2(double(p2->y - p1->y),double(p2->x - p1->x));
	else 
		return CV_PI;
}

/**Pone a zero tutti i pixel dell'immagine binaria che sono inferiori al pixel p*/
void threshold(IplImage * img, CvPoint * p){
	if (p->x < img->height || p->y < img->width){
		double max = cvGet2D(img,p->x,p->y).val[0];
		for(int i=0;i<img->height;i++)
			for(int j=0;j<img->width;j++)
				if (cvGet2D(img,i,j).val[0] < max)
					cvSet2D(img,i,j,cvRealScalar(0));
	}
}

void bonfa(){
	char* nomeImmagine = "leon.jpg";
	char* immagineIntermedia = "saved.tif";

	IplImage *img = cvLoadImage(nomeImmagine,0);
	elaboraImmagine(img);
	cvShowImage("originale",img);
	cvSaveImage(immagineIntermedia,img,0);
	
	printf("Immagine salvata\n");
	printf("Passo l'immagine a Tesseract per l'ocr\n");
	//ocr(immagineIntermedia);  //decommentare una volta installato tesseract

	//cvWaitKey(0);
	exit(0);
}

/** Assegna a width e height passati come parametri i corrispettivi valori della ROI dell'immagine*/
void setDimension(IplImage * img, int *height, int *width){
	if (img->roi!=NULL) {
		*height = img->roi->height;
		*width = img->roi->width;
		}
	else {
		*height = img->roi->height;
		*width = img->roi->width;
		}
}