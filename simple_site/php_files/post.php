#!/usr/bin/php-cgi

<?php
	$entityBody = file_get_contents('php://input');
	var_dump($entityBody);
?>