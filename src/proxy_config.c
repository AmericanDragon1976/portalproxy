#include "proxy_structures.h"
#include "proxy.h"
#include "proxy_config.h"
#include "proxy_events.h"

/*
 * When written this function will increase the siace of the array used to hold services dinamically. 
 */
 void increase_svc_list_size()
{
    // STUB, EXPAND TO MAKE PROXY CAPABLE OF HANDLING VARIABLE AMOUNTS OF SERVICES.
}

/* 
 * Recives the name and path to athe config file. Returns an int congtining the
 * length of the config file. There is a good probility that 
 * the file will contain 0's and so strlen() can not be used to determine 
 * the length of the char array after it is read in. 
 */
int 
get_config_file_len(char *name) 
{
    int     file_len = 0;
    char    *buffer = NULL;
    FILE    *file_pointer = NULL;

    file_pointer = fopen(name, "r");
    if (file_pointer == NULL) {
        fprintf(stderr, "Unable to open config file. chech file name and path!\n");
        exit(0);
    }

    fseek(file_pointer, 0, SEEK_END);
    file_len = ftell(file_pointer);
    fclose(file_pointer);

    return (file_len);
}

/* 
 * Recives the name a path to the config file, and a int containing the length 
 * of the file. Returns pointer to a char array holding contents of the file.  
 */
char* 
read_file(char *name, int len)
{
    char    *buffer = (char *) malloc(sizeof(char) * (len));
    FILE    *file_pointer = NULL;
    size_t  result;

    file_pointer = fopen(name, "r");
    if (file_pointer == NULL) {
        fprintf(stderr, "Unable to open file, check file name and path!\n");
        exit(0);
    }

    if (buffer == NULL) {
        fprintf(stderr, "Memory Error, creation of File Buffer Failed!\n");
        exit(0);
    }
    result = fread(buffer, 1, len, file_pointer);
    if (result != len) {
        fprintf(stderr, "Error reading file.\n");
        exit(0);
    }
    fclose(file_pointer);
    return (buffer);
}

/* 
 * Recives a char pointer to the buffer containing the config file text. 
 * Returns a pointer to the head of a linked list of services. 
 */
void
parse_config_file(char *name, service svc_list[])
{
    int         current_svc = 0;
    char        service_start_identifier[] = "service";
    char        temp_addr[complete_address_len];
    int 		len = get_config_file_len(name);
    char 		*buffer = read_file(name, len);

    int j = 0;
    int i = 0;

    while (i < len){
        for (j = 0; j < sizeof(service_start_identifier) - 1; j++)  // read Identifier "service"
            service_start_identifier[j] = buffer[i++];

        i++;                                                         // advance past white space

        if (strcmp(service_start_identifier, "service") != 0){           // returns 0 only if they are equal
            fprintf(stderr, "Config file Corrupted. \n");
            exit(0);
        }

        j = 0;

        while (buffer[i] != '\n') 
            svc_list[current_svc].name[j++] = buffer[i++];   // read service name

        svc_list[current_svc].name[j] = '\0';
        i++;                                        // disgard \n

        for (; buffer[i++] != 'n';);                  // find end of identifier "listen"

        i++;                                        // advance beyond white spaceczz
        
        for (j = 0; j < complete_address_len; )
            temp_addr[j++] = '0';

        j = 0;

        while (buffer[i] != '\n')
            temp_addr[j++] = buffer[i++]; // read listen address

        if (check_for_address_collision(temp_addr, svc_list)){
            printf("Address collision in config file. \n");
            exit(0);
        }

        svc_list[current_svc].listen[j] = '\0';

        for (;buffer[i++] != 'r';)                    // find end of identifier "monitor"
            ;                   
        i++;  

        for (j = 0; j < complete_address_len; )
            temp_addr[j++] = '0';
                                                  // advance beyond white space
        j = 0;

        while (i < len && buffer[i] != '\n')
            temp_addr[j++] = buffer[i++];           // read monitor address

        temp_addr[j] = '\0';

        if (check_for_address_collision(temp_addr, svc_list)){
            printf("Address collision in config file. \n");
            exit(0);
        }

        if (i < len){
            current_svc++;

        if (current_svc > list_size)
            increase_svc_list_size();                   // THIS FUNCTION IS ONLY A STUB AT THIS TIME.

            for (; buffer[i++] != 's';)              // advance to next record
                ;

            i--;
        }
    }
    free(buffer);
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
            port_now = true; 
            j = 0;
        }

        if (port_now == false) {
            if (j >= ip_len)
                return (port_now);
            ip_address[j++] = address_to_parse[i++];
        }
        else {
            if (j >= port_len)
                return (port_now);
            port_number[j++] = address_to_parse[i++];
        }
    }
    return port_now;
}
