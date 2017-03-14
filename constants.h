#define DATA_MSG_TYPE    1
#define ACK_MSG_TYPE     2
#define SEQ_MSG_TYPE     3

#define MAX_MSG_COUNT   1000
#define MAX_HOST        100
#define MAX_PENDING     100
#define MAX_DELAY       1

#define MSG_SIZE        sizeof(struct Message)
#define ACK_HEADER_SIZE sizeof(struct AckRecordHeader)

#define ACK_MSG_SIZE    sizeof(struct AckMessage)
#define DATA_MSG_SIZE   sizeof(struct DataMessage)
#define SEQ_MSG_SIZE    sizeof(struct SeqMessage)

#define ENDLESS_LOOP    1
#define SEND_FLAG       1
#define RELIABLE_FLAG   1

#define BUF_SIZE        512
