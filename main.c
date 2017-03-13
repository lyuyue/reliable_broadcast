#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "message.h"
#include "udp_socket.h"

struct Message *deliver_queue = NULL;
struct Message *msg_queue = NULL;

struct addr_info_node hostlist_head = NULL;
struct AckRecordHeader ack_list[MAX_MSG_COUNT];

int port = 0;
int msg_count = 0;
int seq = 0;
int ack_recv[MAX_MSG][MAX_HOST];

// Convert a string into int
int stoi(char *data) {
    int result = 0;
    for (int i = 0; i < strlen(data); i++) {
        int tmp = data[i] - 48;
        result = result * 10 + tmp;
    }
    return result;
}

int data_msg_handler(struct DataMessage *data_msg) {
    struct Message *tmp_msg = (struct Message *) malloc(MSG_SIZE);
    tmp_msg->seq = -1;
    tmp_msg->seq_proposer = -1;
    tmp_msg->sender = data_msg->sender;
    tmp_msg->msg_id = data_msg->msg_id;
    tmp_msg->data = data_msg->data;

    tmp_msg->next = msg_queue->next;
    msg_queue->next = tmp_msg;

    struct AckMessage *ack_msg = (struct AckMessage *) malloc(ACK_MSG_SIZE);
    ack_msg->type = ACK_MSG_TAG;
    ack_msg->sender = data_msg->sender;
    ack_msg->msg_id = data_msg->msg_id;
    ack_msg->proposed_seq = seq + 1;
    ack_msg->proposer = self_id;

    // TODO: send ack

    free(ack_msg);
    return 0;
}

int ack_msg_handler(struct AckMessage *ack_msg) {
    int msg_id = ack_msg->msg_id;
    if (ack_msg->proposed_seq > ack_list[msg_id].max_seq) {
        ack_list[msg_id].max_seq = ack_msg->proposed_seq;
        ack_list[msg_id].max_proposer = ack_msg->proposer;
    }

    struct AckRecord *itr = &ack_list[msg_id].list;
    while (itr->next != NULL) {
        if (itr->next->receiver_id == ack_msg->proposer) {
            struct AckRecord *tmp_ptr = (struct AckRecord *) itr->next;
            itr->next = tmp_ptr->next;
            free(tmp_ptr);
            break;
        }

        itr = itr->next;
    }

    if (ack_list[msg_id].list.next == NULL) {
        // TODO: broadcast final_seq
    }

    return 0;
}

int seq_msg_handler(struct SeqMessage *seq_msg) {
    struct Message *msg_itr = msg_queue;
    while (msg_itr->next != NULL) {
        if (seq_msg->sender == msg_itr->next->sender 
                && seq_msg->msg_id == msg_itr->next->msg_id) {
            break;
        }

        msg_itr = msg_itr->next;
        continue;
    }

    if (msg_itr->next == NULL) {
        perror("Invalid SeqMessage");
    }

    struct Message *cur_msg = msg_itr->next;

    // write seq info
    cur_msg->seq = seq_msg->final_seq;
    cur_msg->seq_proposer = seq_msg->final_seq_proposer;

    // remove cur_msg from msg_queue
    msg_itr->next = cur_msg->next;
    cur_msg->next = NULL;

    struct Message *deliver_itr = deliver_queue;
    if (deliver_itr->next == NULL) {
        deliver_itr->next = cur_msg;
        continue; 
    }

    if (deliver_itr->next->seq > cur_msg->seq) {
        cur_msg->next = deliver_itr->next;
        deliver_itr->next = cur_msg;
        continue;
    }

    while (deliver_itr->next != NULL) {
        if (deliver_itr->next->seq < cur_msg->seq) {
            deliver_itr = deliver_itr->next;
            continue;
        }
        break;
    }

    cur_msg = deliver_itr->next;
    deliver_itr->next = cur_msg;
    return 0;
}

int main(int argc, char* argv[]) {
    // initialization
    msg_queue = (struct Message *) malloc(MSG_SIZE);
    msg_queue->next = NULL;
    deliver_queue = (struct Message *) malloc(MSG_SIZE);
    deliver_queue->next = NULL;
    bzero(&ack_list[0], sizeof(char *) * MAX_MSG_COUNT);

    // parse arguments
    int arg_itr = 1;
    for (; arg_itr < argc; arg_itr ++) {
        if (strcmp(argv[arg_itr], "-p") == 0) {
            arg_itr ++;
            port = stoi(argv[arg_itr]);
            continue;
        }

        if (strcmp(argv[arg_itr], "-h") == 0) {
            arg_itr ++;
            hostfile = argv[arg_itr];
            continue;
        }

        if (strcmp(argv[arg_itr], "-c") == 0) {
            arg_itr ++;
            msg_count = stoi(argv[arg_itr]);
            continue;
        }
    }



    while (ENDLESS_LOOP) {
        char recv_buf[BUF_SIZE];
        struct addr_info_node *host_itr = hostlist_head;
        
        while (host_itr != NULL) {
            bzero(recv_buf, BUF_SIZE);

            int bytes_recv = recvfrom();
            if (bytes_recv < 0) {
                perror("recvfrom error");
            }

            uint32_t *msg_type = (uint32_t *) recv_buf;

            // Data Message
            if (*msg_type == DATA_MSG_TAG) {
                struct DataMessage *data_msg = (struct DataMessage *) recv_buf;
                if (data_msg_handler(data_msg) != 0) {
                    perror("data_msg_handler error");
                }
            }

            // Ack Message
            if (*msg_type == ACK_MSG_TAG) {
                struct AckMessage *ack_msg = (struct AckMessage *) recv_buf;
                if (ack_msg_handler(ack_msg) != 0) {
                    perror("ack_msg_handler error");
                }
            }

            // Seq Message
            if (*msg_type == SEQ_MSG_TAG) {
                struct SeqMessage *seq_msg = (struct SeqMessage *) recv_buf;
                if (seq_msg_handler(seq_msg) != 0) {
                    perror("seq_msg_handler error");
                }
            }

            host_itr = host_itr->next;
        }

        // send data_msg randomly
        if (SEND) {
            msg_count ++;
            struct DataMessage *data_msg = (struct DataMessage *) malloc(DATA_MSG_SIZE);
            data_msg->type = DATA_MSG_TAG;
            data_msg->sender = self_id;
            data_msg->msg_id = msg_count;
            data_msg->data = msg_count;

            // TODO: unicast to all

            free(data_msg);
        }

    }

    return 0;
}