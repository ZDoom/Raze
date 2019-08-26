
#ifndef __serial_h__
#define __serial_h__

extern short bSendBye;

void UpdateSerialInputs();
void ClearSerialInbuf();
void HangUp();
void UnInitSerial();

#endif
