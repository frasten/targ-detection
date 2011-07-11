#!/usr/bin/php
<?php

/* creadb.php
 * Questo script permette di estrarre le features SIFT da un database di
 * immagini, e salvarne i keypoints in files .sift.
 * Questo per avere una migliore efficienza in fase di riconoscimento:
 * e' molto piu' rapido caricare i keypoints gia' pronti da un file, che
 * estrarli ogni volta da ogni immagine del database.
 *
 * */


// N.B.: tutti questi path devono avere una slash finale.
define( 'BIN_DIR', '../sift/bin/' );
define( 'DB_DIR', 'db/' );
define( 'DB_IMG_DIR', 'db_img_posteriore/' );



if ( ! is_file( BIN_DIR . 'siftfeat' ) ) {
	echo "Errore: mancano i file compilati. Assicurarsi di aver dato\n";
	echo "`make` nella directory delle librerie SIFT.\n";
	exit( 1 );
}

if ( ! is_dir( DB_DIR ) ) {
	mkdir( DB_DIR );
}

/* Scansione della directory delle immagini. */
if ( $handle = opendir( DB_IMG_DIR ) ) {
	while ( false !== ( $model_dir = readdir( $handle ) ) ) {
		if ( $model_dir == '.' || $model_dir == '..' ) continue; // escludo directory inutili
		if ( ! is_dir( DB_IMG_DIR . $model_dir ) ) continue; // voglio solo directory

		/* Per ogni directory in DB_IMG_DIR, viene associato un modello di
		 * automobile. */

		// Se la directory corrispondente nel database SIFT esiste gia', la
		// svuoto, poi la creo vuota.
		if ( is_dir( DB_DIR . $model_dir ) ) {
			$objects = scandir( DB_DIR . $model_dir );
			foreach ( $objects as $object ) {
				if ( is_dir( DB_DIR . "$model_dir/$object" ) ) continue;
				unlink( DB_DIR . "$model_dir/$object" );
			}
			reset( $objects );
			rmdir( DB_DIR . $model_dir );
		}
		mkdir( DB_DIR . $model_dir );

		// Ora, per ogni modello di macchina, creiamo il db per ognuna.
		if ( $handle2 = opendir( DB_IMG_DIR . $model_dir ) ) {
			$filedata = array();
			while ( false !== ( $img = readdir( $handle2 ) ) ) {
				/* Ogni immagine nella directory relativa al modello entrera' a
				 * far parte dei campioni con cui fare il matching. */

				if ( is_dir( DB_IMG_DIR . "$model_dir/$img" ) ) continue;
				if ( ! preg_match( '/^(.+)\.jpg$/', $img, $match ) ) continue;

				/* ESTRAZIONE DELLE FEATURES */
				printf( "Analizzo l'immagine %s...", $img );

				// Esempio: 01-punto.jpg ==> 01-punto.sift
				$siftfile = DB_DIR . $model_dir . "/$match[1].sift";
				$imgfile = DB_IMG_DIR . "$model_dir/$img";

				$cmd = BIN_DIR . "/siftfeat -x -o $siftfile $imgfile 2>&1";
				$output = `$cmd`;

				echo "fatto.\n";

				// Leggo i dati restituiti dall'estrattore:
				preg_match( '/Found (\d+) /m', $output, $match );
				$filedata[] = "$img\t$match[1]";
			}
			closedir( $handle2 );

			// Salvo un indice per i vari files, salvando il numero di keypoints
			// per ognuno.
			file_put_contents( DB_DIR . "$model_dir/index.txt", implode( "\n", $filedata ) );
		}

	}
	closedir( $handle );
}



?>
