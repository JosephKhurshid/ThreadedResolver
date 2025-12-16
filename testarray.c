#include "array.h"
#include <stdlib.h>
#include <stdio.h>

int main() {
    array my_array;
    char *name_host;
    char test_hostnames[8][24] = {"youtube.com/1", "youtube.com/2", "youtube.com/3", "youtube.com/4", "youtube.com/5", "youtube.com/6", "youtube.com/7","youtube.com/8"};
    int hostname_index = 0;

    if (array_init(&my_array) < 0) {
        exit(-1);
    }

    while ( hostname_index < 7){
        array_put(&my_array, test_hostnames[hostname_index] );
        hostname_index = hostname_index + 1;
    }

    hostname_index =0;

    while ( hostname_index < 7) {
        array_get(&my_array, &name_host);
        printf("Get Value from My Array: %s\n", name_host);
        hostname_index = hostname_index + 1;
    }
    
    array_free(&my_array);
    exit(0);
}
