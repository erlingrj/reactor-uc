#include "reactor-uc/platform/posix/tcp_ip_bundle.h"
#include "reactor-uc/reactor-uc.h"
#include <sys/socket.h>
#include <unistd.h>

int main() {
  TcpIpBundle bundle;

  const char *host = "127.0.0.1";
  unsigned short port = 8902; // NOLINT

  // creating a server that listens on loopback device on port 8900
  TcpIpBundle_ctor(&bundle, host, port, AF_INET);

  // binding to that address
  bundle.bind(&bundle);

  // change the bundle to non-blocking
  bundle.change_block_state(&bundle, false);

  // accept one connection
  bool new_connection;
  do {
    new_connection = bundle.accept(&bundle);
  } while (!new_connection);

  // waiting for messages from client
  TaggedMessage *message = NULL;
  do {
    message = bundle.receive(&bundle);
    sleep(1);
  } while (message == NULL);

  printf("Received message with connection number %i and content %s\n", message->conn_id,
         (char *)message->payload.bytes);

  bundle.send(&bundle, message);

  bundle.close(&bundle);
}
