#define DATA_MSG_TAG    1
#define ACK_MSG_TAG     2
#define SEQ_MSG_TAG     3

#define MAX_MSG_COUNT   1000
#define MAX_ATTMPT      3

#define MSG_SIZE        sizeof(struct Message)
#define ACK_HEADER_SIZE sizeof(struct AckRecordHeader)

#define ACK_MSG_SIZE    sizeof(struct AckMessage)
#define DATA_MSG_SIZE   sizeof(struct DataMessage)