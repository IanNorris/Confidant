<?php

$salt_names = array(
	'Filename',
	'SaltKey',
	'Session'
);

/**
 * Get a random and unique salt string of the length specified
 * 
 * @param $length_bits The number of bits to generate
 * 
 * @return A random salt
 */
function getSalt( $length_bits = 256 )
{
	$bytes = openssl_random_pseudo_bytes( $length_bits / 8 );
	$hex   = bin2hex( $bytes );
	return $hex;
}

if( file_exists( __DIR__ . '\\salts.php' ) )
{
	print( "An existing salts.php file was found, keeping existing values.\n" );
	
	require __DIR__ . '\\salts.php';
}

$fileContent = file_get_contents( __DIR__ . '\\salts.template.php' );

foreach( $salt_names as $saltName )
{
	$value = getSalt();
	if( defined( "Config_Salts::$saltName" ) )
	{
		$value = constant( "Config_Salts::$saltName" );
	}
	else
	{
		print( "$saltName does not exist in old file, adding it.\n" );
	}
	
	$fileContent = str_replace( '%' . $saltName . '%', $value, $fileContent );
}

file_put_contents( __DIR__ . '\\salts.php', $fileContent );

?>