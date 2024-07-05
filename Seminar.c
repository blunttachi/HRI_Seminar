#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#define close closesocket
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#endif

#include "LeapC.h"
#include "ExampleConnection.h"

#define PORT 30002
#define BUFFER_SIZE 32
#define MOVE_STEP 0.02 // Adjust the step size as needed

int64_t lastFrameID = 0; //The last frame received
LEAP_VECTOR pinch_pos; // Position of the hand when the fingers became pinched
LEAP_VECTOR prevPosition;
bool pinching = false;

// Server variables
int server_fd, new_socket;
struct sockaddr_in address;
int addrlen = sizeof(address);
char buffer[BUFFER_SIZE] = {0};

// Normalize a vector
LEAP_VECTOR normalizeVector(LEAP_VECTOR vector) {
    float magnitude = sqrt(vector.x * vector.x + vector.y * vector.y + vector.z * vector.z);
    if (magnitude == 0) {
        return (LEAP_VECTOR){0, 0, 0};
    }
    return (LEAP_VECTOR){
        vector.x / magnitude,
        vector.y / magnitude,
        vector.z / magnitude
    };
}

// Compute and normalize the direction vector
LEAP_VECTOR computeDirectionVector(LEAP_VECTOR currPosition) {
    LEAP_VECTOR directionVector = {
        currPosition.x - pinch_pos.x,
        currPosition.y - pinch_pos.y,
        currPosition.z - pinch_pos.z
    };
    
    LEAP_VECTOR normalizedVector = normalizeVector(directionVector);
    printf("\r DirVec: [%.2f, %.2f, %.2f]", normalizedVector.x, normalizedVector.y, normalizedVector.z);

    return normalizedVector;
}


#ifdef _WIN32
void initializeWinsock() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }
}
#endif

// Initialize server socket
void initializeServer() {
    #ifdef _WIN32
        initializeWinsock();
    #endif

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket to the port
    address.sin_family = AF_INET;
    // address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

     // Convert IP address from text to binary form
    if (inet_pton(AF_INET, "192.168.40.178", &address.sin_addr) <= 0) {
        perror("Invalid address or Address not supported");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server initialized. Waiting for connections...\n");
}

// Accept client connections
void acceptClient() {
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0) {
        perror("Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Client connected.\n");
}

// Send incremental move command
void sendIncrementalMoveCommand(LEAP_VECTOR increment) {

    char command[BUFFER_SIZE];

    snprintf(command, BUFFER_SIZE, "(%.2f %.2f %.2f)\n", increment.x, increment.z, increment.y);

    if (send(new_socket, command, BUFFER_SIZE , 0) < 0) {
        perror("Send failed");
        close(new_socket);
        close(server_fd);
        exit(EXIT_FAILURE);
    }

}

int main(int argc, char** argv) {
    OpenConnection();
    while(!IsConnected)
        millisleep(100); //wait a bit to let the connection complete

    printf("Connected.");
    LEAP_DEVICE_INFO* deviceProps = GetDeviceProperties();
    if(deviceProps)
        printf("Using device %s.\n", deviceProps->serial);

    // Initialize and accept client connection
    initializeServer();
    acceptClient();

    millisleep(300);

    LEAP_VECTOR empty = {0,0,0};
    sendIncrementalMoveCommand(empty);

    for(;;){
        LEAP_TRACKING_EVENT *frame = GetFrame();
        
        if(frame && (frame->tracking_frame_id > lastFrameID)){
            lastFrameID = frame->tracking_frame_id;
            millisleep(240);
            for(uint32_t h = 0; h < frame->nHands; h++){
                LEAP_HAND* hand = &frame->pHands[h];
               
                if (!pinching && hand->pinch_strength > 0.87){ 
                    pinch_pos = hand->palm.position;
                    pinching = true;
                    prevPosition = pinch_pos;
                }

                if (pinching && hand->pinch_strength < 0.86){
                    pinching = false;
                }

                if (pinching){
                    LEAP_VECTOR currPosition = hand->palm.position;
                    int posDiff = abs(currPosition.x - prevPosition.x) + abs(currPosition.y - prevPosition.y) + abs(currPosition.z - prevPosition.z);
                    
                    if (posDiff > 12){
                        LEAP_VECTOR normalizedVector = computeDirectionVector(currPosition);
                        sendIncrementalMoveCommand(empty);
                        millisleep(160);
                        sendIncrementalMoveCommand(normalizedVector);
                    }

                    prevPosition = currPosition;
                }

                if (!pinching){
                    sendIncrementalMoveCommand(empty);
                }
            }
        }
    } //ctrl-c to exit

    close(new_socket);
    close(server_fd);
    #ifdef _WIN32
        WSACleanup();
    #endif
        return 0;
}
// End-of-Sample