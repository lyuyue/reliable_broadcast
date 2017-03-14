struct Message {
    uint32_t seq;           // total ordering sequence
    uint32_t seq_proposer;  // final sequence proposer's id
    uint32_t sender;        // sender of the message
    uint32_t msg_id;        // the identifier of the message generated by the sender
    uint32_t data;          // message data
    struct Message *next;   // next message
};

struct AckRecord {
    uint32_t receiver_id;        // the identifier of the message generated by the sender
    struct AckRecord *next;
};

struct AckRecordHeader {
    uint32_t max_seq;
    uint32_t max_proposer;
    struct AckRecord list;
};

struct DataMessage {
    uint32_t type;          // must equal to 1
    uint32_t sender;        // the sender's id
    uint32_t msg_id;        // the identifier of the message generated by the sender
    uint32_t data;          // a dummy integer
};

struct AckMessage {
    uint32_t type;          // must equal to 2
    uint32_t sender;        // the sender of the DataMessage
    uint32_t msg_id;        // the identifier of the DataMessage generated by the sender
    uint32_t proposed_seq;  // the proposed sequence number
    uint32_t proposer;      // the process id of the proposer
};

struct SeqMessage {
    uint32_t type;          // must equal to 3
    uint32_t sender;        // the sender of the DataMessage
    uint32_t msg_id;        // the identifier of the DataMessage generated by the sender
    uint32_t final_seq;     // the final sequence number selected by the sender
    uint32_t final_seq_proposer;    // the process id of the proposer who proposed the final_seq
};