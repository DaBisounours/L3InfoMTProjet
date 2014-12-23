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
clientInfo playersList[MAX_PLAYERS];
int playersListMask[MAX_PLAYERS];

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
		playersListMask[i]=false;
}

/// Adding a player to the game
int list_addPlayer(clientInfo client){
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
int list_delPlayer(clientInfo client){
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
	list_delPlayer(client);
	//displayPlayerList();
	pthread_exit(OK);
}



void shutDown(int signal)
{
	DEBUG("\n[SERVER] Shut down server.");
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersListMask[i]==true){
			// Force disconnecting all the clients
			kill(playersList[i].pid, SIGINT);
			list_delPlayer(playersList[i]);
		}
	}
	exit(INTERRUPTED);
}

void kickPlayerByInfo(clientInfo client)
{
	int communicationPipe;
	serverMessage messageFromServer;
	messageFromServer.type=KICK;
	messageFromServer.choice=REASON_ROOM_FULL;
	communicationPipe=safeOpen(client.pipeName, O_RDWR);
	write(communicationPipe, &messageFromServer, sizeof(serverMessage));
	close(communicationPipe);
}


/// Gets a command from server terminal
void getCommand(){
	
	int flags = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, flags | O_NONBLOCK);
	int i;
	
	char command[200];
	char arg[200];
	// If a command has been typed on the terminal
	if(read(0,command,100)>0) 
	{
		i=0;while(command[i] != '\n')i++;command[i+1]=0;

		DEBUG("[SERVER] Command : '%s'",command);
		if(!strncmp(command, "quit\n", 5) || !strncmp(command, "q\n", 2))
			shutDown(0);
		else if(!strncmp(command, "kick ", 5) && command[5] != '\n')
		{
			strcpy(arg, &(command[5]));
		}
	}	
}

void endGame(){
	DEBUG("\n[SERVER] Ending game.");
	int i;
	for(i=0; i<MAX_PLAYERS; i++)
	{
		if(playersListMask[i]==true){
			// Force disconnecting all the clients
			kill(playersList[i].pid, SIGUSR1);
			list_delPlayer(playersList[i]);
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
			if(list_addPlayer(newClient))
				//_NEW THREAD FOR CLIENT_
				pthread_create(&thread, NULL, newConnection, &newClient);
			else
			{
				ERR_NOPERROR("Could not accept new player %s: Room full.", newClient.name);
				kickPlayerByInfo(newClient);
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