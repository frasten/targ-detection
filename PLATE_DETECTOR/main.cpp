/*
 * Per la compilazione sotto Linux:
 * sudo apt-get install libcv-dev libhighgui-dev libcvaux-dev tesseract-ocr-dev
 * */

#include <math.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cv.h"
#include "highgui.h"

#include "imgs.h"

#ifdef _MSC_VER
	#include "stdafx.h"
	#include "tessdll.h"
#else // Per linux:
	#include "ocrclass.h"
#endif

#include "unichar.h"
#include "baseapi.h"



#define BOX_PLATE_WIDTH 180
#define BOX_PLATE_HEIGTH 48
#define PI 3.141592653589793

#define VERBOSE 0


/*
PERFEZIONAMENTI
Deallocare la memoria per le stringhe outImg e nomeFile
Gestire la deallocazione della memoria in caso di errore nell'esecuzione del sw
*/

struct StackElem {
	int x;
	int y;
	struct StackElem * next;
} typedef StackElem;


struct Stack {
	StackElem * top;
} typedef Stack;


//IplImage * plate_growing(IplImage * src, CvScalar * regionColor, CvPoint plateCenter);
int character_growing(IplImage * src, IplImage * map,CvPoint seed);
void threshold(IplImage * img, CvPoint * p);
void trovaContorno(IplImage * src, IplImage * dst , int position);
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
char * impostaNomeOutput(char * input);

#ifdef _MSC_VER
/**Effettua l'OCR dell'immagine passata come parametro*/
ETEXT_DESC* ocr(char * imgPath);
#endif

/** Scrive la targa sull'output desiderato. Se output=NULL si usa lo stdout*/
int scriviTargaSuFile(ETEXT_DESC* targa, char *nomeFile,char *preTarga, char *postTarga);
/** Scrive la targa sull'output desiderato. Se output=NULL si usa lo stdout*/
int scriviTargaSuFile(char* targa, char *nomeFile,char *preTarga, char *postTarga);
void morphoProcess(IplImage * img);
void cleanPlate(IplImage * img, char *imgName);
IplImage* plate_growing(IplImage * src, CvScalar * regionColor, CvPoint plateCenter);
void ispeziona(IplImage * img, StackElem * px, Stack * daIspezionare,CvScalar media,double th,IplImage * map,CvMat * ispezionati);
int isSeed(IplImage * img,IplImage * map, int x,int y,double th);
Stack * createStack();
void push(Stack * stack, int x, int y);
StackElem * pop(Stack * stack);
int isGrigioScuro(CvScalar px);
double quantiPxNeri(IplImage * img, double m, double q, int x0, int x1);
void drawRect(IplImage * img, double m, double q, int verticale, double x);
CvPoint findPlate(IplImage * img);
void closing(IplImage * img);
IplImage * matchFilter(IplImage * img);
void gaussianFilter(IplImage * src, IplImage * dst);
void verticalEdgeDetection(IplImage * src, IplImage * dst);

/**Imposta la threshold per un'immagine a 3 canali (a colori).
Se per ciascun canale (RGB) il pixel ha valore che supera la soglia, allora apparterrà al  foreground, altrimenti apparterrà al background.
th: vettore che contiene per ogni canale la threshold
fgColor: colore del foreground
bgColor: colore del background
*/
void threshold3Ch(IplImage * src ,IplImage * dst, CvScalar th, CvScalar fgColor, CvScalar bgColor);

void applyThreshold(IplImage * img , double th);

/**Imposta dinamicamente la threshold per un'immagine a 3 canali (a colori).
Se per ciascun canale (RGB) il pixel ha valore che supera la soglia, allora apparterrà al  foreground, altrimenti apparterrà al background.
th: vettore che contiene per ogni canale la threshold
fgColor: colore del foreground
bgColor: colore del background
*/
void dynamicThreshold3Ch(IplImage * src ,IplImage * dst);
char * impostaNomeFileOutput(char * input);



