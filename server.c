/******************************************************************************/
/******************************************************************************/
/*******************                                        *******************/
/*******************            PROJET MULTITACHE           *******************/
/*******************                                        *******************/
/******************************************************************************/
/******************************************************************************/
/*******************          Auteur: Evan Graïne           *******************/
/***    Description: Mini jeu en pseudo-réseau local consistant à deviner    **/
/***                 un nombre fuyant.                                       **/
/******************************************************************************/
/******************************************************************************/
/**********************  FICHIER CONTENANT LE SERVEUR  ************************/
/******************************************************************************/
/******************************************************************************/

///
/// INCLUDES
///
/// Standard
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
/// Personal
#include "globalconstants.h"

///
/// GLOBAL VALUES
///
clientInfo playersList[MAX_PLAYERS];
int playersListMask[MAX_PLAYERS];
bool gameIsOn = false;

pthread_mutex_t number_mutex;
int theNumber;

int difficulty=DIFFICULTY;


///
/// FUNCTIONS
///


/// Init players list
void list_initPlayers(){
	int i;
	for(i=0;i<MAX_PLAYERS; i++) 
		playersListMask[i]=false;
}

/// Adding a player to the game
bool list_addPlayer(clientInfo client){
	int i=0;
	/// Find an empty space
	while(playersListMask[i]!=false && i<MAX_PLAYERS){i++;}
	/// Add a player if we found a place
	if(i==MAX_PLAYERS) return false;
	else
	{
		playersListMask[i]=true;
		playersList[i]=client;
		return true;
	}
}

/// Deleting a player from the game
bool list_delPlayer(clientInfo client){
	int i=0;
	/// Find the player
	while(playersList[i].pid != client.pid && i < MAX_PLAYERS){i++;}
	/// Delete the player if we found it
	if(i==MAX_PLAYERS) return false;
	else {
		playersListMask[i]=false;
		return true;
	}
}

/// Searching for a player by pid
bool list_getPlayerByPid(pid_t pid, clientInfo *dest_client)
{
	int i=0;
	/// Find the player
	while(playersList[i].pid != pid && i < MAX_PLAYERS){i++;}	
	/// Return a boolean telling if it is in
	if(i==MAX_PLAYERS) return false;
	else {
		/// Update the output clientInfo
		strncpy(dest_client->name, playersList[i].name,
			MAX_CLIENT_NAME_LENGTH);
		dest_client->pid=playersList[i].pid;
		strncpy(dest_client->pipeName, playersList[i].pipeName,
			MAX_NAMED_PIPE_NAME_LENGTH);
		return true;
	}
}

/// Searching for a player by name. If multiple players have the same name,
/// Then another call will output the next one
/// Until the function returns false when no other has been found
/// An other call will then start from the beginning
bool list_getPlayerByName(char *name, clientInfo *dest_client)
{
	static int i=0;

	/// Find the player
	while(i < MAX_PLAYERS)
	{
		if (!strcmp(name, playersList[i].name) && playersListMask[i]==true) 
			break; 
		i++;
	}	
	/// Return a boolean telling if it is in
	if(i==MAX_PLAYERS) 
	{
		i=0;
		return false;
	}
	else 
	{
		/// Update the output clientInfo
		strncpy(dest_client->name, playersList[i].name,
			MAX_CLIENT_NAME_LENGTH);
		dest_client->pid=playersList[i].pid;
		strncpy(dest_client->pipeName, playersList[i].pipeName,
			MAX_NAMED_PIPE_NAME_LENGTH);
		return true;
	}
}

/// Returns boolean telling if the string is numeric
bool isNumeric(char *s)
{
	int i=0;
	if(s==NULL) return false;
	else if(*s == '\0' || *s == '\n') return false;
	while(s[i] <= '9' && s[i] >= '0')i++;
	if (i==strlen(s)) return true;
	else return false;
}

/// Displays client Information
void verboseClientInfo(clientInfo client){
	VERBOSE("Client Information : \n\tName: %s\n"
						"\tPID: %d\n"
						"\tPipe Name: %s\n", 
						client.name, client.pid, client.pipeName);
}

/// Procedure forking a child process to behave like a client
void createVirtualClient(){
	switch(fork()){
		case -1:
			ERR("[SERVER] "ERR_FORK_FAIL_MSG);
			exit(ERR_FORK_FAIL);
		case 0:
			execl("client", "client", VIRTUAL_CLIENT_ARGUMENT, NULL);
			system("pwd");
			ERR("[SERVER] "ERR_EXEC_FAIL_MSG);
			exit(ERR_EXEC_FAIL);
		default:
			break;
	}
}

