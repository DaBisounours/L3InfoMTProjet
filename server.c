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
/// Personal
#include "globalconstants.h"

///
/// GLOBAL VALUES
///
pid_t playersList[MAX_PLAYERS];

///
/// FUNCTIONS
///

/// Display the list
/*void displayPlayerList()
{
	int i;
	DEBUG("--- CLIENT LIST ---");
	for(i=0; i<MAX_PLAYERS;i++)
	{
		DEBUG("%d: %d", i, playersList[i]);
	}
	DEBUG("-------------------");
}*/

/// Init players list
void list_initPlayers(){
	int i;
	for(i=0;i<MAX_PLAYERS; i++) 
		playersList[i]=NO_PLAYER;
}

/// Adding a player to the game
int list_addPlayer(pid_t pid){
	int i=0;
	/// Find an empty space
	while(playersList[i]!=NO_PLAYER && i<MAX_PLAYERS)i++;
	/// Add a player if we found a place
	if(i==MAX_PLAYERS) return false;
	else
	{
		playersList[i]=pid;
		return true;
	}
}

/// Deleting a player from the game
int list_delPlayer(pid_t pid){
	int i=0;
	/// Find the player
	while(playersList[i]!=pid && i<MAX_PLAYERS){
		i++;
	}
	/// Del the player if we found it
	if(i==MAX_PLAYERS) return false;
	else {
		playersList[i]=NO_PLAYER;
		return true;
	}
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

/// Thread main
void *newConnection(void *arg){
	int communicationPipe;
	clientMessage messageFromClient;
	serverMessage messageFromServer;
	clientInfo *p_client = (clientInfo *) arg;
	clientInfo client;
	client.pid=p_client->pid;
	strcpy(client.name,p_client->name);
	strcpy(client.pipeName,p_client->pipeName);

	DEBUG("[SERVER: THREAD %s] Thread created for %s (%d)", 
		client.name,client.name,(int)client.pid);
	
	VERBOSE("%s is connected.",client.name);
	communicationPipe=safeOpen(client.pipeName, O_RDWR);

	// Sending connection confirmation
	messageFromServer.type=ACCEPT;
	messageFromServer.choice=-1;
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
		
	}
	list_delPlayer(client.pid);
	//displayPlayerList();
	pthread_exit(OK);
}

/// Gets a command from server terminal
void getCommand(){
	
	int flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	
	char command[200];
	// If a command has been typed on the terminal
	if(read(0,command,100)>0) 
	{
		if(!strncmp(command, "quit", 4) || !strncmp(command, "q", 4));
			//TODO Auto-send Interrupt to send signals to the clients
	}	
}

void shutDown(int signal)
{
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersList[i]!=NO_PLAYER){
			// Force disconnecting all the clients
			kill(playersList[i], SIGINT);
		}
	}
	exit(INTERRUPTED);
}

void kickPlayer(clientInfo client)
{
	int communicationPipe;
	serverMessage messageFromServer;
	messageFromServer.type=KICK;
	messageFromServer.choice=REASON_ROOM_FULL;
	communicationPipe=safeOpen(client.pipeName, O_RDWR);
	write(communicationPipe, &messageFromServer, sizeof(serverMessage));
	close(communicationPipe);
}

///
/// MAIN
///
int main(int argc, char const *argv[])
{
	int connectionPipe;
	clientInfo newClient;
	pthread_t thread;
	char command[200];

	//_INITIALISATION_

	/// Managing sigint
	signal(SIGINT, shutDown);

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

	//TODO DELETE
	//displayPlayerList();

	while(true){
		usleep(10000);
	
		getCommand();

		// If a client has sent connection information
		if(!(read(connectionPipe, &newClient, sizeof(clientInfo))<=0))
		{
			// Update the list of clients
			if(list_addPlayer(newClient.pid))
				//_NEW THREAD FOR CLIENT_
				pthread_create(&thread, NULL, newConnection, &newClient);
			else
			{
				ERR_NOPERROR("Could not accept new player %s: Room full.", newClient.name);
				kickPlayer(newClient);
			}
			//displayPlayerList();
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