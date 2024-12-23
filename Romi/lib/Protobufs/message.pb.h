/* Automatically generated nanopb header */
/* Generated by nanopb-0.4.5 */

#ifndef PB_MESSAGE_MESSAGE_PB_H_INCLUDED
#define PB_MESSAGE_MESSAGE_PB_H_INCLUDED
#include <pb.h>

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Enum definitions */
typedef enum _message_ServerCommand_State { 
    message_ServerCommand_State_IDLE = 0, 
    message_ServerCommand_State_DRIVING = 1, 
    message_ServerCommand_State_LINING = 2, 
    message_ServerCommand_State_TURNING = 3, 
    message_ServerCommand_State_RAMPING = 4, 
    message_ServerCommand_State_SEARCHING = 5, 
    message_ServerCommand_State_GIMMIE_THAT_TAG = 6, 
    message_ServerCommand_State_TARGETING = 7, 
    message_ServerCommand_State_WEIGHING = 8, 
    message_ServerCommand_State_LIFTING = 9 
} message_ServerCommand_State;

/* Struct definitions */
typedef struct _message_AprilTag { 
    int32_t id; 
    int32_t cx; 
    int32_t cy; 
    int32_t w; 
    int32_t h; 
    int32_t rot; 
} message_AprilTag;

typedef struct _message_GridCell { 
    int32_t x; 
    int32_t y; 
} message_GridCell;

typedef struct _message_Pose { 
    float x; 
    float y; 
    bool has_z;
    float z; 
    bool has_roll;
    float roll; 
    bool has_pitch;
    float pitch; 
    float heading; 
} message_Pose;

typedef struct _message_RomiData { 
    bool has_gridLocation;
    message_GridCell gridLocation; 
} message_RomiData;

typedef struct _message_ServerCommand { /* enum Mode {
     TELEOP = 0;
     AUTO = 1;
     SETUP = 2;
 } */
    bool has_state;
    message_ServerCommand_State state; 
    float baseSpeed; 
    bool has_targetGridCell;
    message_GridCell targetGridCell; 
} message_ServerCommand;


/* Helper constants for enums */
#define _message_ServerCommand_State_MIN message_ServerCommand_State_IDLE
#define _message_ServerCommand_State_MAX message_ServerCommand_State_LIFTING
#define _message_ServerCommand_State_ARRAYSIZE ((message_ServerCommand_State)(message_ServerCommand_State_LIFTING+1))


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define message_GridCell_init_default            {0, 0}
#define message_Pose_init_default                {0, 0, false, 0, false, 0, false, 0, 0}
#define message_AprilTag_init_default            {0, 0, 0, 0, 0, 0}
#define message_ServerCommand_init_default       {false, _message_ServerCommand_State_MIN, 0, false, message_GridCell_init_default}
#define message_RomiData_init_default            {false, message_GridCell_init_default}
#define message_GridCell_init_zero               {0, 0}
#define message_Pose_init_zero                   {0, 0, false, 0, false, 0, false, 0, 0}
#define message_AprilTag_init_zero               {0, 0, 0, 0, 0, 0}
#define message_ServerCommand_init_zero          {false, _message_ServerCommand_State_MIN, 0, false, message_GridCell_init_zero}
#define message_RomiData_init_zero               {false, message_GridCell_init_zero}

/* Field tags (for use in manual encoding/decoding) */
#define message_AprilTag_id_tag                  1
#define message_AprilTag_cx_tag                  2
#define message_AprilTag_cy_tag                  3
#define message_AprilTag_w_tag                   4
#define message_AprilTag_h_tag                   5
#define message_AprilTag_rot_tag                 6
#define message_GridCell_x_tag                   1
#define message_GridCell_y_tag                   2
#define message_Pose_x_tag                       1
#define message_Pose_y_tag                       2
#define message_Pose_z_tag                       3
#define message_Pose_roll_tag                    4
#define message_Pose_pitch_tag                   5
#define message_Pose_heading_tag                 6
#define message_RomiData_gridLocation_tag        1
#define message_ServerCommand_state_tag          1
#define message_ServerCommand_baseSpeed_tag      2
#define message_ServerCommand_targetGridCell_tag 3

/* Struct field encoding specification for nanopb */
#define message_GridCell_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    x,                 1) \
X(a, STATIC,   SINGULAR, INT32,    y,                 2)
#define message_GridCell_CALLBACK NULL
#define message_GridCell_DEFAULT NULL

#define message_Pose_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, FLOAT,    x,                 1) \
X(a, STATIC,   SINGULAR, FLOAT,    y,                 2) \
X(a, STATIC,   OPTIONAL, FLOAT,    z,                 3) \
X(a, STATIC,   OPTIONAL, FLOAT,    roll,              4) \
X(a, STATIC,   OPTIONAL, FLOAT,    pitch,             5) \
X(a, STATIC,   SINGULAR, FLOAT,    heading,           6)
#define message_Pose_CALLBACK NULL
#define message_Pose_DEFAULT NULL

#define message_AprilTag_FIELDLIST(X, a) \
X(a, STATIC,   SINGULAR, INT32,    id,                1) \
X(a, STATIC,   SINGULAR, INT32,    cx,                2) \
X(a, STATIC,   SINGULAR, INT32,    cy,                3) \
X(a, STATIC,   SINGULAR, INT32,    w,                 4) \
X(a, STATIC,   SINGULAR, INT32,    h,                 5) \
X(a, STATIC,   SINGULAR, INT32,    rot,               6)
#define message_AprilTag_CALLBACK NULL
#define message_AprilTag_DEFAULT NULL

#define message_ServerCommand_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, UENUM,    state,             1) \
X(a, STATIC,   SINGULAR, FLOAT,    baseSpeed,         2) \
X(a, STATIC,   OPTIONAL, MESSAGE,  targetGridCell,    3)
#define message_ServerCommand_CALLBACK NULL
#define message_ServerCommand_DEFAULT NULL
#define message_ServerCommand_targetGridCell_MSGTYPE message_GridCell

#define message_RomiData_FIELDLIST(X, a) \
X(a, STATIC,   OPTIONAL, MESSAGE,  gridLocation,      1)
#define message_RomiData_CALLBACK NULL
#define message_RomiData_DEFAULT NULL
#define message_RomiData_gridLocation_MSGTYPE message_GridCell

extern const pb_msgdesc_t message_GridCell_msg;
extern const pb_msgdesc_t message_Pose_msg;
extern const pb_msgdesc_t message_AprilTag_msg;
extern const pb_msgdesc_t message_ServerCommand_msg;
extern const pb_msgdesc_t message_RomiData_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define message_GridCell_fields &message_GridCell_msg
#define message_Pose_fields &message_Pose_msg
#define message_AprilTag_fields &message_AprilTag_msg
#define message_ServerCommand_fields &message_ServerCommand_msg
#define message_RomiData_fields &message_RomiData_msg

/* Maximum encoded size of messages (where known) */
#define message_AprilTag_size                    66
#define message_GridCell_size                    22
#define message_Pose_size                        30
#define message_RomiData_size                    24
#define message_ServerCommand_size               31

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
