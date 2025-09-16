#include <algorithm>
#include <mutex>
#include <myproto/GapGun.grpc.pb.h>
#include <myproto/GapGun.pb.h>

#include <grpc/grpc.h>
#include <grpcpp/server_builder.h>

#include <iostream>
#include <thread>
#include <chrono>

class GapGunRPCServiceImpl final : public GapGunRPCService::Service
{
    public:
        GapGunRPCServiceImpl()
        {
            broadcaster_ = std::thread(&GapGunRPCServiceImpl::BroadcastThread, this);
            broadcaster_.detach();
        }
        virtual ::grpc::Status SetToken(::grpc::ServerContext* context, const ::TokenRequest* token_request, ::TokenRequest* token_response) override
        {
            std::cout << "setting token\n";
            token_response->set_token(token_request->token());
            token_response->set_token("43");
            return ::grpc::Status::OK;
        }

        virtual ::grpc::Status SubscribeToMessages(::grpc::ServerContext* context, ::grpc::ServerReaderWriter<MessageRequest, MessageRequest>* stream) override
        {
            {
                std::lock_guard<std::mutex> lock(streams_mutex);
                active_streams.push_back(stream);
            }

            // Reader loop: handle client responses
            MessageRequest client_msg;
            while (stream->Read(&client_msg)) {
                std::cout << "[Server] Received response id=" 
                          << client_msg.request_id()
                          << ", text=" << client_msg.json() << std::endl;
            }

            // Remove stream from active list on disconnect
            {
                std::lock_guard<std::mutex> lock(streams_mutex);
                active_streams.erase(std::remove(active_streams.begin(), active_streams.end(), stream),
                                     active_streams.end());
            }

            std::cout << "[Server] Client disconnected, stream closed" << std::endl;

            return ::grpc::Status::OK;
        }

    private:
        std::mutex streams_mutex;
        std::vector<grpc::ServerReaderWriter<MessageRequest, MessageRequest>*> active_streams;
        std::thread broadcaster_;

        void BroadcastThread() {
            int counter = 0;
            while (true) {
                MessageRequest msg;
                msg.set_request_id(std::to_string(counter++));
                msg.set_json("Ping from server");

                std::vector<grpc::ServerReaderWriter<MessageRequest, MessageRequest>*> streams_copy;
                {
                    std::lock_guard<std::mutex> lock(streams_mutex);
                    streams_copy = active_streams;  // copy pointers while locked
                }

                for (auto* stream : streams_copy) {
                    if (!stream->Write(msg)) {
                        std::cout << "[Server] Failed to write to a client stream" << std::endl;
                    }
                }

                std::this_thread::sleep_for(std::chrono::seconds(5));
            }
        }
};

int main(int argc, char* argv[])
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort("0.0.0.0:50051", grpc::InsecureServerCredentials());

    GapGunRPCServiceImpl gg_service;
    builder.RegisterService(&gg_service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    server->Wait();
    
    return 0;
}
