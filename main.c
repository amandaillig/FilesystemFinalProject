#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>

#define SERVER_PORT 2004 // CHANGE THIS!
#define BUF_SIZE 256

// We make this a global so that we can refer to it in our signal handler
int serverSocket;

/*
  We need to make sure we close the connection on signal received, otherwise we have to wait
  for server to timeout.
*/
void closeConnection() {
    printf("\nClosing Connection with file descriptor: %d \n", serverSocket);
    close(serverSocket);
    exit(1);
}

typedef struct Command  {
    char * fileName;
    int * partition;
    int * conToClient
};

int fileCheck (char* fileName){
    FILE *file;
    if(file = fopen(fileName, "r")){
        printf("The file exists");
        return 1;
    }
    else {
        return 0;
    }
}

void overWrite(char command[]){


}





// CREATE COMMAND

void writeToFile(char filePath[], char currentLine[]) {
    //Return partitioned filepath
    // This method should
    FILE * file = fopen(filePath, "a");

    if(file != NULL) {
        fprintf(file, "%s\r\n", currentLine);
    }

    fclose(file);
}

void createPartitionFoldersAndMappingFile(char fileName[], int numOfPartitions) {
    // Create Partition folders
    // 1 = 'a', 2 = 'b', etc...

    int num = numOfPartitions;

    int i;

    char * filePath;

    for(i = 1; i <= num; i++) {
        char diskPath[BUF_SIZE] = "disk";
        snprintf(&diskPath[strlen(diskPath)],sizeof(i),"%d\\",i);

        int result = mkdir(diskPath, 0777);

        // Write To Mapping File
        filePath = strcat(diskPath, fileName);
        writeToFile(fileName, filePath);
    }
}

void addToMapper(int partitionCounter, char fileName[], char * filePath) {
    //Find mapper using fileName
    //Add contents of filepath to the file, using partitionCounter as a line
}

char * getFilePathFromMappingFile(char * fileName, int partitionNo, char * filePath) {

    char line[BUF_SIZE];
    FILE * file = fopen(fileName, "r");
    int i = 1;

    while(fgets(line, sizeof(line), file)) {
        if(i == partitionNo) {
            strcpy(filePath, line);
        }
        i++;
    }
    fclose(file);
    return filePath;
}

void * createFile(void * command) {
    struct Command * currentCommand = (struct Command*) command;


    int  numOfPartitions = *currentCommand->partition;
    int  connectionToClient = *currentCommand->conToClient;

    char * fileName = currentCommand->fileName;
    char receiveLineContent[BUF_SIZE];

    createPartitionFoldersAndMappingFile(fileName, numOfPartitions);

    int bytesReadFromClient = (int )malloc(sizeof(int));

     int partitionCounter = 1;

     char sendLine[BUF_SIZE];

    snprintf(sendLine, sizeof(sendLine), "Input file contents.  End Input with an empty line\n");
    write(connectionToClient, sendLine, strlen(sendLine));

    // read client input until it returns an empty string
    while((bytesReadFromClient = read(connectionToClient, receiveLineContent, BUF_SIZE)) > 0 && strcmp(receiveLineContent, "\n") != 0) {
        if(partitionCounter > numOfPartitions) {
            partitionCounter = 1;
        }

        // Write to file
        // TODO: METHOD TO WRITE LINE IN FILE
        char * filePartitionPath = (char *) malloc(sizeof(char) * BUF_SIZE);
        getFilePathFromMappingFile(fileName, partitionCounter, filePartitionPath);
        filePartitionPath[strlen(filePartitionPath) - 2] = 0;

        writeToFile(filePartitionPath, receiveLineContent);

        free(filePartitionPath);

        printf("%d: %s\n", partitionCounter, receiveLineContent);
        partitionCounter++;
        // Zero out the receive line so we do not get artifacts from before
        bzero(&receiveLineContent, sizeof(receiveLineContent));
    }

    free(&bytesReadFromClient);
    bzero(&receiveLineContent, sizeof(receiveLineContent));
}

// READ COMMAND

