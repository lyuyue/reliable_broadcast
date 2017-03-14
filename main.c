#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "constants.h"
#include "message.h"
#include "tcp_helper.h"

struct thread {
    pthread_t tid;
    struct thread *next;
} *thread_head;

struct Message *deliver_queue = NULL;
struct Message *msg_queue = NULL;

int self_id = 0;
char self_hostname[BUF_SIZE];
int self_sock;
struct sockaddr_in self_addr;

int sockfd[MAX_HOST];
struct addrinfo hints, *res, *addr_ptr;
struct sockaddr_in *remote_addr;

struct AckRecordHeader ack_list[MAX_MSG_COUNT];

int port = 0;
char *hostfile;
char *port_str;
int msg_count = 0;
int max_msg_count = 0;
int seq = 0;
int hostlist_len = 0;

pthread_mutex_t seq_lock;

pthread_t * get_thread_id() {
    struct thread *new_thread = (struct thread *) malloc(sizeof(struct thread));
    new_thread->next = thread_head->next;
    thread_head->next = new_thread;
    return &new_thread->tid;
}

void * tcp_send(void *args) {
    uint32_t *msg_type = (uint32_t *) args;
    int len = 0;
    if (*msg_type == DATA_MSG_TYPE) len = DATA_MSG_SIZE;
    if (*msg_type == ACK_MSG_TYPE)  len = ACK_MSG_SIZE;
    if (*msg_type == SEQ_MSG_TYPE)  len = SEQ_MSG_SIZE;

    int *sockfd_addr = (int *) ((void *) args + len);
    if (send(*sockfd_addr, (char *) args, len, 0) != len) {
        perror("send() error");
    }
    return 0;
}


void * send_data_msg(uint32_t *msg_id) {
    struct DataMessage *data_msg[hostlist_len];
    pthread_t pthread_ids[hostlist_len];

    for (int i = 1; i <= hostlist_len; i++) {
        if (i == self_id) continue;
        data_msg[i] = (struct DataMessage *) malloc(DATA_MSG_SIZE + sizeof(int));
        data_msg[i]->type = DATA_MSG_TYPE;
        data_msg[i]->sender = self_id;
        data_msg[i]->msg_id = *msg_id;
        data_msg[i]->data = *msg_id;

        int *sockfd_data = (int *) ((struct DataMessage *) data_msg[i] + 1);
        *sockfd_data = sockfd[i]; 

        pthread_create(&pthread_ids[i], NULL, tcp_send, (void *) data_msg[i]);
    }

    for (int i = 1; i <= hostlist_len; i++) {
        if (i == self_id) continue;
        pthread_join(pthread_ids[i], NULL);
        free(data_msg[i]);
    }

    return 0;
}

void * send_ack_msg(struct DataMessage *data_msg) {
    struct AckMessage *ack_msg = (struct AckMessage *) malloc(ACK_MSG_SIZE + sizeof(int));
    ack_msg->type = ACK_MSG_TYPE;
    ack_msg->sender = data_msg->sender;
    ack_msg->msg_id = data_msg->msg_id;
    pthread_mutex_lock(&seq_lock);
    seq ++;
    ack_msg->proposed_seq = seq;
    pthread_mutex_unlock(&seq_lock);
    ack_msg->proposer = self_id;

    int *sockfd_data = (int *) (ack_msg + 1);
    *sockfd_data = sockfd[data_msg->sender];

    // send AckMessage
    pthread_t pthread_id;
    pthread_create(&pthread_id, NULL, tcp_send, (void *) ack_msg);
    pthread_join(pthread_id, NULL);
    free(ack_msg);

    return 0;
}

void * send_seq_msg(struct SeqMessage *seq_data) {
    struct SeqMessage *seq_msg[hostlist_len];
    pthread_t pthread_ids[hostlist_len];

    for (int i = 1; i <= hostlist_len; i++) {
        if (i == self_id) continue;
        seq_msg[i] = (struct SeqMessage *) malloc(SEQ_MSG_SIZE + sizeof(int));
        seq_msg[i]->type = SEQ_MSG_TYPE;
        seq_msg[i]->sender = seq_data->sender;
        seq_msg[i]->msg_id = seq_data->msg_id;
        seq_msg[i]->final_seq = seq_data->final_seq;
        seq_msg[i]->final_seq_proposer = seq_data->final_seq_proposer;

        int *sockfd_data = (int *) ((struct SeqMessage *) seq_msg[i] + 1);
        *sockfd_data = sockfd[i]; 

        pthread_create(&pthread_ids[i], NULL, tcp_send, seq_msg[i]);
    }

    for (int i = 1; i <= hostlist_len; i++) {
        if (i == self_id) continue;
        pthread_join(pthread_ids[i], NULL);
        free(seq_msg[i]);
    }
    return 0;
}

