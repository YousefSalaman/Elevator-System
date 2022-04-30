
#include <stdlib.h>
#include <string.h>

#include <scheduler.h>

#include "devices.h"



/* Device contants */

// String setter constants

#define STR_NAME_LIMIT   20

// Initialization verification offsets

#define INIT_PLATFORM_OFFSET          0
#define PASS_PLATFORM_OFFSET          1
#define CREATE_TRACKER_OFFSET         2
#define ADD_DEVICE_ATTR_OFFSET        3
#define CREATE_DEVICES_OFFSET         4
#define COMP_SETUP_COMPLETE_OFFSET    5


/* Device variables */

static uint8_t * init_verifier; // Keeps track of what parts of the device setups have been completed

// Device tracker attributes

static uint8_t tracker_count;        // Tracker count in system
static device_tracker_t * trackers;  // Tracker array to access trackers


/* Device private function prototypes */

static void comp_setup_complete(uint8_t * _);
static void update_device_attr_mcu(uint8_t * pkt);
static size_t pass_name_to_pkt(uint8_t * buf, size_t buf_size, const char * name);
static bool setup_device_tracker(uint8_t , uint8_t , deinit_dev_cb , set_dev_attr_cb );


/* Device macro helper functions */

#define get_init_verifier_bit(id, offset) init_verifier[id] & (0x01 << (offset))
#define set_init_verifier_bit(id, offset) init_verifier[id] |= (0x01 << (offset))


/* Public device functions */

// Initializer for the internal setup of the device trackers
void init_device_trackers(uint8_t count)
{
    tracker_count = count;
    init_verifier = malloc(sizeof(uint8_t) * count);
    trackers = malloc(sizeof(device_tracker_t) * count);

    if (trackers != NULL && init_verifier != NULL)
    {
        // Register device tasks
        register_task(COMP_SETUP_COMPLETE, -1, comp_setup_complete);
        register_task(UPDATE_DEVICE_ATTR_MCU, -1, update_device_attr_mcu);

        // Set init platform bits so other parts of the setup can be done
        for (uint8_t i = 0; i < tracker_count; i++)
        {
            set_init_verifier_bit(i, INIT_PLATFORM_OFFSET);
        }
    }
    else // Unintialize device tracker variables if not properly defined
    {   
        free(trackers);
        free(init_verifier);
    }
}

// Pass the platform name to the computer
void register_platform(const char * platform_name)
{
    if (get_init_verifier_bit(0, INIT_PLATFORM_OFFSET))
    {
        size_t name_len;
        uint8_t pkt[STR_NAME_LIMIT];

        // Passes name string to computer
        if (name_len = pass_name_to_pkt(pkt, STR_NAME_LIMIT, platform_name))
        {
            schedule_fast_task(REGISTER_PLATFORM, EXTERNAL_TASK, pkt, name_len);

            // Set pass platform bits so other parts of the setup can be done
            for (uint8_t i = 0; i < tracker_count; i++)
            {
                set_init_verifier_bit(i, PASS_PLATFORM_OFFSET);
            }
        }
        else  // Failed to pass platform name
        {

        }
    }
}

// Setup a device tracker in the MCU and create it on the main computer
void register_device_tracker(const char * name, uint8_t tracker_id, uint8_t device_count, deinit_dev_cb deinit_cb, set_dev_attr_cb set_attr_cb)
{
    if (get_init_verifier_bit(tracker_id, PASS_PLATFORM_OFFSET))
    {
        uint8_t pkt[STR_NAME_LIMIT];
        size_t name_len = pass_name_to_pkt(pkt, STR_NAME_LIMIT, name);

        if (name_len && setup_device_tracker(tracker_id, device_count, deinit_cb, set_attr_cb))
        {
            schedule_fast_task(REGISTER_TRACKER, EXTERNAL_TASK, pkt, name_len);
            set_init_verifier_bit(tracker_id, CREATE_TRACKER_OFFSET);
        }
        else
        {

        }
    }
}

/**Add a device's attributes to the computer
 * 
 * You basically pass an array of the device's attributes that
 * you want the computer to have, and the computer will add these
 * when creating instances of that device. A thing to note is that
 * an attribute id will be registered on the computer, so it can 
 * tell the MCU which attribute to update. The attribute id will
 * be determined by the position you gave it in the attribute
 * array, so it would be wise to mark these down to create the
 * set_attr_cb for a device tracker.
 */ 
void add_device_attrs(uint8_t tracker_id, const char * attrs[], uint8_t array_size)
{
    if (get_init_verifier_bit(tracker_id, CREATE_TRACKER_OFFSET))
    {
        size_t name_len;
        uint8_t pkt[2 + STR_NAME_LIMIT];

        // Pass the device attribute
        pkt[0] = tracker_id;
        for (uint8_t i = 0; i < array_size; i++)
        {
            pkt[1] = i;  // This passes the attribute id to comp
            name_len = pass_name_to_pkt(pkt + 2, STR_NAME_LIMIT, attrs[i]);

            // Exit loop if name does not comply with limits
            if (!name_len)
            {
                // goto failed_attr_pass;
            }

            schedule_fast_task(ADD_DEVICE_ATTR, EXTERNAL_TASK, pkt, 2 + name_len);  // Pass attr name to comp
        }

        set_init_verifier_bit(tracker_id, ADD_DEVICE_ATTR_OFFSET);
        return ;
    }

    // failed_attr_pass:
    // Add some stuff in here for handling this
}

