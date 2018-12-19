#ifndef DSI_SERVICEBROKER_CLIENTIO_H
#define DSI_SERVICEBROKER_CLIENTIO_H


#ifdef __cplusplus
extern "C" {
#endif

#include <sys/uio.h>
#include <unistd.h>

#include "dsi/private/servicebroker.h"


/**
 * Send data given in input vector @c in of length @c inLen via socket and receive appropriate
 * data into @c out which is of length @c outLen elements. The return code will be transmitted via @c rc.
 *
 * @return 0 in case of success, -1 on error in which case "errno" is set.
 */
int sendAndReceiveV(int fd, dcmd_t cmd, size_t inLen, size_t outLen, struct iovec* in, struct iovec* out, int* rc);

/**
 * Send given input @c ptr of length @c inputlen via the given file descriptor and receive
 * at most @c outputlen data. The return code will be transmitted via @c rc.
 *
 * @return 0 in case of success, -1 on error in which case "errno" is set.
 */
int sendAndReceive(int fd, dcmd_t cmd, void* ptr, size_t inputlen, size_t outputlen, int* rc);

/**
 * Send authentication package: Unix connections send credentials as ancillary data.
 * @param sendCredentials 0 if no credentials should be send (for fd referring to a TCP socket), 
 *                        or 1 for fd referring to a Unix socket).
 * @return 0 if successful, else -1.
 */
int sendAuthPackage(int fd, int sendCredentials);

/**
 * Create a unix socket path in abstract namespace from given pid and chid.
 * The buffer must be at least UNIX_PATH_MAX(108) bytes long.
 */
const char* make_unix_path(char* buf, size_t len, int pid, int64_t chid);


#ifdef __cplusplus
}   // extern "C"
#endif


#endif   // DSI_SERVICEBROKER_CLIENTIO_H
