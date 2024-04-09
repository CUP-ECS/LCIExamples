#include "lci.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(int argc, char** argv)
{
    // Initialize LCI
    LCI_initialize();
    
    // Initialize device and endpoint
    LCI_device_t device;
    LCI_device_init(&device);
    LCI_endpoint_t ep;

    // Create property list
    LCI_plist_t plist;
    LCI_plist_create(&plist);

    LCI_plist_set_comp_type(plist, LCI_PORT_COMMAND, LCI_COMPLETION_SYNC);
    LCI_plist_set_comp_type(plist, LCI_PORT_MESSAGE, LCI_COMPLETION_SYNC);

    LCI_plist_set_match_type(plist, LCI_MATCH_RANKTAG);
    LCI_endpoint_init(&ep, device, plist);
    LCI_plist_free(&plist);

    LCI_comp_t sync;
    LCI_sync_create(device, 1, &sync);

    LCI_tag_t tag = 99;
    void* user_context = (void*)9527;
    
    if (LCI_RANK == 0)
    {
        LCI_short_t message;
        *(uint64_t*)&message = 1347;
        while (LCI_sends(ep, message, 1, tag) == LCI_ERR_RETRY)
            LCI_progress(device);
    }
    else if (LCI_RANK == 1)
    {
        LCI_recvs(ep, 0, tag, sync, user_context);
        LCI_request_t request;
        while (LCI_sync_test(sync, &request) == LCI_ERR_RETRY)
            LCI_progress(device);
        assert(request.flag == LCI_OK);
        assert(request.rank == 0);
        assert(request.tag == tag);
        assert(request.type == LCI_IMMEDIATE);
        assert(request.user_context == user_context);
        assert(*(uint64_t*)&request.data.immediate == 1347);
        printf("Short send/recv successful!\n");
    }
    LCI_sync_free(&sync);
    LCI_endpoint_free(&ep);
    LCI_device_free(&device);
    LCI_finalize();
    return 0;
}