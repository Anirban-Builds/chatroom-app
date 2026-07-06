#include "server.h"
#include "helper.h"
#include "client_manager.h"

int main(){
    load_env(".env");
    init_client_manager();
    run_server();
    return 0;
}