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

#define SERVER_PORT 1999 // CHANGE THIS!
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

typedef struct Command {
    char * fileName;
    int * partition;
    int conToClient;
};

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
        char diskPath[BUF_SIZE] = "./disk";
        snprintf(&diskPath[strlen(diskPath)],sizeof(i),"%d/",i);

        int result = mkdir(diskPath, 0777);

        // Write To Mapping File
        filePath = strcat(diskPath, fileName);
        writeToFile(fileName, filePath);
    }
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

int createFile(struct Command * command) {
    struct Command * currentCommand = command;

    int  numOfPartitions = *currentCommand->partition;
    int  connectionToClient = currentCommand->conToClient;

    char * fileName = currentCommand->fileName;
    char receiveLineContent[BUF_SIZE];

    createPartitionFoldersAndMappingFile(fileName, numOfPartitions);

    int partitionCounter = 1;
    int bytesReadFromClient;

    char sendLine[BUF_SIZE];

    snprintf(sendLine, sizeof(sendLine), "Input file contents.  End Input with an empty line\n");
    write(connectionToClient, sendLine, strlen(sendLine));

    // read client input until it returns an empty string
    while((bytesReadFromClient = read(connectionToClient, receiveLineContent, BUF_SIZE)) > 0 && strcmp(receiveLineContent, "\n") != 0) {
        if(partitionCounter > numOfPartitions) {
            partitionCounter = 1;
        }

        // Write to file
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

    bzero(&receiveLineContent, sizeof(receiveLineContent));

    return 1;
}

// READ COMMAND

int ifFileExists(char * filePath) {
    if(access( filePath, F_OK ) != -1) {
        return 1;
    } else {
        return -1;
    }
}
int countLinesInFile (char * fileName){
    char line[BUF_SIZE];
    FILE * file = fopen(fileName, "r");
    int i = 0;

    while(fgets(line, sizeof(line), file)) {
        i++;
    }
    fclose(file);
    return i;
}

int readFile(struct Command * command) {
    // We need to get the mapping file
    // If partitionNo == -1 then read the whole file
    // else, get filepath from getFilePathFromMappingFile and read from only one file

    struct Command * currentCommand = command;

    char * fileName = currentCommand->fileName;
    int * partitionNo = currentCommand->partition;
    int partitionCounter = 1;
    char line[BUF_SIZE];

    if(*partitionNo == -1) {
        fileName[strlen(fileName)-1]=0;
    }

    if(ifFileExists(fileName) == 1) {
        char filePath[BUF_SIZE];
        char sendLine[BUF_SIZE];
        if (*partitionNo == -1) {
            // Read whole file
            int noOfLines = countLinesInFile(fileName);
            int lineCounter = 0;
            int doneReadingFile = 0;
            while(doneReadingFile != 1){
                for(int i =1;i<noOfLines+1;i++){
                    getFilePathFromMappingFile(fileName, i, filePath);
                    filePath[strlen(filePath) - 2] = 0;
                    if(ifFileExists(filePath) == 1) {
                        FILE * file = fopen(filePath, "r");
                        for(int j=0;j<lineCounter+1;j++){
                            if(!fgets(line,sizeof(line),file)){
                                doneReadingFile =1;
                            }
                        }
                        if(doneReadingFile != 1) {
                            line[strlen(line) - 1] = 0;
                            if(line[0] != 0 && strcmp(&line[0], "\r") != 0) {
                                snprintf(sendLine, sizeof(sendLine), "%s\n",line);
                                write(currentCommand->conToClient, sendLine, strlen(sendLine));
                            }
                        }
                        fclose(file);
                    }
                }
                lineCounter++;
            }
        } else {
            // Read from file in only one partition
            getFilePathFromMappingFile(fileName, *partitionNo, filePath);
            filePath[strlen(filePath) - 2] = 0;
            FILE * file = fopen(filePath, "r");
            int i = 1;

            while(fgets(line, sizeof(line), file)) {
                line[strlen(line) - 1] = 0;
                if(line[0] != 0 && strcmp(&line[0], "\r") != 0) {
                    snprintf(sendLine, sizeof(sendLine), "%s\n",line);
                    write(currentCommand->conToClient, sendLine, strlen(sendLine));
                }
            }
            fclose(file);
        }
        return 1;
    }else{
        return -1;
    }
}

// DELETE COMMAND

int deleteFile(struct Command * command) {
    // Use filename to find the mapping file
    // Loop through directories in mapping file
    // Delete the file in the filepath
    // Once all files are deleted, delete the mapping file
    struct Command * currentCommand = (struct Command*) command;

    char * fileName = currentCommand->fileName;  // filepath for mapping file
    int * partitionNo = currentCommand->partition;

    fileName[strlen(fileName) - 1] = 0;

    if(ifFileExists(fileName) == 1) {
        char line[BUF_SIZE];
        FILE * file = fopen(fileName, "r");
        int i = 1;

        while(fgets(line, sizeof(line), file)) {
            line[strlen(line) - 2] = 0;
            remove(line);
        }
        fclose(file);
        remove(fileName);

        return 1;
    } else {
        return -1;
    }

}

void tokenizeCommand(char receiveLine[], char commandType[], char * token, char fileName[], int * partitionNo) {
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

    // Get Partitions
    token = strtok(NULL, " ");
    if(token != NULL) {
        token[sizeof(token) - 1] = 0;
        sscanf(token, "%d",partitionNo);
    } else {
        *partitionNo = -1;
    }


}

// TODO: Need to run pthreads for these requests
void * processInputFromClient(void * ctc) {

    int bytesReadFromClient;

    int connectionToClient = *(int *) ctc;

    int stopConnection = 0;

    // These are the buffers for sending data to the client and receiving
    char sendLine[BUF_SIZE];
    char receiveLine[BUF_SIZE];

    char fileName[BUF_SIZE];
    int numOfPartitions = 0;

    snprintf(sendLine, sizeof(sendLine), "Input either Create, Read, or Delete Command.  Type 'exit' to leave server or 'stop server' to stop the server.\n");
    write(connectionToClient, sendLine, strlen(sendLine));

    // Get first command
    if((bytesReadFromClient = read(connectionToClient, receiveLine, BUF_SIZE)) > 0) {

        // Need to put a NULL string terminator at end
        receiveLine[bytesReadFromClient] = 0;

        // Show what client sent
        printf("Received: %s\n", receiveLine);

        if(strcmp(receiveLine, "stop server\n") == 0) {
            snprintf(sendLine, sizeof(sendLine), "Stopping server...\n");
            write(connectionToClient, sendLine, strlen(sendLine));
            stopConnection = 1;
            closeConnection();
        } else if(strcmp(receiveLine, "exit\n") == 0) {
            printf("Closing connection");
            snprintf(sendLine, sizeof(sendLine), "Closing connection...\n");
            write(connectionToClient, sendLine, strlen(sendLine));
            stopConnection = 1;
        }
        else {
            char commandType[BUF_SIZE];
            char * token;

            tokenizeCommand(receiveLine, commandType, token, fileName, &numOfPartitions);

            struct Command command = {fileName, &numOfPartitions, connectionToClient };

            //CREATING
            if(strcmp(commandType, "create") == 0) {
                if(createFile(&command) == 1){
                    snprintf(sendLine, sizeof(sendLine), "Created file %s\n", command.fileName);
                    write(connectionToClient, sendLine, strlen(sendLine));
                } else {
                    snprintf(sendLine, sizeof(sendLine), "Sorry, unable to create file: %s\n", command.fileName);
                    write(connectionToClient, sendLine, strlen(sendLine));
                }
            }
                // READING
            else if(strcmp(commandType, "read") == 0) {
                snprintf(sendLine, sizeof(sendLine), "Reading File: %s\n", command.fileName);
                write(connectionToClient, sendLine, strlen(sendLine));

                if(readFile(&command) == 1) {
                    snprintf(sendLine, sizeof(sendLine), "-End of File-\n");
                    write(connectionToClient, sendLine, strlen(sendLine));
                } else {
                    snprintf(sendLine, sizeof(sendLine), "Sorry, unable to read file: %s\n", command.fileName);
                    write(connectionToClient, sendLine, strlen(sendLine));
                }
            }

                // DELETING
            else if(strcmp(commandType, "delete") == 0) {
                snprintf(sendLine, sizeof(sendLine), "Deleting file: %s\n", command.fileName);
                write(connectionToClient, sendLine, strlen(sendLine));

                if(deleteFile(&command) == 1){
                    snprintf(sendLine, sizeof(sendLine), "Deleted file: %s successfully\n", command.fileName);
                    write(connectionToClient, sendLine, strlen(sendLine));
                } else {
                    snprintf(sendLine, sizeof(sendLine), "Sorry, was unable to delete file: %s\n", command.fileName);
                    write(connectionToClient, sendLine, strlen(sendLine));
                }
            }


        }
    }

    if(stopConnection == 1) {
        close(connectionToClient);
    } else {
        processInputFromClient(&connectionToClient);
    }

}

void connectToClient() {

    int connectionToClient = 0;

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


    if(listen(serverSocket, 10) == 0) {
        // Listen and queue up to 10 connections
        int i = 0;
        pthread_t processInputThread[10];
        while(connectionToClient = accept(serverSocket, (struct sockaddr *) NULL, NULL)) {
            int * clientPointer = &connectionToClient;
            char sendLine[BUF_SIZE];

            snprintf(sendLine, sizeof(sendLine), "Connected to Server on port: %d\n", SERVER_PORT);
            write(connectionToClient, sendLine, strlen(sendLine));

            snprintf(sendLine, sizeof(sendLine), "Client ID: %d\n", connectionToClient);
            write(connectionToClient, sendLine, strlen(sendLine));

            printf("Connection to client %d started\n", connectionToClient);

            if(pthread_create(&processInputThread[i++], NULL, processInputFromClient, (void*)clientPointer) != 0) {
                printf("Failed to create thread");
            }

            if(i >= 10) {
                i = 0;
                while(i < 10) {
                    pthread_join(processInputThread[i++], NULL);
                }
                i = 0;
            }
        }
    }


    //closeConnection();
}

int main(int argc, char * argv[]) {
    connectToClient();
}