/*
  Detects SIFT features in two images and finds matches between them.

  Copyright (C) 2006-2010  Rob Hess <hess@eecs.oregonstate.edu>

  @version 1.1.2-20100521
*/

//#include "sift.h"
#include "imgfeatures.h"
#include "kdtree.h"
#include "utils.h"
#include "xform.h"

#include <opencv/cv.h>
//#include <opencv/cxcore.h>
#include <opencv/highgui.h>

//#include <stdio.h>


/* the maximum number of keypoint NN candidates to check during BBF search */
#define KDTREE_BBF_MAX_NN_CHKS 200
//#define KDTREE_BBF_MAX_NN_CHKS 200

/* threshold on squared ratio of distances between NN and 2nd NN */
//#define NN_SQ_DIST_RATIO_THR 0.49
#define NN_SQ_DIST_RATIO_THR 0.80
//#define NN_SQ_DIST_RATIO_THR 0.64

int feat_type = FEATURE_LOWE;


int main( int argc, char** argv ) {
  struct feature* feat1, * feat2, * feat;
  struct feature** nbrs;
  struct kd_node* kd_root;
  double d0, d1;
  int n1, n2, k, i, m = 0;

  if( argc != 3 )
      fatal_error( "usage: %s <keydescr1> <keydescr2>", argv[0] );

  n1 = import_features( argv[1], feat_type, &feat1 );
  n2 = import_features( argv[2], feat_type, &feat2 );

  if( n1 < 0 )
      fatal_error( "unable to load key descriptors from %s", argv[1] );
  if( n2 < 0 )
      fatal_error( "unable to load key descriptors from %s", argv[2] );

  printf("%d features presenti nel modello.\n", n1);
  printf("%d features presenti nell'immagine.\n", n2);
  kd_root = kdtree_build( feat2, n2 );
  for( i = 0; i < n1; i++ ) {
      feat = feat1 + i;
      k = kdtree_bbf_knn( kd_root, feat, 2, &nbrs, KDTREE_BBF_MAX_NN_CHKS );
      if( k == 2 ) {
          d0 = descr_dist_sq( feat, nbrs[0] );
          d1 = descr_dist_sq( feat, nbrs[1] );
          if( d0 < d1 * NN_SQ_DIST_RATIO_THR ) {
              m++;
              feat1[i].fwd_match = nbrs[0];
          }
      }
      free( nbrs );
    }

#ifdef RANSAC
  {
    CvMat* H;
    IplImage* xformed;
    struct feature ** inliers;
    int n_inliers;
    H = ransac_xform( feat1, n1, FEATURE_FWD_MATCH, lsq_homog, 4, 0.01, homog_xfer_err, 4.0, &inliers, &n_inliers );
    if( H )
    {
/*
      xformed = cvCreateImage( cvGetSize( img2 ), IPL_DEPTH_8U, 3 );
      cvWarpPerspective( img1, xformed, H, CV_INTER_LINEAR + CV_WARP_FILL_OUTLIERS, cvScalarAll( 0 ) );
      cvNamedWindow( "Xformed", 1 );
      cvShowImage( "Xformed", xformed );
      cvWaitKey( 0 );
      cvReleaseImage( &xformed );
*/
      cvReleaseMat( &H );
    }
    printf("N. inliers: %d\n", n_inliers);
  }
#endif








    fprintf( stderr, "Found %d total matches\n", m );

    kdtree_release( kd_root );
    free( feat1 );
    free( feat2 );
    return 0;
}
