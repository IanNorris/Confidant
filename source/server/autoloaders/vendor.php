<?php

$vendor_root = '/../../../vendor/';
$relative_vendor_root = __DIR__ . $vendor_root;

$autoload_registry = array(
	'Slim\\' => 'slim/Slim/',
	'FastRoute\\' => 'fastroute/src/',
	'Psr\\Http\\Message\\' => 'httpmessage/src/',
	'Interop\\' => 'container-interop/src/Interop/',
	'Pimple\\' => 'pimple/src/Pimple/',
);

$autoload_includes = array(
	'fastroute/src/functions.php'
);

spl_autoload_register( function( $class )
{
	global $autoload_registry;
	global $relative_vendor_root;
	
	foreach( $autoload_registry as $prefix => $base )
	{
		$len = strlen( $prefix );
		if( strncmp( $prefix, $class, $len ) == 0 )
		{
			// get the relative class name
			$relative_class = substr( $class, $len );

			// replace the namespace prefix with the base directory, replace namespace
			// separators with directory separators in the relative class name, append
			// with .php
			$file = $relative_vendor_root . $base . str_replace('\\', '/', $relative_class) . '.php';
			
			// if the file exists, require it
			if (file_exists($file)) {
			
				require $file;
				return;
			}
		}
	}
});

foreach( $autoload_includes as $include )
{
	require $relative_vendor_root . $include;
}

?>