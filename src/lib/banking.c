//
// Created by almdudleer on 04.05.2020.
//

#include <stdlib.h>
#include "entity.h"
#include "ipc_utils.h"

void transfer(void* parent_data, local_id src, local_id dst,
              balance_t amount) {
    Unit* self = parent_data;

    // Create transfer message
    TransferOrder transfer_order;
    transfer_order.s_amount = amount;
    transfer_order.s_dst = dst;
    transfer_order.s_src = src;
    Message* transfer_message = Message_new(TRANSFER, &transfer_order, sizeof(TransferOrder));

    // Initiate transfer
    if (send(self, src, transfer_message) < 0) {
        perror("send");
        exit(EXIT_FAILURE);
    }

    // Wait for ACK
    Message in_msg;
    if (receive_or_die(self, dst, &in_msg) < 0) {
        perror("receive_or_die");
        exit(EXIT_FAILURE);
    }
    if (in_msg.s_header.s_type != ACK) {
        fprintf(stderr, "Process %d: bad message type %d, expected ACK(%d)\n",
                self->lid, in_msg.s_header.s_type, ACK);
        exit(EXIT_FAILURE);
    }
}