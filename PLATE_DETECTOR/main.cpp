#include <math.h>

#include <stdlib.h>
#include <string.h>

#include "cv.h"
#include "highgui.h"

#define TOP 1
#define BOTTOM 0

#define BOX_PLATE_WIDTH 180
#define BOX_PLATE_HEIGTH 48
/*
PERFEZIONAMENTI

calcolare i valori delle threshold
*/
IplImage * plate_growing(IplImage * src, CvScalar * regionColor);
double pendenzaRetta(CvPoint * p1, CvPoint * p2);
double MaxHorzStraightLineDetection(IplImage * src);
void elaboraImmagine(IplImage * img);
double angle( CvPoint* pt1, CvPoint* pt2, CvPoint* pt0 );
void threshold(IplImage * img, CvPoint * p);
void setDimension(IplImage * img, int * height, int * width);
//void bonfa();
void myHoughLines(IplImage * img, double *H, double *R, int *count, int *maxPend);
void disegnaRetteHR(IplImage * img,double *H, double *R, int count);
void disegnaRetteMQ(IplImage * img,double *M, double *Q, char * verticale, int count);
void antitrasformata(double *H, double *R, double *M, double *Q, char * verticale,int count, int rMax);

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

void threshold3Ch(IplImage * img , CvScalar th, CvScalar fgColor, CvScalar bgColor){
	int i,j;
	CvScalar px;
	for(i=0; i<img->width; i++){
		for(j=0; j<img->height;j++){
	
			px= cvGet2D(img,j,i );
			
			if(px.val[0]<th.val[0] && px.val[1]<th.val[1] && px.val[2]<th.val[2])
				cvSet2D(img,j,i,fgColor);
			else
				cvSet2D(img,j,i,bgColor);
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
	cvErode(img,img,structuringElem,1);
	cvDilate(img,img,structuringElem,1);

//	cvThreshold(img,img,200,255,CV_THRESH_BINARY);

	cvResetImageROI(img);


}

//CvRect findCandidate(IplImage * img){
//
//}

IplImage * findPlate(IplImage * img){
	IplImage * gauss, * match, *edge, *plate, * imgGray;
	CvRect ROI;

	/**/
	double minval, maxval;
	CvPoint maxloc, minloc;
	int i,j;
	/**/
	edge= cvCreateImage(cvGetSize(img),IPL_DEPTH_32F,1);
	gauss= cvCreateImage(cvGetSize(img),IPL_DEPTH_32F,1);
	imgGray= cvCreateImage(cvGetSize(img),IPL_DEPTH_8U,1);
	cvCvtColor(img,imgGray,CV_RGB2GRAY);

	verticalEdgeDetection(imgGray,edge);
//	cvShowImage("edge",edge);

	gaussianFilter(edge,gauss);
//	cvShowImage("gauss",gauss);


	match=matchFilter(gauss);
//	cvShowImage("matchFilter",match);	
//	cvShowImage("gauss",img);
	
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
	

		
	ROI=cvRect(maxloc.x,maxloc.y,BOX_PLATE_WIDTH,BOX_PLATE_HEIGTH);

//	morphoProcess(edge,ROI);
/*MASCHERA*/
/*	for(i=0; i< img->width; i++)
		for(j=0; j< img->height; j++)
			if(cvGet2D(edge,j,i).val[0]==0)
				cvSet2D(img,j,i,cvScalar(0,0,0,0));
*/		
	cvSetImageROI(img,ROI);
	plate= cvCreateImage(cvSize(img->roi->width,img->roi->height),img->depth,img->nChannels);

	cvCopyImage(img,plate);
	return plate;
}

void drawRect(IplImage * img, double m, double q){
	int x,y;

	for(x=0; x< img->width; x++){
		y= cvRound( m*x+q);
		if(y>0 && y< img->height){
		//	CvScalar px= cvGet2D(img,y,x);
		//	printf("%f %f %f\n",px.val[0],px.val[1],px.val[2]);
			cvSet2D(img,y,x,cvScalar(0,0,0,0));
		//	cvShowImage("a",img);
		//	cvWaitKey(0);
			
		}
	}

	cvShowImage("a",img);
	cvWaitKey(0);
}

CvScalar integraRetta(IplImage * img, double m, double q){
	int x,y;
	CvScalar res;
	res= cvScalar(0,0,0,0);
	for(x=0; x< img->width; x++){
		y= cvRound( m*x+q);
		if(y>0 && y< img->height){
			res.val[0]+= cvGet2D(img,y,x).val[0];
			res.val[1]+= cvGet2D(img,y,x).val[1];
			res.val[2]+= cvGet2D(img,y,x).val[2];
			res.val[3]+= cvGet2D(img,y,x).val[3];

			
		}
	}

	return res;
}

void findBorder(IplImage * img ,int type, double * mOUT ,double * qOUT,/*CvPoint * sx, CvPoint * dx,*/CvScalar * plateColor){
	double m,q,x,y;
	int i,j;
	double minval, maxval;
	CvPoint maxloc, minloc;
	
	IplImage * integrali, * imgBin;
	integrali= cvCreateImage(cvSize(1,img->height),IPL_DEPTH_32F,3);
	
	imgBin=plate_growing(img, plateColor);
	cvShowImage("i",imgBin);
	m=MaxHorzStraightLineDetection(imgBin);
	
	y=img->height/2.0;
	x=img->width/2.0;

	q=y -m*x;
/*	IplImage * imgGray= cvCreateImage(cvGetSize(img),IPL_DEPTH_8U,1);
	cvCvtColor(img,imgGray,CV_RGB2GRAY);
	cvCanny(imgGray,imgGray,100,50,3);
	cvThreshold(imgGray,imgGray,10,255,CV_THRESH_BINARY_INV);
*/
	if (type== BOTTOM){
		for(i=0; i< integrali->height; i++ ){
			cvSet2D(integrali,i,0,integraRetta(imgBin,m,q+i));
			
		}	
	}
	else {
		for(i=0; i > -integrali->height; i-- ){
			cvSet2D(integrali,-i,0,integraRetta(imgBin,m,q+i));
			
		}		
	
	}


//	cvShowImage("int",integrali);
//	cvShowImage("img",imgGray);
	j=0;
	while(cvGet2D(integrali,j,0).val[0]>10){
			j++;		
	}

	if(type==TOP)
		*qOUT=q-j;
	else
		*qOUT=q+j;


	*mOUT=m;


/*
	if(type==TOP)
		q=q-j+5;
	else
		q=q+j-7;


	for(i=0;i<imgBin->width;i++){
		y=m * i+q;
		if(y<imgBin->height && y >=0){
			printf("%f\n",cvGet2D(imgBin,y,i).val[0]);
			cvSet2D(imgBin,y,i,cvScalarAll(126));
			cvShowImage("S",imgBin);
			cvWaitKey(0);
		}

	}


*/	
}

int cmpCornerTL(CvPoint2D32f pnt0,CvPoint2D32f pnt1,double m){

	if(m>=0){
		if(pnt0.y <pnt1.y)
			return -1;
		else if(pnt0.y == pnt1.y)
			return 0;
		else
			return 1;
	}
	else{
		if(pnt0.x <pnt1.x)
			return -1;
		else if(pnt0.x == pnt1.x)
			return 0;
		else
			return 1;
	
	}

}

int cmpCornerTR(CvPoint2D32f pnt0,CvPoint2D32f pnt1,double m){

	if(m<0){
		if(pnt0.y <pnt1.y)
			return -1;
		else if(pnt0.y == pnt1.y)
			return 0;
		else
			return 1;
	}
	else{
		if(pnt0.x >pnt1.x)
			return -1;
		else if(pnt0.x == pnt1.x)
			return 0;
		else
			return 1;
	
	}

}

int cmpCornerBL(CvPoint2D32f pnt0,CvPoint2D32f pnt1,double m){

	if(m<0){
		if(pnt0.y >pnt1.y)
			return -1;
		else if(pnt0.y == pnt1.y)
			return 0;
		else
			return 1;
	}
	else{
		if(pnt0.x <pnt1.x)
			return -1;
		else if(pnt0.x == pnt1.x)
			return 0;
		else
			return 1;
	
	}

}

int cmpCornerBR(CvPoint2D32f pnt0,CvPoint2D32f pnt1,double m){

	if(m>0){
		if(pnt0.y >pnt1.y)
			return -1;
		else if(pnt0.y == pnt1.y)
			return 0;
		else
			return 1;
	}
	else{
		if(pnt0.x > pnt1.x)
			return -1;
		else if(pnt0.x == pnt1.x)
			return 0;
		else
			return 1;
	
	}

}
void cleanPlate(IplImage * img){
	
	double q,m,q1,m1;
	CvScalar plateColor;
	IplImage * imgBin;

	findBorder(img,TOP,&m,&q, &plateColor);
	findBorder(img,BOTTOM,&m1,&q1, &plateColor);
//	drawRect(img,m,q);
//	drawRect(img,m1,q1);

//	drawRect(img,1/m,q);
	imgBin= cvCreateImage(cvGetSize(img),CV_8S,1);
	imgBin=plate_growing(img, &plateColor);
	
	CvPoint2D32f srcPoint[4];

	int i,j;
	double y;

	//angolo in alto a sinistra: 
	srcPoint[0].x=imgBin->width;
	srcPoint[0].y=imgBin->height;
	for(i=0;i<BOX_PLATE_WIDTH/2; i++){
		for(j=0;j<(q1-q)/2;j++){
			y= m*i +q+j;			
			if(y>=0 && y< imgBin->height){
				if(cvGet2D(imgBin,y,i).val[0]>0 && cmpCornerTL(srcPoint[0],cvPoint2D32f(i,y),m)==1 ){
					srcPoint[0].x=i;
					srcPoint[0].y=y;
				}
			}
		}
	}

	//angolo in alto a destra: 
	srcPoint[2].x=0;
	srcPoint[2].y=imgBin->height;
	for(i=BOX_PLATE_WIDTH/2;i<BOX_PLATE_WIDTH; i++){
		for(j=0;j<(q1-q)/2;j++){
			y= m*i +q+j;			
			if(y>=0 && y< imgBin->height){
				if(cvGet2D(imgBin,y,i).val[0]>0 && cmpCornerTR(srcPoint[2],cvPoint2D32f(i,y),m)==1 ){
					srcPoint[2].x=i;
					srcPoint[2].y=y;
				}
			}
		}
	}

	//angolo in basso a sinistra: 
	srcPoint[1].x=imgBin->width;
	srcPoint[1].y=0;
	for(i=0;i<BOX_PLATE_WIDTH/2; i++){
		for(j=(q1-q)/2;j<q1-q;j++){
			y= m*i +q+j;			
			if(y>=0 && y< imgBin->height){
				if(cvGet2D(imgBin,y,i).val[0]>0 && cmpCornerBL(srcPoint[1],cvPoint2D32f(i,y),m)==1 ){
					srcPoint[1].x=i;
					srcPoint[1].y=y;
				}
			}
		}
	}


	//angolo in basso a destra: 
	srcPoint[3].x=0;
	srcPoint[3].y=0;
	for(i=BOX_PLATE_WIDTH/2;i<BOX_PLATE_WIDTH; i++){
		for(j=(q1-q)/2;j<(q1-q);j++){
			y= m*i +q+j;			
			if(y>=0 && y< imgBin->height){
				if(cvGet2D(imgBin,y,i).val[0]>0 && cmpCornerBR(srcPoint[3],cvPoint2D32f(i,y),m)==1 ){
					srcPoint[3].x=i;
					srcPoint[3].y=y;
				}
			}
		}
	}
	IplImage * plateClean= cvCreateImage(cvSize(img->width,q1-q),img->depth,img->nChannels);
	CvPoint2D32f/* srcPoint[4],*/ dstPoint[4];
	CvMat * map= cvCreateMat(3,3,CV_32F);

	srcPoint[0].y= srcPoint[0].y-4;
	srcPoint[2].y= srcPoint[2].y-4;
	srcPoint[1].y= srcPoint[1].y+4;
	srcPoint[3].y= srcPoint[3].y+4;

	dstPoint[0]= cvPoint2D32f(0,0);
	dstPoint[1]= cvPoint2D32f(0,q1-q);
	dstPoint[2]= cvPoint2D32f(img->width,0);
	dstPoint[3]= cvPoint2D32f(img->width,q1-q);

	cvGetPerspectiveTransform(srcPoint,dstPoint , map);
	cvWarpPerspective(img,plateClean,map,CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS,plateColor);

	for(i=0; i<4; i++)
		cvDrawCircle(img,cvPoint(srcPoint[i].x,srcPoint[i].y),1,cvScalar(0,255,0,0),1,8,0);
	cvShowImage("Q",img);

//	threshold3Ch(plateClean,cvScalar(150,150,150,0),cvScalar(0,0,0,0),cvScalar(255,255,255,255));
	
	cvShowImage("b",plateClean);

}


int main(){
	IplImage * img, * plate;

	char* immagineIntermedia = "saved.tif";
 
 
	img= cvLoadImage("test/targa_piccola2.jpg",1);
	assert(img->depth== IPL_DEPTH_8U);
	plate=findPlate(img);
	cleanPlate(plate);
cvWaitKey(0);
	img= cvLoadImage("test/targa_piccola3.jpg",1);
	assert(img->depth== IPL_DEPTH_8U);
	plate=findPlate(img);
	cleanPlate(plate);
cvWaitKey(0);
	img= cvLoadImage("test/targa_media.jpg",1);
	assert(img->depth== IPL_DEPTH_8U);
	plate=findPlate(img);
	cleanPlate(plate);
cvWaitKey(0);
	img= cvLoadImage("test/targa_media2.jpg",1);
	assert(img->depth== IPL_DEPTH_8U);
	plate=findPlate(img);
	cleanPlate(plate);
cvWaitKey(0);
	img= cvLoadImage("test/targa_grande1.jpg",1);
	assert(img->depth== IPL_DEPTH_8U);
	plate=findPlate(img);
	cleanPlate(plate);
cvWaitKey(0);
	img= cvLoadImage("test/targa_grande2.jpg",1);
	assert(img->depth== IPL_DEPTH_8U);
	plate=findPlate(img);
	cleanPlate(plate);
cvWaitKey(0);
	img= cvLoadImage("test/grandepunto1.jpg",1);
	assert(img->depth== IPL_DEPTH_8U);
	plate=findPlate(img);
	cleanPlate(plate);
cvWaitKey(0);
	img= cvLoadImage("test/panda.jpg",1);
	assert(img->depth== IPL_DEPTH_8U);
	plate=findPlate(img);
	cleanPlate(plate);

	cvWaitKey(0);
/*
	cvShowImage("pre-processing",img);
	elaboraImmagine(img);
	cvShowImage("post-processing",img);
	cvSaveImage(immagineIntermedia,img,0);
	
	printf("Immagine salvata\n");
	printf("Passo l'immagine a Tesseract per l'ocr\n");
	//ocr(immagineIntermedia);  //decommentare una volta installato tesseract

	
	cvWaitKey(0);
*/
	

}


/**Effettua l'elaborazione che precede l'ocr*/
void elaboraImmagine(IplImage * img){
	int height,width;
	setDimension(img,&height,&width);
	CvPoint * minPoint, * maxPoint;
	minPoint = &cvPoint(-1,-1);
	maxPoint = &cvPoint(-1,-1);
	double angoloRotazione=0;
	if ((angoloRotazione=MaxHorzStraightLineDetection(img))!=0){
			CvMat *mapMatrix = cvCreateMat(2,3, CV_32F);
			//cv2DRotationMatrix(cvPoint2D32f(((double) img->height)/2.0,((double) img->width)/2.0),angoloRotazione*180/CV_PI,1.0,mapMatrix);
			cv2DRotationMatrix(cvPoint2D32f(((double) width)/2.0,((double) height)/2.0),angoloRotazione*180.0/CV_PI,1.0,mapMatrix);
			IplImage * rotated = cvCreateImage(cvGetSize(img),img->depth,img->nChannels);
			cvWarpAffine(img, rotated, mapMatrix);
			cvCopy(rotated,img,0);
			//cvShowImage("ruotata",img);
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

//void bonfa(){
//	char* nomeImmagine = "leon.jpg";
//	char* immagineIntermedia = "saved.tif";
//
//	IplImage *img = cvLoadImage(nomeImmagine,0);
//	elaboraImmagine(img);
//	cvShowImage("originale",img);
//	cvSaveImage(immagineIntermedia,img,0);
//	
//	printf("Immagine salvata\n");
//	printf("Passo l'immagine a Tesseract per l'ocr\n");
//	//ocr(immagineIntermedia);  //decommentare una volta installato tesseract
//
//	//cvWaitKey(0);
//	exit(0);
//}

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

/** Cerca la retta orizzontale più lunga.
	IplImage * src - l'immagine in cui cercare la retta
	cvPoint * p1 - Il primo punto che limita la retta
	cvPoint * p2 - Il secondo punto che limita la retta
*/
double MaxHorzStraightLineDetection(IplImage * src){
	double maxLength=0;
	int countMaxLength=-1;

	IplImage *edge = cvCreateImage(cvGetSize(src),src->depth,src->nChannels);
	IplImage * horzEdge = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 1);
	//CvMemStorage * storage = cvCreateMemStorage(0);
	
	cvCanny(src,edge,50,150,3);
	//cvShowImage("edge",src);
	
	IplImage * edgeU = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
	cvConvertScale(horzEdge,edgeU,edge->depth/IPL_DEPTH_8U,0);
	
	cvSobel(src, horzEdge, 0, 1, 3);
//	cvShowImage("horzEdge",horzEdge);
		
	cvConvertScale(horzEdge,edgeU,IPL_DEPTH_32F/IPL_DEPTH_8U,0);
//	cvShowImage("horzU",edgeU);
	

	
	
	double * h = (double *)malloc(100*sizeof(double));
	double * r = (double *)malloc(100*sizeof(double));
	int count = 0;
	int maxPend = -1;
	myHoughLines(edgeU,h,r,&count,&maxPend);

	/*for(int i=0;i<count;i++){
		printf("%2.2f\t %2.2f\n",m[i],q[i]);
	}*/

	
	IplImage * righe = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 1);
	disegnaRetteHR(righe,h,r,count);

	//m e q sono due vettori che contengono H e R
	//delle count rette con lunghezza maggiore dell'immagine
	double * m = (double *)malloc(100*sizeof(double));
	double * q = (double *)malloc(100*sizeof(double));
	char * verticale = (char *)malloc(100*sizeof(char));
	int rMax = (int) floor((double)sqrt(pow((double)edgeU->height,2)+pow((double)edgeU->width,2)))+1;
	antitrasformata(h,r,m,q,verticale,count,rMax);
	IplImage * righe2 = cvCreateImage(cvGetSize(src), IPL_DEPTH_32F, 1);
	disegnaRetteMQ(righe2,m,q,verticale,count);

	//int p = 1;
	//double h_rad = h[p]*CV_PI/180;
	//for(int j=0;j<src->width;j++){
	//	double  i = (r[p]-rMax-j*cos(h_rad))/sin(h_rad);
	//	//if(i>=0 && i<src->height){
	//			printf("H=%2.4f\tR=%2.4f\tI=%2.2f\tJ=%d\n",h_rad,r[p],i,j);
	//			printf("M=%2.4f\tQ=%2.4f\tI=%2.2f\tJ=%d\n\n",m[p],q[p],m[p]*j+q[p],j);
	//}
	
	free(h);
	free(r);
	double angolo = 0;
	if (maxPend!=-1)
		angolo = m[maxPend];
	else if (count != 0)
		angolo = m[0];
	free(m);
	free(q);
	free(verticale);
	return angolo;
}

void myHoughLines(IplImage * img, double *H, double *R, int *count, int *maxPend){
	IplImage *edge,*accumulation;
	
	//Edge Detection
	edge = cvCreateImage(cvGetSize(img),img->depth,1);
	cvCanny(img,edge,10,100,3);

	//Creazione della ACCUMULATION MATRIX
	int thetaMax = 180;
	int rMax = (int) floor((double)sqrt(pow((double)img->height,2)+pow((double)img->width,2)))+1;
	accumulation = cvCreateImage(cvSize(2*rMax,thetaMax),img->depth,1);
	for (int i=0;i<accumulation->height;i++)
		for(int j=0;j<accumulation->width;j++)
			cvSet2D(accumulation,i,j,cvRealScalar(0));

	//Disegno la sinusoide per ogni punto dell'edge
	for(int i=0;i<img->height;i++)
		for(int j=0;j<img->width;j++)
			if (cvGet2D(edge,i,j).val[0]==255)
				for(int h=0;h<thetaMax;h++){ // h cicla su theta
					double h_rad = h*CV_PI/180;
					double r = j*cos(h_rad)+i*sin(h_rad)+ rMax; //calcolo di r
					//printf("H=%d\tR=%2.2f\n",h,r);
					cvSet2D(accumulation,h,(int)r,cvRealScalar(cvGet2D(accumulation,h,(int)r).val[0]+1));
					}

	//Seleziono i punti di massimo della matrice di accumulazione e memorizzo h e r in una sequenza
	double soglia;
	cvMinMaxLoc(accumulation,0,&soglia,0,0,0);
	soglia = soglia - 10;
	int quante = 0;
	*maxPend = -1;
	for(int h=0;h<accumulation->height;h++)
		for(int l=0;l<accumulation->width;l++)
			if (cvGet2D(accumulation,h,l).val[0] > soglia){
				if(quante < 100) {
					//double h_rad = ((double)h)*CV_PI/180.0;
					H[quante] = h;
					R[quante] = l;
					quante++;
					if (cvGet2D(accumulation,h,l).val[0]==soglia+10 && *maxPend==-1)
						*maxPend = quante-1;
				}
			}
	*count = quante;
}


/** Disegna le count rette descritte dalle coppie H e R, parametri della trasformata di Hough */
void disegnaRetteHR(IplImage * img,double *H, double *R, int count){
	int rMax = (int) floor((double)sqrt(pow((double)img->height,2)+pow((double)img->width,2)))+1;
	for(int p=0;p<count;p++) {
			double h_rad = H[p]*CV_PI/180.0;
			if (H[p]!=0)
			for(int j=0;j<img->width;j++){
				//Disegna la retta
				double  i = (R[p]-rMax-j*cos(h_rad))/sin(h_rad);
				if(i>=0 && i<img->height)
					//printf("H=%d\tL=%d\tI=%2.2f\tJ=%d\n",h,l,i,j);
					cvSet2D(img,(int)i,j,cvRealScalar(255));
				}
			else
				for(int i=0;i<img->height;i++)
					cvSet2D(img,i,R[p]-rMax,cvRealScalar(255));	
		}
	//cvShowImage("rette",img);
}

/** Calcola M e Q delle count rette descritte dalle coppie H e R della trasformata di Hough 
  * con origine il centro dell'immagine */
void antitrasformata(double *H, double *R, double *M, double *Q, char * verticale,int count,int rMax) {
	for(int p=0;p<count;p++) {
			double h_rad = H[p]*CV_PI/180.0;
			if (H[p]!=0){
				M[p] = -cos(h_rad)/sin(h_rad);
				Q[p] = ((double)(R[p]-rMax))/sin(h_rad);
				verticale[p] = 0;
			}
			else {
				M[p]=0;
				Q[p]=R[p]-rMax;
				verticale[p]=1;
			}
	}
}

/** Disegna le count rette descritte dalle coppie M e Q e dal flag verticale */
void disegnaRetteMQ(IplImage * img,double *M, double *Q, char * verticale, int count){
	double i;
	for(int p=0;p<count;p++) {
			if (!verticale[p])
				for(int j=0;j<img->width;j++) {
					//printf("M=%2.4f\tQ=%2.4f\tI=%2.2f\tJ=%d\n",M[p],Q[p],M[p]*j+Q[p],j);
					if((i=M[p]*j+Q[p])>=0 && i<img->height)
						cvSet2D(img,(int)i,j,cvRealScalar(255));}
			else
				for(int i=0;i<img->height;i++)
					cvSet2D(img,i,Q[p],cvRealScalar(255));	
		}	
	//cvShowImage("retteMQ",img);
}


struct StackElem {

	int x;
	int y;

	struct StackElem * next;

} typedef StackElem;

struct Stack {
	StackElem * top;

} typedef Stack;

Stack * createStack(){
	Stack * s= (Stack*)malloc(sizeof(Stack));
	s->top= NULL;
	return s;
}

void push(Stack * stack, int x, int y){
	StackElem * newElem=(StackElem*)malloc(sizeof(StackElem));
	newElem->x=x;
	newElem->y=y;
	newElem->next= stack->top;
	stack->top=newElem;	

}

StackElem * pop(Stack * stack){
	StackElem * top= stack->top;
	if(top)
		stack->top= stack->top->next;

	return top;
}
int isSeed(IplImage * img,IplImage * map, int x,int y,double th){
	int i,j;
	double media=0.0;

	if(!cvGet2D(map,x,y).val[0]<0)
		return 0;

	
	for(i=x-1; i<=x+1; i++){
		for(j=y-1; j<=y+1; j++){
			if(i>=0 && j>=0 &&i< img->height && j< img->width)	
			media+=cvGet2D(img,i,j).val[0];
		}
	}

	media/=9;

	if(fabs(media-cvGet2D(img,x,y).val[0]) < th)
		return 1;
	else
		return 0;
}

void ispeziona(IplImage * img, StackElem * px, Stack * daIspezionare,CvScalar media,double th,IplImage * map,CvMat * ispezionati){
	int i,j;
//	printf("px %d %d\n",px->x,px->y);
	cvSet2D(map,px->x,px->y,cvScalar(255,0,0,0));
	for(i=px->x-1; i<=px->x+1; i++){
		for(j=px->y-1; j<=px->y+1; j++){
			if(i>=0 && j>=0 && i< img->height && j< img->width &&
					fabs(cvGet2D(img,i,j).val[0]-media.val[0])<th && 
						fabs(cvGet2D(img,i,j).val[1]-media.val[1])<th &&
							fabs(cvGet2D(img,i,j).val[2]-media.val[2])<th &&
								cvGet2D(ispezionati,i,j).val[0]<0){
				push(daIspezionare,i,j);
			//	printf("p%f ",cvGet2D(ispezionati,i,j).val[0]);
				cvSet2D(ispezionati,i,j,cvScalar(1,0,0,0));
			//	printf("%d %d\n",i,j);
			//	printf("d%f\n\n ",cvGet2D(ispezionati,i,j).val[0]);
				//getchar();
			
			}

		}
	}
	
	
}



IplImage* plate_growing(IplImage * src, CvScalar * regionColor){
	int i,j;
	Stack * daIspezionare;
	StackElem * px;
	double th=30.0;
	CvScalar media;
	int n;
	CvPoint seed;

	IplImage * map =cvCreateImage(cvSize(src->width,src->height),IPL_DEPTH_8U,1);
	CvMat * ispezionati = cvCreateMat(src->height,src->width,CV_8S);
	cvSet(map,cvScalar(-1.0,0,0,0),0);
	
	
	seed= cvPoint(BOX_PLATE_WIDTH/2-10,BOX_PLATE_HEIGTH/2);
	daIspezionare=createStack();
	n=0;
	
	while(n<800){
		seed.x++;
		push(daIspezionare,seed.y,seed.x);

		media=cvScalarAll(0);
		n=0;
		cvSet(ispezionati,cvScalar(-1,0,0,0),0);

				//	printf("%f\n",r);
				//	getchar();
					
		while(px=pop(daIspezionare)){
			media.val[0]= ((media.val[0]*n)+ cvGet2D(src,px->x,px->y).val[0])/(double)(n+1);
			media.val[1]= ((media.val[1]*n)+ cvGet2D(src,px->x,px->y).val[1])/(double)(n+1);
			media.val[2]= ((media.val[2]*n)+ cvGet2D(src,px->x,px->y).val[2])/(double)(n+1);
			n++;
			ispeziona(src,px,daIspezionare,media,th,map,ispezionati);
		}

	}
					
	*regionColor= media;	
	return	 map;

}