void * readFile(void * command) {
    // We need to get the mapping file
    // If command.partition == -1 then read the whole file
    // else, get filepath from getFilePathFromMappingFile and read from only one file

    struct Command * currentCommand = (struct Command*) command;

    char * fileName = currentCommand->fileName;
    int * partitionNo = currentCommand->partition;
    int partitionCounter = 1;

    char * filePath = (char *) malloc(sizeof(char) * BUF_SIZE);

    if(*partitionNo == -1) {
        // Read whole file
    } else {
        // Read from file in only one partition
        getFilePathFromMappingFile(fileName, partitionNo, filePath);
    }
}

// DELETE COMMAND

void * deleteFile(void * command) {
    struct Command * currentCommand = (struct Command*) command;

    char * fileName = currentCommand->fileName;
    int * partitionNo = currentCommand->partition;
}

void getCommandType(char receiveLine[], char commandType[], char * token, char fileName[]) {
    // get the command type
    token = strtok(receiveLine, " ");
    if(token != NULL) {
        strcpy(commandType, token);
    }

    // Get fileName
    token = strtok(NULL, " ");
    if(token != NULL) {
        strcpy(fileName, token);
    }
}

int getConnectionToClient() {
    int connectionToClient;

    // Create a server socket
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serverAddress;
    bzero(&serverAddress, sizeof(serverAddress));
    serverAddress.sin_family      = AF_INET;

    // INADDR_ANY means we will listen to any address
    // htonl and htons converts address/ports to network formats
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port        = htons(SERVER_PORT);

    // Bind to port
    if (bind(serverSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) == -1) {
        printf("Unable to bind to port just yet, perhaps the connection has to be timed out\n");
        exit(-1);
    }

    // Before we listen, register for Ctrl+C being sent so we can close our connection
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = closeConnection;
    sigIntHandler.sa_flags = 0;

    sigaction(SIGINT, &sigIntHandler, NULL);

    // Listen and queue up to 10 connections
    listen(serverSocket, 10);

    connectionToClient = accept(serverSocket, (struct sockaddr *) NULL, NULL);

    return connectionToClient;
}

// TODO: Need to run pthreads for these requests
void processInputFromClient(int connectionToClient) {

    int bytesReadFromClient;

    // These are the buffers for sending data to the client and receiving
    char sendLine[BUF_SIZE];
    char receiveLine[BUF_SIZE];

    char fileName[BUF_SIZE];
    int * numOfPartitions = (int* )malloc(sizeof(int));

    snprintf(sendLine, sizeof(sendLine), "Input either Create, Read, or Delete Command\n");
    write(connectionToClient, sendLine, strlen(sendLine));

    // Get first command
    if((bytesReadFromClient = read(connectionToClient, receiveLine, BUF_SIZE)) > 0) {
        pthread_t commandThread;

        // Need to put a NULL string terminator at end
        receiveLine[bytesReadFromClient] = 0;

        // Show what client sent
        printf("Received: %s\n", receiveLine);

        char commandType[BUF_SIZE];
        char * token;

        getCommandType(receiveLine, commandType, token, fileName);

        //CREATING
        if(strcmp(commandType, "create") == 0) {

            // Get number of partitions
            token = strtok(NULL, " ");
            if(token != NULL) {
                token[sizeof(token) - 1] = 0;
                sscanf(token, "%d",numOfPartitions);
            }
            struct Command createCommand = {fileName, numOfPartitions, &connectionToClient };

            // Start create pthread
            pthread_create(&commandThread, NULL, createFile, (void*) &createCommand);
            pthread_join(commandThread, NULL);
        }

        // READING
        else if(strcmp(commandType, "read") == 0) {
            // READ LOGIC
            struct Command readCommand = {fileName, numOfPartitions, &connectionToClient };

            // Start read pthread
            pthread_create(&commandThread, NULL, readFile, (void*) &readCommand);
            pthread_join(commandThread, NULL);

        }

        // DELETING
        else if(strcmp(commandType, "delete") == 0) {
            // DELETE LOGIC
            struct Command deleteCommand = {fileName, numOfPartitions, &connectionToClient };

            // Start delete pthread
            pthread_create(&commandThread, NULL, deleteFile, (void*) &deleteCommand);
            pthread_join(commandThread, NULL);
        }

        free(numOfPartitions);
    }
}

int main(int argc, char * argv[]) {
    int connectionToClient;
    connectionToClient = getConnectionToClient();

    processInputFromClient(connectionToClient);

    close(connectionToClient);
    closeConnection();
}