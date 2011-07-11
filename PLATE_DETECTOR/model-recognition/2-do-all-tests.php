#!/usr/bin/php
<?php

/* 2-do-all-tests.php
 * Questo script esegue, per ogni immagine presente nella directory di
 * test, un match con il database.
 * Cio' e' utile per fini statistici, per vedere come si comporta il
 * metodo con un numero sufficiente di campioni.
 *
 * NOTA: e' richiesto questo formato per ogni file in questa directory:
 * <nomemodello>-xx.jpg
 * dove xx e' un numero a piu' cifre, e nomemodello e' il nome del
 * modello della macchina che stiamo testando. Grazie a questo possiamo
 * sapere se il nostro script si sta comportando bene o meno.
 *
 * Esempio:
 * alfa-brera-1.jpg
 *
 * */

$testdir = 'test';

$voto_tot = 0;
$n_files = 0;
if ( $handle = opendir( $testdir ) ) {
	while ( false !== ( $testimg = readdir( $handle ) ) ) {
		// Leggo ogni immagine nella directory di test
		if ( ! preg_match( "/^(.+)-\d+.jpg$/", $testimg, $match1 ) ) continue;

		$n_files++;
		$testimg = "$testdir/$testimg";

		/* Chiamiamo per quest'immagine lo script che effettua il matching */
		$output = `./1-discover.php $testimg`;

		/* Analizziamo l'output del nostro script per dare una votazione ad
		 * ogni test. */
		preg_match( "/^(\d+)\) " . preg_quote( $match1[1], '/' ) . "\s+/m", $output, $match2 );
		echo "File: [$match1[0]] Expected: [$match1[1]] Posizione: [$match2[1]]\n";

		/* VOTAZIONE:
		 * idea: voglio penalizzare di piu' chi scende parecchio in classifica.
		 * (TODO), per ora invece cerchiamo di minimizzare la somma delle
		 * posizioni totali.
		 * Il numero minimo raggiungibile (posizione sempre 1) e' quindi
		 * pari al numero dei test effettuati, e deve corrispondere al voto
		 * 100.
		 *
		 * voto_tot : n.cosi = 100
		 *
		 * */
		$voto_tot += $match2[1];
	}
	closedir( $handle );
}

printf( "VOTO TOTALE (+ alto meglio e'): %d\n", 100 * $n_files / $voto_tot );
?>
