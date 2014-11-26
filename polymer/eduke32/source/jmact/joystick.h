
#ifndef __joystick_h
#define __joystick_h

#ifdef __cplusplus
extern "C" {
#endif

#if defined(GEKKO)
#define WII_A           0x00000001
#define WII_B           0x00000002
#define WII_1           0x00000004
#define WII_2           0x00000008
#define WII_MINUS       0x00000010
#define WII_PLUS        0x00000020
#define WII_HOME        0x00000040
#define WII_Z           0x00000080
#define WII_C           0x00000100
#define WII_X           0x00000200
#define WII_Y           0x00000400
#define WII_FULL_L      0x00000800
#define WII_FULL_R      0x00001000
#define WII_ZL          0x00002000
#define WII_ZR          0x00004000
#define WII_DPAD_UP     0x00008000
#define WII_DPAD_RIGHT  0x00010000
#define WII_DPAD_DOWN   0x00020000
#define WII_DPAD_LEFT   0x00040000
#endif

#define HAT_CENTERED    0x00
#define HAT_UP          0x01
#define HAT_RIGHT       0x02
#define HAT_DOWN        0x04
#define HAT_LEFT        0x08
#define HAT_RIGHTUP	    (HAT_RIGHT|HAT_UP)
#define HAT_RIGHTDOWN   (HAT_RIGHT|HAT_DOWN)
#define HAT_LEFTUP      (HAT_LEFT|HAT_UP)
#define HAT_LEFTDOWN    (HAT_LEFT|HAT_DOWN)

int32_t	JOYSTICK_GetButtons( void );
int32_t	JOYSTICK_ClearButton( int32_t b );
void	JOYSTICK_ClearAllButtons( void );

int32_t	JOYSTICK_GetHat( int32_t h );
void	JOYSTICK_ClearHat( int32_t h );
void	JOYSTICK_ClearAllHats( void );

int32_t	JOYSTICK_GetAxis( int32_t a );
void	JOYSTICK_ClearAxis( int32_t a );
void	JOYSTICK_ClearAllAxes( void );

#ifdef __cplusplus
}
#endif
#endif /* __joystick_h */
