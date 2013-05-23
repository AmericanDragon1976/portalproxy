/*
 * Proxy provides the proxying services for the portal proxy service. Accepts 
 * with Monitor to know where to direct clients, listens for direction to 
 * change from Monitor and passes traffic back and forth from clients to services.
 */

#include "proxy.h"
#include "proxy_config.h"
#include "proxy_events.h"


/* 
 * Called by to_ event every time it times out. They body is commented out
 * because its current use is for testing some aspects and it may be removed 
 * from the final product. 
 */
void 
timeout_cb(evutil_socket_t fd, short what, void *arg) 
{ 
/*  service *test = (service *) arg;
    bufferevent_write(test->b_monitor, test->name, sizeof(test->name));
    printf("timeout_cb called\n");
    printf("Server up\n");
*/
}

/* 
 * Prints to screen the proper syntax for running the program, then exits.
 */
void 
usage()
{
    printf("Usage is as follows: \n");
    printf("    portal-proxy space seperated flags /path/to/config/file\n");
    printf("Example: \n");
    printf("    portal-proxy -C ../../deps/config.txt\n");
    exit(0);
}

/* 
 * Verifies the command line arguments. The arguments must include flag -C and 
 * the path/to/comfig.txt. If specific uses of the -C flag or other flags are 
 * added then this function should be altered accordingly.
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
 * Allocates memory for a new service node and inicilizes all pointer members 
 * to NULL, returns a pointer to this new node.
 */
service* 
new_null_service_node() 
{
    service     *new_node = (service *) calloc(1, sizeof(service));

    new_node->next = NULL;
    new_node->listener = NULL;
    new_node->b_monitor = NULL;
    new_node->client_list = NULL;

    return (new_node);
}

/* 
 * Allocates memory for a new service node and inicilizes all pointer members 
 * to the values passed in via parameters,  returns a pointer to this new 
 * node. 
 */
service* 
new_service_node(service *nxt, struct evconnlistener *lstnr, 
               struct bufferevent *bevm, svc_cli_pair *scp) 
{
    service     *new_node = (service *) calloc(1, sizeof(service));

    new_node->next = nxt;
    new_node->listener = lstnr;
    new_node->b_monitor = bevm;
    new_node->client_list = scp;

    return new_node;
}

/*  
 * Allocates memeory for a new svc_cli_pair and sets their members inicial values to NULL
 */
svc_cli_pair* 
new_null_svc_cli_pair () {
    svc_cli_pair   *new_pair = (svc_cli_pair *) malloc(sizeof(svc_cli_pair));

    new_pair->b_client = NULL;
    new_pair->b_service = NULL;
    new_pair->next = NULL;

    return new_pair;
}


/* 
 * Allocates memeory for a new svc_cli_pair and sets their members inicial values as supplied by caller 
 */
svc_cli_pair* 
new_svc_cli_pair(struct bufferevent *client, struct bufferevent *service, svc_cli_pair *nxt)
{
    svc_cli_pair   *new_pair = (svc_cli_pair *) malloc(sizeof(svc_cli_pair));

    new_pair->b_client = client;
    new_pair->b_service = service;
    new_pair->next = nxt;

    return new_pair;
}


/* 
 * allocates memory for a new service_package sets all pointers to NULL and returns a pointer to the new
 * service_package 
 */
service_pack* 
new_null_service_package() 
{
    service_pack    *new_serv = (service_pack *) malloc(sizeof(service_pack));

    new_serv->serv = NULL;
    new_serv->pair = NULL;

    return new_serv;
}

/* 
 * allocates memory for a new service_package sets all pointers to the values passed in by parameters and
 * returns a pointer to the new service_package 
 */
service_pack* 
new_service_package(service *svc, svc_cli_pair *par) 
{
    service_pack    *new_serv = (service_pack *) malloc(sizeof(service_pack));

    new_serv->serv = svc;
    new_serv->pair = par;

    return new_serv;
}

/* 
 * Goes through list of services, and for each service it :
 *   connects to monitor,
 *   requests service addr.
 */
void 
init_services(struct event_base *eBase, service *service_list) 
{
    service             *svc_list = (service *) service_list;
    struct addrinfo     *server = NULL;
    struct addrinfo     *hints = NULL;

    while (svc_list != NULL) {
        char ip_addr[16], port_num[6];
        int i = 0; 
        int j = 0; 

        if (!parse_address(svc_list->monitor, ip_addr, port_num))
            fprintf(stderr, "Bad address unable to connect to monitor for %s\n", svc_list->name);
        else {
            hints =  set_criteria_addrinfo();
            i = getaddrinfo(ip_addr, port_num, hints, &server);

            if (i != 0){                                                         
                fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(i));
                svc_list = svc_list->next;
                continue;
            } 

            svc_list->b_monitor = bufferevent_socket_new(eBase, -1, BEV_OPT_CLOSE_ON_FREE|EV_PERSIST); 
            bufferevent_setcb(svc_list->b_monitor, monitor_read_cb, NULL, event_cb, service_list); 

            if(bufferevent_socket_connect(svc_list->b_monitor, server->ai_addr, server->ai_addrlen) != 0) { 
                fprintf(stderr, "Error connecting to monitor\n"); 
                bufferevent_free(svc_list->b_monitor); 
            } 

            bufferevent_enable(svc_list->b_monitor, EV_READ|EV_WRITE);
            bufferevent_write(svc_list->b_monitor, svc_list->name, sizeof(svc_list->name));
        }
        svc_list = svc_list->next;
    }
}

