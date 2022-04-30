#ifndef DEVICES_H
#define DEVICES_H

#include <stdlib.h>

#include <scheduler.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Device constants */

// Priority types for testers

#define NORMAL   0
#define PRIORITY 1
#define FAST     2

// Registering task numbers

#define REGISTER_PLATFORM       255
#define REGISTER_TRACKER        254
#define REGISTER_DEVICE         253
#define REGISTER_TESTER_TASK    252
#define ADD_DEVICE_ATTR         251
#define ALERT_SETUP_COMPLETION  250
#define COMP_SETUP_COMPLETE     249
#define UPDATE_DEVICE_ATTR_COMP 248
#define UPDATE_DEVICE_ATTR_MCU  247


// Attribute type constants

#define ATTR_SIZE_T      PRINT_SIZE_T    // Sends size_t variable
#define ATTR_SSIZE_T     PRINT_SSIZE_T   // Sends ssize_t variable
#define ATTR_INT8_T      PRINT_INT8_T    // Sends signed char and expects 1 byte
#define ATTR_UINT8_T     PRINT_UINT8_T   // Sends unsigned char and expects 1 byte
#define ATTR_INT16_T     PRINT_INT16_T   // Sends signed short and expects 2 bytes
#define ATTR_UINT16_T    PRINT_UINT16_T  // Sends unsigned short and expects 2 bytes
#define ATTR_INT32_T     PRINT_INT32_T   // Sends signed integer and expects 4 bytes
#define ATTR_UINT32_T    PRINT_UINT32_T  // Sends unsigned integer and expects 4 bytes
#define ATTR_INT64_T     PRINT_INT64_T   // Sends signed long long and expects 8 bytes
#define ATTR_UINT64_T    PRINT_UINT64_T  // Sends unsigned long long and expects 8 bytes
#define ATTR_FLOAT16_T   PRINT_FLOAT16_T // Sends half-precision float and expects 2 bytes
#define ATTR_FLOAT32_T   PRINT_FLOAT32_T // Sends single-precision float and expects 4 bytes
#define ATTR_FLOAT64_T   PRINT_FLOAT64_T // Sends double-precision float and expects 8 bytes
#define ATTR_BOOL        PRINT_BOOL      // Sends the correspoding boolean value and expects 1 byte
#define ATTR_CHAR        PRINT_CHAR      // Sends the corresponding ASCII/Unicode character and expects 1 byte

/* Device objects and helper types */

/**Device container object
 * 
 * This object contains information to access its counterpart 
 * on the computer and modify values over there. Aside from
 * this, it contains the device itself, so this object acts
 * as a container to specific device objects with addtional
 * information.
 */ 
typedef struct
{
    void * device;
    uint8_t device_id;
    uint8_t tracker_id;
    
} device_t;


typedef void (*set_dev_attr_cb) (uint8_t *);

typedef void (*deinit_dev_cb) (device_t *, uint8_t);


/**Device tracker object
 * 
 * A device tracker contains an array with device objects and stores
 * information related to the tracker itself. This includes how to
 * update different parts of the device attributes throught the set
 * attribute callback and the callback that uninitializes the system's
 * objects.
 */
typedef struct
{
    uint8_t count;                 // Amount of a device in device tracker
    device_t * devices;            // Array with the devices
    deinit_dev_cb deinit_cb;       // Destructor for an individual device
    set_dev_attr_cb set_attr_cb;   // Task to update simple device attributes
    
} device_tracker_t;


/* Device methods */

// Shorthand to extract device_t obj from the device tracker with a given device id
#define get_device_obj(device_tracker, device_id) (device_tracker)->devices[device_id]

// Shorthand to extract and cast a specific device object from a device tracker
#define get_device(device_tracker, device_id) (device_tracker)->devices[device_id].device

// Setup device methods

void init_device_trackers(uint8_t count);
void register_platform(const char * platform_name);
void register_device_tracker(const char * name, uint8_t tracker_id, uint8_t device_count, deinit_dev_cb deinit_cb, set_dev_attr_cb set_attr_cb);
void add_device_attrs(uint8_t tracker_id, const char * attrs[], uint8_t array_size);
void create_device_instances(uint8_t tracker_id);
bool is_comp_setup_complete(void);
bool is_comp_device_setup_complete(uint8_t tracker_id);
void _register_device_task(const char * name, uint8_t id, uint8_t payload_size, task_t task, uint8_t priority_type);

#define alert_setup_completion() schedule_fast_task(ALERT_SETUP_COMPLETION, EXTERNAL_TASK, NULL, 0)  // Alert setup completion to the computer
#define register_device_task(name, id, payload_size, task, priority_type) _register_device_task(name, id, payload_size, (task_t) task, priority_type)

// Miscellaneous device methods

void deinit_devices(void);
bool device_initialized(uint8_t tracker_id);
device_tracker_t * get_tracker(uint8_t tracker_id);
void deinit_null_cb(device_t * , uint8_t );
void set_null_attr_cb(uint8_t * );

#ifdef __cplusplus
}
#endif

#endif