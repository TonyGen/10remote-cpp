/* Maintain a persistent connection to each requested servers
 * TODO: Remove idle connections
 */

#ifndef CONNPOOL_H_
#define CONNPOOL_H_

#include <10util/io.h>
#include <10util/network.h>
#include <10util/mvar.h>

namespace connpool {

/** Return protected connection to server, creating one if necessary */
MVAR (io::IOStream) connection (network::HostPort server);

}

#endif /* CONNPOOL_H_ */
