#include <name_server.h>
#include <bool.h>
#include <sys_call.h>
#include <bwio.h>
#include <int_types.h>
#include <string.h>
#include <stddef.h>

#include <internal/task_descriptor.h>
#include <internal/queue.h>

#define NAME_SERVER_MAX_NAMES 256

typedef struct {
    char name[NAME_SERVER_MAX_NAME_LEN];
    uint8_t tid;
} name_map_entry_t;

typedef struct {
    queue_t queue;
    queue_node_t nodes[NAME_SERVER_MAX_NAMES];
    name_map_entry_t node_data[NAME_SERVER_MAX_NAMES];
} name_map_t;

typedef enum {
    NAME_SERVER_MSG_TYPE_REG,
    NAME_SERVER_MSG_TYPE_WHO,
    NAME_SERVER_MSG_TYPE_EXIT
} name_server_msg_type_t;

typedef struct {
    name_server_msg_type_t type;
    char name[NAME_SERVER_MAX_NAME_LEN];
} name_server_msg_t;

// copies name into the msg struct, sets the struct's type to type
void name_server_msg_init(name_server_msg_t *ctx, name_server_msg_type_t type, char *name) {
    ctx->type = type;
    strncpy(ctx->name, name, NAME_SERVER_MAX_NAME_LEN);
}

int RegisterAs(char *name) {
    // create msg struct to send to name_server
    name_server_msg_t msg;
    name_server_msg_init(&msg, NAME_SERVER_MSG_TYPE_REG, name);

    // blocks here
    int rep;
    int res = Send(NAME_SERVER_TID, &msg, sizeof(msg), &rep, sizeof(rep));
    if (res) {
        bwprintf(COM2, "Unable to register with name_server\n\r");
        if (res == -1) {
            return NAME_SERVER_INVALID_TID;
        } else {
            return NAME_SERVER_OTHER_ERROR;
        }
    }
    return 0;
}

int WhoIs(char *name) {
    // create msg struct to send to name_server
    name_server_msg_t msg;
    name_server_msg_init(&msg, NAME_SERVER_MSG_TYPE_WHO, name);

    // blocks here
    int rep;
    do {
        int res = Send(NAME_SERVER_TID, &msg, sizeof(msg), &rep, sizeof(rep));
        if (res) {
            bwprintf(COM2, "Unable to register with name_server\n\r");
            if (res == -1) {
                return NAME_SERVER_INVALID_TID;
            } else {
                return NAME_SERVER_OTHER_ERROR;
            }
        }
    } while (rep == 1);
    return rep;
}

// set name string to null, set tid to 0
void name_map_entry_init(name_map_entry_t *ctx) {
    ctx->name[0] = 0;
    ctx->tid = 0;
}

// init queue, nodes and node data, queue starts empty
void name_map_init(name_map_t *ctx) {
    queue_init(&ctx->queue);
    for (size_t i = 0; i < NAME_SERVER_MAX_NAMES; i++) {
        name_map_entry_init(&ctx->node_data[i]);
        queue_node_init(&ctx->nodes[i], &ctx->node_data[i]);
    }
}

// Prepares unused node to be added to name_map, then inserts it
// returns non-zero if node insertion fails (name_map state is still valid)
uint8_t name_map_insert(name_map_t *ctx, char *name, uint8_t tid) {
    size_t elements = queue_size(&ctx->queue);
    queue_node_t *new_node = &ctx->nodes[elements];
    name_map_entry_t *new_entry = (name_map_entry_t*)new_node->data;

    strncpy(new_entry->name, name, sizeof(char) * NAME_SERVER_MAX_NAME_LEN);
    new_entry->tid = tid;
    
    return queue_put_front(&ctx->queue, new_node);;
}

// searches for an element in the map with the given name, returns the entry if
// found, returns NULL otherwise
name_map_entry_t *name_map_find(name_map_t *ctx, char *name) {
    name_map_entry_t *ret = NULL;
    queue_node_t *prev_node = NULL;
    queue_node_t *next_node = ctx->queue.front;

    while (next_node != NULL) {
        name_map_entry_t *entry = (name_map_entry_t*)next_node->data;
        if (strncmp(entry->name, name, NAME_SERVER_MAX_NAME_LEN) == 0) {
            ret = entry;
            //queue_mtf(&ctx->queue, prev_node);    // TODO broken
            break;
        }
        prev_node = next_node;
        next_node = next_node->next;
    }

    return ret;
}

void name_server_main() {
    // initialize name map struct
    name_map_t nm;
    name_map_init(&nm);
    name_server_msg_t msg;
    uint8_t sender_tid = 0;

    do {
        int32_t ret = Receive(&sender_tid, &msg, sizeof(name_server_msg_t));
        name_map_entry_t *entry = NULL;
        int rep = 0;

        if (ret <  0) {
            bwprintf(COM2, "Name server Receive failed: %d\n\r", ret);
            continue;
        }

        switch(msg.type) {
            case NAME_SERVER_MSG_TYPE_REG:
                // insert entry, return success or failure result of insert
                rep = name_map_insert(&nm, msg.name, sender_tid);
                break;
            case NAME_SERVER_MSG_TYPE_WHO:
                // search for entry, return tid on success, 1 on failure
                entry = name_map_find(&nm, msg.name);
                if (entry == NULL) {
                    rep = 1;
                } else {
                    rep = entry->tid;
                }
                break;
            default:
                continue;
        }

        // send reply back to blocked client
        ret = Reply(sender_tid, &rep, sizeof(rep));
        if (ret) {
            bwprintf(COM2, "Name server reply error, tid:%d might be blocked\n\r", sender_tid);
        }
    } while(msg.type != NAME_SERVER_MSG_TYPE_EXIT);
    msg.type = NAME_SERVER_MSG_TYPE_EXIT;
    bwprintf(COM2, "Name server shutting down\n\r");
    Reply(sender_tid, &msg, sizeof(msg));
    Exit();
}

void name_server_exit() {
    name_server_msg_t msg, reply;
    msg.type = NAME_SERVER_MSG_TYPE_EXIT;
    int res = Send(NAME_SERVER_TID, &msg, sizeof(msg), &reply, sizeof(reply));
    if (res) {
        bwprintf(COM2, "Error shutting down name server\n\r");
    }
}
