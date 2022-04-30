

NULL_FLOOR = 0  # Floor that represents an invalid floor to go to
ELEVATOR_TRACKER = 0  # Tracker id for the elevator devices
SET_TASKS = {
    "light_state": "uint8_t",
    "door_state": "uint8_t",
    "current_floor": "uint8_t",
    "temperature": "uint8_t",
    "weight": "uint16_t",
    "maintenance_state": "uint8_t"
}

# Tasks ids for the elevator system

PASS_ELEVATOR_FLOOR_NAME = 0
ALERT_FLOOR_ARRIVAL = 1
ALERT_PERSON_ADDITION = 2
REMOVE_CAR_FROM_FLOOR = 3
ALERT_PERSON_REMOVAL = 4
ENTER_ELEVATOR = 5
REQUEST_ELEVATOR = 6
