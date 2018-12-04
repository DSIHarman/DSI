#ifndef DSI_SERVICEBROKER_FRAMES_H
#define DSI_SERVICEBROKER_FRAMES_H

#include <stdint.h>


/**
 * Socket communication frame.
 */
typedef struct sb_frame
{
   int32_t fr_magic;    ///< an SB magic to be checked in order to preserve 'lost bytes'
   int32_t fr_size;     ///< size of this frame
   
} sb_frame_t;


/**
 * A SB request frame.
 */
typedef struct sb_request_frame
{
   sb_frame_t fr_envelope;   ///< request header frame
   
   int32_t fr_cmd;           ///< command to be executed by servicebroker
   int32_t fr_padding;       ///< padding bytes for alignment
   
} sb_request_frame_t;


/**
 * A SB response frame.
 */
typedef struct sb_response_frame
{
   sb_frame_t fr_envelope;     ///< response header frame
   
   int32_t fr_returncode;      ///< return code from servicebroker
   int32_t fr_padding;         ///< padding bytes for alignment
   
} sb_response_frame_t;


/// The frames magic, equal to 'S' 'B' 'R' 'K'.
#define SB_FRAME_MAGIC 1263682131

/// initialize the request frame
#define INIT_REQUEST_FRAME(pVar, size, cmd) {      \
   (pVar)->fr_envelope.fr_magic = SB_FRAME_MAGIC;  \
   (pVar)->fr_envelope.fr_size = size;             \
   (pVar)->fr_cmd = cmd;                           \
   (pVar)->fr_padding = 0;                         \
}

/// initialize the response frame
#define INIT_RESPONSE_FRAME(pVar, size, retval) {  \
   (pVar)->fr_envelope.fr_magic = SB_FRAME_MAGIC;  \
   (pVar)->fr_envelope.fr_size = size;             \
   (pVar)->fr_returncode = retval;                 \
   (pVar)->fr_padding = 0;                         \
}


#endif   // DSI_SERVICEBROKER_FRAMES_H
