#include "agent.h" 
#include "agent_config.h"
#include "agent_events.h"

/*
 * Outputs to the user how to start the program. 
 */
void 
usage()
{
    printf("Usage is as follows: \n");
    printf("    agent space seperated flags /path/to/config/file\n");
    printf("Example: \n");
    printf("    ./agent -C ../../deps/config.txt\n");
    exit(0);
}

/* 
 * Verifys the command line areguament for the monitor, returns ture if correct 
 * argumets are supplied and false otherwise. 
 */
bool 
validate_args(int argc, char **argv) 
{
    if (argc < 3)         
        return (false);

    if (strcmp(argv[1], "-C") != 0) // if other flags added or effect of -C flag changes alter here. 
        return (false);

    return (true);
}

/*
 * Allocates memory for a new svc_list, sets all pointer members to NULL, and
 * returns a pointer to it. 
 */
svc_list*
new_null_svc_list()
{
    svc_list        *new_svc_list = (svc_list *) malloc(sizeof(svc_list));

    new_svc_list->next = NULL;
    new_svc_list->hook_list = NULL;

    return (new_svc_list);
}

/*
 * Allocates memory for a new svc_list, sets all pointer members to the value 
 * passed in and returns a pointer to it. 
 */
svc_list*
new_svc_list(svc_list *next, hook_path_pair *hook_list_head)
{
    svc_list        *new_svc_list = (svc_list *) malloc(sizeof(svc_list));

    new_svc_list->next = next;
    new_svc_list->hook_list = hook_list_head;

    return (new_svc_list);
}

/*
 * Allocates memory for a new hook_path_pair, sets all pointer members to NULL
 * and returns a pointer to it.
 */
hook_path_pair*
new_null_hook_path_pair()
{
    hook_path_pair     *new_hook_path_pair = (hook_path_pair *) malloc(sizeof(hook_path_pair));

    new_hook_path_pair->next = NULL;

    return (new_hook_path_pair);
}

/*
 * Allocates memory for a new hook_path_pair, sets all pointer members to the
 * value passed in and returns a pointer to it. 
 */
hook_path_pair*
new_hook_path_pair(hook_path_pair *next)
{
    hook_path_pair     *new_hook_path_pair = (hook_path_pair *) malloc(sizeof(hook_path_pair));

    new_hook_path_pair->next = next;

    return (new_hook_path_pair);
}

/* 
 * Takes an adrress in the form a.b.c.d:port_numberber and parses it storing the ip
 * address and port number in the approiate char arrays, address_to_parse[22], ip_address[16] 
 * and port_number[6], returns true if successful otherwise returns false 
 */
bool 
parse_address(char *address_to_parse, char *ip_address, char* port_number) 
{
    int     i, j;
    bool    port_now = false;

    j = 0;

    if ( address_to_parse == NULL)
        return port_now;

    for (i = 0; i < complete_address_len; ){
        if (address_to_parse[i] == ':') {
            i++;
            ip_address[j] = '\0';
            port_now = true; 
            j = 0;
        }

        if (port_now == false)
            ip_address[j++] = address_to_parse[i++];
        else 
            port_number[j++] = address_to_parse[i++];
    }

    port_number[j] = '\0';

    return port_now;
}

/*
 * Open listeners on a preset ip address port combination to recieve monitors
 * connections and instructions. Recives the event base to add events to, the 
 * listener create, and the heads of the linked lists for services, and buffer
 * events. Listener is created and added to the base, services are already set
 * but will be needed by the cb function, adds to the buffer event list. 
 */
void
listen_for_monitors(struct event_base *event_loop, struct evconnlistener *local_listener, list_heads *heads)
{
    char                ip[ip_len], port[port_len];
    int                 port_number;
    struct sockaddr_in  monitor_address;
    struct in_addr      *inp = (struct in_addr *) malloc(sizeof(struct in_addr));

    if (!parse_address(monitor_address, ip, port)){
        fprintf(stderr, "Bad address agent unable listen for monitor!!\n");
    } else {
        port_number = atoi(port);
        inet_aton(ip, inp);
        memset(&monitor_address, 0, sizeof(monitor_address));
        monitor_address.sin_family = AF_INET;
        monitor_address.sin_addr.s_addr = (*inp).s_addr;
        monitor_address.sin_port = htons(port_number);
        local_listener = evconnlistener_new_bind(event_loop, monitor_connect_cb, heads, LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, -1, (struct sockaddr *) &monitor_address, sizeof(monitor_address));

        if (!local_listener)
            fprintf(stderr, "Couldn't create listner for monitors.\n");

        // one listener will create a event buffer for each monitor that connects
        // event buffers will all call the same call back, will parse out service and execute the hook sent with service on that service.
    }
}

/*
 * Recoved the event_loop (event base) and adds an event to it that will trigger 
 * when a signal is recived. It handles those signals, for the kill signal it frees
 * all memory and shuts down the event loop instead of simply letting it crash.
 */
void init_signals(event_loop)
{    
    struct event *signal_event = NULL;

    signal_event = evsignal_new(event_loop, SIGINT, signal_cb, (void *) event_loop);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        fprintf(stderr, "Could not create/add signal event.\n");
        exit(0);
    }
}

 /*
 * main
 */
int
main (int argc, char **argv) 
{
    char                    file_name[file_name_len];
    char                    *file_buffer = NULL, **command_args = NULL;
    FILE                    *file_pointer = NULL;
    list_heads              services_and_buffer_events;
    int                     file_size = 0;
    struct event_base       *event_loop = NULL;
    struct event            *signal_event = NULL;
    struct evconnlistener   *listener = NULL;

    services_and_buffer_events.list_of_services = NULL;
    services_and_buffer_events.list_of_buffer_events = NULL;

    if (!validate_args(argc, argv)) 
    	usage();

    services_and_buffer_events.list_of_services = parse_config_file(argv[argc - 1]); 
    
    event_loop = event_base_new();                                
    listen_for_monitors(event_loop, listener, &services_and_buffer_events);

    signal_event = evsignal_new(event_loop, SIGINT, signal_cb, (void *) event_loop);
    if (!signal_event || event_add(signal_event, NULL) < 0) {
        fprintf(stderr, "Could not create/add signal event.\n");
        exit(0);
    }

    event_base_dispatch(event_loop);
    evconnlistener_free(listener);
    free_lists_memory(&services_and_buffer_events);
    event_base_free(event_loop);
}