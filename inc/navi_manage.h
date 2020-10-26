#ifndef NAVI_MANAGE_h
#define NAVI_MANAGE_h

#ifdef __cplusplus
extern "C" {
#endif


extern char  GLOBAL_STATUS;
extern char  GLOBAL_SWITCH;
#define STANDBY_STATUS 1
#define ROTATE_STATUS 2
#define MOVE_STATUS  3
#define AVOIDOBJ_STATUS 4
#define WAYPOINTARRIVE_STATUS 5
#define STOP_STATUS 6
#define MANUAL_STATUS 7

#define SUBSUMPTION_INTERVAL 100
#define LED_BLINK_INTERVAL 1000
#define CALCULATE_GPS_HEADING_INTERVAL 1000

extern char  GLOBAL_STATUS ;
extern unsigned long millis();
extern   void RotateDegrees(int degrees);
extern   void MoveDistance(int meters);
extern  void *navimanage_handle (void *arg);

#ifdef __cplusplus
}
#endif

#endif
