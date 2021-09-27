/**This header file is intended to house the parameters that define
 * key parameters in the system. 
 * 
 * There are two types of constants:
 * 
 * 1. Ones that are meant to be modified by other users of the
 *    scheduling system. These are marked as "modifiable".
 * 
 * 2. The rest of the constants can be modified if you are 
 *    modifying the code itself, but these should be left alone
 *    if you are not doing this. 
*/


/* Table constants */

// Modifiable table constants

#define TABLE_SIZE 23  // Number of individual table entries


/* Scheduling lists constants */

// Modifiable scheduling list constants

# define QUEUE_SIZE 5  // Max amount of tasks can be allocated in the scheduler


/* Packet constants */

// Modifiable packet constants

#define MAX_PAYLOAD_SIZE 25

// Immutable packet constants

// Packet size constants

#define ENCODED_HDR_SIZE         5
#define DECODED_HDR_SIZE         4
#define MAX_ALLOWED_PKT_SIZE     255
#define MAX_DECODED_PKT_BUF_SIZE DECODED_HDR_SIZE + MAX_PAYLOAD_SIZE
#define MAX_ENCODED_PKT_BUF_SIZE ENCODED_HDR_SIZE + MAX_PAYLOAD_SIZE + 1  // A

// Packet offsets (these offsets assume the packet is not COBS encoded)

#define CRC16_OFFSET      0
#define TASK_ID_OFFSET    2
#define TASK_TYPE_OFFSET  3
#define PAYLOAD_OFFSET    4


/* Scheduler constants */

// Modifiable Public scheduler constants

#define MAX_SEND_TYPE double  // Biggest C primitive data type the MODIFY_TASK_VAL command can send

// Task time range for the other system to reply

#define SHORT_TIMER 350  // Allowed time for the first reply time range
#define LONG_TIMER  500  // Allowed time for the second reply time range

/* Immutable Scheduler constants */

// Task types

#define INTERNAL_TASK 0
#define EXTERNAL_TASK 1

// Internal command ids

#define ALERT_SYSTEM     0
#define PRINT_MESSAGE    1
#define UNSCHEDULE_TASK  2
#define MODIFY_TASK_VAL  3
#define PKT_DECODE       4
#define PKT_ENCODE       5
#define TASK_LOOKUP      6
#define TASK_REGISTER    7