/// Procedure creating the connection pipe
void createConnectionPipe(){
	if (mkfifo(CONNECTION_NAMED_PIPE_NAME, 700)==-1)
	{
		if (unlink(CONNECTION_NAMED_PIPE_NAME)==-1)
		{
			ERR("[SERVER] "ERR_NAMED_PIPE_CREATION_FAIL_MSG);
			exit(ERR_NAMED_PIPE_CREATION_FAIL);
		}
		mkfifo(CONNECTION_NAMED_PIPE_NAME, 700);
	}
}



/// Procedure to open 'safely' a file
int safeOpen(char *pathname, int mode)
{
	int file;
	if ((file=open(pathname, mode))==-1)
	{
		ERR("[SERVER] "ERR_OPEN_FAIL_MSG);
		exit(ERR_OPEN_FAIL);
	}
	return file;
}

/// Randomisation function with a range
int randRange(int left, int right){
	return (rand()%(right-left))+left;
}

/// Randomisation function with a range from 0
int randZeroTo(int right){
	return rand()%right;
}

/// Game initialisation
void initGame(){
	int i;

	/// Lock the number
	pthread_mutex_lock(&number_mutex);

	//TODO Increase with difficulty
	theNumber = randRange(-10*DIFFICULTY, (DIFFICULTY==0 ? 10 : 10*DIFFICULTY));
	gameIsOn=true;
	DEBUG("[SERVER] The number: %d", theNumber);

	/// Unlock the number
	pthread_mutex_unlock(&number_mutex);

	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersListMask[i]==true)
		{
			kill(playersList[i].pid, SIG_START);
		}
	}

	/// Start timer
	alarm(BASE_TIMEOUT/ (difficulty<1? 1 : difficulty) + 10);
}


/// Function to test number
int testNumber(int number)
{
	//TODO Implement this
	return HIGHER;
}


/// Thread main
void *newConnection(void *arg){
	int communicationPipe;
	clientMessage messageFromClient;
	serverMessage messageFromServer;
	clientInfo *p_client = (clientInfo *) arg;
	clientInfo client;
	int highOrLow;

	client.pid=p_client->pid;
	strcpy(client.name,p_client->name);
	strcpy(client.pipeName,p_client->pipeName);

	DEBUG("[SERVER: THREAD %s] Thread created for %s (%d)", 
		client.name,client.name,(int)client.pid);
	
	VERBOSE("%s is connected.",client.name);
	communicationPipe=safeOpen(client.pipeName, O_RDWR);

	// Sending connection confirmation
	messageFromServer.type=ACCEPT;
	if(gameIsOn) messageFromServer.choice=GAME;
	else messageFromServer.choice=GAME_NOT_ON;
	write(communicationPipe, &messageFromServer, sizeof(serverMessage));

	while(true)
	{
		usleep(100000);
		read(communicationPipe, &messageFromClient, sizeof(clientMessage));
		DEBUG("[SERVER: THREAD %s] Received from %s:\n\t\t"
			  			"type=%d\n\t\tchoice=%d",  
		client.name,client.name,
		messageFromClient.type,messageFromClient.choice);
		
		if(messageFromClient.type==QUIT)
		{
			VERBOSE("%s has left the game.", client.name);
			break;
		}
		else if(messageFromClient.type==GUESS && gameIsOn)
		{
			messageFromServer.type=GAME;
			VERBOSE("%s has submitted %d", client.name, messageFromClient.choice);
			if((highOrLow=testNumber(messageFromClient.choice))==0){
				//WON
				messageFromServer.choice=WIN;
				//TODO End the game
				//TELL EVERYONE WHO WON ???
			}
			else if(highOrLow > 0)
				messageFromServer.choice=HIGHER;
			else
				messageFromServer.choice=LOWER;

			/// Sending message
			write(communicationPipe, &messageFromServer, 
					sizeof(serverMessage));
		}
		else if (!gameIsOn)
		{
			messageFromServer.type=GAME;
		}


	}
	list_delPlayer(client);
	
	pthread_exit(OK);
}

/// Kicks a player from the server
void kickPlayer(clientInfo client, int reason)
{
	int communicationPipe;
	serverMessage messageFromServer;

	messageFromServer.type=KICK;
	messageFromServer.choice=reason;
	communicationPipe=safeOpen(client.pipeName, O_WRONLY);
	kill(client.pid, SIGINT);
	usleep(1000);
	write(communicationPipe, &messageFromServer, sizeof(serverMessage));
	close(communicationPipe);
	list_delPlayer(client);
}

/// Shuts down the server on SIGINT
void shutDown(int signal)
{
	DEBUG("\n[SERVER] Shut down server.");
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersListMask[i]==true){
			// Force disconnecting all the clients
			kickPlayer(playersList[i], REASON_SERVER_INTERRUPTION);
		}
	}
	exit(INTERRUPTED);
}


