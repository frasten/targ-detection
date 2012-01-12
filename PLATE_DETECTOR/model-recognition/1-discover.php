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
$db_img_dir = 'db_img_posteriore';
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
$testimg = escapeshellarg($testimg);
$output = `$bin_dir/siftfeat -x -o $sift_testfile $testimg 2>&1`;
preg_match("/Found (\d+) /m", $output, $match);
$test_keypoints = $match[1];
echo " fatto.\n\n";

// Controlliamo ogni modello nel database
$votazioni = array();
$votomax = array();
$imgmax = '';
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
		// Saltiamo i modelli per i quali abbiamo pochi samples
		if (sizeof($indexdata) <= 2) continue;
		foreach( $indexdata as $row ) {
			$row = explode( "\t", $row );
			$index[$row[0]] = $row[1];
		}

		// Leggiamo i vari files
		echo "* Modello: $modello\n";
		$tot = 0;
		$votomax[$modello] = 0;
		$imgmax[$modello] = null;
		foreach ( $index as $file => $keypoints ) {

			// Facciamo il matching vero e proprio tra i due descriptors.
			echo "  Testo il file $file...";
			preg_match( "/^(.+).jpg$/i", $file, $match );
			$output = `$bin_dir/match_db $db_dir/$modello/$match[1].sift $sift_testfile 2>&1`;
			//$output = `$bin_dir/match_db $sift_testfile $db_dir/$modello/$match[1].sift 2>&1`;
			echo " fatto.";

			if ( preg_match("/RANSAC OK/m", $output) ) {
				preg_match("/inliers: (\d+)$/m", $output, $inliers_match);
				echo "Inliers: $inliers_match[1]\n";
				$percent = 100 * $inliers_match[1] / min( $keypoints, $test_keypoints );
				$percent = 100 * $inliers_match[1] / $keypoints;
			}
			else {
				$percent = 0;
			}
			//preg_match( "/Found (\d+) total matches/m", $output, $match_m );

			// Decido ora di pesare il numero di keypoints matchanti trovati.
			// Metto una percentuale, e poi ne faccio la media.
			// TODO: questo metodo evidentemente fa schifo.
			//$percent = 100 * $match_m[1] / min( $keypoints, $test_keypoints );
			// Aggiorno eventualmente il voto massimo
			if ($percent > $votomax[$modello]) {
				$votomax[$modello] = $percent;
				$imgmax[$modello] = "$db_img_dir/$modello/$file";
			}
			//$percent = 100 * $match_m[1] / $keypoints;
			//$percent = 100 * $match_m[1] / $test_keypoints;

			printf(" (%s%%)\n", str_pad( number_format( $percent, 2 ), 5, ' ', STR_PAD_LEFT ) );
			$tot += $percent; // Sommo, e qui sotto dividero' per il numero di elementi
		}
		$tot /= sizeof( $index ); // (media)

		echo "VOTO per il modello $modello: " . number_format($tot, 2) . "%\n\n";
		$votazioni[$modello] = $tot;

	}

	closedir( $handle );
}


if (array_sum($votazioni) == 0) {
	echo "Riconoscimento fallito.\n";
	exit;
}

arsort( $votazioni );
echo "== CLASSIFICA per [$testimg] ==\n";
$posizione = 1;
$vincitore = null;
foreach ( $votazioni as $modello => $voto ) {
	if ($voto == 0) continue;
	echo "$posizione) " . str_pad( $modello, 26, ' ', STR_PAD_RIGHT ) . number_format( $voto, 2 ) . "%\n";
	if ($posizione == 1) $vincitore = $modello;
	$posizione++;
}


$INTERACTIVE = ! in_array('--batch', $argv);

// Visualizziamo il best match:
if ($INTERACTIVE) {
	/* TODO: devo in realta' mostrare il miglior match all'interno dei match
	della posizione 1. */

	print "Visualizziamo il miglior match rilevato con l'immagine:\n $imgmax[$vincitore]...\n";
	$output = `$bin_dir/match $imgmax[$vincitore] $testimg 2>&1`;
	echo $output;
}

?>