// Sends to the computer the amount of device instances it should create for this MCU
void create_device_instances(uint8_t tracker_id)
{
    if (get_init_verifier_bit(tracker_id, ADD_DEVICE_ATTR_OFFSET))
    {
        device_tracker_t * tracker = get_tracker(tracker_id);

        if (tracker->devices != NULL)
        {
            // Partially setup device object
            for (uint8_t i = 0; i < tracker->count; i++)
            {
                device_t * device = &tracker->devices[i];
                device->device_id = i;
                device->tracker_id = tracker_id;
            }

            // Set corresponding bit and create the devices on the computer
            set_init_verifier_bit(tracker_id, CREATE_DEVICES_OFFSET);
            schedule_fast_task(REGISTER_DEVICE, EXTERNAL_TASK, ((uint8_t []){tracker_id, tracker->count}), sizeof(uint8_t) * 2);
        }
    }
}

// Set the computer's setup bit to true (from the computer)
static void comp_setup_complete(uint8_t * _)
{
    for (uint8_t i = 0; i < tracker_count; i++)
    {
        set_init_verifier_bit(i, COMP_SETUP_COMPLETE_OFFSET);
    }
}

// Verify if the computer's setup was completed
bool is_comp_setup_complete(void)
{
    for (uint8_t i = 0; i <tracker_count; i++)
    {
        if (!(get_init_verifier_bit(i, COMP_SETUP_COMPLETE_OFFSET)))
        {
            return false;
        }
    }
    return true;
}

// Verify if the computer's setup for a specific device has been completed
bool is_comp_device_setup_complete(uint8_t tracker_id)
{
    return (tracker_id < tracker_count)? get_init_verifier_bit(tracker_id, COMP_SETUP_COMPLETE_OFFSET): false;
}

// Uninitializer for the device trackers
void deinit_devices(void)
{
    // Uninitialize each device instance
    for (uint8_t i = 0; i <= tracker_count; i++)
    {
        device_tracker_t * tracker = &trackers[i];
        tracker->deinit_cb(tracker->devices, tracker->count);
        free(tracker->devices);
    }

    // Unintialize device tracker variables
    free(trackers);
    free(init_verifier);
}

// Get a tracker from the tracker container
device_tracker_t * get_tracker(uint8_t tracker_id)
{
    return (tracker_id < tracker_count)? &trackers[tracker_id]: NULL;
}

// Registers a task in the MCU and sends the name to the computer
void _register_device_task(const char * name, uint8_t id, uint8_t payload_size, task_t task, uint8_t priority_type)
{
    size_t name_len;
    uint8_t pkt[2 + STR_NAME_LIMIT];

    pkt[0] = id;
    pkt[1] = priority_type;
    
    if (name_len = pass_name_to_pkt(pkt + 2, STR_NAME_LIMIT, name))
    {
        schedule_fast_task(REGISTER_TESTER_TASK, EXTERNAL_TASK, pkt, name_len + 2);
        register_task(id, payload_size, task);
    }
    else // Failed task name passing 
    {

    }
}

// Check if the device was initialized correctly
bool device_initialized(uint8_t tracker_id)
{
    device_tracker_t * tracker = get_tracker(tracker_id);

    return (tracker != NULL)? get_init_verifier_bit(tracker_id, CREATE_DEVICES_OFFSET): false;
}

/**Null destructor callback
 * 
 * A null desctructor for when you don't need to uninitialize 
 * a specific device object.
 */ 
void deinit_null_cb(device_t * _, uint8_t __){}

/**Null attribute setter callback
 * 
 * A null attribute setter for when a specific device type doesn't
 * need it's attributes to be updated.
 */ 
void set_null_attr_cb(uint8_t * _){}


/* Private device functions */

// Device task to trigger the update attribute callback
static void update_device_attr_mcu(uint8_t * pkt)
{
    uint8_t tracker_id = pkt[0];

    device_tracker_t * tracker = get_tracker(tracker_id);
    if (tracker != NULL)
    {
        tracker->set_attr_cb(pkt);
    }
}

// Helper function to pass a string to a pkt
static size_t pass_name_to_pkt(uint8_t * buf, size_t buf_size, const char * name)
{
    size_t name_len = strlen(name);
    if (name_len <= (size_t) STR_NAME_LIMIT && name_len <= buf_size)
    {
        memcpy(buf, name, name_len);
        return name_len;
    }
    return 0;
}

// Setup for a specific device tracker
static bool setup_device_tracker(uint8_t tracker_id, uint8_t device_count, deinit_dev_cb deinit_cb, set_dev_attr_cb set_attr_cb)
{   
    device_tracker_t * tracker = get_tracker(tracker_id);
    if (tracker != NULL)
    {
        tracker->devices = malloc(sizeof(device_t) * device_count);
        if (tracker->devices != NULL)
        {
            // Setup tracker attributes
            tracker->count = device_count;
            tracker->deinit_cb = deinit_cb;
            tracker->set_attr_cb = set_attr_cb;

            return true;
        }
    }

    return false;
}
