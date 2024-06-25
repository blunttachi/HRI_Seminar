/* Copyright (C) 2012-2017 Ultraleap Limited. All rights reserved.
 *
 * Use of this code is subject to the terms of the Ultraleap SDK agreement
 * available at https://central.leapmotion.com/agreements/SdkAgreement unless
 * Ultraleap has signed a separate license agreement with you or your
 * organisation.
 *
 */

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
LEAP_VECTOR pinch_pos;
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
LEAP_VECTOR computeDirectionVector(LEAP_VECTOR currPosition) {
    LEAP_VECTOR directionVector = {
        currPosition.x - pinch_pos.x,
        currPosition.y - pinch_pos.y,
        currPosition.z - pinch_pos.z
    };
    
    LEAP_VECTOR normalizedVector = normalizeVector(directionVector);
    printf("\Direction Vector: [%f, %f, %f]", directionVector.x, directionVector.y, directionVector.z);
    fflush(stdout);
    Sleep(100);
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
      // printf("Frame %lli with %i hands.\n", (long long int)frame->tracking_frame_id, frame->nHands);s
      for(uint32_t h = 0; h < frame->nHands; h++){
        LEAP_HAND* hand = &frame->pHands[h];
        // printf("\r%s handposition (%.2f, %.2f, %.2f) SpeedX: %.2f SpeedY: %.2f SpeedZ: %.2f Pin. dist.: %.2f  ",
        //   // hand->id,
        //   (hand->type == eLeapHandType_Left ? "left" : "right"),
        //   hand->palm.position.x,
        //   hand->palm.position.y,
        //   hand->palm.position.z,
        //   hand->palm.velocity.x,
        //   hand->palm.velocity.y,
        //   hand->palm.velocity.z,
        //   hand->pinch_strength);

        // fflush(stdout);
        // Sleep(100);

        if (!pinching && hand->pinch_strength > 0.87)
        {
          pinch_pos = hand->palm.position;
          pinching = true;
          printf("position pinch %.2f %.2f %.2f \n", pinch_pos.x, pinch_pos.y, pinch_pos.z );
        }

        if (pinching && hand->pinch_strength < 0.86){
          pinching = false;
        }

        if (pinching){
          computeDirectionVector(hand->palm.position);
          printf("position curr %.2f %.2f %.2f \n", hand->palm.position.x, hand->palm.position.y, hand->palm.position.z );
        }
        
      }
    }
  } //ctrl-c to exit
  return 0;
}
//End-of-Sample
