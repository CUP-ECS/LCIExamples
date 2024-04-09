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

    LCI_mbuffer_t buf;
    int msg_size = 4096;
    int buf_len = 10;
    if (LCI_RANK == 0)
    {
        buf.address = malloc(msg_size);
        buf.length = buf_len;
        memset(buf.address, 'a' + LCI_RANK, buf_len);
        LCI_barrier();
        while (LCI_sendm(ep, buf, 1, tag) == LCI_ERR_RETRY)
            LCI_progress(device);
    }
    else if (LCI_RANK == 1)
    {
        buf.address = malloc(msg_size);
        buf.length = buf_len;
        memset(buf.address, 0, buf_len);
        LCI_recvm(ep, buf, 0, tag, sync, user_context);
        LCI_request_t request;
        LCI_barrier();
        while (LCI_sync_test(sync, &request) == LCI_ERR_RETRY)
            LCI_progress(device);
        assert(request.flag == LCI_OK);
        assert(request.rank == 0);
        assert(request.tag == tag);
        assert(request.type == LCI_MEDIUM);
        assert(request.user_context == user_context);
        assert(request.data.mbuffer.address == buf.address);
        assert(request.data.mbuffer.length == buf.length);
        for (int i = 0; i < buf_len; i++) {
            assert(((char*)buf.address)[i] == 'a');
        }
        printf("Medium send/recv successful!\n");
    }
    free(buf.address);
    LCI_sync_free(&sync);
    LCI_endpoint_free(&ep);
    LCI_device_free(&device);
    LCI_finalize();
    return 0;
}
