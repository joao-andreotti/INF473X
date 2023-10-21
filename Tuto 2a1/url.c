/**
 *  Jiazi Yi
 *
 * LIX, Ecole Polytechnique
 * jiazi.yi@polytechnique.edu
 *
 * Updated by Pierre Pfister
 *
 * Cisco Systems
 * ppfister@cisco.com
 *
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"url.h"

/**
 * parse a URL and store the information in info.
 * return 0 on success, or an integer on failure.
 *
 * Note that this function will heavily modify the 'url' string
 * by adding end-of-string delimiters '\0' into it, hence chunking it into smaller pieces.
 * In general this would be avoided, but it will be simpler this way.
 */

int parse_url(char* url, url_info *info)
{
	// url format: [http://]<hostname>[:<port>]/<path>
	// e.g. https://www.polytechnique.edu:80/index.php

	char *column_slash_slash, *host_name_path, *protocol;

	column_slash_slash = strstr(url, "://");
	// it searches :// in the string. if exists,
	//return the pointer to the first char of the match string

	if(column_slash_slash != NULL){ //protocol type exists
		*column_slash_slash = '\0'; //end of the protocol type
		host_name_path = column_slash_slash + 3; //jump to the host name
		protocol = url;
	} else {	//no protocol type: using http by default.
		host_name_path = url;
		protocol = "http";
	}

	/*
	 * To be completed:
	 * 	 store info->protocol
	 */
	size_t protocol_size;
	if (column_slash_slash)
		protocol_size = column_slash_slash - url;
	else
		protocol_size = strlen(protocol);
	
	char *protocol_string = (char *) malloc(sizeof(char) * (protocol_size + 1));

	strncpy(protocol_string, protocol, protocol_size);
	protocol_string[protocol_size] = '\x00';

	info->protocol = protocol_string;

	/*
     * To be completed:
	 * 	 Return an error (PARSE_URL_PROTOCOL_UNKNOWN) if the protocol is not 'http' using strcmp.
	 */

	if (strcmp(protocol_string, "http") != 0)
		return PARSE_URL_PROTOCOL_UNKNOWN;

	/*
	 * To be completed:
	 * 	 Look for the first '/' in host_name_path using strchr
	 * 	 Return an error (PARSE_URL_NO_SLASH) if there is no such '/'
	 * 	 Store the hostname and the queried path in the structure.
	 *
	 * 	 Note: The path is stored WITHOUT the first '/'.
	 * 	       It simplifies this function, but I'll let you understand how. :-)
	 */

	char *end_of_hostname = strchr(host_name_path, '/');
	if (end_of_hostname == NULL)
		return PARSE_URL_NO_SLASH;
	
	char *path = end_of_hostname + 1;

	size_t hostname_size = end_of_hostname - host_name_path;
	char *hostname_string = (char *) malloc(sizeof(char) * (hostname_size + 1));
	strncpy(hostname_string, host_name_path, hostname_size);
	hostname_string[hostname_size] = '\x00';

	char *path_string = (char *) malloc(sizeof(char) * (strlen(path) + 1));
	strcpy(path_string, path);
	

	info->host = hostname_string;
	info->path = path_string;

	/*
	 * To be completed:
	 * 	 Look for ':' in the hostname
	 * 	 If ':' is not found, the port is 80 (store it)
	 * 	 If ':' is found, split the string and use sscanf to parse the port.
	 * 	 Return an error if the port is not a number, and store it otherwise.
	 */

	char *colon = strchr(hostname_string, ':');
	if (colon == NULL) {
		// No port specified
		info->port = 80;
		return 0;
	}

	int n_read = sscanf(colon + 1, "%d", &info->port);
	if (n_read != 1)
		return PARSE_URL_INVALID_PORT;
	

	// If everything went well, return 0.
	return 0;
}

/**
 * print the url info to std output
 */
void print_url_info(url_info *info){
	printf("The URL contains following information: \n");
	printf("Protocol:\t%s\n", info->protocol);
	printf("Host name:\t%s\n", info->host);
	printf("Port No.:\t%d\n", info->port);
	printf("Path:\t\t/%s\n", info->path);
}
