
#ifndef __joystick_h
#define __joystick_h



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
int32_t	JOYSTICK_GetGameControllerButtons( void );
int32_t	JOYSTICK_ClearButton( int32_t b );
void	JOYSTICK_ClearGameControllerButton( int32_t b );
void	JOYSTICK_ClearAllButtons( void );

int32_t	JOYSTICK_GetHat( int32_t h );
void	JOYSTICK_ClearHat( int32_t h );
void	JOYSTICK_ClearAllHats( void );

int32_t	JOYSTICK_GetAxis( int32_t a );
void	JOYSTICK_ClearAxis( int32_t a );
void	JOYSTICK_ClearAllAxes( void );

#endif /* __joystick_h */