/// Gets a command from server terminal
void getCommand(){
	
	int flags = fcntl(0, F_GETFL, 0);
	int i;
	clientInfo client;

	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	
	
	char command[200];
	char arg[200];
	// If a command has been typed on the terminal
	if(read(0,command,100)>0) 
	{
		i=0;while(command[i] != '\n')i++;command[i+1]=0;

		DEBUG("[SERVER] Command : '%s'",command);
		if(!strncmp(command, "quit\n", 5) 
		|| !strncmp(command, "q\n", 2) 
		|| !strncmp(command, "exit\n", 5))
			shutDown(0);
		/// KICK COMMAND
		else if(!strncmp(command, "kick ", 5) && command[5] != '\n')
		{
			/// Separating argument
			strcpy(arg, &(command[5]));
			arg[strlen(arg)-1]=0;
			DEBUG("[SERVER] Kick: '%s'",arg);
			if (isNumeric(arg)) 
			{
				if(list_getPlayerByPid(atoi(arg), &client)) 
				{
					VERBOSE("%s has been kicked.", client.name);
					kickPlayer(client, REASON_SERVER_KICK);
				}
			}
			else
			{
				
				if(list_getPlayerByName(arg, &client))
				{
					VERBOSE("%s has been kicked.", client.name);
					kickPlayer(client, REASON_SERVER_KICK);
				}
			}
		}
		/// INFO COMMAND
		else if(!strncmp(command, "info ", 5) && command[5] != '\n')
		{
			/// Separating argument
			strcpy(arg, &(command[5]));
			arg[strlen(arg)-1]=0;

			DEBUG("[SERVER] Info: '%s'",arg);
			if (isNumeric(arg)) 
			{
				if(list_getPlayerByPid(atoi(arg), &client)) 
					verboseClientInfo(client);			
			}
			else
			{
				if(list_getPlayerByName(arg, &client))
					verboseClientInfo(client);
			}
		}
		/// START COMMAND
		else if(!strncmp(command, "start\n", 6) && command[6] != '\n')
		{
			initGame();
		}
		/// UNKNOWN COMMAND
		else
		{
			VERBOSE("Unknown command.");
		}
	}	
}

void endGame(int signal){
	VERBOSE("\nTime is out!");
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersListMask[i]==true){
			// Tell all the clients that they have lost
			kill(playersList[i].pid, SIG_STOP);
		}
	}
}

///
/// MAIN
///
int main(int argc, char const *argv[])
{
	int connectionPipe;
	clientInfo newClient;
	pthread_t thread;

	//_INITIALISATION_

	/// Managing sigint
	signal(SIGINT, shutDown);
	/// Managing sigalarm
	signal(SIGALRM, endGame);


	/// Randomisation srand initialisation
	srand(time(NULL));

	/// Clear the screen
	system("clear");

	/// Little welcome message
	VERBOSE("\t.::|[    GAME SERVER    ]|::.\n"
			"\t^^^^^^^^^^^^^^^^^^^^^^^^^^^^^\n");
	
	VERBOSE("Initialisation.");

	// Create named pipe
	DEBUG("[SERVER] Creating connection named pipe.");
	createConnectionPipe();
	

	//WARNING Find another option
	//Forcing access to the pipe to the clients
	system("chmod 700 "CONNECTION_NAMED_PIPE_NAME);

	// Create a virtual client that will connect the pipe
	DEBUG("[SERVER] Starting virtual client.");
	createVirtualClient();

	// Open named pipe
	DEBUG("[SERVER] Opening connection named pipe.");
	connectionPipe = safeOpen(CONNECTION_NAMED_PIPE_NAME, O_RDONLY);


	//_LISTENLOOP_
	VERBOSE("Server is ready to accept new players.");

	list_initPlayers();

	while(true){
		usleep(10000);
	
		getCommand();

		// If a client has sent connection information
		if(!(read(connectionPipe, &newClient, sizeof(clientInfo))<=0))
		{
			// Update the list of clients
			if(list_addPlayer(newClient))
				//_NEW THREAD FOR CLIENT_
				pthread_create(&thread, NULL, newConnection, &newClient);
			else
			{
				ERR_NOPERROR("Could not accept new player %s: Room full.", newClient.name);
				kickPlayer(newClient, REASON_ROOM_FULL);
			}
		}	
	}

	//_END_

	DEBUG("[SERVER] Unlinking the connection named pipe");
	close(connectionPipe);
	if (unlink(CONNECTION_NAMED_PIPE_NAME)==-1) {
		ERR("[SERVER] Did not manage to unlink connection named pipe");
	}
	

	return 0;
}