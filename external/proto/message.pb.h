/* Automatically generated nanopb header */
/* Generated by nanopb-1.0.0-dev */

#ifndef PB_MESSAGE_PB_H_INCLUDED
#define PB_MESSAGE_PB_H_INCLUDED
#include "nanopb/pb.h"

#if PB_PROTO_HEADER_VERSION != 40
#error Regenerate this file with the current version of nanopb generator.
#endif

/* Struct definitions */
typedef struct _Tag {
    int64_t time;
    uint32_t microstep;
} Tag;

typedef PB_BYTES_ARRAY_T(832) TaggedMessage_payload_t;
typedef struct _TaggedMessage {
    Tag tag;
    int32_t conn_id;
    TaggedMessage_payload_t payload;
} TaggedMessage;


#ifdef __cplusplus
extern "C" {
#endif

/* Initializer values for message structs */
#define Tag_init_default                         {0, 0}
#define TaggedMessage_init_default               {Tag_init_default, 0, {0, {0}}}
#define Tag_init_zero                            {0, 0}
#define TaggedMessage_init_zero                  {Tag_init_zero, 0, {0, {0}}}

/* Field tags (for use in manual encoding/decoding) */
#define Tag_time_tag                             1
#define Tag_microstep_tag                        2
#define TaggedMessage_tag_tag                    1
#define TaggedMessage_conn_id_tag                2
#define TaggedMessage_payload_tag                3

/* Struct field encoding specification for nanopb */
#define Tag_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, INT64,    time,              1) \
X(a, STATIC,   REQUIRED, UINT32,   microstep,         2)
#define Tag_CALLBACK NULL
#define Tag_DEFAULT NULL

#define TaggedMessage_FIELDLIST(X, a) \
X(a, STATIC,   REQUIRED, MESSAGE,  tag,               1) \
X(a, STATIC,   REQUIRED, INT32,    conn_id,           2) \
X(a, STATIC,   REQUIRED, BYTES,    payload,           3)
#define TaggedMessage_CALLBACK NULL
#define TaggedMessage_DEFAULT NULL
#define TaggedMessage_tag_MSGTYPE Tag

extern const pb_msgdesc_t Tag_msg;
extern const pb_msgdesc_t TaggedMessage_msg;

/* Defines for backwards compatibility with code written before nanopb-0.4.0 */
#define Tag_fields &Tag_msg
#define TaggedMessage_fields &TaggedMessage_msg

/* Maximum encoded size of messages (where known) */
#define MESSAGE_PB_H_MAX_SIZE                    TaggedMessage_size
#define Tag_size                                 17
#define TaggedMessage_size                       865

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
