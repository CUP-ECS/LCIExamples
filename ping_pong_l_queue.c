#include "lci.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

int main(int argc, char** argv)
{

  // Call `LCI_initialize` to initialize the runtime
  LCI_initialize();
  int msgs_size, num_msgs;
  if (argc != 3)
  {
    if (LCI_RANK == 0)
      fprintf(stderr, "Usage: %s <num msgs> <msg size (bytes)>\n", argv[0]);
    exit(1);
  }
  else
  {
    num_msgs = atoi(argv[1]);
    msgs_size = atoi(argv[2]);
  }
  // Initialize a device. A LCI device is associated with a set of communication
  // resources (matching table, low-level network resources, etc).
  // Alternatively, users can use LCI_UR_DEVICE, which
  // has been initialized by the runtime in LCI_initialize().
  LCI_device_t device;
  LCI_device_init(&device);
  // Initialize an endpoint. Alternatively, users can use LCI_UR_ENDPOINT,
  // which has been initialized by the runtime in LCI_initialize().
  LCI_endpoint_t ep;
  // We need a property list to initialize the endpoint. Multiple properties of
  // the endpoint can be specified by the property list.
  // Currently, a LCI endpoint is not associated with any low-level
  // communication resources, it is just a way to specify a bunch of
  // configurations.
  LCI_plist_t plist;
  LCI_plist_create(&plist);
  // We set the completion mechanism on the sender side to be completion queue.
  LCI_plist_set_comp_type(plist, LCI_PORT_COMMAND, LCI_COMPLETION_QUEUE);
  // We set the completion mechanism on the receiver side to be completion
  // queue.
  LCI_plist_set_comp_type(plist, LCI_PORT_MESSAGE, LCI_COMPLETION_QUEUE);
  // We set the matching rule to be rank+tag, so a pair of LCI send and recv
  // will be matched through (source rank, destination rank, tag).
  // Alternatively, you can set the matching rule to tag-only by LCI_MATCH_TAG.
  LCI_plist_set_match_type(plist, LCI_MATCH_RANKTAG);
  LCI_endpoint_init(&ep, device, plist);
  LCI_plist_free(&plist);

  // Create a completion queue.
  LCI_comp_t cq;
  LCI_queue_create(device, &cq);

  // For long messages (> LCI_MEDIUM_SIZE bytes), we need to use LCI_sendl
  // and LCI_recvl to send and receive messages. To use LCI_sendl and LCI_recvl,
  // we need to register our own memory buffers.
  printf("LCI_SHORT_SIZE: %d \t LCI_MEDIUM_SIZE: %d\n", LCI_SHORT_SIZE, LCI_MEDIUM_SIZE);
  // We are using LCI_sendm/recvm in this example, so we need to allocate our
  // own send/recv buffers
  LCI_lbuffer_t src_buf, dst_buf;
  src_buf.address = malloc(msgs_size);
  src_buf.length = msgs_size;
  memset(src_buf.address, 'a' + LCI_RANK, msgs_size);
  dst_buf.address = malloc(msgs_size);
  dst_buf.length = msgs_size;
  memset(dst_buf.address, 0, msgs_size);

  int peer_rank;
  if (LCI_NUM_PROCESSES == 1) {
    // We will do loopback messages
    peer_rank = 0;
  } else if (LCI_NUM_PROCESSES == 2) {
    // We will do a simple ping-pong here between rank 0 and rank 1.
    // The destination rank to send and recv messages.
    peer_rank = 1 - LCI_RANK;
  } else {
    fprintf(stderr, "Unexpected process number!");
  }
  // The tag of the messages. Send and recv will be matched based on the tag
  // or rank+tag, depending on the matching rule. You can set the match rule by
  // LCI_plist_set_match_type.
  LCI_tag_t tag = 99;
  // User-defined contexts.
  void* user_context = (void*)9527;
  if (LCI_RANK == 0) {
    for (int i = 0; i < num_msgs; i++) {
      // Send a long message using LCI_sendl
      // A LCI send function can return LCI_ERR_RETRY, so we use a while loop
      // here to make sure the message is sent
      while (LCI_sendl(ep, src_buf, peer_rank, tag, cq, user_context) == LCI_ERR_RETRY)
        // Users have to call LCI_progress frequently to make progress on the
        // background work.
        LCI_progress(device);
      // Recv a long message using LCI_recvl
      while (LCI_recvl(ep, dst_buf, peer_rank, tag, cq, user_context) == LCI_ERR_RETRY)
        // Users have to call LCI_progress frequently to make progress on the
        // background work.
        LCI_progress(device);

      LCI_request_t request;
      // Try to pop a entry from the completion queue.
      while (LCI_queue_pop(cq, &request) == LCI_ERR_RETRY)
        // Users have to call LCI_progress frequently to make progress on the
        // background work.
        LCI_progress(device);
      // Once an entry is popped, the completion information will be
      // written to the request object.
      assert(request.flag == LCI_OK);
      assert(request.rank == peer_rank);
      assert(request.tag == tag);
      assert(request.type == LCI_LONG);
      assert(request.user_context == user_context);
      assert(request.data.lbuffer.address == dst_buf.address);
      assert(request.data.lbuffer.length == dst_buf.length);
      for (int j = 0; j < msgs_size; ++j) {
        assert(((char*)dst_buf.address)[j] == 'a' + peer_rank);
      }
    }
  } else {
    for (int i = 0; i < num_msgs; i++) {
      while(LCI_recvl(ep, dst_buf, peer_rank, tag, cq, user_context) == LCI_ERR_RETRY)
        LCI_progress(device);
      LCI_request_t request;
      while (LCI_queue_pop(cq, &request) == LCI_ERR_RETRY)
        LCI_progress(device);
      assert(request.flag == LCI_OK);
      assert(request.rank == peer_rank);
      assert(request.tag == tag);
      assert(request.type == LCI_LONG);
      assert(request.user_context == user_context);
      assert(request.data.lbuffer.address == dst_buf.address);
      assert(request.data.lbuffer.length == dst_buf.length);
      for (int j = 0; j < msgs_size; ++j) {
        assert(((char*)dst_buf.address)[j] == 'a' + peer_rank);
      }
      while (LCI_sendl(ep, src_buf, peer_rank, tag, cq, user_context) == LCI_ERR_RETRY)
        LCI_progress(device);
    }
  }
  // Free all the resources
  free(src_buf.address);
  free(dst_buf.address);
  LCI_queue_free(&cq);
  LCI_endpoint_free(&ep);
  LCI_device_free(&device);
  // Call `LCI_finalize` to finalize the runtime
  LCI_finalize();
  return 0;
}
