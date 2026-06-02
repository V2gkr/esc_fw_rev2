#ifndef __CAN_COMM_H
#define __CAN_COMM_H

#define ESC_DATA_FILTER         0x100
#define ESC_TELEMETRY_ID        0x001
#define ESC_TEMP_ID             0x002
#define ESC_VBUS_ID             0x003
#define ESC_ALARMS_ID           0x004

#define CONTROL_BOARD_FILTER    0x200

void CAN_PublishMessages(void);

#endif