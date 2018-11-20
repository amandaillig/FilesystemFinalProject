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

void fileCheck (char* fileName){
    FILE *file;
    if( file = fopen(filename, "r")){
        printf("The file exists");
        return 1;
    }
    else(return 0);
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

        } else if(strcmp(commandType, "read") == 0) {
            // Read Command

        } else if(strcmp(commandType, "delete") == 0) {
            // Delete Command

        }
    //}
}