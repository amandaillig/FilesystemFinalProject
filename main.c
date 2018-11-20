#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

void getCommandType(char command[], char * commandType) {
    char commandToken[500];
    size_t destination_size = sizeof (commandToken);

    strncpy(commandToken, command, destination_size);

    char * token;

    token = strtok(commandToken, " ");
    if(token != NULL) {
        strcpy(commandType, token);
    }
}


void getCreateParameters(char command[], int * n, char * fileName) {
    char commandToken[500];
    size_t destination_size = sizeof (commandToken);

    strncpy(commandToken, command, destination_size);

    char * token;
    token = strtok(commandToken, " ");
    for(int i = 0; i < 3; i ++) {
        if(token != NULL && strlen(token) > 0) {
            if(i == 1) {
                strcpy(fileName, token);
            } else if(i == 2) {
                token[strlen(token) - 1] = 0;
                *n = *token - '0';
            }
            token = strtok(NULL, " ");
        }
    }
}

void createFileMapping(char * filePath) {

}


char * writeToFile(int partitionCounter, char * fileName, char currentLine[]) {
    //Return partitioned filepath
}

void inputFileContents(int n, char * fileName) {
    int endOfFileContents = 0;
    int partitionCounter = 1;
    int lineNo = 1;
    char* partitionFilePaths[n];

    while(!endOfFileContents) {

        if(partitionCounter > n) {
            partitionCounter = 1;
        }

        char currentLine[500];
        printf("Enter the contents of the file, terminated by an empty line:\n");
        printf("%d: ", lineNo);
        fgets(currentLine, 500, stdin);

        if(strcmp(currentLine, "\n") == 0) {
            endOfFileContents = 1;
        } else {
            partitionFilePaths[partitionCounter - 1] = writeToFile(partitionCounter, fileName, currentLine);
            lineNo++;
            partitionCounter++;
        }
    }
    createFileMapping(partitionFilePaths);
}


 int fileCheck (char* fileName){
    FILE *file;
    if( file = fopen(fileName, "r")){
        printf("The file exists");
        return 1;
    }
    else {
        return 0;
    }
}

void overWrite(char command[]){


}

int main(int argc, char * argv[]) {
    printf("FileSystem Final Project\n");
    int stopLoop = 1;
    //while(stopLoop) {
        printf("Input command:\n");

        char command[500];

        fgets(command, 500, stdin);

        char * commandType;
        getCommandType(command, commandType);

        if(strcmp(commandType, "create") == 0) {
            // Create Command

            int noOfPartitions = 0;
            char * fileName = (char*)malloc(100);

            getCreateParameters(command, &noOfPartitions, fileName);

            inputFileContents(noOfPartitions, fileName);

        } else if(strcmp(commandType, "read") == 0) {
            // Read Command

        } else if(strcmp(commandType, "delete") == 0) {
            // Delete Command

        }
    //}
}