int main(int argc, char *argv[]){
	IplImage * img;

	char *immagineIntermedia = "saved.tif";
	char *preTarga="";
	char *postTarga="";

	if (argc > 1)
		for (int j=1;j<argc;j++){
			char *percorsoImmagine=argv[j];
			ETEXT_DESC* targa = NULL;
			char *nomeFile=impostaNomeFileOutput(argv[j]);
			char * outImg;
			if(!(img= cvLoadImage(percorsoImmagine,1))){
				printf("Immagine non trovata\n");			
				scriviTargaSuFile("no input file",nomeFile,"",postTarga);
			}
			else {
				outImg= impostaNomeOutput(argv[j]);
				assert(img->depth== IPL_DEPTH_8U);
				explain(img,percorsoImmagine);
				try {
					cleanPlate(img,outImg);
#ifdef _MSC_VER
					targa=ocr(outImg);
					explain(img,"");
					cvReleaseImage(&img);
					scriviTargaSuFile(targa,nomeFile,"",postTarga);
#endif
				}
				catch (...){
					printf("UNKNOWN ERROR\n");
					getchar();
				}
			}
		}
	else {
		printf("ERROR: not enough parameters\n");
		getchar();
		exit(1);
	}	
	exit(0);		
}

char * impostaNomeFileOutput(char * input){
	// abc.jpg => abc.txt
	// Quindi la dimensione e' la stessa (+1 carattere del '\0')
	char *output = (char *)malloc(sizeof(char)*(int)(strlen(input) + 1));
	strncpy(output, input, strlen(input)-4);
	output[strlen(input)-4]='\0';
	strcat(output,".txt");
	return output;
}

char * impostaNomeOutput(char * input){
	// abc.jpg => abc_targa.tif
	//    7            13
	// Quindi la dimensione e' 6 caratteri in piu' (+1 carattere del '\0')
	char *output = (char *)malloc(sizeof(char)*(int)(strlen(input)+6+1));
	strncpy(output, input, strlen(input)-4);
	output[strlen(input)-4]='\0';
	strcat(output,"_targa.tif");
	return output;
}


#ifdef _MSC_VER
ETEXT_DESC* ocr(char * imgPath){
	IMAGE image;
	//Definizione della libreria API
	if (image.read_header(imgPath) < 0) {
		printf("Can't open %s\n", imgPath);
		return NULL;
	}
	if (image.read(image.get_ysize()) < 0) {
		printf("Can't read %s\n", imgPath);
		return NULL;
	}
	TessDllBeginPageUprightBPP(image.get_xsize(),image.get_ysize(),image.get_buffer(),"eng",image.get_bpp());
	ETEXT_DESC* output = TessDllRecognize_all_Words();
	image.destroy();
    return output;
}
#endif


int scriviTargaSuFile(ETEXT_DESC* targa, char *nomeFile,char *preTarga, char *postTarga){
	printf("\nTarga: ");

	FILE *file = fopen(nomeFile,"w");
	
		if (targa !=NULL){
		for (int i = 0; i < targa->count; i++)
			if (((targa->text[i].char_code)<='Z' && (targa->text[i].char_code)>='A') || ((targa->text[i].char_code)<='9' && (targa->text[i].char_code)>='0') ){
				fprintf(file,"%c ",targa->text[i].char_code);
				printf("%c",targa->text[i].char_code);
			}
		}
	
		fclose(file);

	printf("\n\n");
		return 0;
}

int scriviTargaSuFile(char* targa, char *nomeFile,char *preTarga, char *postTarga){
	FILE *file = fopen(nomeFile,"w");
	if (targa !=NULL){
		fprintf(file,"%s ",targa);
	}
	fclose(file);
	return 0;
}


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
	if (! tmpl8 ) {
		printf("Error: missing template file!\n");
		exit(-1);
	}
	assert(tmpl8->depth== IPL_DEPTH_8U);
	tmpl32F= cvCreateImage(cvGetSize(tmpl8),IPL_DEPTH_32F,1);
	cvConvertScale(tmpl8,tmpl32F,1./255.,0);

	matchDstWidth= img->width - tmpl32F->width + 1;
	matchDstHeigth = img->height - tmpl32F->height + 1;
	
	match = cvCreateImage( cvSize( matchDstWidth,matchDstHeigth ), IPL_DEPTH_32F, 1 );

	cvMatchTemplate( img, tmpl32F, match, CV_TM_CCORR );

	applyThreshold(match,150.0);
		

	cvConvertScale(match,match,1./500.,0);

	cvReleaseImage(&tmpl8);
	cvReleaseImage(&tmpl32F);

	return match;
}


