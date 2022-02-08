
# Scheduler constants

# Task time range for the other system to reply
SHORT_TIMER = .350  # Allowed time for the first reply time range
LONG_TIMER = .5  # Allowed time for the second reply time range

# Immutable scheduler constants

# Task types
INTERNAL_TASK = 0
EXTERNAL_TASK = 1

# Task priority types
NORMAL_TASK = 0
PRIORITY_TASK = 1
FAST_TASK = 2

# Internal command ids
ALERT_SYSTEM = 0
PRINT_MESSAGE = 1
UNSCHEDULE_TASK = 2
MODIFY_TASK_VAL = 3
PKT_DECODE = 4
PKT_ENCODE = 5
TASK_LOOKUP = 6
TASK_REGISTER = 7

# Packet constants

# Modifiable packet constants

MAX_PAYLOAD_SIZE = 25

# Immutable packet constants

ENCODED_HDR_SIZE = 5
DECODED_HDR_SIZE = 4
MAX_ALLOWED_PKT_SIZE = 255
MAX_DECODED_PKT_BUF_SIZE = DECODED_HDR_SIZE + MAX_PAYLOAD_SIZE
MAX_ENCODED_PKT_BUF_SIZE = ENCODED_HDR_SIZE + MAX_PAYLOAD_SIZE + 1

# Packet offsets (these offsets assume the packet is not COBS encoded)

CRC16_OFFSET = 0
TASK_ID_OFFSET = 2
TASK_TYPE_OFFSET = 3
PAYLOAD_OFFSET = 4
