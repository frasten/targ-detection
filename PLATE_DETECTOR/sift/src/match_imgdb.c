/*
  Detects SIFT features in two images and finds matches between them.

  Copyright (C) 2006-2010  Rob Hess <hess@eecs.oregonstate.edu>

  @version 1.1.2-20100521
*/

#include "sift.h"
#include "imgfeatures.h"
#include "kdtree.h"
#include "utils.h"
#include "xform.h"

#include <opencv/cv.h>
#include <opencv/cxcore.h>
#include <opencv/highgui.h>

#include <stdio.h>


/* the maximum number of keypoint NN candidates to check during BBF search */
#define KDTREE_BBF_MAX_NN_CHKS 200

/* threshold on squared ratio of distances between NN and 2nd NN */
#define NN_SQ_DIST_RATIO_THR 0.49

int feat_type = FEATURE_LOWE;


int main( int argc, char** argv ) {
  IplImage* img2, * stacked;
  struct feature* feat1, * feat2, * feat;
  struct feature** nbrs;
  struct kd_node* kd_root;
  CvPoint pt1, pt2;
  double d0, d1;
  int n1, n2, k, i, m = 0;

  if( argc != 3 )
      fatal_error( "usage: %s <keydescr> <img>", argv[0] );

  n1 = import_features( argv[1], feat_type, &feat1 );

  if( n1 < 0 )
      fatal_error( "unable to load key descriptors from %s", argv[1] );
  img2 = cvLoadImage( argv[2], 1 );
  if( ! img2 )
      fatal_error( "unable to load image from %s", argv[2] );

  fprintf( stderr, "Finding features in %s...\n", argv[2] );
  n2 = sift_features( img2, &feat2 );
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
//            pt1 = cvPoint( cvRound( feat->x ), cvRound( feat->y ) );
//            pt2 = cvPoint( cvRound( nbrs[0]->x ), cvRound( nbrs[0]->y ) );
//            pt2.y += img1->height;
//            cvLine( stacked, pt1, pt2, CV_RGB(255,0,255), 1, 8, 0 );
              m++;
              feat1[i].fwd_match = nbrs[0];
          }
      }
      free( nbrs );
    }

    fprintf( stderr, "Found %d total matches\n", m );

//  cvSaveImage( "out.jpg",stacked, 0);

  //display_big_img( stacked, "Matches" );

  //cvWaitKey( 0 );


//  cvReleaseImage( &stacked );
    cvReleaseImage( &img2 );
    kdtree_release( kd_root );
    free( feat1 );
    free( feat2 );
    return 0;
}