/* 
 * goes through the list of services, creats a listener to accept new clients
 * for each service in the list 
 */
void 
init_service_listeners(struct event_base *eBase, service *svc_list) 
{
    int                 port_no;
    struct sockaddr_in  svc_addr;
    struct in_addr      *inp = (struct in_addr *) malloc (sizeof(struct in_addr));

    while (svc_list != NULL) {
        char ip_addr[ip_len], port_num[port_len];

        if (!parse_address(svc_list->listen, ip_addr, port_num)) {
            fprintf(stderr, "Bad address unable listen for clients for service %s\n", svc_list->name);
        } else {
            port_no = atoi(port_num);
            inet_aton(ip_addr, inp); 
            memset(&svc_addr, 0, sizeof(svc_addr));
            svc_addr.sin_family = AF_INET;
            svc_addr.sin_addr.s_addr = (*inp).s_addr; 
            svc_addr.sin_port = htons(port_no); 
            svc_list->listener = evconnlistener_new_bind(eBase, client_connect_cb, svc_list, 
                                                          LEV_OPT_CLOSE_ON_FREE|LEV_OPT_REUSEABLE, 
                                                         -1, (struct sockaddr *) &svc_addr, sizeof(svc_addr)); 
            if (!svc_list->listener)
                printf("Couldn't create Listener\n");
        }
        svc_list = svc_list->next;
    }
}

/* 
 * Sets the information in an addrinfo structure to be used as the critera 
 * stuctrue passed into getaddrinfo().
 */
struct addrinfo* 
set_criteria_addrinfo() 
{
    struct addrinfo     *hints = (struct addrinfo *) malloc(sizeof(struct addrinfo));

    hints->ai_family = AF_INET;
    hints->ai_socktype = SOCK_STREAM;
    hints->ai_flags = 0;
    hints->ai_protocol = 0;

    return hints;
}

/* 
 * Recives a pointer to the linked list of services. For each node in the list
 * frees the monitor buffer, frees the client list, and then frees the node.  
 */
void 
free_all_service_nodes(service *svc_list)
{
    service         *temp = svc_list;

    while (svc_list != NULL) {
        bufferevent_free(svc_list->b_monitor);
        free_pair_list(svc_list->client_list);
        evconnlistener_free(svc_list->listener);
        svc_list = svc_list->next;
        free(temp);
        temp = svc_list;
    }
}

/*
 * Recives a pointer to a linked list of service client pairs. Frees both 
 * buffer events and then frees the node, for each node in the linked list. 
 */
void 
free_pair_list(svc_cli_pair *pair)
{
    svc_cli_pair   *temp = pair;

    while (pair != NULL){
        bufferevent_free(pair->b_client);
        bufferevent_free(pair->b_service);
        pair = pair->next;
        free(temp);
        temp = pair;
    }
}

int 
main(int argc, char **argv) 
{
    service             *service_list = NULL;
    struct event_base   *event_loop = NULL;
    struct event        *signal_event = NULL;

    /* 
     * Currently -C flag is required but has no effect, if  others are
     * added later to change behavior change the verfyComndlnArgs() function 
     * to change what they do
     */

    if (!validate_args(argc, argv))
        usage();

    service_list = parse_config_file(argv[argc - 1]);    
 
    event_loop = event_base_new();

    init_services(event_loop, service_list);
    init_service_listeners(event_loop, service_list); 

    // This time out event only needed for testing, can be removed in final version
 /*   struct timeval      five_seconds = {5, 0};
    struct event        *to_event; 

    to_event = event_new(event_loop, -1, EV_PERSIST, timeout_cb, service_list);
    event_add(to_event, &five_seconds);
*/

    // init_signals();
    // this would be in the init_signals() function
    // kill_event = evsignal_new(event_loop, SIGINT, signal_cb, (void *) event_loop);
    // if (!kill_event || event_add(kill_event, NULL) < 0) {
    //     fprintf(stderr, "Could not create/add signal event.\n");
    //     exit(0);
    // }

    event_base_dispatch(event_loop);
    free_all_service_nodes(service_list);
    event_base_free(event_loop);
}