int data_msg_handler(struct DataMessage *data_msg) {
    struct Message *tmp_msg = (struct Message *) malloc(MSG_SIZE);
    tmp_msg->seq = -1;
    tmp_msg->seq_proposer = -1;
    tmp_msg->sender = data_msg->sender;
    tmp_msg->msg_id = data_msg->msg_id;

    tmp_msg->next = msg_queue->next;
    msg_queue->next = tmp_msg;

    // send ack_msg
    pthread_t *new_thread_id = get_thread_id();
    pthread_create(new_thread_id, NULL, (void *) send_ack_msg, data_msg);
    return 0;
}

int deliver_msg(struct SeqMessage *seq_msg) {
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
        return -1;
    }

    struct Message *cur_msg = msg_itr->next;

    // write seq info
    cur_msg->seq = seq_msg->final_seq;
    cur_msg->seq_proposer = seq_msg->final_seq_proposer;

    // remove cur_msg from msg_queue
    msg_itr->next = cur_msg->next;
    cur_msg->next = NULL;

    struct Message *deliver_itr = deliver_queue;
    // empty deliver_queue
    if (deliver_itr->next == NULL) {
        deliver_itr->next = cur_msg;
        return 0; 
    }

    // insert to the head of deliver_queue
    if (deliver_itr->next->seq > cur_msg->seq) {
        cur_msg->next = deliver_itr->next;
        deliver_itr->next = cur_msg;
        return 0;
    }

    while (deliver_itr->next != NULL) {
        if (deliver_itr->next->seq < cur_msg->seq) {
            deliver_itr = deliver_itr->next;
            continue;
        }
        break;
    }

    cur_msg->next = deliver_itr->next;
    deliver_itr->next = cur_msg;

    printf("%d: Processed message %d from sender %d with seq (%d, %d)\n",
        self_id, cur_msg->msg_id, cur_msg->sender, cur_msg->seq, cur_msg->seq_proposer);

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
        // self_deliver
        struct SeqMessage *seq_msg = (struct SeqMessage *) malloc(SEQ_MSG_SIZE);
        seq_msg->type = SEQ_MSG_TYPE;
        seq_msg->sender = self_id;
        seq_msg->msg_id = msg_id;
        seq_msg->final_seq = ack_list[msg_id].max_seq;
        seq_msg->final_seq_proposer = ack_list[msg_id].max_proposer;

        deliver_msg(seq_msg);

        // broadcast final_seq
        pthread_t *new_thread_id = get_thread_id();
        pthread_create(new_thread_id, NULL, (void *) send_seq_msg, seq_msg);

        free(seq_msg);
    }

    return 0;
}


int seq_msg_handler(struct SeqMessage *seq_msg) {
    pthread_mutex_lock(&seq_lock);
    if (seq < seq_msg->final_seq)
        seq = seq_msg->final_seq;
    pthread_mutex_unlock(&seq_lock);

    deliver_msg(seq_msg);
    return 0;
}

