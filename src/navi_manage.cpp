#include "navi_manage.h"


#include "osp_syslog.h"

#include <time.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <ctype.h>
#include "PID_v1.h"
#include "kalman.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/wait.h>
//#include "include.h"
#include "geocoords.h"
#include "gps.h"
#include "imu.h"
#include <stdexcept>
#include <errno.h>
#include <RTIMULib.h>
#include"stm32_control.h"

#define MAX_SPEED 150
#define NORMAL_SPEED 30


char  GLOBAL_STATUS =0;

float rollOffset = 0.0; //-0.4;
float pitchOffset = 0.0; // 3.3;


Kalman headingFilter(0.125, 4, 1, 0);

// PID controller variables for heading
double targetHeading, headingPIDInput, headingPIDOutput;
PID headingPID(&headingPIDInput, &headingPIDOutput, &targetHeading,1,0,0, DIRECT);





GeoCoordinate waypoints[256];
int waypointCount = 0;
int currentWaypoint = 0;
double waypointRange = 0.0;


float voltage1;
unsigned long voltage1_t;
float voltage2;
unsigned long voltage2_t;

bool voltageHysteresis = 0;

/*******************************************************************************
* function name	: ReadWaypointsFile
* description	: from file waypoint.dat read datas from internet ,format :lat|long  
*				  call heatbeat main func
* param[in] 	: task_table[4]
* param[out] 	: none
* return 		: none
 *******************************************************************************/

void ReadWaypointsFile()
{
    FILE *waypointFile = fopen("waypoints.dat", "r");
    if (waypointFile == 0)
	return;

    waypointCount = 0;
    while (!feof(waypointFile))
    {
	char line[256];
	fgets(line, 256, waypointFile);
	const char *wpLat = strtok(line, "|");
	const char *wpLong = strtok(0, "|");
	if (wpLat && wpLong)
	{
	    GeoCoordinate waypoint(wpLat, wpLong);
	    waypoints[waypointCount] = waypoint;
	    waypointCount++;
	}
    }
    fclose(waypointFile);
}
// Quick and dirty function to get elapsed time in milliseconds.  This will wrap at 32 bits (unsigned long), so
// it's not an absolute time-since-boot indication.  It is useful for measuring short time intervals in constructs such
// as 'if (lastMillis - millis() > 1000)'.  Just watch out for the wrapping issue, which will happen every 4,294,967,295
// milliseconds - unless you account for this, I don't recommend using this function for anything that will cause death or
// disembowelment when it suddenly wraps around to zero (e.g. avionics control on an aircraft)...
unsigned long millis()
{
    struct timeval tv;
    gettimeofday(&tv, 0);
    unsigned long count = tv.tv_sec * 1000000 + tv.tv_usec;
    return count / 1000;
}

/*****
 *    判断是否在范围内
 * @param raduis    圆的半径
 * @param lat       点的纬度
 * @param lng       点的经度
 * @param lat1      圆的纬度
 * @param lng1      圆的经度
 * @return  
 */
 char isInRange(int raduis,double present_lat,double present_lng,double lat_circle,double lng_circle){
    double R = 6378137.0;
     float const PI_F = 3.14159265F;
    double dLat = (lat_circle- present_lat ) * M_PI / 180;
    double dLng = (lng_circle - present_lng )* M_PI / 180;
    double a = sin(dLat / 2) * sin(dLat / 2) + cos(present_lat * PI / 180) * cos(lat_circle* PI / 180) * sin(dLng / 2) * sin(dLng / 2);
    double c = 2 * atan2(sqrt(a), sqrt(1 - a));
    double d = R * c;
    double dis = round(d);//近似值
    if (dis <= raduis){  //点在圆内
        return 1;
    }else {
        return 0;
    }
}
 
 // Initialization stuff - open and configure the serial device, etc.
 void Setup()
 {

 
     // Set up the PID controllers for heading and wall following
     headingPIDInput = 0;
     headingPID.SetOutputLimits(-NORMAL_SPEED, NORMAL_SPEED);
     headingPID.SetMode(AUTOMATIC);
 
  
 }
 
 float prevHeading = 0;
 