void closing(IplImage * img){
	IplConvKernel * structuringElem;
	structuringElem= cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_RECT,0);
	cvErode(img,img,structuringElem,1);
	cvDilate(img,img,structuringElem,1);
	cvReleaseStructuringElement(&structuringElem);
}


void morphoProcess(IplImage * img){
	IplConvKernel * structuringElem;
	structuringElem= cvCreateStructuringElementEx(3,3,1,1,CV_SHAPE_RECT,0);
	cvDilate(img,img,structuringElem,1);
	cvErode(img,img,structuringElem,1);
	cvReleaseStructuringElement(&structuringElem);
}


CvPoint findPlate(IplImage * img){
	IplImage *gauss, *match, *edge, *imgGray;
	
	/**/
	double minval, maxval;
	CvPoint maxloc, minloc;
	
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

	cvReleaseImage(&explainImg);
	cvReleaseImage(&match);
	cvReleaseImage(&edge);
	cvReleaseImage(&gauss);
	cvReleaseImage(&imgGray);

	return maxloc;
}



void drawRect(IplImage * img, double m, double q, int verticale, double x){


	int x0,y0,x1,y1;

	

	
	if(!verticale){
		x0=5;
		x1= img->width-5;
		y0= m*x0 + q;
		y1= m* x1 +q;

	}
	else{
		x0=x;
		x1=x;
		y0= img->height-5;
		y1= 5;
	}

	

	cvDrawLine(img,cvPoint(x0,y0),cvPoint(x1,y1),cvScalar(0,0,255,0),1,8,0);
}

//vecchia versione

