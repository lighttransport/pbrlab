#include <stdlib.h>

#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
#include <glog/logging.h>
#endif

int main(int argc, char **argv) {
  (void)argc;
  (void)argv;
#ifdef PBRLAB_USE_STACK_TRACE_LOGGER
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();
#endif

  return EXIT_SUCCESS;
}
