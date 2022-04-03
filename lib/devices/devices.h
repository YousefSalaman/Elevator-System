#ifndef DEVICES_H
#define DEVICES_H

#include <stdlib.h>

#include <scheduler.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Device constants */

// Registering task numbers

#define REGISTER_PLATFORM       255
#define REGISTER_TRACKER        254
#define REGISTER_DEVICE         253
#define REGISTER_TESTER_TASK    252
#define ADD_DEVICE_ATTR         251
#define ALERT_SETUP_COMPLETION  250
#define UPDATE_DEVICE_ATTR_COMP 249
#define UPDATE_DEVICE_ATTR_MCU  248


/* Device objects and helper types */

typedef void (*set_dev_attr_cb) (uint8_t *);

typedef void (*deinit_dev_cb) (device_t *, uint8_t);


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

// Alert setup completion to the computer
#define alert_setup_completion() schedule_normal_task(ALERT_SETUP_COMPLETION, (uint8_t []){0}, sizeof(uint8_t))

// Shorthand to extract device_t obj from the device tracker with a given device id
#define get_device_obj(device_tracker, device_id) (device_tracker).devices[device_id]

// Shorthand to extract and cast a specific device object from a device tracker
#define get_device(device_type, device_tracker, device_id) (device_type *) ((device_tracker).devices[device_id].device)

void init_device_trackers(uint8_t count);
void register_platform(char * platform_name);
void register_device_tracker(char * name, uint8_t tracker_id, uint8_t device_count, size_t device_size, deinit_dev_cb deinit_cb, set_dev_attr_cb set_attr_cb);
void add_device_attributes(uint8_t tracker_id, char * attrs[], uint8_t array_size);
void register_device_task(char * name, uint8_t id, uint8_t payload_size, task_t task, uint8_t priority_type);
void deinit_devices(void);
device_tracker_t * get_tracker(uint8_t tracker_id);
void deinit_null_cb(device_t * , uint8_t );
void set_null_attr_cb(uint8_t * );

#ifdef __cplusplus
}
#endif

#endif