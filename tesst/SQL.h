#pragma once

// create and bind sockets here

fd_set read_fds;
FD_ZERO(&read_fds);

// add all sockets to fd_set
FD_SET(socket1, &read_fds);
FD_SET(socket2, &read_fds);
FD_SET(socket3, &read_fds);

while (true) {
    // wait for any of the sockets to be ready for I/O
    int num_ready = select(0, &read_fds, NULL, NULL, NULL);
    if (num_ready == SOCKET_ERROR) {
        // handle error
        break;
    }

    // check which sockets are ready for I/O
    if (FD_ISSET(socket1, &read_fds)) {
        // handle I/O for socket1
    }

    if (FD_ISSET(socket2, &read_fds)) {
        // handle I/O for socket2
    }

    if (FD_ISSET(socket3, &read_fds)) {
        // handle I/O for socket3
    }

    // clear fd_set for next iteration
    FD_ZERO(&read_fds);
    FD_SET(socket1, &read_fds);
    FD_SET(socket2, &read_fds);
    FD_SET(socket3, &read_fds);
}