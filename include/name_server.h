#ifndef NAME_SERVER_H_INCLUDED_
#define NAME_SERVER_H_INCLUDED_

#define NAME_SERVER_MAX_NAME_LEN 12
#define NAME_SERVER_INVALID_TID -1
#define NAME_SERVER_OTHER_ERROR -2
#define NAME_SERVER_TID 2

// Entry point for name server task
void name_server_main();

// exit the name server
void name_server_exit();

// Registers the task ID of the caller to the given name, overwites previous
// all following calls to WhoIs with the given name will return the callers tid
// Return 0 on success, -1 on error due invalid name server tid, all other
// errors return -2
int RegisterAs(char *name);

// Returns the tid of the task associated with the given name, blocks if no
// task is registered with the name until one is registered. Returns -1 if name
// server tid is invalid, -2 for another error
int WhoIs(char *name);

#endif // NAME_SERVER_H_INCLUDED_

