#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

void set_timeout(int sockfd) {
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(tv)) < 0) {
        perror("setsockopt() error");
    }

    return;
}

void *send_msg(int sockfd, char *send_buf, int len) {
    if (send(sockfd, send_buf, len, 0) != len) {
        perror("send() error");
    }
    return;
}

void *send_data_msg(int msg_id) {
    struct DataMessage *data_msg = (struct DataMessage *) malloc(DATA_MSG_SIZE);
    data_msg->type = DATA_MSG_TYPE;
    data_msg->sender = self_id;
    data_msg->msg_id = msg_id;
    data_msg->data = msg_id;

    pthread_t pthread_ids[hostlist_len];

    for (int i = 1; i < hostlist_len; i++) {
        if (i == self_id) continue;
        pthread_create(&pthread_ids[i], send_msg, sockfd[i], (char *) data_msg, DATA_MSG_SIZE);
    }

    for (int i = 1; i < hostlist_len; i++) {
        if (i == self_id) continue;
        pthread_join(pthread_ids[i], NULL);
    }

    free(data_msg);

    return;
}

void *send_ack_msg(struct DataMessage *data_msg) {
    struct AckMessage *ack_msg = (struct AckMessage *) malloc(ACK_MSG_SIZE);
    ack_msg->type = ACK_MSG_TYPE;
    ack_msg->sender = data_msg->sender;
    ack_msg->msg_id = data_msg->msg_id;
    pthread_mutex_lock(&seq_lock);
    seq ++;
    ack_msg->proposed_seq = seq;
    pthread_mutex_unlock(&seq_lock);
    ack_msg->proposer = self_id;

    // send AckMessage
    pthread_t pthread_id;
    pthread_create(&pthread_id, send_msg, sockfd, (char *) ack_msg, ACK_MSG_SIZE);
    pthread_join(pthread_id, NULL);
    free(ack_msg);

    return;
}

void *send_seq_msg(struct SeqMessage *seq_msg) {
    pthread_t pthread_ids[hostlist_len];

    for (int i = 1; i < hostlist_len; i++) {
        if (i == self_id) continue;
        pthread_create(&pthread_ids[i], send_msg, sockfd[i], (char *) seq_msg, SEQ_MSG_SIZE);
    }

    for (int i = 1; i < hostlist_len; i++) {
        if (i == self_id) continue;
        pthread_join(pthread_ids[i], NULL);
    }

    free(seq_msg);

    return;
}