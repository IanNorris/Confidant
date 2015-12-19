<?php

namespace App\Http\Controllers;

class UserController extends Controller
{
	/**
	* Register a new user
	*
	* @param  int  $id
	* @return Response
	*/
	public function register($id)
	{
		//return view('user.profile', ['user' => User::findOrFail($id)]);
		return "Hello $id";
	}
}
