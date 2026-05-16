#include "GrpcKeyValueServiceImpl.hpp"
#include "MiniKeyValue.hpp"

#include <grpcpp/grpcpp.h>
#include <grpcpp/security/server_credentials.h>

#include <iostream>
#include <string>

static void printUsage(std::ostream& os) {
    os << "usage: minikvalue-grpc-server --listen HOST:PORT --data-path PATH --shard-index N --shard-count M\n";
}

int main(int argc, char** argv) {
    std::string listen = "0.0.0.0:50051";
    std::string data_path;
    int shard_index = -1;
    int shard_count = -1;

    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--listen") {
            if (++i >= argc) {
                printUsage(std::cerr);
                return 1;
            }
            listen = argv[i];
        } else if (a == "--data-path") {
            if (++i >= argc) {
                printUsage(std::cerr);
                return 1;
            }
            data_path = argv[i];
        } else if (a == "--shard-index") {
            if (++i >= argc) {
                printUsage(std::cerr);
                return 1;
            }
            shard_index = std::stoi(argv[i]);
        } else if (a == "--shard-count") {
            if (++i >= argc) {
                printUsage(std::cerr);
                return 1;
            }
            shard_count = std::stoi(argv[i]);
        } else if (a == "--help" || a == "-h") {
            printUsage(std::cout);
            return 0;
        } else {
            printUsage(std::cerr);
            return 1;
        }
    }

    if (data_path.empty() || shard_count < 1 || shard_index < 0 || shard_index >= shard_count) {
        printUsage(std::cerr);
        return 1;
    }

    std::shared_ptr<MiniKeyValue> store = MiniKeyValue::createMiniKeyValue(data_path);
    auto service = std::make_unique<GrpcKeyValueServiceImpl>(store, shard_index, shard_count);

    grpc::ServerBuilder builder;
    builder.AddListeningPort(listen, grpc::InsecureServerCredentials());
    builder.RegisterService(service.get());
    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "failed to listen on " << listen << '\n';
        return 1;
    }

    std::cout << "minikvalue shard " << shard_index << "/" << shard_count << " data-path=" << data_path
              << " listen=" << listen << '\n';
    server->Wait();
    return 0;
}
