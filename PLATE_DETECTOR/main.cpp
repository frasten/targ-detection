#include <math.h>

#include <stdlib.h>
#include <string.h>

#include "cv.h"
#include "highgui.h"

#define TOP 1
#define BOTTOM 0

#define BOX_PLATE_WIDTH 180
#define BOX_PLATE_HEIGTH 48

#define PI 3.141592653589793

/*
PERFEZIONAMENTI

calcolare i valori delle threshold
*/

IplImage * plate_growing(IplImage * src, CvScalar * regionColor, CvPoint plateCenter);

int character_growing(IplImage * src, IplImage * map,CvPoint seed);
//double pendenzaRetta(CvPoint * p1, CvPoint * p2);
//double MaxHorzStraightLineDetection(IplImage * src);
//void elaboraImmagine(IplImage * img);
//double angle( CvPoint* pt1, CvPoint* pt2, CvPoint* pt0 );
void threshold(IplImage * img, CvPoint * p);
//void setDimension(IplImage * img, int * height, int * width);
//void bonfa();
//void myHoughLines(IplImage * img, double *H, double *R, int *count, int *maxPend);
//void disegnaRetteHR(IplImage * img,double *H, double *R, int count);
//void disegnaRetteMQ(IplImage * img,double *M, double *Q, char * verticale, int count);
//void antitrasformata(double *H, double *R, double *M, double *Q, char * verticale,int count, int rMax);
//void intersezioneRette(IplImage * img, double theta1, double theta2, double r1, double r2);

void trovaContorno (IplImage * src, IplImage * dst , int position);

void explain(IplImage * img, char * msg);


/*
dato un punto x y disegna la sinusoide accumulando il valore dei pixel nello spazio di Hough
hough(r,theta) [righe ,colonne]
*/
void RH(int x, int y, IplImage * hough);
/*dallo spazio di hough alla retta, disegna in img la retta x, y dello psazio di hogh*/
	//l'offset a r deve essere gia' stato applicato
	//theta in gradi da 0 a 179
void HR(int theta, int r , IplImage * img, double * m1, double * q1 );

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

