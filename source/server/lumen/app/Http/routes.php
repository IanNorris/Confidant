<?php

/*
|--------------------------------------------------------------------------
| Application Routes
|--------------------------------------------------------------------------
|
| Here is where you can register all of the routes for an application.
| It is a breeze. Simply tell Lumen the URIs it should respond to
| and give it the Closure to call when that URI is requested.
|
*/

$app->get('/', function () use ($app) {
	return 'Hello World';
});

//NOTE: Despite the namespace being defined in app.php, you need to define it in every group again.
//See https://github.com/laravel/lumen-framework/issues/239

$app->group(['prefix' => 'user', 'namespace' => 'App\Http\Controllers'], function ($group) {
	$group->get('/register/{id}', 'UserController@register' );
});
