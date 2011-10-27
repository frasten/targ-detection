#!/usr/bin/php
<?php

/* 3-choose-threshold.php
 * Questo script permette di determinare la bonta' del nostro test, al
 * variare delle threshold nel matching con il database.
 *
 * Per farlo va a modificare il valore della threshold nel codice
 * sorgente, ricompila il codice ed infine chiama lo script per
 * l'esecuzione di tutti i test.
 * Salva infine ogni corrispondenza threshold => voto in un file in
 * formato JSON, utile per plottare grafici.
 * */

// Parametri di configurazione per i valori della threshold
$T_MIN = 0.00;
$T_MAX = 1.20;
$DELTA_T = 0.01; // di quando far variare la threshold per ogni prova

$src_file = '../sift/src/match_db.c';
$log_file = 'grafici/threshold_values.json';



// Carico il contenuto del file sorgente .c
$src_content = file_get_contents( $src_file );


$data = array(); // Dati JSON da scrivere successivamente
for ( $t = $T_MIN; $t <= $T_MAX; $t += $DELTA_T ) {
	$t = number_format( $t, 2 );
	echo "Threshold: $t working...";

	// Andiamo a modificare il file sorgente con questo valore di threshold
	// Esempio: #define NN_SQ_DIST_RATIO_THR 0.78
	$src_content = preg_replace( "/^(#define NN_SQ_DIST_RATIO_THR) .+$/m", "\\1 $t", $src_content );
	file_put_contents( $src_file, $src_content );

	// COMPILO
	$srcdir = dirname( $src_file );
	`cd $srcdir; make -j5 match_db`;

	// TESTO
	$output = `./2-do-all-tests.php`;
	preg_match( "/^VOTO TOTALE .+ (\d+)$/m", $output, $match );

	// Aggiungo la votazione al file JSON
	$data[] = array($t, $match[1]);

	echo " done. ($match[1])\n";
	$t = floatval($t);
}

// Scrivo il file JSON
file_put_contents( $log_file, json_encode( $data ) );



?>
