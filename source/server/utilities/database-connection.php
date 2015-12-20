<?php
/*This class creates and maintains a database connection.
 * The class has a minimal public interface.
 * 
 * To create a connection:
 * $connection = DatabaseConnection::get()
 * 
 */

class DatabaseConnection
{
	private $existing_connection = null;
	
	/* 
	 * Creates a new instance.
	 * 
	 * @return A new instance of the DB connector 
	 */
	private static function getInstance()
	{
		static $instance = null;
		
		if( $instance == null )
		{
			$instance = new DatabaseConnection();
		}
		
		return $instance;
	}
	
	/*
	 * Connect to the DB, returns the existing DB connection
	 * if one exists.
	 * 
	 * @return A PDO connection object. This function does not return
	 * if an error occurs making the connection.
	 */
	private function connect()
	{
		if( $this->existing_connection == null )
		{
			$server = Config_DB::Server;
			$name = Config_DB::Name;
			
			$connection_string = "mysql:host=$server;dbname=$name";
			
			try
			{
				$this->existing_connection = new PDO( $connection_string, Config_DB::User, Config_DB::Password,
													  array( PDO::ATTR_PERSISTENT => true) );
				
				$this->existing_connection->setAttribute(PDO::ATTR_ERRMODE, PDO::ERRMODE_EXCEPTION);
				$this->existing_connection->setAttribute(PDO::ATTR_EMULATE_PREPARES, false);
			}
			catch( PDOException $e )
			{
				die( "Connection to DB failed" );
			}
		}
		
		return $this->existing_connection;
	}
	
	/*
	 * Get a database connection, if one does not already exist
	 * then one is created.
	 * 
	 * @return A DB connection. This function does not return
	 * if an error occurs making the connection.
	 */
	public static function get()
	{
		return DatabaseConnection::getInstance()->connect();
	}
}
?>