double quantiPxNeri(IplImage * img, double m, double q, int x0, int x1){
	int x,y;
	
	CvScalar media=cvScalarAll(0);
	CvScalar var=cvScalarAll(0);
	CvScalar px;
	
	for(x=x0; x< x1; x++){
		y= cvRound( m*x+q);
		if(y>0 && y< img->height && x>=0 && x< img->width){
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


void cleanPlate(IplImage * img, char *imgName){
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
	double  xRetta[3]; //per gestire le rette verticali
	int verticale[3];
	
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
		



		if(i==0){
			for(theta=45; theta<125; theta++){
						
				for(ro=0; ro<hough->height; ro++){
					px=cvGet2D(hough,ro,theta);
				
					if(px.val[0]>=maxV){
						maxV=px.val[0];
						maxP=cvPoint(theta,ro);
					}
				}
			}
		}
		else{
			for(theta=0; theta<45; theta++){
						
				for(ro=0; ro<hough->height; ro++){
					px=cvGet2D(hough,ro,theta);
				
					if(px.val[0]>=maxV){
						maxV=px.val[0];
						maxP=cvPoint(theta,ro);
					}

				}
			}

			for(theta=125; theta<180; theta++){
						
				for(ro=0; ro<hough->height; ro++){
					px=cvGet2D(hough,ro,theta);
				
					if(px.val[0]>=maxV){
						maxV=px.val[0];
						maxP=cvPoint(theta,ro);
					}
				}
			}	
		}
		
		
		
		HR(maxP.x,maxP.y-diag,contorno,&m[i],&q[i]);	
		xRetta[i]=maxP.y-diag;
		if(maxP.x==0)
			verticale[i]=1;
		else
			verticale[i]=0;

		//explain
		drawRect(explImg,m[i],q[i],verticale[i],xRetta[i]);
		explain(explImg,"Uso la trasformata hough per trovare le rette passanti per i bordi");
		//end	

		cvReleaseImage(&hough);
		cvReleaseImage(&dest);
		cvReleaseImage(&contorno);
	//FINE HOUGH
	}
	
	//bottom left
	if(!verticale[1]){
		srcPoint[1].x=((q[0]-q[1])/(m[1]-m[0]));
		srcPoint[1].y=m[0]*srcPoint[1].x+q[0];
	}
	else{
		srcPoint[1].x=xRetta[1];
		srcPoint[1].y=m[0]*srcPoint[1].x+q[0];
	}


	//bottom right
	if(!verticale[2]){
		srcPoint[3].x=((q[0]-q[2])/(m[2]-m[0]));
		srcPoint[3].y=m[0]*srcPoint[3].x+q[0];
	}
	else{
		srcPoint[3].x=xRetta[2];
		srcPoint[3].y=m[0]*srcPoint[3].x+q[0];
	}
	
	//explain
	cvDrawCircle(explImg,cvPoint(srcPoint[1].x,srcPoint[1].y),2,cvScalar(0,255,0,0),1,8,0);
	cvDrawCircle(explImg,cvPoint(srcPoint[3].x,srcPoint[3].y),2,cvScalar(0,255,0,0),1,8,0);
	explain(explImg,"Calcolo le intersezioni tra le rette per trovare gli angoli inferiori\n(Solo quelli inferiori perche' non influenzati dall'ombra)");

	int x0,x1;
	

	double realWidthPlate=sqrt(pow(srcPoint[1].x-srcPoint[3].x,2)+pow(srcPoint[1].y-srcPoint[3].y,2));//width della targa trovata (non stimata)
	double heightPlateStimato=realWidthPlate/5.0;	
	j=-cvRound(heightPlateStimato/2.0);

/*debug*/
	IplImage * imgT = cvCreateImage(cvGetSize(img),img->depth,img->nChannels);
		cvCopyImage(img,imgT);
	
	/**/

	int z=1;
	int Z_MAX=5;//numero righe per calcolare thNeri
	double pxNeri=0;
	double thNeri=0;

	double thNeriPerc=0.4;

	do{	
		if(!verticale[1])

			x0=(srcPoint[1].y+j-q[1]-10)/m[1];
		else
			x0=xRetta[1]-j;
		
		if(!verticale[2])
			x1=(srcPoint[3].y+j-q[2]-10)/m[2];
		else
			x1=xRetta[2]-j;

		j--;

	
	/*debug*/
		
	/*
	drawRect(imgT,m[0],q[0]+j,0,0);
	cvShowImage("a",imgT);
	cvWaitKey(0);
	*/

	/*end debug*/

		pxNeri=quantiPxNeri(img,m[0],q[0]+j,x0,x1);	
		if(z<Z_MAX){
			thNeri= ((thNeri *(z-1))+pxNeri)/z;
		}
		z++;

	}
	while (z<Z_MAX || (pxNeri >thNeri*thNeriPerc && -j < 2* heightPlateStimato));


	/**/

	//top left
	if(!verticale[1]){
		srcPoint[0].x=((q[0]-q[1]+j)/(m[1]-m[0]));
		srcPoint[0].y=m[0]*srcPoint[0].x+q[0]+j;
	}
	else{
		srcPoint[0].x=xRetta[1];
		srcPoint[0].y=m[0]*srcPoint[0].x+q[0]+j;
	}
	
	//top right
	if(!verticale[2]){
		srcPoint[2].x=((q[0]-q[2]+j)/(m[2]-m[0]));
		srcPoint[2].y=m[0]*srcPoint[2].x+q[0]+j;
	}
	else{
		srcPoint[2].x=xRetta[2];
		srcPoint[2].y=m[0]*srcPoint[2].x+q[0]+j;
	}

	//explain
	drawRect(explImg,m[0],q[0]+j,0,0);
	explain(explImg,"Scansiono la targa dal basso verso l'alto con una retta parallela al bordo inferiore\n Mi fermo quando la retta non contiene pixel neri,\ncioe' quando ho raggiunto il bordo superiore\n ");
	
	cvDrawCircle(explImg,cvPoint(srcPoint[0].x,srcPoint[0].y),2,cvScalar(0,255,0,0),1,8,0);
	cvDrawCircle(explImg,cvPoint(srcPoint[2].x,srcPoint[2].y),2,cvScalar(0,255,0,0),1,8,0);
	explain(explImg,"Calcolo l'intersezione tra le rette per trovare gli angoli superiori");
	//end


	double plateWidth=  sqrtf(pow(srcPoint[0].x - srcPoint[2].x,2) +  pow(srcPoint[0].y -srcPoint[2].y,2));

	/* If the detected plate size is greater than 60% of the image size,
	 * then it's probably wrong. */
	if (plateWidth > 0.6 * img->width ||
		(srcPoint[1].y-srcPoint[0].y) > 0.6 * img->height
	) {
		printf("Errore nel rilevamento della targa (troppo grande).\n");
		exit(-1);
	}

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
	

	dynamicThreshold3Ch(plateClean,plateCleanTh);
	explain(plateCleanTh,"Applico una soglia sui 3 canali, per evidenziare le lettere\nLa soglia e' calcolata usando Otsu");

	

	int x,y;
	y= plateCleanTh->height/2;
	
	for(x=0; x< plateCleanTh->width; x++){

		if(cvGet2D(plateCleanBin,y,x).val[0]==0 && cvGet2D(plateCleanTh,y,x).val[0]>0){
			int numBianchi=character_growing(plateClean,tmp,cvPoint(x,y));
			//printf("%d\n",numBianchi);
			if(numBianchi>100 && numBianchi < 400 ){
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
	

	explain(plateCleanBin,"Elimino eventuali imperfezioni nella parte superiore ed inferiroe della targa\nOra l'immagine e' pronta per Tesseract");
	
	
	cvSaveImage(imgName,plateCleanBin,0);

	//libero la memoria per tutte le immagini a parte img
	cvReleaseImage(&explImg);
	cvReleaseImage(&edge);
	cvReleaseImage(&plateClean);
	cvReleaseImage(&imgBin);
	cvReleaseImage(&plateCleanBin);
	cvReleaseImage(&tmp);
	cvReleaseImage(&plateCleanTh);
	
	//libero la memoria per tutti gli altri dati della libreria OpenCv usati nel metodo
	cvReleaseMat(&map);

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
		free(px);
	}
	
	morphoProcess(map);
	free(daIspezionare);
	cvReleaseMat(&ispezionati);
	return	 n;
}


IplImage* plate_growing(IplImage * src, CvScalar * regionColor, CvPoint plateCenter){

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
			free(px);
		}

	}
					
	*regionColor= media;

	morphoProcess(map);
	free(daIspezionare);
	cvReleaseMat(&ispezionati);
	return	 map;

}


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
	CvScalar px;


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
#if VERBOSE == 1
	printf("%s\n",msg);
	cvShowImage("Immagine Console", img);
	cvWaitKey(0);
#endif
}


void dynamicThreshold3Ch(IplImage * src ,IplImage * dst){
	IplImage *Rimg = cvCreateImage(cvGetSize(src),src->depth,1);
	IplImage *Gimg = cvCreateImage(cvGetSize(src),src->depth,1);
	IplImage *Bimg = cvCreateImage(cvGetSize(src),src->depth,1);
	cvSplit(src,Bimg,Gimg,Rimg,0);

	IplImage *Rthresh = cvCreateImage(cvGetSize(src),8,1);
	IplImage *Gthresh = cvCreateImage(cvGetSize(src),8,1);
	IplImage *Bthresh = cvCreateImage(cvGetSize(src),8,1);

	double max = 255;
	cvThreshold(Bimg,Bthresh,0,max, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);
	cvThreshold(Gimg,Gthresh,0,max, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);
	cvThreshold(Rimg,Rthresh,0,max, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);


	IplImage *flt = cvCreateImage(cvGetSize(src),8,1);
	cvMul(Bthresh,Gthresh,flt);
	cvMul(Rthresh,flt,flt);
	
	cvCopy(flt,dst);

	cvReleaseImage(&Rimg);
	cvReleaseImage(&Gimg);
	cvReleaseImage(&Bimg);
	cvReleaseImage(&Rthresh);
	cvReleaseImage(&Gthresh);
	cvReleaseImage(&Bthresh);
	cvReleaseImage(&flt);

}
