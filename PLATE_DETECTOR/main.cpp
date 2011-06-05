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


	//img= cvLoadImage("test/targa_grande1.jpg",0);
	//img= cvLoadImage("test/targa_media2.jpg",0);
	img= cvLoadImage("test/targa_piccola3.jpg",0);
	assert(img->depth== IPL_DEPTH_8U);
	findPlate(img);

	cvShowImage("pre-processing",img);
	elaboraImmagine(img);
	cvShowImage("post-processing",img);
	cvSaveImage(immagineIntermedia,img,0);
	
	printf("Immagine salvata\n");
	printf("Passo l'immagine a Tesseract per l'ocr\n");
	//ocr(immagineIntermedia);  //decommentare una volta installato tesseract

	
	cvWaitKey(0);

	

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
	cvShowImage("edge",edge);
	
	IplImage * edgeU = cvCreateImage(cvGetSize(src), IPL_DEPTH_8U, 1);
	cvConvertScale(horzEdge,edgeU,edge->depth/IPL_DEPTH_8U,0);
	
	cvSobel(src, horzEdge, 0, 1, 3);
	cvShowImage("horzEdge",horzEdge);
		
	cvConvertScale(horzEdge,edgeU,IPL_DEPTH_32F/IPL_DEPTH_8U,0);
	cvShowImage("horzU",edgeU);
	

	
	
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
	cvShowImage("rette",img);
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
	cvShowImage("retteMQ",img);
}