/*******************************************************************************
* function name	: SteerToHeading
* description	: heartbeat function ,if receive new data ,clear counter,or,
*				  call heatbeat main func
* param[in] 	: task_table[4]
* param[out] 	: none
* return 		: none
*
* Steer to heading subsumption task.  If active and not subsumed by a higher priority task, 
* this will set the motor speeds
* to steer to the given heading (targetHeading)
 *******************************************************************************/
 void SteerToHeading()
 {
     // Filter the mag data to eliminate noise
 //    xFilter.update(magX);
 //    magX = xFilter.GetValue();
 //    yFilter.update(magY);
 //    magY = yFilter.GetValue();
 //    zFilter.update(magZ);
 //    magZ = zFilter.GetValue();
 
     // Do the same with the accelerometer data
 //    xAccFilter.update(accelX);
 //    accelX = xAccFilter.GetValue();
 //    yAccFilter.update(accelY);
 //    accelY = yAccFilter.GetValue();
 //    zAccFilter.update(accelZ);
 //    accelZ = zAccFilter.GetValue();
 
     float filteredHeading = heading;
     float adjustedHeading = filteredHeading;
 
     // Deal with the 0 == 360 problem
     float diff = targetHeading - filteredHeading;
     if (diff > 180)
     adjustedHeading += 360;
     else if (diff < -180)
     adjustedHeading -= 360;
 
 
     // If we've just crossed the 0/360 boundary, reset the filter so the compass updates immediately
     // instead of waiting for the filter to wrap around and catch up
     if (filteredHeading < 90 && prevHeading > 270)
     headingFilter.reset(0.125, 4, 1, 0);
     else if (filteredHeading > 270 && prevHeading < 90)
     headingFilter.reset(0.125, 4, 1, 360);
     prevHeading = filteredHeading;
 

 
     headingFilter.update(filteredHeading);
     filteredHeading = headingFilter.GetValue();
	 
     headingPIDInput = adjustedHeading;//filteredHeading;  // adjustedHeading ?
     headingPID.Compute();
 #if 0 
     printf("\033[2J");
     printf("\033[H");
     printf("Roll Angle:  %c%3.1f degrees\n", rollAngle + rollOffset < 0 ? '\0' : ' ', rollAngle + rollOffset);
     printf("Pitch Angle: %c%3.1f degrees\n", pitchAngle + pitchOffset < 0 ? '\0' : ' ', pitchAngle + pitchOffset);
     printf("Raw Heading:         %f\n", heading);

     printf("\033[1mFiltered Heading:    %d\033[0m \n", (int)filteredHeading);
     printf("\033[1mTarget Heading:      %d\033[0m \n", (int)targetHeading);
     printf("PID error:           %d\n", (int)headingPIDOutput);
#endif
   //  fprintf(outFile, "Roll Angle:  %c%3.1f degrees\n",  rollAngle + rollOffset < 0 ? ' ' : ' ', rollAngle + rollOffset);
   //  fprintf(outFile, "Pitch Angle: %c%3.1f degrees\n",  pitchAngle + pitchOffset < 0 ? ' ' : ' ', pitchAngle + pitchOffset);
    // fprintf(outFile, "Heading: %d\n", (int)filteredHeading);
    // fprintf(outFile, "Target Heading: %d\n", (int)targetHeading);
   //  fprintf(outFile, "PID error:           %d\n", (int)headingPIDOutput);
 
 
  //   steerToHeadingControl->leftMotorPower = NORMAL_SPEED - headingPIDOutput;
   //  steerToHeadingControl->rightMotorPower = NORMAL_SPEED + headingPIDOutput;
 
  //   steerToHeadingControl->active = steerToHeadingMode;
 }
 
 /*******************************************************************************
 * function name : DetectObstacles
 * description   : heartbeat function ,if receive new data ,clear counter,or,
 *                 call heatbeat main func
 * param[in]     : task_table[4]
 * param[out]    : none
 * return        : none
 *
 * Detect obstacles subsumption task.  If an obstacle is detected, set active flag to subsume all other tasks.  This
 * will generally be the highest priority task (except for manual control), since we always want to avoid obstacles
 * regardless of what other tasks are active.
 * return degree 
 *******************************************************************************/

 int  DetectObstacles()
 {
     char ret = -1;
     // Need to set the servo to LEFT, CENTER, or RIGHT, then wait a few hundres ms for it to get there, then grab the
     // distance reading.  Reading the distance while the servo is moving will generate too much noise.
     //
     // ...
     //
     // This doesn't do anything currently
     // TODO:  Do something useful here
     //
 //    int distanceAhead = distance1;
 //    if (distanceAhead > 4 && distanceAhead < 40 ) // cm
 //    {
 //        detectObstaclesControl->active = true;
 //    }
 //    else
      //   detectObstaclesControl->active = false;

      //如果存在障碍物，返回避障动作,及角度值
 }
  /*******************************************************************************
 * function name : CalculateHeadingToWaypoint
 * description   : 计算当前距离到航点的角度
 *                 call heatbeat main func
 * param[in]     : task_table[4]
 * param[out]    : none
 * return        : none
 *
 *******************************************************************************/
 void CalculateHeadingToWaypoint()
 {
     GeoCoordinate current(latitude, longitude);
     GeoCoordinate waypoint = waypoints[currentWaypoint];
 
     // getBearing() expects its waypoint coordinates in radians
     waypoint.latitude = waypoint.latitude * PI / 180.0;
     waypoint.longitude = waypoint.longitude * PI / 180.0;
 
     // targetHeading is the value used by the heading PID controller.  By changing this, we change the heading
     // to which the SteerToHeading subsumption task will try to steer us.
     targetHeading = getBearing(current, waypoint);
 
     return;
 }
   /*******************************************************************************
 * function name : CalculateDistanceToWaypoint
 * description   : caclulate  到航向点的距离distance
 *                 call heatbeat main func
 * param[in]     : task_table[4]
 * param[out]    : none
 * return        : none
 *
 *******************************************************************************/
 void CalculateDistanceToWaypoint()
 {
     GeoCoordinate current(latitude, longitude);
     GeoCoordinate waypoint = waypoints[currentWaypoint];
 
     // getDistance() expects its waypoint coordinates in radians
     waypoint.latitude = waypoint.latitude * PI / 180.0;
     waypoint.longitude = waypoint.longitude * PI / 180.0;
 
     // targetHeading is the value used by the heading PID controller.  By changing this, we change the heading
     // to which the SteerToHeading subsumption task will try to steer us.
     waypointRange = getDistance(current, waypoint);
 
     if (waypointRange < 0.0030) // 3.0 meters
     currentWaypoint++;
     if (currentWaypoint >= waypointCount)
     currentWaypoint = 0;
 
     return;
 }
 
 /*******************************************************************************
 * function name : rotateDegreesThread
 * description   : caclulate  rotate Degrees Thread
 *                 call heatbeat main func
 * param[in]     : task_table[4]
 * param[out]    : none
 * return        : none
 *
 *******************************************************************************/
 static void *rotateDegreesThread(void *threadParam)
 {
     // Make sure there's only one rotate thread running at a time.
     // TODO: proper thread synchronization would be better here
     static bool threadActive = false;
     if (threadActive)
     return 0;
     threadActive = true;
 
     int degrees = *(int*)threadParam;
     free(threadParam);  // Must have been malloc()'d by the caller of this thread routine!!
 
    printf("rotateDegreesThread  start 8****************\n");
     int startHeading = headingFilter.GetValue();
     printf("startHeading %d  \n",startHeading);
     int targetHeading = startHeading + degrees;
     if (targetHeading < 0)
     targetHeading += 360;
     if (targetHeading > 359)
     targetHeading -=360;
     char  done = 0;
	  printf("targetHeading %d ,degrees:%d \n",targetHeading,degrees);
  do{
	     if (degrees < 0)
	     {
	  
	        cmd_send(3,0);
	     }
	     else
	     {

	         cmd_send(4,0);
	     }

     // Backup method - use the magnetometer to see what direction we're facing.  Stop turning when we reach the target heading.
	     int currentHeading = (int)heading;//headingFilter.GetValue();
	     printf("Rotating: currentHeading = %d   targetHeading = %d\n", currentHeading, targetHeading);
	     if ((currentHeading >= targetHeading) && (degrees > 0) && (startHeading < targetHeading))
	     {
	         done = 1;
	     }
	     if ((currentHeading <= targetHeading) && (degrees < 0) && (startHeading > targetHeading))
	     {
	         done = 1;
	     }
	     if (currentHeading < startHeading && degrees > 0)
	         startHeading = currentHeading;
	     if (currentHeading > startHeading && degrees < 0)
	         startHeading = currentHeading;
 
     usleep(100000);
     }
     while (!done);
    cmd_send(0,0);
     threadActive = 0;
     return 0;
 }
 
 /*******************************************************************************
 * function name : rotateDegreesThread
 * description   : caclulate  rotate Degrees Thread
 *                 call heatbeat main func
 * param[in]     : task_table[4]
 * param[out]    : none
 * return        : none
 *
 *******************************************************************************/
 static void *moveDistanceThread(void *threadParam)
 {
     // Make sure there's only one rotate thread running at a time.
     // TODO: proper thread synchronization would be better here
     static bool threadActive = false;
     if (threadActive)
     return 0;
     threadActive = true;
 
     int meters = *(int*)threadParam;
     free(threadParam);  // Must have been malloc()'d by the caller of this thread routine!!
 
    printf("moveDistanceThread  start 8****************\n");
     int startPosition = positionx;
     printf("startPosition %d  \n",startPosition);
     int targetPosition = startPosition + meters;
    
     char  done = 0;
	printf("targetHeading %d ,meters:%d \n",targetPosition,meters);
  do{
  	 
	     if (meters < 0)
	     {
	            cmd_send(1,0);
	     }
	     else
	     {
	           if(targetPosition - (int)positionx > 20)
	            cmd_send(2,50);
	           else  cmd_send(2,30);
	     }


     // Backup method - use the magnetometer to see what direction we're facing.  Stop turning when we reach the target heading.
     int currentPosition = (int)positionx;
     printf("MOve: currentPosition = %d   targetHeading = %d\n", currentPosition, targetPosition);
     if ((currentPosition <= targetPosition) && (meters < 0) && (startPosition > targetPosition))
     {
         done = 1;
     }
     if ((currentPosition >= targetPosition) && (meters > 0) && (startPosition < targetPosition))
     {
         done = 1;
     }

     usleep(10000);
     }
     while ((!done)&&(GLOBAL_STATUS == MOVE_STATUS));
    cmd_send(0,0);
     threadActive = 0;
     return 0;
 }
 
 /*******************************************************************************
 * function name : RotateDegrees
 * description   : caclulate  rotate Degrees  N DEGREE 
 *                 call heatbeat main func
 * param[in]     : task_table[4]
 * param[out]    : none
 * return        : none
 *
 *******************************************************************************/
  void RotateDegrees(int degrees)
 {
     int *rotationDegrees = (int *)malloc(sizeof(int));
     *rotationDegrees = degrees;
     pthread_t rotThreadId;
     pthread_create(&rotThreadId, NULL, rotateDegreesThread, rotationDegrees);
 }

 /*******************************************************************************
 * function name : MoveDistance
 * description   : caclulate    distance  N meters 
 *                 call heatbeat main func
 * param[in]     : task_table[4]
 * param[out]    : none
 * return        : none
 *
 *******************************************************************************/
  void MoveDistance(int meters)
 {
     int *MoviMeters = (int *)malloc(sizeof(int));
     *MoviMeters = meters;
     pthread_t rotThreadId;
     pthread_create(&rotThreadId, NULL, moveDistanceThread, MoviMeters);
 }