int main(int argc, char* argv[]) {
    // initialization
    msg_queue = (struct Message *) malloc(MSG_SIZE);
    msg_queue->next = NULL;
    deliver_queue = (struct Message *) malloc(MSG_SIZE);
    deliver_queue->next = NULL;

    bzero(&ack_list[0], sizeof(struct AckRecordHeader) * MAX_MSG_COUNT);
    bzero(&sockfd[0], sizeof(int) * MAX_HOST);

    if (pthread_mutex_init(&seq_lock, NULL) != 0) {
        perror("pthread_mutex_init() error");
        return -1;
    }

    gethostname(self_hostname, BUF_SIZE);

    // parse arguments
    int arg_itr = 1;
    for (; arg_itr < argc; arg_itr ++) {
        if (strcmp(argv[arg_itr], "-p") == 0) {
            arg_itr ++;
            port_str = (char *) argv[arg_itr];
            port = atoi(argv[arg_itr]);
            continue;
        }

        if (strcmp(argv[arg_itr], "-h") == 0) {
            arg_itr ++;
            hostfile = argv[arg_itr];
            continue;
        }

        if (strcmp(argv[arg_itr], "-c") == 0) {
            arg_itr ++;
            max_msg_count = atoi(argv[arg_itr]);
            continue;
        }
    }

    printf("port: %d, hostfile: %s, max_msg_count: %d\n", port, hostfile, max_msg_count);

    // build host_list
    FILE *fp;
    char *line_buffer = (char *) malloc(sizeof(char) * BUF_SIZE);

    fp = fopen(hostfile, "r");
    if (fp < 0) {
        perror("fopen() error");
    }

    while (fgets(line_buffer, BUF_SIZE, (FILE *) fp)) {
        hostlist_len ++;
        if (self_id > 0) continue;
        *(line_buffer + strlen(line_buffer) - 1) = '\0';

        if (strcmp(line_buffer, self_hostname) == 0) {
            self_id = hostlist_len;
            continue;
        }

        memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;

        if (getaddrinfo(line_buffer, port_str, &hints, &res) != 0) {
            perror("getaddrinfo() error");
            return -1;
        }

        if ((sockfd[hostlist_len] = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
            perror("socket() error");
            return -1;
        }
        printf("Connecting %d ...\n", hostlist_len);
        if (connect(sockfd[hostlist_len], res->ai_addr, res->ai_addrlen) < 0) {
            perror("connect() error");
            return -1;
        }
    }
    
    free(line_buffer);
    printf("self_id: %d\n", self_id);
    printf("hostlist_len: %d\n", hostlist_len);

    // initialize local_socket
    self_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (self_sock < 0) {
        perror("socket() error");
        return -1;
    }

    memset(&self_addr, 0, sizeof(self_addr));
    self_addr.sin_family = AF_INET;
    self_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    self_addr.sin_port = htons((unsigned short) port);

    // bind to local socket
    if (bind(self_sock, (struct sockaddr *) &self_addr, sizeof(self_addr)) < 0) {
        perror("bind() error");
        return -1;
    }

    if (listen(self_sock, MAX_PENDING) < 0) {
        perror("listen() error");
        return -1;
    }
    for (int i = self_id + 1; i <= hostlist_len;) {
        printf("Listening connection from %d\n", i);
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        sockfd[i] = accept(self_sock, (struct sockaddr *) &client_addr, (socklen_t *) &client_len);
        if (sockfd[i] < 0) {
            continue;
        }
        printf("TCP connection to host %d established\n", i);
        i ++;
    }

    for (int i = 1; i <= hostlist_len; i++) {
        if (i == self_id) continue;
        set_timeout(sockfd[i]);
    }

    // sleep for all connection reach stable
    sleep(3);

    while (ENDLESS_LOOP) {
        char recv_buf[BUF_SIZE];

        for (int i = 1; i <= hostlist_len; i++) {
            if (i == self_id) continue;

            bzero(recv_buf, BUF_SIZE);

            int bytes_recv = recv(sockfd[i], recv_buf, BUF_SIZE, 0);
            if (bytes_recv < 0) {
                perror("");
            }

            uint32_t *msg_type = (uint32_t *) recv_buf;

            // DataMessage
            if (*msg_type == DATA_MSG_TYPE) {
                struct DataMessage *data_msg = (struct DataMessage *) recv_buf;
                if (data_msg_handler(data_msg) != 0) {
                    perror("data_msg_handler() error");
                    return -1;
                }
            }

            // AckMessage
            if (*msg_type == ACK_MSG_TYPE) {
                struct AckMessage *ack_msg = (struct AckMessage *) recv_buf;
                if (ack_msg_handler(ack_msg) != 0) {
                    perror("ack_msg_handler error");
                    return -1;
                }
            }
                // SeqMessage
            if (*msg_type == SEQ_MSG_TYPE) {
                struct SeqMessage *seq_msg = (struct SeqMessage *) recv_buf;
                if (seq_msg_handler(seq_msg) != 0) {
                    perror("seq_msg_handler error");
                    return -1;
                }
            }
        }

        // send data_msg randomly
        if (SEND_FLAG && msg_count < max_msg_count) {
            struct Message *tmp_msg = (struct Message *) malloc(MSG_SIZE);
            tmp_msg->seq = -1;
            tmp_msg->seq_proposer = -1;
            tmp_msg->sender = self_id;
            tmp_msg->msg_id = msg_count;

            tmp_msg->next = msg_queue->next;
            msg_queue->next = tmp_msg;

            for (int i = 1; i <= hostlist_len; i ++) {
                if (i == self_id) continue;
                struct AckRecord *new_record = (struct AckRecord *) malloc(sizeof(struct AckRecord));
                new_record->next = ack_list[msg_count].list.next;
                ack_list[msg_count].list.next = new_record;
            }

            pthread_t *new_thread_id = get_thread_id();
            pthread_create(new_thread_id, NULL, (void *) send_data_msg, &msg_count);
            // increase counter
            msg_count ++;
        }


        // free thread_id info
        struct thread *thread_itr = thread_head;
        while (thread_itr->next != NULL) {
            if (pthread_kill(thread_itr->next->tid, 0) == 0) {
                struct thread *tmp = thread_itr->next->next;
                free(thread_itr->next);
                thread_itr->next = tmp;
            }
            thread_itr = thread_itr->next; 
        }
    }

    return 0;
}