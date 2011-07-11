#!/usr/bin/php
<?php

/* 1-discover.php
 * Script utilizzato per il matching tra un'immagine di test e un
 * database di modelli d'automobile.
 * Vengono estratte le features SIFT dall'immagine da testare, e vengono
 * confrontate con quelle di ogni immagine campione nel database.
 * Infine viene stilata una classifica in base alla percentuale di
 * keypoints che matchano.
 * */

/* TODO: mostrare graficamente alla fine la corrispondenza tra l'immagine
 * di test e l'immagine corrispondente nel database */


$bin_dir = '../sift/bin/'; // con slash finale
$db_dir = 'db';
$sift_testfile = 'test.sift'; // File temporaneo contenente i keypoints


if ( empty( $argv[1] ) ) {
	echo "Uso: $argv[0] <nome_img>\n";
	exit;
}


if ( ! is_file( "$bin_dir/siftfeat" ) ||
	   ! is_file( "$bin_dir/match_db" ) ) {
	echo "Errore: mancano i file compilati. Assicurarsi di aver dato\n";
	echo "`make` nella directory delle librerie SIFT.\n";
	exit( 1 );
}



$testimg = $argv[1];

if ( ! is_file( $testimg ) ) {
	die( "File $testimg non valido.\n" );
}


echo "== Immagine da testare: [$testimg] ==\n";

echo "Ricavo le features dell'immagine da testare...";
/* Ricaviamo i keypoints SIFT per la nostra immagine di test, e li
 * salviamo nel file $sift_testfile. */
$output = `$bin_dir/siftfeat -x -o $sift_testfile $testimg 2>&1`;
preg_match("/Found (\d+) /m", $output, $match);
$test_keypoints = $match[1];
echo " fatto.\n\n";

// Controlliamo ogni modello nel database
$votazioni = array();
if ( $handle = opendir( $db_dir ) ) {
	while ( false !== ( $modello = readdir( $handle ) ) ) {
		if ( $modello == '.' || $modello == '..' ) continue;
		if ( ! is_dir( "$db_dir/$modello" ) ) continue;

		// Leggiamo l'indice dei files, in cui sono salvati la lista dei
		// files modello, e per ognuno il numero di keypoints presenti.
		$indexfilename = "$db_dir/$modello/index.txt";
		if ( ! is_file( $indexfilename ) ) continue;
		$indexdata = file_get_contents( $indexfilename );
		$indexdata = explode( "\n", $indexdata );
		$index = array();
		foreach( $indexdata as $row ) {
			$row = explode( "\t", $row );
			$index[$row[0]] = $row[1];
		}

		// Leggiamo i vari files
		echo "* Modello: $modello\n";
		$tot = 0;
		foreach ( $index as $file => $keypoints ) {

			// Facciamo il matching vero e proprio tra i due descriptors.
			echo "  Testo il file $file...";
			preg_match( "/^(.+).jpg$/", $file, $match );
			//$output = `$bin_dir/match_db $db_dir/$modello/$match[1].sift $sift_testfile 2>&1`;
			$output = `$bin_dir/match_db $sift_testfile $db_dir/$modello/$match[1].sift 2>&1`;
			echo " fatto.";

			preg_match( "/Found (\d+) total matches/m", $output, $match_m );

			// Decido ora di pesare il numero di keypoints matchanti trovati.
			// Metto una percentuale, e poi ne faccio la media.
			$percent = 100 * $match_m[1] / max( $keypoints, $test_keypoints );

			printf(" (%s%%)\n", str_pad( number_format( $percent, 2 ), 5, ' ', STR_PAD_LEFT ) );
			$tot += $percent; // Sommo, e qui sotto dividero' per il numero di elementi
		}
		$tot /= sizeof( $index ); // (media)

		echo "VOTO per il modello $modello: " . number_format($tot, 2) . "%\n\n";
		$votazioni[$modello] = $tot;

	}

	closedir( $handle );
}



arsort( $votazioni );
echo "== CLASSIFICA per [$testimg] ==\n";
$posizione = 1;
foreach ( $votazioni as $modello => $voto ) {
	echo "$posizione) " . str_pad( $modello, 15, ' ', STR_PAD_RIGHT ) . number_format( $voto, 2 ) . "%\n";
	$posizione++;
}



?>