void threshold3Ch(IplImage * src ,IplImage * dst, CvScalar th, CvScalar fgColor, CvScalar bgColor){
	int i,j;
	CvScalar px;
	for(i=0; i<src->width; i++){
		for(j=0; j<src->height;j++){
	
			px= cvGet2D(src,j,i );
			
			if(px.val[0]<th.val[0] && px.val[1]<th.val[1] && px.val[2]<th.val[2])
				cvSet2D(dst,j,i,fgColor);
			else
				cvSet2D(dst,j,i,bgColor);
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

void closing(IplImage * img){
	IplConvKernel * structuringElem;
	structuringElem= cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_RECT,0);
	cvErode(img,img,structuringElem,1);
	cvDilate(img,img,structuringElem,1);
	
}


void morphoProcess(IplImage * img){
	IplConvKernel * structuringElem;
	structuringElem= cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_RECT,0);
	cvDilate(img,img,structuringElem,1);
	cvErode(img,img,structuringElem,1);
}

//CvRect findCandidate(IplImage * img){
//
//}

CvPoint findPlate(IplImage * img){
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
	explain(edge,"Trovo gli edge verticali");

	gaussianFilter(edge,gauss);
	explain(gauss,"Applico un filtro gaussiano");


	match=matchFilter(gauss);
	
	cvMinMaxLoc( match, &minval, &maxval, &minloc, &maxloc, 0 );

	//EXPLAIN
	IplImage * explainImg= cvCreateImage(cvGetSize(img),img->depth,img->nChannels);
	cvCopyImage(img,explainImg);

	cvRectangle( explainImg, 
				 cvPoint( maxloc.x, maxloc.y ), 
				 cvPoint( maxloc.x+180, maxloc.y+48),
				 cvScalar( 0, 255, 0, 0 ), 1, 0, 0 );

	explain(explainImg,"Trovo la targa usando la crosscorrelazione");

	
	maxloc.x+= BOX_PLATE_WIDTH/2;
	maxloc.y+= BOX_PLATE_HEIGTH/2;
	return maxloc;
}

void drawRect(IplImage * img, double m, double q){
	int x0,y0,x1,y1;

	x0=img->width/4;
	x1= img->width/4*3;

	y0= m*x0 + q;
	y1= m* x1 +q;

	cvDrawLine(img,cvPoint(x0,y0),cvPoint(x1,y1),cvScalar(0,0,255,0),1,8,0);
	

}



double quantiPxNeri(IplImage * img, double m, double q, int x0, int x1){
	int x,y;
	
	CvScalar media=cvScalarAll(0);
	CvScalar var=cvScalarAll(0);
	CvScalar px;
		
	for(x=x0; x< x1; x++){
		y= cvRound( m*x+q);
		if(y>0 && y< img->height){
			px=cvGet2D(img,y,x);
			
			if(px.val[0]<100)
				var.val[0]++;
			if(px.val[1]<100)
				var.val[1]++;
			if(px.val[2]<100)
				var.val[2]++;
		}
	}
	
	return var.val[0]+var.val[1]+var.val[2];
}


int cmpCornerTL(CvPoint2D32f pnt0,CvPoint2D32f pnt1,double m){

	if(m>=0){
		if(pnt0.y <pnt1.y)
			return -1;
		else
			return 1;
	}
	else{
		if(pnt0.x <pnt1.x)
			return -1;
		else
			return 1;
	}
}


int cmpCornerTR(CvPoint2D32f pnt0,CvPoint2D32f pnt1,double m){

	if(m<0){
		if(pnt0.y <pnt1.y)
			return -1;
		else
			return 1;
	}
	else{
		if(pnt0.x >pnt1.x)
			return -1;
		else
			return 1;
	
	}

}

int cmpCornerBL(CvPoint2D32f pnt0,CvPoint2D32f pnt1,double m){

	if(m<0){
		if(pnt0.y >pnt1.y)
			return -1;
		else
			return 1;
	}
	else{
		if(pnt0.x <pnt1.x)
			return -1;
		else
			return 1;
	
	}

}

int cmpCornerBR(CvPoint2D32f pnt0,CvPoint2D32f pnt1,double m){

	if(m>0){
		if(pnt0.y >pnt1.y)
			return -1;
		else
			return 1;
	}
	else{
		if(pnt0.x > pnt1.x)
			return -1;
		else
			return 1;
	
	}

}

int isGrigioScuro(CvScalar px){
	double val=px.val[0];
	if(val>210)
		return 0;

	if(px.val[1] < val-20.0 || px.val[1] > val+20)
		return 0;

	if(px.val[2] < val -20 || px.val[2] > val+20)
		return 0;

	return 1;
}
void cleanPlate(IplImage * img){

	CvPoint plateCenter= findPlate(img);
	
	CvScalar plateColor;
	IplImage * imgBin;

	CvPoint2D32f srcPoint[4];

	imgBin= cvCreateImage(cvGetSize(img),CV_8S,1);
	imgBin=plate_growing(img, &plateColor, plateCenter);

	explain(imgBin,"Utilizzo region growing per isolare la targa");
	
	//explain
	IplImage * explImg= cvCreateImage(cvGetSize(img),img->depth,img->nChannels);
	cvCopyImage(img,explImg);
	//end

	int i,j;

	double  q[3];
	double  m[3];
	
	for(i=0; i<3; i++){
			
		//USO TRASFORMATA HOUGH

		IplImage * hough,* dest, * contorno;
		int diag,x,y;
		CvScalar px;

		contorno = cvCreateImage(cvGetSize(imgBin),imgBin->depth,imgBin->nChannels);
		cvSet(contorno,cvScalarAll(0),0);
		trovaContorno(imgBin,contorno,i);

		explain(contorno,"Evidenzio i contorni della targa");

		diag=ceil(sqrt(pow((double)contorno->width,2)+pow((double)contorno->height,2)));

		hough= cvCreateImage(cvSize(180,2*diag),IPL_DEPTH_64F,3);
		dest= cvCreateImage(cvSize(contorno->width,contorno->height),IPL_DEPTH_8U,1);
		
		cvSet(hough,cvScalar(0,0,0,0),0);

		cvSet(dest,cvScalar(0,0,0,0),0);

		for(x=0; x<contorno->width; x++){
			for(y=0; y<contorno->height; y++){
				px=cvGet2D(contorno,y,x);
				if(px.val[0]>0)
					RH(x,y,hough);
			}
		}

		

		int theta,ro;
		CvPoint maxP=cvPoint(0,0);
		double maxV=0.0;
		


		for(theta=0; theta<180; theta++){
					
			for(ro=0; ro<hough->height; ro++){
				px=cvGet2D(hough,ro,theta);
			
				if(px.val[0]>=maxV){
					maxV=px.val[0];
					maxP=cvPoint(theta,ro);
				}
			}
		}
		
		
		
		HR(maxP.x,maxP.y-diag,contorno,&m[i],&q[i]);	
		


		//explain
		drawRect(explImg,m[i],q[i]);
		explain(explImg,"Uso la trasformata hough per trovare le rette passanti per i bordi");
		//end	

		
	//FINE HOUGH
	}
	
	//bottom left
	srcPoint[1].x=((q[0]-q[1])/(m[1]-m[0]));;
	srcPoint[1].y=m[0]*srcPoint[1].x+q[0];

	//bottom right
	srcPoint[3].x=((q[0]-q[2])/(m[2]-m[0]));;
	srcPoint[3].y=m[0]*srcPoint[3].x+q[0];
	
	
	
	
	//explain
	cvDrawCircle(explImg,cvPoint(srcPoint[1].x,srcPoint[1].y),2,cvScalar(0,255,0,0),1,8,0);
	cvDrawCircle(explImg,cvPoint(srcPoint[3].x,srcPoint[3].y),2,cvScalar(0,255,0,0),1,8,0);
	explain(explImg,"Calcolo le intersezioni tra le rette per trovare gli angoli inferiori\n(Solo quelli inferiroi perche' non influenzati dall'ombra)");

	int x0,x1;
	j=-10;

	do{	
		x0=(srcPoint[1].y+j-q[1]-10)/m[1];
		x1=(srcPoint[3].y+j-q[2]-10)/m[2];
		j--;
//	printf("%f\n",quantiPxNeri(img,m[0],q[0]+j,x0,x1));
//	drawRect(explImg,m[0],q[0]+j);
//	cvShowImage("a",explImg);cvWaitKey(0);
	
	}
	while (quantiPxNeri(img,m[0],q[0]+j,x0,x1)>30);

	/**/

	//top left
	
	srcPoint[0].x=((q[0]-q[1]+j)/(m[1]-m[0]));
	srcPoint[0].y=m[0]*srcPoint[0].x+q[0]+j;
	
	//top right
	srcPoint[2].x=((q[0]-q[2]+j)/(m[2]-m[0]));
	srcPoint[2].y=m[0]*srcPoint[2].x+q[0]+j;


	//explain
	drawRect(explImg,m[0],q[0]+j);
	explain(explImg,"Scansiono la targa dal basso verso l'alto con una retta parallela al bordo inferiore\n Mi fermo quando la retta non contiene pixel neri,\ncioe' quando ho raggiunto il bordo superiore\n ");
	
	cvDrawCircle(explImg,cvPoint(srcPoint[0].x,srcPoint[0].y),2,cvScalar(0,255,0,0),1,8,0);
	cvDrawCircle(explImg,cvPoint(srcPoint[2].x,srcPoint[2].y),2,cvScalar(0,255,0,0),1,8,0);
	explain(explImg,"Calcolo l'intersezione tra le rette per trovare gli angoli superiori");
	//end


	double plateWidth=  sqrtf(pow(srcPoint[0].x - srcPoint[2].x,2) +  pow(srcPoint[0].y -srcPoint[2].y,2));
	
	IplImage * plateClean= cvCreateImage(cvSize(plateWidth+2,srcPoint[1].y-srcPoint[0].y+2),img->depth,img->nChannels);
	CvPoint2D32f dstPoint[4];
	CvMat * map= cvCreateMat(3,3,CV_32F);

	dstPoint[0]= cvPoint2D32f(0,0);
	dstPoint[1]= cvPoint2D32f(0,srcPoint[1].y-srcPoint[0].y);
	dstPoint[2]= cvPoint2D32f(plateWidth,0);
	dstPoint[3]= cvPoint2D32f(plateWidth,srcPoint[1].y-srcPoint[0].y);


	cvGetPerspectiveTransform(srcPoint,dstPoint , map);
	cvWarpPerspective(img,plateClean,map,CV_INTER_LINEAR+CV_WARP_FILL_OUTLIERS,plateColor);


	explain(plateClean,"Utilizzo i quattro angoli per raddrizzare la targa");
	
	IplImage * plateCleanBin = cvCreateImage(cvGetSize(plateClean),8,1);
	IplImage * plateCleanTh = cvCreateImage(cvGetSize(plateClean),8,1);
	IplImage * tmp = cvCreateImage(cvGetSize(plateClean),8,1);
	IplImage * edge = cvCreateImage(cvGetSize(plateClean),8,1);

	cvCvtColor(plateClean,tmp,CV_RGB2GRAY);
	cvCanny(tmp,edge,20,200,3);

	cvSet(plateCleanBin,cvScalarAll(0),0);
	cvSet(tmp,cvScalarAll(0),0);
	
	threshold3Ch(plateClean,plateCleanTh,cvScalarAll(120),cvScalarAll(255),cvScalarAll(0));//RENDI DINAMICA
	
	explain(plateCleanTh,"Applico una soglia sui 3 canali, per evidenziare le lettere");


	int x,y;
	y= plateCleanTh->height/2;
	
	for(x=0; x< plateCleanTh->width; x++){

		if(cvGet2D(plateCleanBin,y,x).val[0]==0 && cvGet2D(plateCleanTh,y,x).val[0]>0){

			if(character_growing(plateClean,tmp,cvPoint(x,y))>100){
				character_growing(plateClean,plateCleanBin,cvPoint(x,y));
				//cvDrawCircle(plateCleanBin,cvPoint(x,y),2,cvScalar(123,0,0,0),1,8,0);
				explain(plateCleanBin,"Utilizzo region growing per isolare i singoli caratteri");			
			}
		}
		
	}

	for(x=0; x< plateCleanTh->width; x++){
		for(y=0; y< plateCleanTh->height; y++){
			if(cvGet2D(plateCleanTh,y,x).val[0]>0){
				cvSet2D(plateCleanBin,y,x,cvScalarAll(255));
			}
		}
	}

	explain(plateCleanBin,"Unisco i risultati di region growing a quelli della soglia per perfezionare l'immagine");

	int found=0;

	for(y=plateCleanTh->height/2; y< plateCleanTh->height; y++){
		if(found || quantiPxNeri(plateCleanBin,0,y,0,plateClean->width)> plateClean->width*3-3){
			found=1;
			for(x=0; x< plateCleanTh->width; x++)	
				cvSet2D(plateCleanBin,y,x,cvScalarAll(0));
		}
	}


	found=0;
	for(y=plateCleanTh->height/2; y>=0; y--){
		if(found || quantiPxNeri(plateCleanBin,0,y,0,plateClean->width)> plateClean->width*3-3){
			found=1;
			for(x=0; x< plateCleanTh->width; x++)	
				cvSet2D(plateCleanBin,y,x,cvScalarAll(0));
			
		}
	}
	

	explain(plateCleanBin,"Elimino eventuali imperfezioni nella parte superiore e d inferiroe della targa\nOra l'immagine e' pronta per Tesseract");
	
}

int main(){
	IplImage * img, * plate;

	char* immagineIntermedia = "saved.tif";
	char * pathImg="test3/immagine (28).jpg";

	img= cvLoadImage(pathImg,1);
	explain(img,pathImg);


	assert(img->depth== IPL_DEPTH_8U);
	cleanPlate(img);
	
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

	cvSet2D(map,px->x,px->y,cvScalar(255,0,0,0));
	for(i=px->x-1; i<=px->x+1; i++){
		for(j=px->y-1; j<=px->y+1; j++){
			if(i>=0 && j>=0 && i< img->height && j< img->width &&
					fabs(cvGet2D(img,i,j).val[0]-media.val[0])<th && 
						fabs(cvGet2D(img,i,j).val[1]-media.val[1])<th &&
							fabs(cvGet2D(img,i,j).val[2]-media.val[2])<th &&
								cvGet2D(ispezionati,i,j).val[0]<0){
				push(daIspezionare,i,j);
				cvSet2D(ispezionati,i,j,cvScalar(1,0,0,0));
			}
			
		}
	}
	
	
}

int character_growing(IplImage * src, IplImage * map,CvPoint seed){

	int i,j;
	Stack * daIspezionare;
	StackElem * px;
	double th=30.0;
	CvScalar media;
	int n;
	

	CvMat * ispezionati = cvCreateMat(src->height,src->width,CV_8S);
	
	daIspezionare=createStack();
	n=0;
	
	
	push(daIspezionare,seed.y,seed.x);
	media=cvScalarAll(0);
	n=0;
	cvSet(ispezionati,cvScalar(-1,0,0,0),0);

	while(px=pop(daIspezionare)){
		media.val[0]= ((media.val[0]*n)+ cvGet2D(src,px->x,px->y).val[0])/(double)(n+1);
		media.val[1]= ((media.val[1]*n)+ cvGet2D(src,px->x,px->y).val[1])/(double)(n+1);
		media.val[2]= ((media.val[2]*n)+ cvGet2D(src,px->x,px->y).val[2])/(double)(n+1);
		n++;
		ispeziona(src,px,daIspezionare,media,th,map,ispezionati);
		
	}
	
	morphoProcess(map);

	return	 n;
}

IplImage* plate_growing(IplImage * src, CvScalar * regionColor, CvPoint plateCenter){

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
	
	
	seed= cvPoint(plateCenter.x-10,plateCenter.y);
	
	daIspezionare=createStack();
	n=0;
	
	while(n<800){
		seed.x++;
		push(daIspezionare,seed.y,seed.x);

		media=cvScalarAll(0);
		n=0;
		cvSet(ispezionati,cvScalar(-1,0,0,0),0);
					
		while(px=pop(daIspezionare)){
			media.val[0]= ((media.val[0]*n)+ cvGet2D(src,px->x,px->y).val[0])/(double)(n+1);
			media.val[1]= ((media.val[1]*n)+ cvGet2D(src,px->x,px->y).val[1])/(double)(n+1);
			media.val[2]= ((media.val[2]*n)+ cvGet2D(src,px->x,px->y).val[2])/(double)(n+1);
			n++;
			ispeziona(src,px,daIspezionare,media,th,map,ispezionati);
		}

	}
					
	*regionColor= media;

	morphoProcess(map);

	return	 map;

}


/*dallo spazio di hough alla retta, disegna in img la retta x, y dello psazio di hogh*/
	//l'offset a r deve essere gia' stato applicato
	//theta in gradi da 0 a 179
void HR(int theta, int r , IplImage * img ,double * m1,double * q1){
	double x1,y1,x2,y2;

	
	double thetaRad;

	thetaRad=theta*PI/180;
	
	x1=0;
	y1= ((double)r)/sin(thetaRad);	

	x2=img->width;
	y2= ((double)r-(double)x2*cos(thetaRad))/sin(thetaRad);
	
	*m1= (y2-y1)/(x2-x1);
	*q1=y2- *m1*x2;
}



/*
dato un punto x y disegna la sinusoide accumulando il valore dei pixel nello spazio di Hough
hough(r,theta) [righe ,colonne]
*/
void RH(int x, int y, IplImage * hough){
	int r, theta;
	double thetaRad;
	CvScalar px;
	int offset=cvFloor(hough->height/2);
	

	for(theta=0; theta<180; theta++){
		thetaRad=theta*PI/180;
		r= cvRound(x*cos(thetaRad)+y*sin(thetaRad));
		r=r+offset;

		
		if(r>=0&& r< hough->height){
			px=cvGet2D(hough,r,theta);
			px.val[0]+=1.0;
			cvSet2D(hough,r,theta,px);
		}
	}
	
}

void trovaContorno (IplImage * src, IplImage * dst , int position){

	int x, y, found;
	CvScalar px, pxL, pxR;


	if(position==0){//bordo inferiore
		
		for(x=0; x<src->width;x++){
			found=0;
			for(y=src->height-1; y>=0; y--){
				px=cvGet2D(src,y,x);
				if(!found && px.val[0]>0){
					cvSet2D(dst,y,x,cvScalarAll(255));
					found=1;
				}
				else
					cvSet2D(dst,y,x,cvScalarAll(0));
			}
		
		}


	}

	else if(position ==1){ //lato sinistro
		for(y=0; y< src->height;y++){
			found=0;
			for(x=0; x<src->width;x++){
				px=cvGet2D(src,y,x);
				if(!found && px.val[0]>0){
					cvSet2D(dst,y,x,cvScalarAll(255));
					found=1;
				}
				else
					cvSet2D(dst,y,x,cvScalarAll(0));
			}
		}
	
	}
	else {//lato destro
		for(y=0; y< src->height;y++){
			found=0;
			for(x=src->width-1;x>=0;x--){
				px=cvGet2D(src,y,x);
				if(!found && px.val[0]>0){
					cvSet2D(dst,y,x,cvScalarAll(255));
					found=1;
				}
				else
					cvSet2D(dst,y,x,cvScalarAll(0));
				}
		}
	
	}
}

void explain(IplImage * img, char * msg){
	printf("%s\n",msg);
	cvShowImage("Immagine Console", img);
	cvWaitKey(0);
}