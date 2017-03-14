#define DATA_MSG_TYPE    1
#define ACK_MSG_TYPE     2
#define SEQ_MSG_TYPE     3

#define MAX_MSG_COUNT   1000
#define MAX_ATTMPT      3
#define MAX_PENDING     5

#define MSG_SIZE        sizeof(struct Message)
#define ACK_HEADER_SIZE sizeof(struct AckRecordHeader)

#define ACK_MSG_SIZE    sizeof(struct AckMessage)
#define DATA_MSG_SIZE   sizeof(struct DataMessage)

#define ENDLESS_LOOP    1
#define SEND_FLAG       0
