<?php

//We are not using composer, so use our own autoloader
require __DIR__ . '/../autoloaders/vendor.php';
require __DIR__ . '/../../../config/server/database.php';
require __DIR__ . '/../../../config/server/salts.php';
require __DIR__ . '/../utilities/database-connection.php';

/**
 * Step 2: Instantiate a Slim application
 *
 * This example instantiates a Slim application using
 * its default settings. However, you will usually configure
 * your Slim application now by passing an associative array
 * of setting names and values into the application constructor.
 */
$app = new Slim\App();

/**
 * Step 3: Define the Slim application routes
 *
 * Here we define several Slim application routes that respond
 * to appropriate HTTP request methods. In this example, the second
 * argument for `Slim::get`, `Slim::post`, `Slim::put`, `Slim::patch`, and `Slim::delete`
 * is an anonymous function.
 */
$app->get('/', function ($request, $response, $args) {
	$response->write("Welcome to Slim!");
	$connection = DatabaseConnection::get();
	return $response;
});

$app->get('/hello[/{name}]', function ($request, $response, $args) {
	$response->write("Hello, " . $args['name']);
	return $response;
})->setArgument('name', 'World!');

$app->post('/queryServer', function ($request, $response, $args) {
	$response = $response->withAddedHeader( 'Content-Type', 'application/json' );
	$response = $response->withStatus(200);
	$body = $response->getBody();
	$body->write( json_encode( array( 'salt' => Config_Salts::SaltKey ) ) );
	return $response;
});

/**
 * Step 4: Run the Slim application
 *
 * This method should be called last. This executes the Slim application
 * and returns the HTTP response to the HTTP client.
 */
$app->run();
