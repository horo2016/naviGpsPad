#ifndef CHECK_DIS_MODULE_H
#define CHECK_DIS_MODULE_H

#ifdef __cplusplus
extern "C" {
#endif

extern void *getUltrasonicThread(void *arg);
extern void car_stop();
extern void car_forward();
extern void turn_left();
extern void turn_right();

extern int global_dis;




#ifdef __cplusplus
}
#endif

#endif 
