#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include "LeapC.h"
#include "ExampleConnection.h"

int64_t lastFrameID = 0; //The last frame received
int64_t pinchFrameID = 0; //The frame when fingers became pinched
LEAP_VECTOR pinch_pos; // Position of the hand when the fingers became pinched
LEAP_VECTOR pinch_norm; //Normal from the palm at the time of pinching
LEAP_VECTOR normVector = {0,0,0};
LEAP_VECTOR prevPosition;
LEAP_VECTOR prevNormal;
bool pinching;

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
LEAP_VECTOR computeDirectionVector(LEAP_VECTOR currPosition, LEAP_VECTOR currNormal) {
    LEAP_VECTOR directionVector = {
        currPosition.x - pinch_pos.x,
        currPosition.y - pinch_pos.y,
        currPosition.z - pinch_pos.z
    };

    LEAP_VECTOR normalVector = {
        currNormal.x - pinch_norm.x,
        currNormal.y - pinch_norm.y,
        currNormal.z - pinch_norm.z
    };
    
    LEAP_VECTOR normalizedVector = normalizeVector(directionVector);
    // printf("\r DirVec: [%.2f, %.2f, %.2f] NorVec: [%.2f, %.2f, %.2f]", normalizedVector.x, normalizedVector.y, normalizedVector.z, normalVector.x, normalVector.y, normalVector.z);
    fflush(stdout);
    millisleep(100);

    return normalizedVector;
}

int main(int argc, char** argv) {
  OpenConnection();
  while(!IsConnected)
    millisleep(100); //wait a bit to let the connection complete

  printf("Connected.");
  LEAP_DEVICE_INFO* deviceProps = GetDeviceProperties();
  if(deviceProps)
    printf("Using device %s.\n", deviceProps->serial);

  for(;;){
    LEAP_TRACKING_EVENT *frame = GetFrame();
    if(frame && (frame->tracking_frame_id > lastFrameID)){
      lastFrameID = frame->tracking_frame_id;
      millisleep(80);
      for(uint32_t h = 0; h < frame->nHands; h++){
        LEAP_HAND* hand = &frame->pHands[h];
        printf("Dir: [%.2f,%.2f,%.2f] \r", hand->palm.direction.x, hand->palm.direction.y, hand->palm.direction.z);
        if (!pinching && hand->pinch_strength > 0.87){
          pinch_pos = hand->palm.position;
          pinch_norm = hand->palm.normal;
          pinching = true;
          prevPosition = pinch_pos;
          // pinchFrameID = lastFrameID;
        }

        if (pinching && hand->pinch_strength < 0.86){
          pinching = false;
        }

        if (pinching){
          // millisleep(500);
          LEAP_VECTOR currPosition = hand->palm.position;
          LEAP_VECTOR currNormal = hand->palm.normal;
          int posDiff = abs(currPosition.x - prevPosition.x) + abs(currPosition.y - prevPosition.y) + abs(currPosition.z - prevPosition.z);
          int normDiff = abs(currNormal.x - prevNormal.x) + abs(currNormal.y - prevNormal.y) + abs(currNormal.z - prevNormal.z);
          // printf("\r%d\n", posDiff);
          
          if (posDiff > 12 || normDiff > 3){
            computeDirectionVector(currPosition, currNormal);
            
            prevPosition = currPosition;
            prevNormal = currNormal;
          }
          
        }

      }
    }
  } //ctrl-c to exit
  return 0;
}
//End-of-Sample