/*******************************************************************************
* function name	: loop_handle
* description	: heartbeat function ,if receive new data ,clear counter,or,
*				  call heatbeat main func
* param[in] 	: task_table[4]
* param[out] 	: none
* return 		: none
*******************************************************************************/
void *navimanage_handle (void *arg)
{

	char ret = -1;

   
    unsigned long lastLEDMillis = 0;
    unsigned long lastSubMillis = 0;
    unsigned long lastGPSMillis = 0;
    unsigned long motorsOffMillis = 0;
    bool motorsRunning = false;
 //   ReadWaypointsFile();
    DEBUG(LOG_DEBUG,"enc8888888888888888888888888888888888888 handle \n");
    printf("Starting main loop\n");
    while (1)
     {
         unsigned long loopTime = millis();
       
    	

         
        if ((millis() - lastSubMillis > SUBSUMPTION_INTERVAL))
        {
           ret =  isInRange(3, latitude , longitude, waypoints[currentWaypoint].latitude, waypoints[currentWaypoint].longitude);
           if (ret == 1) //点在圆圈内 
           GLOBAL_STATUS = WAYPOINTARRIVE_STATUS ;
           else GLOBAL_STATUS = STANDBY_STATUS ;
           switch(GLOBAL_STATUS)
           {
              case STANDBY_STATUS:
                lastGPSMillis =0 ;
		   ReadWaypointsFile();
                GLOBAL_STATUS = ROTATE_STATUS ;
                break;
              case ROTATE_STATUS :
           
                RotateDegrees(targetHeading);//这里需要根据求出的角度进行转动
                GLOBAL_STATUS = MOVE_STATUS ;
                break;
              case MOVE_STATUS :
		  MoveDistance(waypointRange);
                    SteerToHeading();//行驶中依然根据航向脚纠偏算法
                    break;
              case AVOIDOBJ_STATUS:
                // 根据超声波获得反馈值进行避障
              //  int tmp_degree = DetectObstacles();
              //    if (tmp_degree)
              //       GLOBAL_STATUS = ROTATE_STATUS ;
              break;
              case WAYPOINTARRIVE_STATUS:
                if(currentWaypoint < waypointCount )
                    {
                    currentWaypoint ++;
                    GLOBAL_STATUS = STANDBY_STATUS ;
                }
                else  if(currentWaypoint >= waypointCount ){
                    GLOBAL_STATUS = STOP_STATUS ;
                }
                break;
              case STOP_STATUS :
                
                break;
              case MANUAL_STATUS :
                break;
              default :
              break;
             
                    
                

           }
     
          lastSubMillis = millis();
         
        }
          if ( millis() - lastGPSMillis > CALCULATE_GPS_HEADING_INTERVAL)
        {
            CalculateHeadingToWaypoint();
	        CalculateDistanceToWaypoint();
            lastGPSMillis = millis();
        }

          	// Shut down if the battery level drops below 10.8V
    	if (voltage1 > 11.2)
    	    voltageHysteresis = 1;
    	if (voltageHysteresis  && voltage1 < 10.8)
    	{
    	    signal(SIGCHLD, SIG_IGN);
    	    long shutdownPID = fork();
    	    if (shutdownPID >= 0)
    	    {
        		if (shutdownPID == 0)
        		{
        		    // Child process
        		    execl(getenv("SHELL"),"sh","-c","sudo shutdown -h now",NULL);
        		    _exit(0);
        		}
    	    }
    	}
        unsigned long now = millis();
    	if (now - loopTime < 1)
            usleep((1 - (now - loopTime)) * 1000);
    }

}
