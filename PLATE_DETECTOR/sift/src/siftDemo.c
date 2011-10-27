/*
A Demonstration of the SIFT algorithm in action

Version: 1.1.1-20101021
*/

#include "sift.h"
#include "imgfeatures.h"
#include "utils.h"
#include "kdtree.h"
#include "xform.h"
#include <cv.h>
#include <cxcore.h>
#include <highgui.h>
#include <stdio.h>

static IplImage* find_matches( struct feature*, int, struct kd_node*, IplImage*, IplImage* );
static void draw_xform( IplImage*, IplImage*, CvMat* );

#define KDTREE_BBF_MAX_NN_CHKS 50
#define NN_SQ_DIST_RATIO_THR 0.64

char* target_img_file = "../smart1.jpg";
char* overlay_img_file = "../smart2.jpg";

int main( int argc, char** argv )
{
	CvCapture* camera;
	IplImage* target, * overlay, * frame, * matches;
	CvMat* H = NULL;
	struct feature* target_features, * features;
	struct kd_node* target_kd_root;
	int i, k, nt, n = 0, show_xform = 0, show_overlay = 0;

	fprintf( stderr, "Loading images..." );
	fflush( stderr );
	target = cvLoadImage( target_img_file, CV_LOAD_IMAGE_COLOR );
	overlay= cvLoadImage( overlay_img_file, CV_LOAD_IMAGE_COLOR );
	nt = _sift_features( target, &target_features, SIFT_INTVLS, SIFT_SIGMA, SIFT_CONTR_THR, SIFT_CURV_THR, 0, SIFT_DESCR_WIDTH, SIFT_DESCR_HIST_BINS );
	target_kd_root = kdtree_build( target_features, nt );
	fprintf( stderr, " done\n" );
	camera = cvCaptureFromCAM( -1 );
	if( ! camera )
	{
		fprintf( stderr, "unable to open camera stream\n" );
		exit( 1 );
	}
	cvNamedWindow( "Camera", 1 );
	cvNamedWindow( "Matches", 1 );
	while( frame = cvQueryFrame( camera ) )
	{
		n = _sift_features( frame, &features, SIFT_INTVLS, SIFT_SIGMA, SIFT_CONTR_THR, SIFT_CURV_THR, 0, SIFT_DESCR_WIDTH, SIFT_DESCR_HIST_BINS );
		for( i = 0; i < nt; i++ )
			target_features[i].fwd_match = NULL;
		matches = find_matches( features, n, target_kd_root, frame, target );
		if( show_xform  ||  show_overlay )
		{
			H = ransac_xform( target_features, nt, FEATURE_FWD_MATCH, lsq_homog, 4, 0.01, homog_xfer_err, 3.0, NULL, NULL );
		}
		if( show_xform  &&  H )
			draw_xform( frame, target, H );
		if( show_overlay  &&  H )
			cvWarpPerspective( overlay, frame, H, CV_INTER_CUBIC, cvScalarAll( 0 ) );

		cvShowImage( "Matches", matches );
		cvShowImage( "Camera", frame );
		k = cvWaitKey( 10 );
		free( features );
		cvReleaseImage( &matches );
		if( H )
			cvReleaseMat( &H );
		if( k == 'q' )
			break;
		else if( k == ' ' )
		{
			show_xform = ! show_xform;
			show_overlay = 0;
		}
		else if( k == 'a'  ||  k == 'A' )
		{
			show_overlay = ! show_overlay;
			show_xform = 0;
		}
	}
	cvReleaseCapture( &camera );
}



/*
	Find and display matches between images

	@param features SIFT features from the input image
	@param n number of features
	@param target_kd_root root node of the kd-tree containing target features
	@param img input image
	@param target target image

	@return Returns an image depicting matches between img and target
*/
static IplImage* find_matches( struct feature* features, int n, struct kd_node* target_kd_root, IplImage* img, IplImage* target )
{
	IplImage* matches;
	struct feature** nbrs, * feat;
	CvPoint pt1, pt2;
	double d0, d1;
	int i, k;

	matches = stack_imgs( target, img );
	for( i = 0; i < n; i++ )
	{
		feat = &features[i];
		k = kdtree_bbf_knn( target_kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS );
		if( k == 2 )
		{
			d0 = descr_dist_sq( feat, nbrs[0] );
			d1 = descr_dist_sq( feat, nbrs[1] );
			if( d0 < d1 * NN_SQ_DIST_RATIO_THR )
			{
				pt1 = cvPoint( cvRound( feat->x ), cvRound( feat->y ) );
				pt2 = cvPoint( cvRound( nbrs[0]->x ), cvRound( nbrs[0]->y ) );
				pt1.y += target->height;
				cvLine( matches, pt1, pt2, CV_RGB(255,0,255), 1, 8, 0 );
				nbrs[0]->fwd_match = feat;
			}
		}
		free( nbrs );
	}
	return matches;
}



/*
	Depict a transform on an image by drawing the transformed corners of the target

	@param img image on which to draw transform
	@param target target image
	@param H transform representing the location of target in img
*/
static void draw_xform( IplImage* img, IplImage* target, CvMat* H )
{
	CvPoint2D64f xc[4], c[4] = { { 0, 0 }, { target->width-1, 0 }, { target->width-1, target->height-1 }, { 0, target->height-1 } };
	int i;

	for( i = 0; i < 4; i++ )
		xc[i] = persp_xform_pt( c[i], H );
	for( i = 0; i < 4; i++ )
		cvLine( img, cvPoint( xc[i].x, xc[i].y ), cvPoint( xc[(i+1)%4].x, xc[(i+1)%4].y ), CV_RGB( 0, 255, 0 ), 10, CV_AA, 0 );
}
